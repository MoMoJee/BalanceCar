# CLion + CMake 迁移说明

这个工程原来由 Keil/MDK 管理，源码目录里包含已经改过的外设库和硬件驱动。迁移到 CLion 时不要重新生成或覆盖 `Hardware`、`Library`、`Start`、`System`、`User` 这些目录，CMake 只负责把它们按原清单编译进去。

## 建议保留的结构

```text
balancecar 直立环/
  CMakeLists.txt
  CMakePresets.json
  cmake/
    arm-none-eabi-gcc.cmake
    stm32f103c8tx_flash.ld
  Start/
    startup_stm32f10x_md.s          # Keil/ARMASM 版本，保留给 Keil
    startup_stm32f103c8tx_gcc.s     # GCC/CMake 版本
    core_cm3.c
    system_stm32f10x.c
    stm32f10x.h
  Library/                         # 保留当前自定义改动
  Hardware/                        # 保留当前自定义改动
  System/
  User/
  Project.uvprojx                  # 可继续保留给 Keil 对照
```

## 关键配置

- MCU：`STM32F103C8`，Flash `64K`，RAM `20K`。
- CMake 预定义：`STM32F10X_MD`、`USE_STDPERIPH_DRIVER`。
- include 路径沿用 Keil：`Start`、`Library`、`User`、`System`、`Hardware`。
- Keil 的 `startup_stm32f10x_md.s` 是 ARMASM 语法，GCC 不能直接汇编，所以新增了 `Start/startup_stm32f103c8tx_gcc.s`。
- 链接脚本 `cmake/stm32f103c8tx_flash.ld` 按 Keil 工程里的 `IROM(0x08000000,0x10000)` 和 `IRAM(0x20000000,0x5000)` 编写。

## CLion 使用方式

1. 安装 ARM GNU Toolchain，并确认 `arm-none-eabi-gcc` 在 `PATH` 中。
2. 用 CLion 打开当前项目根目录。
3. 选择 CMake preset：`arm-gcc-debug` 或 `arm-gcc-release`。
4. 构建目标 `balancecar.elf`。

如果在 CLion 里手动配置了 `CMAKE_C_COMPILER=.../arm-none-eabi-gcc.exe`，工程也会在 `project()` 前自动切换为裸机交叉编译模式。若之前已经生成过错误的 build 目录，并看到 `-lkernel32`、`--major-image-version` 或 `balancecar.elf.exe`，需要删除对应的 `cmake-build-*` 目录后重新 Reload CMake Project。

不要使用 CLion 对 `User/main.c` 生成的单文件运行配置。如果输出里出现类似下面的命令，说明当前点到的是本机 `gcc.exe` 单文件编译，不是本项目的嵌入式构建：

```text
gcc.exe "D:\PROJECTS\balancecar 直立环\User\main.c" -o main
```

这个命令不会带 `Start`、`Library`、`System`、`Hardware` 等 include 路径，也不会使用 ARM 链接脚本，因此一定会报 `stm32f10x.h: No such file or directory` 或后续链接错误。正确做法是在 CLion 右上角运行配置中选择 CMake 目标 `balancecar.elf`，或者在终端执行：

```powershell
cmake --build --preset arm-gcc-debug
```

不要用普通 Application 配置直接运行 `balancecar.elf`。它是 Cortex-M3 固件，不是 Windows 可执行程序；如果看到 `CreateProcess error=193, %1 不是有效的 Win32 应用程序`，说明 CLion 正在把 ELF 当本机程序启动。

烧录时使用 CMake 目标 `flash`，它会先生成 `balancecar.hex`，再调用 STM32CubeProgrammer 通过 ST-Link/SWD 下载并复位：

```powershell
cmake --build --preset arm-gcc-debug --target flash
```

在 CLion 里可以从 CMake/Build Targets 中选择 `flash` 目标进行构建；调试则使用 Embedded GDB Server / ST-Link 配置，不要使用普通 CMake Application 的 Run。

构建成功后会在 CMake build 目录里生成：

- `balancecar.elf`
- `balancecar.hex`
- `balancecar.bin`
- `balancecar.map`

## 后续可选整理

如果确认以后不再使用 Keil，可以再把 Keil 产物目录 `Objects`、`Listings`、`DebugConfig` 移到备份或加入忽略。但当前建议先保留它们，方便对照原始工程配置。