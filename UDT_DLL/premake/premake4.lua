---------
-- UDT --
---------
os.mkdir("../bin")

solution "UDT"

	location ("../build/" .. _ACTION)
	platforms { "x32" }
	configurations { "Debug", "Release" }

	project "UDT"

		location ("../build/" .. _ACTION)
		kind "SharedLib"
		language "C++"

		files { "../src/*.cpp", "../src/*.hpp" }
		includedirs { "../src" }

		defines { "UDT_CREATE_DLL", "_CRT_SECURE_NO_WARNINGS", "WIN32" }
		flags { "Symbols", "NoNativeWChar", "NoPCH", "NoRTTI", "StaticRuntime", "NoManifest", "ExtraWarnings", "FatalWarnings" }
		
		configuration "Debug"
			defines { "DEBUG", "_DEBUG" }
			flags { }

		configuration "Release"
			defines { "NDEBUG" }
			flags { "NoMinimalRebuild", "Optimize", "NoFramePointer", "EnableSSE2", "FloatFast" } -- "NoIncrementalLink"
			
		--
		-- Visual Studio
		--
		configuration { "Debug", "vs*" }
			targetdir ( "../bin/" .. _ACTION .. "/debug" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			
		configuration { "Release", "vs*" }
			targetdir ( "../bin/" .. _ACTION .. "/release" )
			buildoptions { "/Ot", "/GT", "/GS-" } -- Directly passed to the compiler.
			linkoptions { "/OPT:REF", "/OPT:ICF" } -- Directly passed to the linker.
			
		--
		-- GCC
		--
		configuration { "Debug", "gmake" }
			targetdir ( "../bin/gcc/debug" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			
		configuration { "Release", "gmake" }
			targetdir ( "../bin/gcc/release" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
	


























	