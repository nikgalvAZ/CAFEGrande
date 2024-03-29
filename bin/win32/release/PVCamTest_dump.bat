@echo off

set APP=PVCamTest.exe

set PROCDUMP=procdump.exe
if not exist "%PROCDUMP%" (
    echo The ProcDump tool not found.
    echo Please download it into the same folder as application from:
    echo     https://technet.microsoft.com/en-us/sysinternals/dd996900.aspx
    echo.
    pause
    exit /B 1
)
start "Watching %APP%" "%APP%" %*
"%PROCDUMP%" -ma -e -w -accepteula "%APP%"
