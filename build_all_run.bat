@echo off
call find-cl.bat
call shader_build.bat
call code\build.bat a
echo Build all done!
echo Launching release build...

call build\release_vulkan
