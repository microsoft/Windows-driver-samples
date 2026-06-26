# Crash Detect USB Setup Guide: <br>Automating Crash Detection and OS Reinstall of Target Test Systems

---

## A) Overview

This guide walks through setting up a new bootable WinPE USB drive using:

- A **Host Controller** (to build the USB)
- A **bootable WinPE USB** (automatic crash detection and OS reinstall)
- A **target test system**

### High-level flow

1. Download Windows image (ISO)
2. Install Windows ADK + WinPE add-on
3. Create bootable WinPE USB
4. Copy image + scripts to USB
5. Boot up target system
6. OPTIONAL: Install OS on new Target System

---

## B) Requirements

### Host Controller
- Windows 11 25H2
- Administrator privileges
- USB drive (>= 64GB recommended)
- Internet connection

### Target System
- Windows 11 25H2
- BIOS boot priority set to **USB boot** and **Secure Boot disabled**
- Windows System failure recovery set to **"Automatically restart"**
- Willing to wipe disk (automatic OS reinstall from crash)

---

## C) Download Required Software and Scripts onto the Host Controller

### Windows OS Image (ISO)

- [Download Windows 11 (official)](https://www.microsoft.com/en-us/software-download/windows11)
- Go to section "Download Windows 11 Disk Image (ISO) for x64 devices".
- Select the option "Windows 11 (multi-edition ISO for x64 devices)".
- Click "Confirm" button.
- Section "Select the product language" should appear.
- Select your language option. (Ex: "English (United States)")
- Click "Confirm" button.
- Section "Download - Windows 11 English" should appear.
- Click "64-bit Download" button.

Mount ISO:
- In File Explorer, go to the location you downloaded the ISO file `Win11_25H2_English_x64_v2.iso` to.
- Right-click on the ISO file and select "Mount".
  - If the "Open File - Security Warning" prompt pops up after a minute then click "Open".
  - (The prompt may be hidden behind other Windows.)
- Create new folder and subfolder `C:\WinPE_USB\Images`.
- Go to the `%MountDriveLetter%:\sources` folder and copy the **`install.wim`** file to **`C:\WinPE_USB\Images`**.
  - This is the Windows 11 OS image file that the DISM tool will need to deploy the OS.
  - This file will be copied over to the USB later after bootable WinPE USB creation.
- Right-click on the %MountDriveLetter% and select "Eject" to unmount the ISO image.

---

### Windows Assessment and Deployment Kit (Deployment Tools) & Windows PE Add-on

- [Download Windows ADK & WinPE Add-on](https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-install)
- Go to section "Download the ADK 10.1.26100.2454 (December 2024)".
- Click on the link "Download the Windows ADK 10.1.26100.2454 (December 2024)" to download the `adksetup.exe` installer.
- Click on the link "Download the Windows PE add-on for the Windows ADK 10.1.26100.2454 (December 2024)" to download the `adkwinpesetup.exe` installer.

Install ADK:
- Double-click on the `adksetup.exe` from the location you downloaded the file to launch the installer.
- "Specify Location" page, click "Next", to install at default location.
- "Windows Kits Privacy" page, select your privacy option, click "Next".
- "License Agreement" page, click "Accept".
- "Select the features you want to install" page, confirm "Deployment Tools" is checked, then click "Install".
- If "User Account Control" prompt appears, click "Yes" to begin installation process.
- "Installing features..." page, wait for installation process to complete.
- "Welcome to the Windows Assessment and Deployment Kit!" page, click "Close".

Install WinPE Add-on: <br>**Important:** Install **ADK first**, then WinPE add-on
- Double-click on the `adkwinpesetup.exe` from the location you downloaded the file to launch the installer.
- "Specify Location" page, click "Next", to install at default location.
- "Windows Kits Privacy" page, select your privacy option, click "Next".
- "License Agreement" page, click "Accept".
- "Select the features you want to install" page, confirm "Windows Preinstallation Environment (Windows PE)" is checked, then click "Install".
- If "User Account Control" prompt appears, click "Yes" to begin installation process.
- "Installing features..." page, wait for installation process to complete.
- "Welcome to the Windows Assessment and Deployment Kit Windows Preinstallation Environment Add-ons!" page, click "Close".

---

### Unattend and Script Files from GitHub
- Download files from [Windows-driver-samples/tree/main/.github/crashdetect](https://github.com/microsoft/Windows-driver-samples/tree/main/.github/crashdetect)
- Create directory **`C:\WinPE_USB\Scripts\`** and copy the following downloaded files to there.
  - `CrashDetectCreateUsb.cmd`
  - `CrashDetectOsReinstall.cmd`
  - `Unattend.xml`

---

## D) Create Bootable WinPE USB
- **TIP:** It's a good idea to make a backup copy of the original **"winpe.wim"** image file before editing it in the following steps.
  - `C:\Program Files (x86)\Windows Kits\10\Assessment and Deployment Kit\Windows Preinstallation Environment\amd64\en-us\winpe.wim`

- Plug a USB drive into the Host Controller.
- Check to make sure drive letters **A:** and **B:** are not currently used by any other drive. If used by the target USB drive, it's okay.
- Confirm the folder **`C:\WinPE_USB`** and its subfolders **`Scripts`** and **`Images`** exist.
- Confirm the **`Scripts`** subfolder contains the following files that were downloaded from previous steps.
  - `CrashDetectOsReinstall.cmd`
  - `Unattend.xml`
- Confirm the **`Images`** subfolder contains the Win11 OS image file.
  - `install.wim`

### OPTION 1: Use the `CrashDetectCreateUsb.cmd` script to create the USB automatically.

- Start a `Command Prompt` running as administrator.
- Run the script by typing the following line into the Command Prompt.
```cmd
C:\WinPE_USB\Scripts\CrashDetectCreateUsb.cmd
```
- The script will display a list of detected disk drives, usually Disk 0 is the OS disk, DO NOT select that disk.
- Prompt 1: will ask you to enter the Disk number of your USB drive.
- Prompt 2: will ask you to confirm one last time before wiping out the USB drive.
- The last step will copy over the `install.wim` OS image to the USB, which could take a while.

### OPTION 2: Follow the steps below to create the USB manually.

#### 1. Make sure your PC has the ADK and ADK Windows PE add-on installed.
  - Start the `Deployment and Imaging Tools Environment` running as administrator.

#### 2. Update the `startnet.cmd` autorun script in the WinPE boot image.
  - Mount the WinPE boot image (`winpe.wim`) using DISM.
  - Adds the `CrashDetectOsReinstall.cmd` script to the `startnet.cmd` script.
```cmd
(
echo wpeinit
echo.
echo @echo off
echo REM Find USB drive.
echo for %%D in (D E F G H I J K L M N O P Q R S T U V W Y Z^) do ^(
echo     if exist %%D:Scripts\CrashDetectOsReinstall.cmd ^(
echo         call %%D:Scripts\CrashDetectOsReinstall.cmd
echo         goto EOF
echo     ^)
echo ^)
) > "C:\WinPE_USB\WinPE_amd64\mount\Windows\System32\startnet.cmd"
```
  - Unmount the WinPE image using DISM.
```cmd
Dism /Unmount-Image /MountDir:"C:\WinPE_USB\WinPE_amd64\mount" /commit
```
  - Delete folder `C:\WinPE_USB\WinPE_amd64`, else the `copype.cmd` below will fail if the folder is already present.
```cmd
rmdir /s /q "C:\WinPE_USB\WinPE_amd64"
```

#### 3. Create and format a multiple partition USB drive. 
  - Attach a USB large enough for 2GB WinPE partition + WinUSB partition (Win11 WIM 8GB + Memory dump files 16GB-64GB + Scripts).
  - Enter the following commands into the command prompt.
```cmd
diskpart
list disk
select disk X    (where X is your USB drive)
clean
create partition primary size=2048
active
format fs=FAT32 quick label="WinPE"
assign letter=A
create partition primary
format fs=NTFS quick label="WinUSB"
assign letter=B
exit
```

#### 4. Create a bootable Windows PE USB drive.
  - Copying WinPE boot files to a working directory.
```cmd
copype.cmd amd64 "C:\WinPE_USB\WinPE_amd64"
```
  - Copy the WinPE files to the WinPE partition on USB.
```cmd
MakeWinPEMedia.cmd /UFD /F "C:\WinPE_USB\WinPE_amd64" "A:" /bootex
```

---

#### 5. Copy scripts and OS install image over to WinUSB partition on USB. 

  - Copy Script files over to USB.
```cmd
xcopy "C:\WinPE_USB\Scripts\" "B:\Scripts\" /E /I /R /Y
```
  - Copy Windows 11 OS WIM file over to USB, this could take a while...
```cmd
robocopy "C:\WinPE_USB\Images" "B:\Images" install.wim /ETA /J
```

---

## E) Boot Up Target System

- Insert USB into target PC  
- Power on  
- Enter boot menu (F12 / ESC / DEL depending on vendor)  
- Confirm BIOS/UEFI setting has USB Drive as the first boot priority. (Varies among vendors)
- Confirm Secure Boot setting is Disabled  
- Save BIOS settings to reboot target system.
- WinPE will automatically launch the "startnet.cmd" script we edited earlier in the "winpe.wim" image.
- The script will call "wpeinit", then our "CrashDetectOsReinstall.cmd" script to begin automatic OS crash detection and reimage for WDK driver testing.

---

## F) OPTIONAL: Install OS on new Target System
### On the bootable USB's second partition `WinPE_USB`
- Create the folder **`Logs`**.
- Create an empty file **`InstallOs.flg`** in that folder.
  - (In File Explorer, ensure file name extensions are visible, else the filename may be accidentally set to `InstallOs.flg.txt`)
- Plug USB into target system and reboot into USB.
- The USB will detect the flag and begin reinstalling the OS immediately.
  - **WARNING**: There will be **NO** prompt to reconfirm OS install, be sure to plug into the correct target system.
  - Do **NOT** leave this USB plugged into the Host Controller when this flag is set, to avoid accidental OS reinstall.

---

## G) OPTIONAL: Add a Custom Script to Windows Setup
### Setupcomplete.cmd and ErrorHandler.cmd
- These are custom scripts that run during or after the Windows Setup process. They can be used to install applications or run other tasks by using cscript/wscript scripts.
- Follow instructions on this website:
  - (https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/add-a-custom-script-to-windows-setup)

---

## Troubleshooting
### ERROR: Script "Makewinpemedia.cmd" failed to make WinPE USB bootable!
- If your Host Controller is connected to a secured IT network, the actions in this script may have been blocked.
- Check to see if **"bootsect.exe"** was blocked by Windows Security.
  - Run "Windows Security"
  - Select "Virus & threat protection"
  - Click link "Manage ransomware protection" at the bottom
  - Click link "Allow an app through Controlled folder access"
  - Click button "Add an allowed app" button, then select option "Recently blocked apps"
  - Scroll down and look for the "bootsect.exe" app to add to allow list
### USB won't boot
- Check BIOS boot order
- Disable Secure Boot
### Disk not visible in WinPE
- Missing storage drivers
### Windows doesn't boot
- Re-run `bcdboot`
- Verify partition layout

---

## Reference Documentation
- [WinPE overview](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/winpe-intro)
- [Create a USB drive with WinPE and data partitions](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/winpe--use-a-single-usb-key-for-winpe-and-a-wim-file---wim#create-a-usb-drive-with-winpe-and-data-partitions)
- [WinPE: Create bootable media](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/winpe-create-usb-bootable-drive)
- [Capture and apply Windows (WIM)](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/capture-and-apply-windows-using-a-single-wim)
- [Answer files (unattend.xml)](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/update-windows-settings-and-scripts-create-your-own-answer-file-sxs)