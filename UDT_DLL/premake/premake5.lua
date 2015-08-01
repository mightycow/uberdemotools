path_root = ".."
path_src_core = path_root.."/src"
path_src_apps = path_root.."/src/apps"
path_inc = path_root.."/include"
path_build = path_root.."/.build"
path_bin = path_root.."/.bin"

local function ApplyProjectSettings() 

	--
	-- General
	--
	filter { }

	language "C++"

	location ( path_build.."/".._ACTION )

	files { path_src_core.."/*.cpp", path_src_core.."/*.hpp", path_inc.."/*.h", path_inc.."/*.hpp" }
	includedirs { path_src_core, path_src_apps, path_inc }

	flags { "Symbols", "NoNativeWChar", "NoPCH", "NoRTTI", "StaticRuntime", "NoManifest", "ExtraWarnings", "FatalWarnings", "NoExceptions" }
	
	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }
		flags { }

	filter "configurations:Release"
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
	
	filter { "configurations:Debug", "platforms:x32" }
		targetdir ( path_bin.."/".._ACTION.."/x86/debug" )
		
	filter { "configurations:Debug", "platforms:x64" }
		targetdir ( path_bin.."/".._ACTION.."/x64/debug" )
		
	filter { "configurations:Release", "platforms:x32" }
		targetdir ( path_bin.."/".._ACTION.."/x86/release" )
		
	filter { "configurations:Release", "platforms:x64" }
		targetdir ( path_bin.."/".._ACTION.."/x64/release" )
		
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
	
	filter "action:vs*"
		defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
	
	filter { "action:vs*", "configurations:Debug" }
		buildoptions { "" } -- Directly passed to the compiler.
		linkoptions { "" } -- Directly passed to the linker.
		
	filter { "action:vs*", "configurations:Release" }
		buildoptions { "/GS-" } -- Directly passed to the compiler.
		linkoptions { "/OPT:REF", "/OPT:ICF" } -- Directly passed to the linker.
		
	--
	-- GCC
	--
	filter "action:gmake"
		buildoptions { "-std=c++11 -Wno-invalid-offsetof -Wno-narrowing" } -- Directly passed to the compiler.
		linkoptions { "-rdynamic" } -- Directly passed to the linker.
	
	filter { "action:gmake", "configurations:Debug" }
		buildoptions { "" } -- Directly passed to the compiler.
		linkoptions { "" } -- Directly passed to the linker.
		
	filter { "action:gmake", "configurations:Release" }
		buildoptions { "" } -- Directly passed to the compiler.
		linkoptions { "" } -- Directly passed to the linker.
	
end

os.mkdir(path_bin)

solution "UDT"

	location ( path_build.."/".._ACTION )
	platforms { "x32", "x64" }
	configurations { "Debug", "Release" }

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
		files { path_src_apps.."/json_writer.cpp" }
		files { path_src_apps.."/json_writer.hpp" }
		ApplyProjectSettings()
			
			
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
