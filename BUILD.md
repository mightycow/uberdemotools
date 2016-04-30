# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

Here you'll learn the few steps required to build the UDT binaries for your platform and compiler.

Windows + Visual C++
--------------------

The steps:

1. You'll need **Visual C++ 2010**<sup>[a]</sup> or later installed (the codebase now uses some C++11 features).
2. Navigate to `UDT_DLL\premake`.
3. Run `vs_generate.cmd` and follow the instructions.
4. Run `vs_compile.cmd` and follow the instructions.

Notes:

* The temporary build files will be found in `UDT_DLL\.build\vs${year}`.
* The output files will be found in `UDT_DLL\.bin\vs${year}`.
* You can also open the solution file generated in step 3 at `UDT_DLL\.build\vs${year}\UDT.sln` to program changes and/or build from the IDE.

a) I'm not entirely sure which VC++ verson is required at a minimum because I didn't get to try an older version than VC++ 2013 yet.

Windows + GCC
-------------

I highly recommend the use of the **TDM-GCC** MingW GCC distribution found here: http://tdm-gcc.tdragon.net/download.
If you run a 64-bit system, make sure you to grab a **tdm64-gcc** release so you can build both 32-bit and 64-bit builds.

The steps:

1. You'll need some version of MingW GCC installed with the binaries in your **PATH**<sup>[a]</sup>.
2. Navigate to `UDT_DLL\premake`.
3. Run `gcc_generate.cmd`.
4. Run `gcc_compile.cmd` and follow the instructions.

Notes:

* The temporary build files will be found in `UDT_DLL\.build\gmake`.
* The output files will be found in `UDT_DLL\.bin\gmake`.

a) That, or do a local update of your **PATH** in the command prompt before invoking the batch files like this: `PATH=extra_folder;%PATH%`.

Linux + GCC
-----------

The steps:

1. You'll need all the basic build tools installed.
2. Navigate to `UDT_DLL\premake`.
3. Run `gcc_generate.sh`.
4. Run `gcc_compile.sh` and follow the instructions.

Notes:

* The temporary build files will be found in `UDT_DLL\.build\gmake`.
* The output files will be found in `UDT_DLL\.bin\gmake`.

Windows + Visual C# (GUI application)
-------------------------------------

The steps:

1. You'll need **Visual C# 2013** or later installed.
2. Open the Visual Studio solution file `UDT_GUI\UDT.sln`.
3. Select your settings (x86 or x64, debug or release) and build.

Notes:

* The temporary build files will be found in `UDT_GUI\.build`.
* The output file will be found in `UDT_GUI\.bin`.

Additional information
----------------------

For the C++ code, the repository doesn't host any makefile. Instead, it has a **premake** Lua script used to either generate *Visual Studio* solution and project files or makefiles for *GNU Make*.  
You don't need to install it nor download it, the specific alpha builds of **premake** needed for Windows and Linux are in the repository already.
