@echo off
setlocal
cd /d "%~dp0"
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist build\obj mkdir build\obj
set "FLAGS=/nologo /std:c++20 /EHsc /W4 /permissive- /utf-8 /DNDEBUG /MD /O2 /Iinclude /Fobuild\obj\"
echo Building Release generator
cl %FLAGS% src\generator.cpp /Febuild\generator.exe
if errorlevel 1 exit /b 1
echo Building Release lab4
cl %FLAGS% src\main.cpp /Febuild\lab4.exe
if errorlevel 1 exit /b 1
echo BUILD SUCCEEDED: build\generator.exe, build\lab4.exe
exit /b 0
