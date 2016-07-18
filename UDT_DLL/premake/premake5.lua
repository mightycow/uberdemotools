path_root = ".."
path_src_core = path_root.."/src"
path_src_apps = path_root.."/src/apps"
path_inc = path_root.."/include"
path_libs = path_root.."/libs"
path_build = path_root.."/.build"
path_bin = path_root.."/.bin"

local function SetTargetAndLink(option) 

	targetdir(option)
	libdirs(option)

end

local function ApplyTargetAndLinkSettings() 

	filter { "configurations:Debug", "platforms:x32" }
		SetTargetAndLink ( path_bin.."/".._ACTION.."/x86/debug" )
		
	filter { "configurations:Debug", "platforms:x64" }
		SetTargetAndLink ( path_bin.."/".._ACTION.."/x64/debug" )
	
	-- Release, ReleaseInst, ReleaseOpt	
	filter { "configurations:Release*", "platforms:x32" }
		SetTargetAndLink ( path_bin.."/".._ACTION.."/x86/release" )
		
	-- Release, ReleaseInst, ReleaseOpt
	filter { "configurations:Release*", "platforms:x64" }
		SetTargetAndLink ( path_bin.."/".._ACTION.."/x64/release" )

end

local function ApplyProjectSettings() 

	--
	-- General
	--
	filter { }

	language "C++"

	location ( path_build.."/".._ACTION )

	files { path_src_core.."/*.cpp", path_src_core.."/*.hpp", path_inc.."/*.h" }
	includedirs { path_src_core, path_src_apps, path_inc }

	rtti "Off"
	exceptionhandling "Off"
	flags { "Unicode", "NoPCH", "StaticRuntime", "NoManifest", "ExtraWarnings" } -- "FatalWarnings"
	
	-- The PG instrumented and PG optimized builds need to share their .obj files.
	filter { "configurations:ReleaseInst", "platforms:x32" }
		objdir "!../.build/vs_pgo/obj/x32/ReleaseInst/%{prj.name}"
	filter { "configurations:ReleaseOpt", "platforms:x32" }
		objdir "!../.build/vs_pgo/obj/x32/ReleaseInst/%{prj.name}"
	filter { "configurations:ReleaseInst", "platforms:x64" }
		objdir "!../.build/vs_pgo/obj/x64/ReleaseInst/%{prj.name}"
	filter { "configurations:ReleaseOpt", "platforms:x64" }
		objdir "!../.build/vs_pgo/obj/x64/ReleaseInst/%{prj.name}"
	
	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		flags { }

	-- Release, ReleaseInst, ReleaseOpt
	filter "configurations:Release*"
		defines { "NDEBUG" }
		flags -- others: NoIncrementalLink NoCopyLocal NoImplicitLink NoBufferSecurityChecks
		{ 
			"NoMinimalRebuild", 
			"Optimize", 
			"NoFramePointer", 
			"EnableSSE2", 
			"FloatFast", 
			"LinkTimeOptimization", 
			"MultiProcessorCompile", 
			"NoRuntimeChecks" 
		}
	
	ApplyTargetAndLinkSettings()
		
	filter "system:windows"
		defines { "WIN32" }
		links { "Winmm" }
		
	filter "system:not windows"
		links { "pthread", "rt" }

	--
	-- Visual Studio
	--
	
	-- Some build options:
	-- /GT  => Support Fiber-Safe Thread-Local Storage
	-- /GS- => Buffer Security Check disabled
	-- /GL  => Whole Program Optimization
	
	filter "action:vs*"
		flags { "Symbols" }
		defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
		
	filter { "action:vs*", "kind:ConsoleApp" }
		entrypoint "wmainCRTStartup"
		
	filter { "action:vs*", "kind:WindowedApp" }
		flags "WinMain"
	
	filter { "action:vs*", "configurations:Debug" }
		buildoptions { "" }
		linkoptions { "" }
		
	filter { "action:vs*", "configurations:Release" }
		buildoptions { "/GS-", "/GL" }
		linkoptions { "/OPT:REF", "/OPT:ICF" }
		
	filter { "action:vs*", "configurations:ReleaseInst" }
		buildoptions { "/GS-", "/GL" }
		linkoptions { "/OPT:REF", "/OPT:ICF", "/LTCG:PGINSTRUMENT" }
		
	filter { "action:vs*", "configurations:ReleaseOpt" }
		buildoptions { "/GS-", "/GL" }
		linkoptions { "/OPT:REF", "/OPT:ICF", "/LTCG:PGOPTIMIZE" }
		
	filter "action:vs2015"
		buildoptions { "/wd4577" --[[ noexcept --]] }
		linkoptions { "" }
		
	--
	-- GCC
	--

	filter { "action:gmake", "system:windows" }
		buildoptions { "" }
		linkoptions { "-municode" } -- This is to define the Unicode wmain entry point on MingW to get access to the UTF16 Unicode command-line.
		defines { "_WIN32_WINNT=0x0601", "WINVER=0x0601", "NTDDI_VERSION=0x06010000" } -- We build on and target Windows 7 at a minimum.

	filter "action:gmake"
		buildoptions { "-std=c++11 -Wno-invalid-offsetof -Wno-narrowing" }
		linkoptions { "" }
		
	filter { "action:gmake", "configurations:Debug" }
		buildoptions { "" }
		linkoptions { "" }
		
	filter { "action:gmake", "configurations:Release" }
		buildoptions { "" }
		linkoptions { "" }

end

local function ApplyTutorialProjectSettings()

	filter { }
	kind "ConsoleApp"
	language "C++"
	location ( path_build.."/".._ACTION )
	includedirs { path_src_apps, path_inc }
	rtti "Off"
	exceptionhandling "On"
	flags { "Symbols", "NoPCH", "StaticRuntime", "NoManifest", "ExtraWarnings" }
	links { "UDT" }
	
	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		
	filter "configurations:Release"
		defines { "NDEBUG" }
		
	ApplyTargetAndLinkSettings()
	
	filter "system:windows"
		defines { "WIN32" }
		
	filter "action:vs*"
		defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
		
	filter "action:gmake"
		buildoptions { "-std=c++11 -pedantic" }

end

os.mkdir(path_bin)

solution "UDT"

	location ( path_build.."/".._ACTION )
	platforms { "x32", "x64" }
	configurations { "Debug", "Release", "ReleaseInst", "ReleaseOpt" }

	project "UDT"
	
		kind "SharedLib"
		defines { "UDT_CREATE_DLL" }
		ApplyProjectSettings()

	project "UDT_cutter"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_cutter.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	project "UDT_splitter"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_splitter.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	project "UDT_timeshifter"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_time_shifter.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	project "UDT_merger"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_merger.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()

	project "UDT_json"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_json.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	project "UDT_captures"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_captures.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	project "UDT_converter"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/app_demo_converter.cpp" }
		files { path_src_apps.."/shared.cpp" }
		ApplyProjectSettings()
		
	-- This project exists only to test the API in C89 mode to ensure nothing got messed up for C programmers.
	project "UDT_c89"
	
		filter { }
		kind "ConsoleApp"
		language "C"
		location ( path_build.."/".._ACTION )
		files { path_src_apps.."/app_c89.c" }
		includedirs { path_src_apps, path_inc }
		rtti "Off"
		exceptionhandling "Off"
		flags { "Symbols", "NoPCH", "StaticRuntime", "NoManifest", "ExtraWarnings" }
		links { "UDT" }
		filter "configurations:Debug"
			defines { "DEBUG", "_DEBUG" }
		filter "configurations:Release"
			defines { "NDEBUG" }
		ApplyTargetAndLinkSettings()
		filter "system:windows"
			defines { "WIN32" }
		filter "action:vs*"
			defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
			--buildoptions { "/Za" } -- /Za: disable language extensions
		filter "action:gmake"
			buildoptions { "-std=c89 -pedantic" } -- -ansi is used to force ISO C90 mode in GCC
			
	project "tut_multi_rail"
	
		filter { }
		files { path_src_apps.."/tut_multi_rail.cpp" }
		ApplyTutorialProjectSettings()
		
	project "tut_players"
	
		filter { }
		files { path_src_apps.."/tut_players.cpp" }
		ApplyTutorialProjectSettings()
		
	project "UDT_viewer"
	
		kind "WindowedApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_core.."/viewer/*.cpp" }
		files { path_src_core.."/viewer/*.hpp" }
		ApplyProjectSettings()
		filter "system:windows"
			links { "D3D11" }
		filter "system:not windows"
			links { "GL", "glfw" }
		filter "action:gmake"
			buildoptions { "-g" } -- Generate debug symbols.
			linkoptions { "-rdynamic" } -- Embed the debug symbols in the executable.
	
	-- OpenGL version of the Windows viewer for testing purposes
	project "UDT_viewer_glfw"
	
		kind "WindowedApp"
		defines { "UDT_CREATE_DLL", "UDT_VIEWER_WINDOWS_GLFW" }
		files { path_src_core.."/viewer/*.cpp" }
		files { path_src_core.."/viewer/*.hpp" }
		ApplyProjectSettings()
		filter "system:windows"
			links { "OpenGL32", "glew32", "glfw3dll" }
			filter "platforms:x32"
				libdirs ( path_libs.."/x86" )
			filter "platforms:x64"
				libdirs ( path_libs.."/x64" )
		
	project "viewer_data_gen"
	
		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL", "UDT_DONT_RESET_CD" }
		files { path_src_core.."/viewer_data_gen/*.cpp" }
		files { path_src_core.."/viewer_data_gen/*.hpp" }
		files { path_src_apps.."/shared.cpp" }
		includedirs { path_src_core.."/viewer" }
		ApplyProjectSettings()
