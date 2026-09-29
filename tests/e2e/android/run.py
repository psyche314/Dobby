"""Package an already-built E2E JNI library and run it on an explicitly selected device."""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import time
from zipfile import ZipFile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdk", required=True, type=Path)
parser.add_argument("--build-dir", required=True, type=Path)
parser.add_argument("--abi", required=True, choices=["arm64-v8a", "armeabi-v7a", "x86", "x86_64"])
parser.add_argument("--serial", required=True)
parser.add_argument("--out", required=True, type=Path)
parser.add_argument("--build-tools", default="36.1.0")
parser.add_argument("--platform", default="android-35")
args = parser.parse_args()
source = Path(__file__).resolve().parent
output = args.out.resolve()
output.mkdir(parents=True, exist_ok=True)
library = args.build_dir.resolve() / "tests/e2e/libdobby_e2e_jni.so"
header = library.read_bytes()[:20]
expected = {"arm64-v8a": 183, "armeabi-v7a": 40, "x86": 3, "x86_64": 62}
if header[:4] != b"\x7fELF" or struct.unpack_from("<H", header, 18)[0] != expected[args.abi]:
    parser.error("JNI library does not match --abi")
tools = args.sdk.resolve() / "build-tools" / args.build_tools
android_jar = args.sdk.resolve() / "platforms" / args.platform / "android.jar"
adb = args.sdk.resolve() / "platform-tools" / ("adb.exe" if os.name == "nt" else "adb")

def run(*command, capture=False):
    result = subprocess.run([str(part) for part in command], check=True,
                            stdout=subprocess.PIPE if capture else None, text=True)
    return result.stdout.strip() if capture else None

def android_tool(name):
    suffix = ".bat" if name in {"d8", "apksigner"} else ".exe"
    return tools / (name + suffix if os.name == "nt" else name)

classes, dex = output / "classes", output / "dex"
classes.mkdir(exist_ok=True)
dex.mkdir(exist_ok=True)
run("javac", "--release", "8", "-Xlint:-options", "-classpath", android_jar,
    "-d", classes, source / "E2EActivity.java")
run(android_tool("d8"), "--lib", android_jar, "--output", dex,
    classes / "org/psyche/dobby/E2EActivity.class")
unsigned, aligned, signed = [output / name for name in ("unsigned.apk", "aligned.apk", "dobby-e2e.apk")]
run(android_tool("aapt2"), "link", "--manifest", source / "AndroidManifest.xml",
    "-I", android_jar, "-o", unsigned)
with ZipFile(unsigned, "a") as apk:
    apk.write(dex / "classes.dex", "classes.dex")
    apk.write(library, f"lib/{args.abi}/libdobby_e2e_jni.so")
key = output / "e2e.keystore"
if not key.exists():
    run("keytool", "-genkeypair", "-keystore", key, "-alias", "e2e", "-keyalg", "RSA",
        "-keysize", "2048", "-validity", "365", "-dname", "CN=Dobby E2E",
        "-storepass", "android", "-keypass", "android")
run(android_tool("zipalign"), "-f", "-P", "16", "4", unsigned, aligned)
run(android_tool("apksigner"), "sign", "--ks", key, "--ks-pass", "pass:android",
    "--key-pass", "pass:android", "--out", signed, aligned)
run(adb, "-s", args.serial, "install", "-r", signed)
run(adb, "-s", args.serial, "shell", "am", "force-stop", "org.psyche.dobby")
run(adb, "-s", args.serial, "shell", "am", "start", "-W", "-n", "org.psyche.dobby/.E2EActivity")
pid = run(adb, "-s", args.serial, "shell", "pidof", "org.psyche.dobby", capture=True)
if not pid.isdecimal():
    raise SystemExit("E2E process exited or has an ambiguous PID")
for _ in range(20):
    log = run(adb, "-s", args.serial, "logcat", "-d", f"--pid={pid}",
              "-s", "DobbyE2E:I", "AndroidRuntime:E", capture=True)
    if "PASS all Dobby native E2E checks" in log:
        print(log)
        break
    time.sleep(1)
else:
    raise SystemExit("E2E did not complete successfully:\n" + log)
