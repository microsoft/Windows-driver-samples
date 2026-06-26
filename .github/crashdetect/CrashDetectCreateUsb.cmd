@echo off
setlocal

set WINPE_DRIVE=A
set WINUSB_DRIVE=B
set EXITCODE=0

rem Pre-check
if not exist "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Deployment Tools\amd64\DISM\dism.exe" (
    echo ERROR: Missing DISM tool!
    goto error
)
if not exist "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Windows Preinstallation Environment\amd64\en-us\winpe.wim" (
    echo ERROR: Missing winpe.wim image!
    goto error
)
if not exist "C:\WinPE_USB\Scripts\CrashDetectOsReinstall.cmd" (
    echo ERROR: Missing script C:\WinPE_USB\Scripts\CrashDetectOsReinstall.cmd!
    goto error
)
if not exist "C:\WinPE_USB\Scripts\Unattend.xml" (
    echo ERROR: Missing XML C:\WinPE_USB\Scripts\Unattend.xml!
    goto error
)
if not exist "C:\WinPE_USB\Images\install.wim" (
    echo ERROR: Missing WIM C:\WinPE_USB\Images\install.wim!
    goto error
)

rem Environment setup.
echo Setting up environment for ADK tools...
cd "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Deployment Tools>"
call DandISetEnv.bat
echo Done setting up environment.

rem Ask for the USB disk number displayed in Diskpart.
echo Displaying detected disk drives..
echo Creating DiskPart script...
(
echo list disk
) > C:\WinPE_USB\diskpart.txt
diskpart /s C:\WinPE_USB\diskpart.txt
del C:\WinPE_USB\diskpart.txt
echo(
echo(
echo Note that Disk 0 is often the OS drive you do NOT want to format!
echo(
echo Ensure drive letters %WINPE_DRIVE%: and %WINUSB_DRIVE%: are not currently assigned to other drives!
echo It's ok if it's assigned to the target USB.
echo(
set /p USBDISK=Enter the Disk Number of the USB drive you want to make WinPE bootable: 
echo(
echo Disk %USBDISK% selected.

rem Create a USB drive with WinPE and data partitions.
echo(
echo WARNING!!!: VERIFY DISK %USBDISK% IS THE CORRECT USB DISK TO FORMAT!
choice /C YN /M "ARE YOU SURE YOU WANT TO FORMAT DISK %USBDISK%?"
echo(
if %errorlevel% equ 2 (
    echo You chose NO, exiting without modifying USB key.
    exit /b 0
)
if %errorlevel% equ 1 (
    echo You chose YES, proceeding with formatting USB key.
)

:dism
rem Update startnet.cmd autorun script in WinPE boot image (winpe.wim).
cd /d "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Windows Preinstallation Environment\amd64"
echo(
if not exist "C:\WinPE_USB\WinPE_amd64\mount" (
    echo Creating temp folders C:\WinPE_USB\WinPE_amd64\mount...
    mkdir "C:\WinPE_USB\WinPE_amd64\mount"
)
echo Mounting WinPE image to modify...
Dism /Mount-Image /ImageFile:"en-us\winpe.wim" /index:1 /MountDir:"C:\WinPE_USB\WinPE_amd64\mount"
if errorlevel 1 (
    echo ERROR: DISM failed to mount ImageFile:"en-us\winpe.wim"!
    goto error
)
echo Done mounting winpe.wim image.
echo Updating startnet.cmd autorun script in WinPE boot image...
(
echo wpeinit
echo.
echo @echo off
echo rem Find USB drive.
echo for %%%%D in (D E F G H I J K L M N O P Q R S T U V W Y Z^) do ^(
echo     if exist %%%%D:Scripts\CrashDetectOsReinstall.cmd ^(
echo         call %%%%D:Scripts\CrashDetectOsReinstall.cmd
echo         goto EOF
echo     ^)
echo ^)
) > "C:\WinPE_USB\WinPE_amd64\mount\Windows\System32\startnet.cmd"
echo Done adding CrashDetectOsReinstall.cmd to startnet.cmd.
echo Unmounting WinPE image with committed changes...
Dism /Unmount-Image /MountDir:"C:\WinPE_USB\WinPE_amd64\mount" /commit
if errorlevel 1 (
    echo ERROR: DISM failed to unmount ImageFile:"en-us\winpe.wim"!
    echo USB drive have not been touched yet.
    goto error
)
echo Done unmounting winpe.wim image.

rem Delete temp "WinPE_amd64" folder, else the "copype.cmd" below will fail if the folder is already present.
rmdir /s /q "C:\WinPE_USB\WinPE_amd64"
echo Deleted temp folder "C:\WinPE_USB\WinPE_amd64".
echo(
echo Wiping out USB and creating 2 partitions...
echo Creating DiskPart script...
(
echo select disk %USBDISK%
echo clean
echo create partition primary size=2048
echo active
echo format fs=FAT32 quick label="WinPE"
echo assign letter=%WINPE_DRIVE%
echo create partition primary
echo format fs=NTFS quick label="WinUSB"
echo assign letter=%WINUSB_DRIVE%
) > C:\WinPE_USB\diskpart.txt
echo Partitioning USB...
diskpart /s C:\WinPE_USB\diskpart.txt
if errorlevel 1 (
    echo ERROR: DiskPart failed during partitioning and formatting USB!
    del C:\WinPE_USB\diskpart.txt
    goto error
)
del C:\WinPE_USB\diskpart.txt
echo Done partitioning and formatting USB.
echo(
rem The "copype.cmd" script will create working directory "WinPE_amd64", it will fail if directory already exist.
echo Copying WinPE working files to "C:\WinPE_USB\WinPE_amd64"...
call copype.cmd amd64 "C:\WinPE_USB\WinPE_amd64"
if errorlevel 1 (
    echo ERROR: Script "copype.cmd" failed to copy working files!
    goto error
)
echo Done copying WinPE files.
echo(
rem Install WinPE to the USB and make it bootable.
echo Creating bootable WinPE USB...
call Makewinpemedia.cmd /ufd /f "C:\WinPE_USB\WinPE_amd64" "%WINPE_DRIVE%:" /bootex
if errorlevel 1 (
    echo ERROR: Script "Makewinpemedia.cmd" failed to make WinPE USB bootable!
    goto error
)
echo Done, WinPE USB drive is now bootable.
echo(
rem Copy Unattend.xml, scripts and install.wim over to USB
echo Copying Unattend.xml and scripts over to USB...
xcopy "C:\WinPE_USB\Scripts\" "%WINUSB_DRIVE%:\Scripts\" /E /I /R /Y
if errorlevel 1 (
    echo ERROR: Failed copying scripts from "C:\WinPE_USB\Scripts\" to USB!
    goto error
)
echo Done copying script files.
echo(
echo Copying install.wim over to USB...this could take a while...
robocopy "C:\WinPE_USB\Images" "%WINUSB_DRIVE%:\Images" install.wim /ETA /J
if %errorlevel% geq 8 (
    echo ERROR: Failed copying install.wim from "C:\WinPE_USB\Images\" to USB!
    goto error
)
echo Done copying WIM file.
echo(
goto cleanup

:error
set EXITCODE=1
goto cleanup

:cleanup
rem Clean up any temp folders.
if exist "C:\WinPE_USB\WinPE_amd64" (
    rmdir /s /q "C:\WinPE_USB\WinPE_amd64"
    echo Cleaned up temp folder "C:\WinPE_USB\WinPE_amd64".
)

:done
if %EXITCODE% equ 0 (
    echo(
    echo SUCCESSFULLY CREATED BOOTABLE WINPE USB DRIVE.
) else (
    echo(
    echo FAILED CREATING BOOTABLE WINPE USB DRIVE!
)
endlocal & exit /b %EXITCODE%