# 构建

## 常规构建

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDOBBY_BUILD_TEST=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Windows 在 Visual Studio x64 Native Tools 命令行中增加
`-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`。
Ninja 生成 `dobby.dll`、`dobby_import.lib` 和 `dobby.lib`，输出不冲突。
纯 MSVC 编译器不支持本项目使用的 GNU 汇编，配置时会明确报错。

## Android

使用 NDK 自带工具链，不需要强制 include android/log.h 或降级到旧提交。

```sh
cmake -S . -B build-android -G Ninja -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DCMAKE_BUILD_TYPE=Release -DDOBBY_BUILD_TEST=ON
cmake --build build-android --parallel
```

ABI 可选 arm64-v8a、armeabi-v7a、x86、x86_64。
生成 `libdobby.so`、`libdobby.a` 和原生 E2E 可执行程序。
NativeBridge 的 ARM 库必须在应用进程中加载，不能直接当作 x86
模拟器的 shell 可执行文件运行；使用 tests/e2e/android/run.py 打包验证。

## 静态链接与安装

```sh
cmake -S . -B static -G Ninja -DCMAKE_BUILD_TYPE=Release -DDOBBY_GENERATE_SHARED=OFF
cmake --build static --target dobby
cmake --install static --prefix install
```

作为子目录时用 `add_subdirectory(path/to/Dobby)` 并链接 `dobby_static`。
此目标在静态单库模式下是 `dobby` 的别名。
安装后可以 `find_package(Dobby 1 CONFIG REQUIRED)`，链接 `Dobby::dobby`。
Rust 等非 CMake 调用方还需要链接对应 C++ 运行库，以及平台的 dl/log/thread 依赖。

## 选项

| 选项 | 默认值 | 作用 |
| --- | --- | --- |
| DOBBY_GENERATE_SHARED | ON | 同时构建共享库和静态库 |
| DOBBY_BUILD_TEST | OFF | 构建并注册原生 E2E |
| DOBBY_DEBUG | OFF | 调试日志，与构建类型独立 |
| NearBranch | ON | 优先尝试近跳转，可通过 API 调整 |
| FullFloatingPointRegisterPack | OFF | ARM64 插桩保存 q0–q31，而非 q0–q7 |
| Plugin.SymbolResolver | ON | 构建符号解析接口 |

不支持内核模式、旧的混淆插件与未接入的导入表插件；相关旧选项
会明确拒绝，不会无声地生成缺少功能的库。iOS 的具体签名／权限策略
需要在目标设备单独验收，桌面 macOS 通过不代表所有 iOS 环境可用。

## Android 应用 E2E

先构建对应 ABI 且启用 DOBBY_BUILD_TEST，然后运行：

```sh
python tests/e2e/android/run.py --sdk <android-sdk> --build-dir <build-dir> --abi arm64-v8a --serial <serial> --out <output-dir>
```

脚本只安装 org.psyche.dobby 测试应用，使用输出目录内的专用测试
签名，不读取个人签名文件。重复运行应使用相同输出目录／签名；
若已安装的测试包来自其他签名，先手动卸载该测试包或提供原来的
测试输出目录。SDK 工具和平台版本可通过参数指定。
