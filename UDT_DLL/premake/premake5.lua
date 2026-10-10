-- premake version: 5.0.0-beta8

path_root = ".."
path_src_core = path_root.."/src"
path_src_apps = path_root.."/src/apps"
path_inc = path_root.."/include"
path_libs = path_root.."/libs"
path_build = path_root.."/.build"
path_bin = path_root.."/.bin"
path_natvis = path_root.."/src/natvis"

local function SetAdd(set, key)
	if set ~= nil then
		set[key] = true
	end
end

local function SetRemove(set, key)
	if set ~= nil then
		set[key] = nil
	end
end

local function SetContains(set, key)
	if set ~= nil then
		return set[key] ~= nil
	end
	return false
end

local function SetTargetAndLink(option) 

	targetdir(option)
	libdirs(option)

end

local function GetAction()

	if _ACTION == "gmake" and os.host() == "windows" and os.target() == "windows" then
		return "gmake_windows"
	end

	if _ACTION == "gmake" and os.host() == "windows" and os.target() == "linux" then
		return "gmake_linux"
	end

	return _ACTION

end

local function ApplyTargetAndLinkSettings() 

	filter { "configurations:Debug", "platforms:x64" }
		SetTargetAndLink ( path_bin.."/"..GetAction().."/x64/debug" )

	filter { "configurations:Release", "platforms:x64" }
		SetTargetAndLink ( path_bin.."/"..GetAction().."/x64/release" )

end

local function ApplyProjectSettings(optionsSet) 

	--
	-- General
	--
	filter { }

	language "C++"

	location ( path_build.."/"..GetAction() )

	files { path_src_core.."/*.cpp", path_src_core.."/*.hpp", path_inc.."/*.h", path_natvis.."/*.natvis" }
	includedirs { path_src_core, path_src_apps, path_inc }

	rtti "Off"
	if SetContains(optionsSet, "exceptions") then
		exceptionhandling "SEH"
	else
		exceptionhandling "Off"
	end
	characterset "Unicode"
	staticruntime "On"
	manifest "Off"
	warnings "Extra" --- "Extra", "High", "Everything"
	vectorextensions "SSE2"
	floatingpoint "Fast"
	multiprocessorcompile "On"

	filter "configurations:Debug"
		defines { "DEBUG", "_DEBUG" }

	-- Release
	filter "configurations:Release"
		defines { "NDEBUG" }
		minimalrebuild "Off"
		optimize "Full"
		omitframepointer "On"
		runtimechecks "Off"

	filter { "configurations:Release", "action:vs*" }
		linktimeoptimization "On"

	filter { }
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
		symbols "Full"
		defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
		debugdir "$(TargetDir)"

	filter { "action:vs*", "kind:ConsoleApp" }
		entrypoint "wmainCRTStartup"

	--[[
	filter { "action:vs*", "kind:WindowedApp" }
		flags "WinMain"
	--]]

	filter { "action:vs*", "configurations:Debug" }
		buffersecuritycheck "On"

	filter { "action:vs*", "configurations:Release" }
		buffersecuritycheck "Off"
		buildoptions { "/GL" }
		linkoptions { "/OPT:REF", "/OPT:ICF" }

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
	location ( path_build.."/"..GetAction() )
	includedirs { path_src_apps, path_inc }
	rtti "Off"
	exceptionhandling "On"
	staticruntime "On"
	manifest "Off"
	warnings "Extra" --- "Extra", "High", "Everything"
	symbols "Full"
	enablepch "Off"
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

workspace "UDT"

	location ( path_build.."/"..GetAction() )
	platforms { "x64" }
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
		location ( path_build.."/"..GetAction() )
		files { path_src_apps.."/app_c89.c" }
		includedirs { path_src_apps, path_inc }
		rtti "Off"
		exceptionhandling "Off"
		symbols "Full"
		staticruntime "On"
		manifest "Off"
		warnings "Extra" --- "Extra", "High", "Everything"
		enablepch "Off"
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
		filter "action:gmake"
			buildoptions { "-g" } -- Generate debug symbols.
			linkoptions { "-rdynamic" } -- Embed the debug symbols in the executable.

	if os.target() == "windows" then

		-- OpenGL version of the Windows viewer for testing purposes
		project "UDT_viewer_glfw"

			kind "WindowedApp"
			defines { "UDT_CREATE_DLL", "UDT_VIEWER_WINDOWS_GLFW" }
			files { path_src_core.."/viewer/*.cpp" }
			files { path_src_core.."/viewer/*.hpp" }
			ApplyProjectSettings()
			libdirs ( path_libs.."/x64" )

	end

	project "viewer_data_gen"

		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL", "UDT_DONT_RESET_CD" }
		files { path_src_core.."/viewer_data_gen/*.cpp" }
		files { path_src_core.."/viewer_data_gen/*.hpp" }
		files { path_src_apps.."/shared.cpp" }
		includedirs { path_src_core.."/viewer" }
		ApplyProjectSettings()

	project "tests"

		kind "ConsoleApp"
		defines { "UDT_CREATE_DLL" }
		files { path_src_apps.."/tests*" }
		files { path_src_apps.."/json.h" }
		ApplyProjectSettings( { exceptions=true } )
		filter "system:windows"
			links { "Shlwapi" }

local function GMakeOverride(base, prj, cfg)
	-- run original generator
	base(prj, cfg)

	-- make absolutely sure we use a Windows shell and not some random shell on the system called sh.exe
	if prj.system == "windows" then
		_p("SHELL := cmd.exe")
		_p("SHELLTYPE := windows")
	end
end

if _ACTION == "gmake" then
	premake.override(premake.modules.gmake, "header", GMakeOverride)
end
