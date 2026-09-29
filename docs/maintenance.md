# 维护记录

## 基线与取舍

主线从官方 `5dfc8546954ce3b3198132ab13fddb89ee92cdd7` 开始。

| 证据 | 本分支处理 |
| --- | --- |
| [jmpews/Dobby #222](https://github.com/jmpews/Dobby/issues/222) | Ninja 的静态／导入库命名冲突 |
| [jmpews/Dobby #177](https://github.com/jmpews/Dobby/pull/177) | 跨页补丁；扩展为任意页数、权限保留和完整预检 |
| [jmpews/Dobby #270](https://github.com/jmpews/Dobby/issues/270)、[#280](https://github.com/jmpews/Dobby/issues/280) | 直接使用 CMake 工具链，修复上游接口迁移遗漏 |
| [jmpews/Dobby #208](https://github.com/jmpews/Dobby/issues/208) | 重复安装明确失败，恢复／重复启停带执行级回归 |
| [jmpews/Dobby #265](https://github.com/jmpews/Dobby/issues/265) | 区分管理锁与并发执行安全，不宣称原子热补丁 |
| [BepInEx/Dobby 操作数修复](https://github.com/BepInEx/Dobby/commit/b684bf0d623cf0b19956b32a0f5e4001022cc4bb) | 合并有效 ModR/M 操作数，保留 opcode 默认值 |
| [BepInEx/Dobby SSE 实现](https://github.com/BepInEx/Dobby/commit/1321565296b040b081b6065e478120460ddef751) | 重新按实际前缀与 opcode 索引选择 SSE 表；没有照搬其 ModR/M 索引 |
| [LSPosed/Dobby](https://github.com/LSPosed/Dobby)、[JingMatrix/Dobby](https://github.com/JingMatrix/Dobby) | 对照 Thumb、跨页、构建及内存管理历史；保留可验证的独立修复 |

## 验证边界

E2E 执行真实入口与原函数 trampoline，不依赖模拟的 hook 状态。
覆盖四页补丁、不同页权限、未映射页、边界／溢出、重复安装、
准备取消、100 次启停、空输出、第三方补丁冲突和指令插桩。
x86 回归还执行 SSE、立即数、相对 CALL 与 RIP 相对访问。
ARMv7 覆盖 ARM 与 Thumb 两种入口，以及 Thumb 的两字节对齐。

持续验证矩阵：Linux x64 GCC/Clang、Linux ARM64 GCC、Windows x64
Clang、macOS ARM64 Clang，包含静态／共享库执行及安装后的 C 调用方。
Android 四 ABI 使用 NDK r29 构建；雷电模拟器实测 x86、
x86_64 shell 可执行文件和 ARM64 NativeBridge 应用。
ARMv7 另外用 QEMU 执行；不把 QEMU 结果当成 ARMv7 实机验收。

NativeBridge 会将 guest 可执行页显示为宿主只读页，内部代码补丁
因此保留已知的 guest 执行权限。测试既调用新生成的原函数，也在
启停后实际执行入口，不以“类／方法可以找到”代替 hook 行为验收。

iOS、其他 NativeBridge 实现、未知指令序列及并发执行期间的补丁
不在这些证据覆盖的范围。发布前必须查看对应提交的 CI 结果，
不能把某次成功运行自动归到后续改变代码的提交上。
