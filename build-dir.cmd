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
set name=%1
set fullpath=%2
set name=%name:"=%
set fullpath=%fullpath:"=%
pushd "%name%"
if exist *.sln (
    echo %fullpath%
    echo === %fullpath% >> %logfile%

    for /f %%i in ('dir /b *.sln') do (
        msbuild /nologo /v:q /m /t:rebuild %%i >> %logfile%
    )
) else (
    for /d %%i in (*) do (
        call :check_folder "%%i" "%fullpath%/%%i"
    )
)
popd
