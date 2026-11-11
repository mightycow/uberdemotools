@echo off
if not defined UDT_VM_NAME set "UDT_VM_NAME=Debian"
@echo on
wsl -d %UDT_VM_NAME% ./lin_lin_gmake_gcc.sh
pause