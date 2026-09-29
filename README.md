# Dobby

## 接口

- `DobbyPrepare`：生成原函数 trampoline，不修改入口。
- `DobbyCommit`：激活准备好的 hook。
- `DobbyDisable` / `DobbyEnable`：恢复／重新应用入口，复用原函数 trampoline。
- `DobbyHook`：兼容原有的一步安装接口。
- `DobbyInstrument`：指令插桩也支持停用、启用和销毁。
- `DobbyDestroy`：恢复入口或取消准备，移除 hook 状态。

API 是 C ABI；指针、函数签名及调用约定由调用方负责。
管理操作有互斥锁，**机器码写入不具备原子性，也不会暂停其他线程**。
调用方必须保证提交、启停和销毁时没有相关代码在执行。

```c
static int (*original)(int);
static int replacement(int value) { return original(value) + 1; }

/* 在相关执行线程静止时安装。 */
if (DobbyPrepare(target, (void *)replacement, (void **)&original) == 0) {
    int result = DobbyCommit(target);
    /* 检查 result；-2 表示字节已写入后的系统操作失败。 */
}
```

完整行为与失败处理见 [API 约定](docs/api.md)。

## 构建与验证

CMake 3.18+、C++17，使用 GCC 或 Clang；Windows 使用 Clang 与
Visual Studio 的 Windows SDK/运行库。默认生成共享库和静态库。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDOBBY_BUILD_TEST=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

- [Android、Windows、静态链接与安装包](docs/compile.md)
- [维护范围、上游问题与验证证据](docs/maintenance.md)
- [Android 应用进程 E2E](tests/e2e/android/run.py)

显式版本从 `1.0.0` 开始。`DobbyGetVersion()` 返回该版本，
不会包含编译日期或 commit count。

## 来源与许可

保留上游 Apache-2.0 许可及提交历史。
感谢 [jmpews/Dobby](https://github.com/jmpews/Dobby)、
[BepInEx/Dobby](https://github.com/BepInEx/Dobby)、
[LSPosed/Dobby](https://github.com/LSPosed/Dobby) 的实现和问题分析。
修复出处及采用范围在维护文档中列出。

上游参考项目包括 frida-gum、MinHook、Substrate、V8、Dart 和 VIXL。
