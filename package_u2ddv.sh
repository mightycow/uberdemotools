#!/bin/bash
tar cvf U2DDV_lin.tar.lzma --lzma -p --exclude .svn changelog_dv.txt --xform s:UDT_DV/legal:: UDT_DV/legal/ --xform s:UDT_DV/data:data: UDT_DV/data/ --xform s:UDT_DV/bin/gcc/release:bin: UDT_DV/bin/gcc/release/U2DDV --xform s:UDT_DV/linux_bin_script:bin: UDT_DV/linux_bin_script/ --xform s:UDT_DV/qt/lib_gmake:bin: UDT_DV/qt/lib_gmake/
