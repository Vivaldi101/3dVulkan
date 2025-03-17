@echo off

if not exists "%cd%\bin\assets\shaders" mkdir "%cd%\bin\assets\shaders"

echo "Compiling shaders"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/

