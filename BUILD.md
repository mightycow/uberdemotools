# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

Here you'll learn the few steps required to build the UDT binaries for your platform and compiler.

## Building the library, command-line tools and viewer

This is all done through scripts that you can invoke from your command prompt or shell.

For each toolchain, there are 2 scripts:

1. A script that generates project files whose file name contains `generate` by invoking a local version of Premake.
2. A script that compiles the code whose file name contains `compile`.

| Data | Location |
| :--- | :--- |
| Build scripts | `UDT_DLL\premake` |
| Output files | `UDT_DLL\.bin\${toolchainfolder}` |
| Temporary build files | `UDT_DLL\.build\${toolchainfolder}` |
| Visual Studio solution file | `UDT_DLL\.build\vs${year}\UDT.sln` |

| Host | Target | Toolchain | Scripts | Folder | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Windows | Windows | Visual C++ | `vs_generate.cmd`<br>`vs_compile.cmd` | `vs${year}` | VC++ **2017** or later |
| Windows | Windows | MingW + GCC | `mingw_generate.cmd`<br>`mingw_compile.cmd` | `win_mingw` | Can use WinLibs |
| Windows | Windows | Ninja + Zig | `zig_generate_windows.cmd`<br>`zig_compile_windows.cmd` | `win_zig` | Clang |
| Windows | Linux | Ninja + Zig | `zig_generate_linux.cmd`<br>`zig_compile_linux.cmd` | `win2lin_zig` | Clang |
| Linux | Linux | gmake + GCC | `gcc_generate.sh`<br>`gcc_compile.sh` | `lin_gmake` | |

Notes:

* You can generate makefiles for earlier versions of Visual C++ with Premake but the compilation script will not work because `vswhere.exe` only exists since Visual Studio 2017.
* I recommend WinLibs as a good MinGW + GCC distribution. It's a drop-in replacement for TDM64-GCC, whose last release is from 2021.

## Building the Windows GUI application

You'll need Visual C# **2022** or later installed.

From Visual Studio:

1. Open the Visual Studio solution file `UDT_GUI\UDT.sln`.
2. Select your settings (debug or release) and build.

From the command prompt:

1. Run `UDT_GUI\vs_compile_release.cmd`

| Data | Location |
| :--- | :--- |
| Output files | `UDT_GUI\.bin` |
| Temporary build files | `UDT_GUI\.build` |
