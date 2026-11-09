@echo off
setlocal
cd /d "%~dp0"
if not exist results mkdir results
build\generator.exe data
if errorlevel 1 exit /b 1
build\lab4.exe data > results\results.txt
if errorlevel 1 exit /b 1
type results\results.txt
echo RUN FINISHED: output saved to results\results.txt
