@echo off

echo Running shader_build.bat > shader_build.log

cd /d "%~dp0"

IF NOT EXIST "%cd%\bin\assets\shaders" mkdir "%cd%\bin\assets\shaders"

echo "Compiling shaders..."

%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/Builtin.ObjectShader.vert.glsl -o bin/assets/shaders/Builtin.ObjectShader.vert.spv

IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL%)

%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/Builtin.ObjectShader.frag.glsl -o bin/assets/shaders/Builtin.ObjectShader.frag.spv

IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL%)

echo "Copying assets..."
echo xcopy "assets" "bin\assets" /h /i /c /k /e /r /y
xcopy "assets" "bin\assets" /h /i /c /k /e /r /y
