@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo CrashDetectOsReinstall.cmd...START

echo Searching for the USB drive volume letter...
set "USB="

for %%D in (D E F G H I J K L M N O P Q R S T U V W Y Z) do (
    if exist "%%D:\Scripts\CrashDetectOsReinstall.cmd" (
        set "USB=%%D"
        echo Found USB at drive volume letter !USB!:
    )
)

if not defined USB (
    echo ERROR: Did not find USB drive volume letter
    goto error
)

if not exist "!USB!:\Logs" (
    echo Creating Logs directory on USB
    mkdir "!USB!:\Logs"
)
set "LOG=!USB!:\Logs\CrashDetectOsReinstall.log" 
echo Created log file "%LOG%" on USB
echo [%DATE% %TIME%] CrashDetectOsReinstall.cmd...START >> "%LOG%"
echo [%DATE% %TIME%] Found USB at drive volume letter !USB!: >> "%LOG%"

set "FLAG_RETRIES=!USB!:\Logs\Retries.flg"
set "FLAG_INSTALLOS=!USB!:\Logs\InstallOs.flg"
set "FLAG_DEPLOYOSDONE=!USB!:\Logs\DeployOsDone.flg"


:installos
if exist "%FLAG_INSTALLOS%" (
    if exist "%FLAG_DEPLOYOSDONE%" (
        echo OS reinstall done.
        echo [%DATE% %TIME%] OS reinstall done. >> "%LOG%"
        del "%FLAG_INSTALLOS%" >> "%LOG%" 2>&1
        del "%FLAG_DEPLOYOSDONE%" >> "%LOG%" 2>&1
        goto reboot
    )
    echo Reinstalling OS...
    echo [%DATE% %TIME%] Reinstalling OS... >> "%LOG%"
    goto deployos
)


:detectcrash
if exist "C:\Windows\Minidump\" (
    echo OS crashed on last reboot into the OS!
    echo [%date% %time%] OS crashed on last reboot into the OS! >> "%LOG%"

    set "MININAME="
    for %%F in ("C:\Windows\Minidump\*.dmp") do (
        set "MININAME=%%~nF"
        echo Minidump filename: !MININAME! >> "%LOG%"
    )

    echo Backing up Minidump files...
    copy /y "C:\Windows\Minidump\*.dmp" "!USB!:\Logs" >> "%LOG%" 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to back up Mini dump file
        echo [%date% %time%] ERROR: Failed to back up Mini dump file >> "%LOG%"
    )
    rmdir /s /q "C:\Windows\Minidump" >> "%LOG%" 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to delete Minidump folder
        echo [%date% %time%] Failed to delete Minidump folder >> "%LOG%"
    )

    if exist "C:\Windows\MEMORY.DMP" (
        if defined MININAME (
            ren "C:\Windows\MEMORY.DMP" "MEMORY-!MININAME!.DMP" >> "%LOG%" 2>&1
        ) 
        if !errorlevel! neq 0 (
            echo ERROR: Failed to rename MEMORY.DMP file
            echo [%date% %time%] ERROR: Failed to rename MEMORY.DMP file >> "%LOG%"
            echo Backing up MEMORY.DMP file...
            copy /y "C:\Windows\MEMORY.DMP" "!USB!:\Logs" >> "%LOG%" 2>&1
        ) else (
            echo Backing up MEMORY-!MININAME!.DMP file...
            copy /y "C:\Windows\MEMORY-!MININAME!.DMP" "!USB!:\Logs" >> "%LOG%" 2>&1
        )
        if !errorlevel! neq 0 (
            echo ERROR: Failed to back up MEMORY.DMP file
            echo [%date% %time%] ERROR: Failed to back up MEMORY.DMP file >> "%LOG%"
        )
    )

    echo Done trying to back up files to USB
    echo [%date% %time%] Done trying to back up files to USB >> "%LOG%"
    goto retry
) else (
    rem If OS crashed just now, it will be detected on next reboot.
    rem Minidump folder used for crash detection is not created yet on automatic first crash reboot until OS is loaded.
    echo No OS crash on last reboot into the OS.
    echo [%date% %time%] No OS crash on last reboot into the OS. >> "%LOG%"
    if exist %FLAG_RETRIES% (
        del %FLAG_RETRIES% >> "%LOG%" 2>&1
    )
    goto reboot
)


:retry
set "TRIES=0"
rem Retries are always one less than actual crashes because first crash is not detected.
set "MAX_RETRIES=2"

if not exist "%FLAG_RETRIES%" (
    echo Creating "%FLAG_RETRIES%" with default retries set to 0 >> "%LOG%"
    echo RETRIES=0 > "%FLAG_RETRIES%"
)

for /f "tokens=1,2 delims==" %%A in (%FLAG_RETRIES%) do (
    if /i "%%A"=="RETRIES" (
        set "TRIES=%%B" >> "%LOG%" 2>&1
    )
)

set /a TRIES+=0 2>nul >> "%LOG%" 2>&1
set /a TRIES+=1 >> "%LOG%" 2>&1
echo RETRIES=!TRIES! > %FLAG_RETRIES%

echo Current OS boot retries: !TRIES!
echo Current OS boot retries: !TRIES! >> "%LOG%"
if !TRIES! GEQ %MAX_RETRIES% (
    echo Max %MAX_RETRIES% retries reached, reinstalling OS...
    del %FLAG_RETRIES% >> "%LOG%" 2>&1
    type nul > %FLAG_INSTALLOS%
    goto installos
) else (
    echo Max %MAX_RETRIES% retries allowed, booting into OS...
    goto reboot
)


:deployos
rem Image index is the OS edition to install from a multi-edition OS install image.
rem For "Windows 11 25H2" image, "Windows 11 Pro" is the 6th item on the list of editions available.
rem Adjust accordingly to install other OS editions.
rem For single edition OS images, set index to 1.
set "IMAGE_INDEX=6"
set "TARGET_DISK=0"
set "SYSTEM_DRIVE=S"
set "SYSTEM_LABEL=System"
set "TARGET_DRIVE=W"
set "TARGET_LABEL=TestOS"
set "INSTALL_WIM=install.wim"
set "UNATTEND_XML=Unattend.xml"

echo Setting flag InstallOs.flg for CrashDetectOsReinstall.cmd autorun script to detect OS install. >> "%LOG%"
type nul > "%FLAG_INSTALLOS%"

echo [1/8] Creating DiskPart script...
echo [%DATE% %TIME%] [1/8] Creating DiskPart script... >> "%LOG%"
(
echo select disk "%TARGET_DISK%"
echo clean
echo convert gpt
echo create partition efi size=100
echo format quick fs=fat32 label="%SYSTEM_LABEL%"
echo assign letter="%SYSTEM_DRIVE%"
echo create partition msr size=16
echo create partition primary
echo format quick fs=ntfs label="%TARGET_LABEL%"
echo assign letter="%TARGET_DRIVE%"
) > X:\diskpart.txt

echo [2/8] Partitioning target disk...
echo [%DATE% %TIME%] [2/8] Partitioning target disk... >> "%LOG%"
diskpart /s X:\diskpart.txt >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: DiskPart failed. See %LOG%
    goto error
)

echo [3/8] Applying OS WIM image...
echo [%DATE% %TIME%] [3/8] Applying image... >> "%LOG%"
dism /Apply-Image /ImageFile:"!USB!:\Images\%INSTALL_WIM%" /Index:"%IMAGE_INDEX%" /ApplyDir:"%TARGET_DRIVE%":\  >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: DISM apply failed. See %LOG%
    goto error
)

echo [4/8] Copying "%UNATTEND_XML%"...
echo [%DATE% %TIME%] [4/8] Copying %UNATTEND_XML%... >> "%LOG%"
if not exist "%TARGET_DRIVE%:\Windows\Panther" (
    mkdir "%TARGET_DRIVE%:\Windows\Panther" >> "%LOG%" 2>&1
)
copy /y "!USB!:\Scripts\%UNATTEND_XML%" "%TARGET_DRIVE%:\Windows\Panther\%UNATTEND_XML%"  >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: Failed to copy %UNATTEND_XML%. See %LOG%
    echo [%DATE% %TIME%] ERROR: Failed to copy %UNATTEND_XML%. >> %LOG%
    goto error
)

echo [5/8] Creating boot files...
echo [%DATE% %TIME%] [5/8] Creating boot files... >> "%LOG%"
bcdboot W:\Windows /s S: /f UEFI >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: BCDBOOT failed. See %LOG%
    goto error
)

echo [6/8] Setting one-time bootsequence to OS boot manager...
echo [%DATE% %TIME%] [6/8] Setting one-time bootsequence to OS boot manager... >> "%LOG%"
bcdedit /set {fwbootmgr} bootsequence {bootmgr}
echo bcdedit /set {fwbootmgr} bootsequence {bootmgr} >> "%LOG%"
if errorlevel 1 (
    echo ERROR: BCDEDIT /set {fwbootmgr} bootsequence {bootmgr} failed. See "%LOG%"
    goto error
)

echo [7/8] Deployment complete.
echo [%DATE% %TIME%] [7/8] Deployment complete. >> "%LOG%"
echo Deployment complete. >> "%LOG%"
echo Setting flag DeployOsDone.flg for script to detect OS install complete and boot into OS.
type nul > "%FLAG_DEPLOYOSDONE%"

echo [8/8] Rebooting into OS in 10sec...
echo [%DATE% %TIME%] [8/8] Rebooting into OS in 10sec... >> "%LOG%"
rem Simulate timeout with ping (10 sec)
ping -n 11 127.0.0.1 >nul
wpeutil reboot >> "%LOG%" 2>&1


:reboot
echo Rebooting into OS in 10sec...
echo [%DATE% %TIME%] Rebooting into OS... >> "%LOG%"
rem Force next boot into Windows boot manager.
bcdedit /set {fwbootmgr} bootsequence {bootmgr} >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: BCDEDIT /set {fwbootmgr} bootsequence {bootmgr} - failed. See "%LOG%"
    goto error
)
rem Disable WinRE prompt to attempt system recovery, which requires user intervention.
bcdedit /set {default} recoveryenabled no >> "%LOG%" 2>&1
if errorlevel 1 (
    echo ERROR: BCDEDIT /set {default} recoveryenabled no - failed. See "%LOG%"
)
rem Simulate timeout with ping (10 sec)
ping -n 11 127.0.0.1 >nul
wpeutil reboot >> "%LOG%" 2>&1


:error
echo CrashDetectOsReinstall.cmd...ERROR! See "%LOG%"
echo [%DATE% %TIME%] CrashDetectOsReinstall.cmd...ERROR! >> "%LOG%"
exit /b 1 >> "%LOG%" 2>&1


:done
echo CrashDetectOsReinstall.cmd...DONE!
echo [%DATE% %TIME%] CrashDetectOsReinstall.cmd...DONE! >> "%LOG%"
exit /b 0 >> "%LOG%" 2>&1
