@echo off
call find-cl.bat
call shader_build.bat

call code\build.bat a

echo Build all done!
