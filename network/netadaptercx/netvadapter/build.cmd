@echo off
REM ============================================================================
REM Build wrapper for netvadapter.sln
REM
REM The netvadapter UM driver targets UMDF 2.33 (matching netvadapterum.inf's
REM UmdfLibraryVersion = 2.33.0). It links the shared netvadapterlibrary, whose
REM checked-in project targets UMDF 2.35 (so the wificx sample is unaffected).
REM
REM WDF version must match within a single binary, so this script forces the
REM library + driver to 2.33 *for this solution's build only* via a global
REM MSBuild property. A plain "msbuild netvadapter.sln" (without these props)
REM will fail to link with an unresolved WdfFunctions_02035 symbol.
REM
REM Usage:   build.cmd [Configuration] [Platform]   (defaults: Debug x64)
REM Example: build.cmd Release x64
REM ============================================================================

setlocal
set CONFIG=%~1
set PLAT=%~2
if "%CONFIG%"=="" set CONFIG=Debug
if "%PLAT%"=="" set PLAT=x64

msbuild "%~dp0netvadapter.sln" /t:Build /p:Configuration=%CONFIG% /p:Platform=%PLAT% /p:UMDF_VERSION_MINOR=33 /p:UMDF_MINIMUM_VERSION_REQUIRED=33 /m
endlocal
