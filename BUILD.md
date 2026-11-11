# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

## Building the library, CLI tools and viewer

This is all done through scripts that you can invoke from your command prompt or shell.

For each toolchain, there is a single script that handles the following duties:

1. Generating project files by invoking a local version of Premake.
2. Compiling all the projects.
3. Running the test suite.

| Data | Location |
| :--- | :--- |
| Build scripts | `UDT_DLL/premake` |
| Output files | `UDT_DLL/.bin/${toolchainfolder}` |
| Temporary build files | `UDT_DLL/.build/${toolchainfolder}` |
| Visual Studio solution file | `UDT_DLL/.build/win_win_vs${year}/UDT.sln` |

| Host | Target | Toolchain | Script | Folder | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Windows | Windows | Visual C++ | `win_win_vs.cmd` | `win_win_vs${year}` | VC++ **2017** or later |
| Windows | Windows | MingW + GCC | `win_win_mingw_gcc.cmd` | `win_win_mingw_gcc` | Can use WinLibs |
| Windows | Windows | Ninja + Zig | `win_win_ninja_zig.cmd` | `win_win_ninja_zig` | Clang |
| Windows | Linux | Ninja + Zig | `win_lin_ninja_zig.cmd` | `win_lin_ninja_zig` | Clang |
| WSL | Linux | gmake + GCC | `wsl_lin_gmake_gcc.cmd` | `lin_lin_gmake_gcc` | |
| Linux | Linux | gmake + GCC | `lin_lin_gmake_gcc.sh` | `lin_lin_gmake_gcc` | |

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
