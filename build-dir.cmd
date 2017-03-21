@echo off
setlocal
set logfile=%cd%\build-result.log
del %logfile% 2>nul
call :check_folder . .
goto :eof

rem =========================================
rem check_folder
rem   $1 - sub-folder name
rem   $2 - full path to the sub-folder
rem
:check_folder
pushd %1
if exist *.sln (
    echo %2
    echo === %2 >> %logfile%

    for /f %%i in ('dir /b *.sln') do (
        msbuild /nologo /v:q /m /t:rebuild %%i >> %logfile%
    )
) else (
    for /f %%i in ('dir /b /ad') do call :check_folder %%i %2/%%i
)
popd
