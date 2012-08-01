-----------
-- U2DDV --
-----------
os.mkdir("../bin")

-- Option QtUseSystemLibs
newoption 
{
	trigger     = "QtUseSystemLibs",
	value       = "false",
	description = "Use the local system's Qt libs/includes?",
	allowed = 
	{
		{ "false", "Use the provided Qt libs/includes (default behavior)" },
		{ "true",  "Use the local system's Qt libs/includes" },
	}
}
if not _OPTIONS.QtUseSystemLibs then
	_OPTIONS.QtUseSystemLibs = "false"
end
print("QtUseSystemLibs? " .. _OPTIONS.QtUseSystemLibs)

-- Define all the data required by option QtUseSystemLibs
local uber_qt_paths_all = {}
uber_qt_paths_all["false"]  =
{
	path_from_makefile = "../../qt/bin",
	include_path = "../qt/include",
	lib_path = "../qt/lib_" .. _ACTION,
	core_lib_name = "QtCore4",
	gui_lib_name = "QtGui4",
}
uber_qt_paths_all["true"]  =
{
	path_from_makefile = "/usr/bin",
	include_path = "/usr/include/qt4",
	lib_path = "/usr/lib",
	core_lib_name = "libQtCore.so.4",
	gui_lib_name = "libQtGui.so.4",
}

-- Select the data we must use
local uber_qt_paths = uber_qt_paths_all[_OPTIONS.QtUseSystemLibs]
if uber_qt_paths == nil then
	error("Don't know where to look for Qt files!")
end

uber_ensure_file_exists = function(file_path)
	if not os.isfile(file_path) then
		os.copyfile("template_empty", file_path)
	end
end

uber_ensure_qt_generated_file_exists = function(file_name)
	return uber_ensure_file_exists("../src/qt_gen/" .. file_name)
end

uber_ensure_qt_generated_file_exists("gui_gen.hpp")
uber_ensure_qt_generated_file_exists("gui_gen.cpp")
uber_ensure_qt_generated_file_exists("demo_player_gen.cpp")
uber_ensure_qt_generated_file_exists("paint_widget_gen.cpp")
uber_ensure_qt_generated_file_exists("rsrc_gen.cpp")

solution "U2DDV"

	location ("../build/" .. _ACTION)
	platforms { "x32" }
	configurations { "Debug", "Release" }

	project "U2DDV"

		uber_add_pre_build_command = function(tool_name, input_file, output_file)
			prebuildcommands { path.translate(uber_qt_paths.path_from_makefile .. "/" .. tool_name .. " -o ../../src/qt_gen/" .. output_file .. " ../../src/qt/" .. input_file, nil) }
		end
		
		uber_add_pre_build_command("uic", "gui.ui", "gui_gen.hpp")
		uber_add_pre_build_command("moc", "gui.h", "gui_gen.cpp")
		uber_add_pre_build_command("moc", "demo_player.h", "demo_player_gen.cpp")
		uber_add_pre_build_command("moc", "paint_widget.h", "paint_widget_gen.cpp")
		
		-- Create rsrc_gen.cpp from the resources listed in U2DDV.qrc and create a resource load function invoked like this: Q_INIT_RESOURCE(U2DDV);
		prebuildcommands { path.translate(uber_qt_paths.path_from_makefile .. "/rcc -name U2DDV -o ../../src/qt_gen/rsrc_gen.cpp ../../rsrc/U2DDV.qrc", nil) }

		location ("../build/" .. _ACTION)
		kind "WindowedApp"
		language "C++"

		files { "../src/*.cpp", "../src/*.hpp", "../src/qt/*.cpp", "../src/qt/*.h", "../src/qt/*.ui", "../src/qt_gen/*.cpp", "../src/qt_gen/*.hpp" } -- Local sources
		files { "../../UDT_DLL/src/*.cpp", "../../UDT_DLL/src/*.hpp" } -- DLL project sources included directly
		excludes { "../../UDT_DLL/src/api.cpp" } -- Make sure we don't even try to compile api.cpp (for DLL function export)
		includedirs { "../src", "../src/qt", "../src/qt_gen" } -- Local headers
		includedirs { "../../UDT_DLL/src" } -- DLL project headers
		includedirs { uber_qt_paths.include_path }
		includedirs { uber_qt_paths.include_path .. "/Qt" }
		includedirs { uber_qt_paths.include_path .. "/QtCore" }
		includedirs { uber_qt_paths.include_path .. "/QtGui" }
		libdirs { uber_qt_paths.lib_path }

		flags { "Symbols", "NoNativeWChar", "NoPCH" } -- "NoRTTI", "StaticRuntime", "NoManifest", "ExtraWarnings", "FatalWarnings"
		
		configuration "Debug"
			defines { "DEBUG", "_DEBUG" }
			flags { }
			includedirs { "" }

		configuration "Release"
			defines { "NDEBUG" }
			flags { "NoMinimalRebuild", "Optimize", "NoFramePointer", "EnableSSE2", "FloatFast" } -- "NoIncrementalLink"	
			
		--
		-- Visual Studio
		--
		configuration { "vs*" }
			defines { "_CRT_SECURE_NO_WARNINGS", "WIN32" }
			files { "../rsrc/icon.rc" } -- Application icon
		
		configuration { "Debug", "vs*" }
			targetdir ( "../bin/" .. _ACTION .. "/debug" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			links { "qtmaind" }
			links { "QtCored4" }
			links { "QtGuid4" }
			
		configuration { "Release", "vs*" }
			targetdir ( "../bin/" .. _ACTION .. "/release" )
			buildoptions { "/Ot", "/GT", "/GS-" } -- Directly passed to the compiler.
			linkoptions { "/OPT:REF", "/OPT:ICF" } -- Directly passed to the linker.
			links { "qtmain" }
			links { "QtCore4" }
			links { "QtGui4" }
			
		--
		-- GCC
		--
		configuration { "gmake" }
			links { uber_qt_paths.core_lib_name }
			links { uber_qt_paths.gui_lib_name }
		
		configuration { "Debug", "gmake" }
			targetdir ( "../bin/gcc/debug" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			postbuildcommands { "chmod +x ../../bin/gcc/debug/U2DDV" }
			
		configuration { "Release", "gmake" }
			targetdir ( "../bin/gcc/release" )
			buildoptions { "" } -- Directly passed to the compiler.
			linkoptions { "" } -- Directly passed to the linker.
			postbuildcommands { "chmod +x ../../bin/gcc/release/U2DDV" }

