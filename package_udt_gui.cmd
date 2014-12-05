PATH=%PATH%;C:\Program Files\WinRAR
WinRAR.exe a -ep -rr10p -zpackage_udt_gui_bin_comment.txt udt_bin_x86.rar @package_udt_gui_bin_x86.lst
WinRAR.exe a -ep -rr10p -zpackage_udt_gui_bin_comment.txt udt_bin_x64.rar @package_udt_gui_bin_x64.lst
WinRAR.exe a -rr10p -zpackage_udt_gui_src_comment.txt udt_src.rar @package_udt_gui_src.lst
pause