:: WFPSamplerInstall.cmd
::    Command line script to install the WFPSampler

:SoF

   @ECHO OFF
   SETLOCAL ENABLEEXTENSIONS
   SETLOCAL ENABLEDELAYEDEXPANSION

:Variables

   :: Help
   IF /I "%1" EQU ""   ( GOTO :Usage ) ELSE (
   IF /I "%1" EQU "?"  ( GOTO :Usage ) ELSE (
   IF /I "%1" EQU "-?" ( GOTO :Usage ) ELSE (
   IF /I "%1" EQU "/?" ( GOTO :Usage ))))

   :: Removal
   IF /I "%1" EQU "r"  ( GOTO :Uninstall ) ELSE (
   IF /I "%1" EQU "-r" ( GOTO :Uninstall ) ELSE (
   IF /I "%1" EQU "/r" ( GOTO :Uninstall )))

   :: Architecture
   IF /I "%1" EQU ""      ( GOTO :Usage )    ELSE (
   IF /I "%1" EQU "amd64" (SET WFP_ARCH=x64) ELSE (
   IF /I "%1" EQU "i386"  (SET WFP_ARCH=x86) ELSE (
   IF /I "%1" EQU "x64"   (SET WFP_ARCH=%1)  ELSE (
   IF /I "%1" EQU "x86"   (SET WFP_ARCH=%1)  ELSE ( GOTO :BinPath )))))

   :: OS
   IF /I "%2" EQU ""       ( GOTO :Usage ) ELSE (
   IF /I "%2" EQU "Win7"   (SET WFP_OS=%2) ELSE (
   IF /I "%2" EQU "Win8"   (SET WFP_OS=%2) ELSE (
   IF /I "%2" EQU "Win8.1" (SET WFP_OS=%2) ELSE ( GOTO :Usage ))))

   :: Optimized
   IF /I "%3" EQU ""        ( GOTO :Usage )  ELSE (
   IF /I "%3" EQU "Debug"   (SET WFP_OPZ=%3) ELSE (
   IF /I "%3" EQU "Release" (SET WFP_OPZ=%3) ELSE ( GOTO :Usage )))

:BinPath

   IF "%PROCESSOR_ARCHITECTURE%" EQU "AMD64" (SET _PROCESSOR_ARCHITECTURE_=x64) ELSE (
   IF "%PROCESSOR_ARCHITECTURE%" EQU "I386"  (SET _PROCESSOR_ARCHITECTURE_=x86) ELSE (
                                              SET _PROCESSOR_ARCHITECTURE_=%PROCESSOR_ARCHITECTURE%))

   IF DEFINED WFP_OS (
      :: Use a path relative to where this script is located for finding the binaries
      FOR /F %%A IN ('ECHO "%~dp0" ^| FIND /I "network\trans\WFPSampler\scripts\"') DO (SET SCRIPTS_DIR=TRUE)

      IF DEFINED SCRIPTS_DIR (
         :: Running from Network\Trans\WFPSampler\Scripts
         SET WFPSAMPLER_EXE="%~dp0..\exe\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\WFPSampler.exe"
         SET WFPSAMPLERSERVICE_EXE="%~dp0..\svc\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\WFPSamplerService.exe"
         SET WFPSAMPLERCALLOUTDRIVER_SYS="%~dp0..\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\Package\WFPSamplerCalloutDriver.sys"
         SET WFPSAMPLERCALLOUTDRIVER="%~dp0..\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\Package\WFPSamplerCalloutDriver.*"
         SET SIGNTOOL_PATH="%~dp0..\..\..\..\..\bin\%_PROCESSOR_ARCHITECTURE_%"
      ) ELSE (
         FOR /F %%A IN ('ECHO "%~dp0" ^| FIND /I "network\trans\WFPSampler\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\package"') DO (SET PACKAGE_DIR=TRUE)

         IF DEFINED PACKAGE_DIR (
            :: Running from Package Directory
            SET WFPSAMPLER_EXE="%~dp0..\..\..\exe\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\WFPSampler.exe"
            SET WFPSAMPLERSERVICE_EXE="%~dp0..\..\..\svc\%WFP_ARCH%\%WFP_OS%%WFP_OPZ%\WFPSamplerService.exe"
            SET WFPSAMPLERCALLOUTDRIVER_SYS="%~dp0WFPSamplerCalloutDriver.sys"
            SET WFPSAMPLERCALLOUTDRIVER="%~dp0WFPSamplerCalloutDriver.*"
            SET SIGNTOOL_PATH="%~dp0..\..\..\..\..\..\..\bin\%_PROCESSOR_ARCHITECTURE_%"
         ) ELSE (
            ECHO.
            ECHO Don't know binary location.  Please provide path.
            ECHO    Current directory: "%CD%"
            ECHO    Script directory:  "%~dp0"

            GOTO :Usage
         )
      )
   ) ELSE (
      :: Use the provided path for finding the binaries
      SET WFPSAMPLER_EXE="%1\WFPSampler.exe"
      SET WFPSAMPLERSERVICE_EXE="%1\WFPSamplerService.exe"
      SET WFPSAMPLERCALLOUTDRIVER_SYS="%1\WFPSamplerCalloutDriver.sys"
      SET WFPSAMPLERCALLOUTDRIVER="%1\WFPSamplerCalloutDriver.*"
      SET SIGNTOOL_PATH=
   )

:Install

   ECHO.
   ECHO Installing WFPSampler

   IF DEFINED SIGNTOOL_PATH (
      ECHO.
      ECHO Attempting to sign WFPSampler.Exe
         %SIGNTOOL_PATH%\SignTool.exe Sign -A -V %WFPSAMPLER_EXE%

      ECHO.
      ECHO Attempting to sign WFPSamplerService.Exe
         %SIGNTOOL_PATH%\SignTool.exe Sign -A -V %WFPSAMPLERSERVICE_EXE%

      ECHO.
      ECHO Attempting to sign WFPSamplerCaloutDriver.Sys
         %SIGNTOOL_PATH%\SignTool.exe Sign -A -V %WFPSAMPLERCALLOUTDRIVER_SYS%
   )

   ECHO.
   ECHO Copying WFPSamplerCalloutDriver Bins to %WinDir%\System32\Drivers\
      COPY /Y %WFPSAMPLERCALLOUTDRIVER% %WinDir%\System32\Drivers\

   ECHO.
   ECHO Copying WFPSampler application binaries to %WinDir%\System32\
      COPY /Y %WFPSAMPLER_EXE% %WinDir%\System32\
      COPY /Y %WFPSAMPLERSERVICE_EXE% %WinDir%\System32\
   IF EXIST %WinDir%\System32\WFPSamplerService.Exe (
      ECHO.
      ECHO Registering the WFPSampler Service
         %WinDir%\System32\WFPSamplerService.Exe -i
         Net Start WFPSampler
   )

   IF EXIST %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf (
      IF EXIST %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Sys (
         ECHO.
         ECHO Registering the WFPSampler Callout Driver
            RunDLL32.Exe syssetup,SetupInfObjectInstallAction DefaultInstall 131 %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf
            Net Start WFPSamplerCallouts
      )
   )

:Cleanup

   SET WFPSAMPLERCALLOUTDRIVER=
   SET WFPSAMPLERCALLOUTDRIVER_SYS=
   SET WFPSAMPLERSERVICE_EXE=
   SET WFPSAMPLER_EXE=

   SET PACKAGE_DIR=
   SET SCRIPTS_DIR=

   SET _PROCESSOR_ARCHITECTURE_=

   SET WFP_ARCH=
   SET WFP_OPZ=
   SET WFP_OS=

   GOTO :EoF

:Uninstall

   ECHO.
   ECHO Uninstalling WFPSampler

   IF EXIST %WinDir%\System32\WFPSampler.Exe (
      ECHO.
      ECHO Removing policy
         WFPSampler.exe -clean
   )

   ECHO.
   ECHO Stopping the WFPSampler service
      Net Stop WFPSampler

   ECHO.
   ECHO Stopping the WFPSamplerCallouts service
      Net Stop WFPSamplerCallouts

   IF EXIST %WinDir%\System32\WFPSamplerService.Exe (
      ECHO.
      ECHO Unregistering the WFPSampler Service
         %WinDir%\System32\WFPSamplerService.Exe -u
   )

   IF EXIST %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf (
      IF EXIST %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Sys (
         ECHO.
         ECHO Unregistering the WFPSampler Callout Driver
            RunDLL32.Exe SETUPAPI.DLL,InstallHinfSection DefaultUninstall 132 %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.Inf
      )
   )

   ECHO.
   ECHO Deleting WFPSampler application binaries from %WinDir%\System32\
      ERASE /F /Q %WinDir%\System32\WFPSampler.exe
      ERASE /F /Q %WinDir%\System32\WFPSamplerService.exe

   ECHO.
   ECHO Deleting WFPSamplerCalloutDriver binaries from %WinDir%\System32\Drivers\
      ERASE /F /Q %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.cat
      ERASE /F /Q %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.inf
      ERASE /F /Q %WinDir%\System32\Drivers\WFPSamplerCalloutDriver.sys

   GOTO :EoF

:Usage

      ECHO.
      ECHO WFPSamplerInstall.cmd [PATH ^| ARCH ^| -r] [OS] [OPTIMIZE]
      ECHO.
      ECHO.
      ECHO       ARCH       Specify the architecure the binaries are for
      ECHO                     x86 or x64
      ECHO.
      ECHO       OS         Specify the OS the binaries are for
      ECHO                     Win7, Win8, or Win8.1
      ECHO.
      ECHO       OPTIMIZE   Specify the optimization
      ECHO                     i.e. Debug
      ECHO.
      ECHO    WFPSampler.cmd x86 Win7 Debug
      ECHO.
      ECHO.
      ECHO WFPSamplerInstall.cmd [PATH]
      ECHO.
      ECHO       PATH       Copies binaries from specified path and installs the WFPSampler
      ECHO                     i.e. C:\MyDir
      ECHO.
      ECHO    WFPSampler.cmd C:\MyDir
      ECHO.
      ECHO.
      ECHO WFPSamplerInstall.cmd [-r]
      ECHO.
      ECHO       -r         Uninstalls the WFPSampler and removes binaries
      ECHO.
      ECHO    WFPSampler.cmd -r
      ECHO.

:EoF