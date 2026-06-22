@echo off

REM Environment setup.
echo Setting up environment for ADK tools...
cd "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Deployment Tools>"
call DandISetEnv.bat
echo Done setting up environment.

REM Ask for the USB disk number displayed in Diskpart.
echo Displaying detected disk drives..
echo Creating DiskPart script...
(
echo list disk
) > C:\diskpart.txt
diskpart /s C:\diskpart.txt
del C:\diskpart.txt
echo Note that Disk 0 is often the OS drive you do NOT want to format!
set /p USBDISK=Enter the Disk Number of the USB drive you want to make WinPE bootable: 
echo Disk %USBDISK% selected.

:dism
REM Update startnet.cmd autorun script in WinPE boot image (winpe.wim).
cd /d "C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Windows Preinstallation Environment\amd64"
echo Mounting WinPE image to modify...
md C:\WinPE_amd64\mount
Dism /Mount-Image /ImageFile:"en-us\winpe.wim" /index:1 /MountDir:"C:\WinPE_amd64\mount"
echo Done mounting winpe.wim image.
echo Updating startnet.cmd autorun script in WinPE boot image...
(
echo wpeinit
echo.
echo @echo off
echo REM Find USB drive.
echo for %%%%D in (D E F G H I J K L M N O P Q R S T U V W Y Z^) do ^(
echo     if exist %%%%D:Scripts\CrashDetectOsReinstall.cmd ^(
echo         call %%%%D:Scripts\CrashDetectOsReinstall.cmd
echo         goto EOF
echo     ^)
echo ^)
) > "C:\WinPE_amd64\mount\Windows\System32\startnet.cmd"
echo Done adding CrashDetectOsReinstall.cmd to startnet.cmd.
echo Unmounting WinPE image with committed changes...
Dism /Unmount-Image /MountDir:"C:\WinPE_amd64\mount" /commit
echo Done unmounting winpe.wim image.
rmdir /s /q "C:\WinPE_amd64"
echo Deleted old folder "C:\WinPE_amd64".

REM Create a USB drive with WinPE and data partitions.
REM Warning!
echo(
echo WARNING!!!: VERIFY DISK %USBDISK% IS THE CORRECT USB DISK TO FORMAT!
choice /C YN /M "ARE YOU SURE YOU WANT TO FORMAT DISK %USBDISK%?"
echo(
if %errorlevel% equ 2 (
    echo You chose NO, exiting without touching USB key.
    timeout 5
    exit
    goto eof
)
if %errorlevel% equ 1 (
    echo You chose YES, proceeding with formatting USB key.
)

echo Wiping out USB and creating 2 partitions...
echo Creating DiskPart script...
(
echo select disk %USBDISK%
echo clean
echo create partition primary size=2048
echo active
echo format fs=FAT32 quick label="WinPE"
echo assign letter=P
echo create partition primary
echo format fs=NTFS quick label="WinUSB"
echo assign letter=U
) > C:\diskpart.txt
echo Partitioning USB...
diskpart /s C:\diskpart.txt
del C:\diskpart.txt
echo Done partitioning and formatting USB.

REM Copy WinPE working files to working directory.
echo Copying WinPE working files to "C:\WinPE_amd64"...
call copype amd64 "C:\WinPE_amd64"
echo Done copying files.

REM Install WinPE to the USB.
echo Creating bootable WinPE USB...
call Makewinpemedia /ufd /f C:\WinPE_amd64 P: /bootex
echo Done, WinPE USB drive is now bootable.

REM Copy Unattend.xml, scripts and install.wim over to USB
echo Copying Unattend.xml and scripts over to USB...
xcopy "C:\WinPE_USB\Scripts\" "U:\Scripts\" /e /i
echo Copying install.wim over to USB...this could take a while...
xcopy "C:\WinPE_USB\Images\install.wim" "U:\Images\" /i
echo Done copying files.

echo DONE CREATING BOOTABLE WINPE USB DRIVE!