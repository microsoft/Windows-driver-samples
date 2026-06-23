# mtabow

## Overview

* What: Focus on improving quality of Microsoft Teams on Windows Platform. In the context of this document through extensions of HLK for Teams scenario based testing.
* Web: https://aka.ms/mtabow
* Email: mtabow@microsoft.com

## Terminology
* HLK: Windows Hardware Lab Kit.
* HLK Controller: A Windows Server running HLK's server side components. Orchestrates HLK tests.
* HLK Client: A device under test and under the HLK controller's control.  Also referred to as a Device under Test (DUT) or a Target.
* HLK Test: A specific tests shipped in the HLK.
* Microsoft Video Hardware Validator (MVHV): Microsoft Teams's test suite.  See https://install.appcenter.ms/orgs/ic3-media-platform/apps/microsoft-codec-validation-app/distribution_groups/external%20partners .  Note: To run MVHV through HLK you should not download from this URL but simply rely on version of MVHV shipped inside HLK.
* HLK MVHV tests: New HLK tests for testing Microsoft Teams Scenarios. These are simply thin wrappers around MVHV.  Note: MVHV is shipped as part of HLK MVHV tests.
* "Underlying MVHV tests": The actual MVHV tests that are shipped inside the HLK.

## Releases
* HLK MVHV tests are under active development.  To refer to releases on this page we use terminology HLK_MVHV 0.01, HLK_MVHV 0.02, and so on.
* HLK_MVHV 0.01:
  * Released via EEAP "HLK- MVHV Integration"
  * Link: https://partner.microsoft.com/en-us/dashboard/collaborate/packages/19783
  * Initial release.
  * Release date: 2026/06/03.
* HLK_MVHV 0.02:
  * Upcoming release.
  * New functionality: System tests, more tests, log gathering.
## Mapping HLK  Tests to underlying MVHV Tests

| No  | HLK Test Name                                      | HLK Test Type | MicrosoftVideoHardwareValidator.exe Command      | Comment                  |
|:----|:--------------------------------------------------:|:-------------:|-------------------------------------------------:|:------------------------:|
|   1 | Microsoft teams MVHV Test Version Check            | Device/System | -v                                               |                          |
|   2 | Microsoft Teams MVHV Test Instantiation            | Device/System | execute_testcategory Instantiation               |                          |
|   3 | Microsoft Teams MVHV Test DynamicControl           | Device/System | execute_testcategory DynamicControl              |                          |
|   4 | Microsoft Teams MVHV Test IDR                      | Device/System | execute_testcategory IDR                         |                          |
|   5 | Microsoft Teams MVHV Test Simulcast                | Device/System | execute_testcategory Simulcast                   |                          |
|   6 | Microsoft Teams MVHV Test QualitySingleStream H264 | Device/System | execute_testcategory QualitySingleStream -c H264 |                          |
|   7 | Microsoft Teams MVHV Test QualitySingleStream AV1  | Device/System | execute_testcategory QualitySingleStream -c AV1  |                          |
|   8 | Microsoft Teams MVHV Test QualitySingleStream H265 | Device/System | execute_testcategory QualitySingleStream -c H265 |                          |
|   9 | Microsoft Teams MVHV Test Basic                    | Device/System | execute_testcategory Basic                       |                          |
|  10 | Microsoft Teams MVHV Test Camera                   | Device/System | execute_test --mode Camera                       |                          |
|  11 | Microsoft Teams MVHV Test Audio                    | Device/System | execute_test --mode Audio                        |                          |
|  12 | Microsoft Teams MVHV Test Render                   | Device/System | execute_test --mode Render                       |                          |

Device Test
Device and System Test

## Running  MVHV as part of the Hardware Lab Kit (HLK)

This document assumes rudimentary experience with HLK.

### Prerequisites:
#### Setup HLK Controller
See https://learn.microsoft.com/en-us/windows-hardware/test/hlk/ .   As of 2026/05/27: You will for now need a special EEAP build. Work with your Microsoft EEAP contact and/or reach out to above email address.

#### Setup HLK Client
See https://learn.microsoft.com/en-us/windows-hardware/test/hlk/ .

#### One Time Client Setup
Each time a new client is created you must do a one time further configuration of the client.  Three steps: (1) Clips, (2) .NET, (3) MVHV.
##### Download "Clips" to Client
The underlying Microsoft Teams Test Suite requires a 45 GB (!!!) large collection of Clips that due to size is delivered outside the HLK itself.

Here follows instructions how to install and you may of course adjust those based on your lab setup.  Ultimately however the Clips files must end up on the HLK Client device and packed out in C:\Clips .

```
(Elevated Command Prompt)

powershell

cd C:\

#
# Download MVHV_Clips_260603.iso locally.
# On a fast connection as little as an hour.
# 

$iso = "C:\MVHV_Clips_260603.iso"

Start-BitsTransfer `
  -Source "https://go.microsoft.com/fwlink/?LinkId=2368809" `
  -Destination $iso

#
# Check ISO size and hash.  Both should return true.
#
(Get-Item $iso).Length -eq 62675183616
(((Get-FileHash $iso).Hash) -eq "AF70E5AA5469F1A915095E8B3A9BB1B7F429F9D59739BAF4C1D0C77FCF4F31FB")

#
# Mount and deploy files.
#

$diskImage = Mount-DiskImage -PassThru -ImagePath $iso
$volume = $diskImage | Get-Volume
$driveLetter = $volume.DriveLetter + ":"

robocopy /mir "$driveLetter\" C:\Clips

Dismount-DiskImage -ImagePath $iso
```

##### Install .NET Runtime on Client
The underlying Microsoft Teams Test Suite requires .NET 9 Runtime.  This is not automatically installed. As such you need to manually install.
```
winget install Microsoft.DotNet.Runtime.9
```

## Running HLK MVHV Tests
How to run, how to inspect test ran succesfully, how to analyze results, how to get detailed logs from client

How to Run tests:
----------------
To Run tests from HLK Controller you have to connect controller with a Device that has GPU and a Camera.
In selection screen you have to select your GPU and select Device.Streaming.HMFT.MLVEC by right clicking and adding feature.
To run camera tests you have to do similar step for camera entry in selection screen and picking Device.Streaming.Camera.Videocapture
Afterwards you will see MVHV tests in your Tests tab on your HLK Controller. 
Select the test you wish to run and choose run selected option by right clicking the view 


During Test Execution:
----------------------
While test is running relevant DUT (Device Under Test) will reboot a few times. In first cycle of reboot your device
may enter OOBE setup. OOBE stands for Out Of Box Experience. Device may ask for some permissions and some setup questions.
For the first time in current version please select and satify all the OOBE settings. After first iteration this won't be
asked again until you reset the device or reinstall the Operating System on device.


Test Results:
-------------
If all the tests show green check sign it means tests ran successfully and they passed. Instead if they show red cross sign test failed.
For initial test name and result related log you can check the associated logfile name from table in section "Mapping HLK MVHV Tests to 
underlying MVHV Test Suites" and look for that file under c:\mvhv folder. It is a text file editable in notepad or any other text editor.
For detailed logs there will be a folder called Results under c:\mvhv. For highly technical users please provide these set of logs. 

## FAQ / Tips and Tricks
TODO

## Known issues and planned changes
* TODO: Today you have to manually copy MVHV logs to target.   We hope to fix this by HLK_MVHV 0.0.2.
* TODO: Today you have only a few rudimentary tests.  We hope to have a more complete exposure of all of MVHV test by HLK_MVHV 0.0.2.
* TODO: Today detailed MVHV logs are left on Client machine. We hope to pull them back into HLK test results by HLK_MVHV 0.0.2.
* TODO: Today Clips deployment is a little messy. We hope to clean this up by HLK_MVHV 0.02.
* TODO: Today detecting prerequisites is not done well.  We hope to remove some prerequisites (say, MVHV robocopy) and hope to add better detection of other prerequisites by HLK_MVHV 0.02
* TODO: Redundant copies of MVHV inside HLK. Fixed in HLK_MVHV 0.02.
* TODO. Refine list of tests and targeting type. To be fixed post HLK_MVHV 0.02
* TODO: .NET Installation: Why different/assymmetric installer link styles for AMD64 and for ARM64 versions?  As I can best see the homepage is https://dotnet.microsoft.com/en-us/download/dotnet/9.0 and the specific symmetric download links are https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-9.0.16-windows-x64-installer and https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-9.0.16-windows-arm64-installer , but I'd really prefer just a winget command, see https://learn.microsoft.com/en-us/dotnet/core/install/windows?WT.mc_id=dotnet-35129-website#install-with-windows-package-manager-winget .


# Appendix: Manually run MVHV tests on HLK Client

## 1. Handle prerequisites
See section "Prerequisites" above.

## 2. Manually grab MVHV

On HLK Controller run following commands
```

robocopy /mir /nfl /ndl "C:\Program Files (x86)\Windows Kits\10\Hardware Lab Kit\Tests\amd64\nttest\multimediatest\Microsoft_Video_Hardware_Validator\x64" "C:\Program Files (x86)\Windows Kits\10\Hardware Lab Kit\Controller\WTTInstall\mvhv\x64"

robocopy /mir /nfl /ndl "C:\Program Files (x86)\Windows Kits\10\Hardware Lab Kit\Tests\arm64\nttest\multimediatest\Microsoft_Video_Hardware_Validator\arm64" "C:\Program Files (x86)\Windows Kits\10\Hardware Lab Kit\Controller\WTTInstall\mvhv\arm64"

```


For x64 based Clients:
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\HLKInstall\mvhv\x64 C:\mvhv\
```

For arm64 based Clients:
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\HLKInstall\mvhv\arm64 C:\mvhv\
```

After installing mvhv on client machine please make sure that MicrosoftVideoHardwareValidator.config file is present under c:\mvhv and it has following entries.
```
<?xml version="1.0" encoding="utf-8" ?>
<configuration>
        <appSettings>
                <add key="resultfolder" value="."/>
                <add key="clipsfolder" value="c:\clips\"/>
                <add key="rundxdiag" value="True" />
        </appSettings>
</configuration>
In case any such lines are missing you can manually edit file to have these entries. 
```

## 3. Run test manually
From elevated command prompt run tool using command from table above.

Example:
```
cd /d C:\mvhv
.\MicrosoftVideoHardwareValidator execute_testcategory Basic
```

# Appendix: Our Test Experiments and Results
## Hardware Overview
This is tested on following  hardware:

| No  | System Model                                         | System Type    | Comment           |
|:----|:----------------------------------------------------:|:--------------:|------------------:|
|   1 | Virtual Machine                                      | x64-based PC   |                   |
|   2 | Surface Laptop 6 for Business                        | x64-based PC   |                   |
|   3 | Surface Pro for Business 12in 1st Ed with Snapdragon | ARM64-based PC |                   |

## Hardware Details
### 1. HyperV VM - AMD64
```
OS Name	Microsoft Windows 11 Enterprise Insider Preview
Version	10.0.26640 Build 26640
Other OS Description 	Not Available
OS Manufacturer	Microsoft Corporation
System Name	DESKTOP-RS1M2DO
System Manufacturer	Microsoft Corporation
System Model	Virtual Machine
System Type	x64-based PC
System SKU	None
Processor	12th Gen Intel(R) Core(TM) i9-12900K, 3187 Mhz, 6 Core(s), 12 Logical Processor(s)
BIOS Version/Date	Microsoft Corporation Hyper-V UEFI Release v4.1, 9/25/2025
SMBIOS Version	3.1
Embedded Controller Version	255.255
BIOS Mode	UEFI
BaseBoard Manufacturer	Microsoft Corporation
BaseBoard Product	Virtual Machine
BaseBoard Version	Hyper-V UEFI Release v4.1
Platform Role	Desktop
Secure Boot State	On
PCR7 Configuration	Binding Not Possible
Windows Directory	C:\Windows
System Directory	C:\Windows\system32
Boot Device	\Device\HarddiskVolume1
Locale	United States
Hardware Abstraction Layer	Version = "10.0.26640.1000"
User Name	Not Available
Time Zone	Pacific Daylight Time
Installed Physical Memory (RAM)	4.40 GB
Total Physical Memory	4.40 GB
Available Physical Memory	1.81 GB
Total Virtual Memory	8.40 GB
Available Virtual Memory	4.66 GB
Page File Space	4.00 GB
Page File	C:\pagefile.sys
Kernel DMA Protection	Off
Virtualization-based security	Not enabled
App Control for Business policy	Enforced
App Control for Business user mode policy	Audit
Automatic Device Encryption Support	Reasons for failed automatic device encryption: TPM is not usable, PCR7 binding is not supported, Hardware Security Test Interface failed and device is not Modern Standby, Un-allowed DMA capable bus/device(s) detected, TPM is not usable
A hypervisor has been detected. Features required for Hyper-V will not be displayed.	
```

### 2. Surface Laptop 6 for Business - Surface_Laptop_6_for_Business_2033 - AMD64

```
OS Name	Microsoft Windows 11 Enterprise Insider Preview  
Version	10.0.26605 Build 26605  
Other OS Description 	Not Available   
OS Manufacturer	Microsoft Corporation  
System Name	LAPTOP-EP01SHCO  
System Manufacturer	Microsoft Corporation  
System Model	Surface Laptop 6 for Business  
System Type	x64-based PC  
System SKU	Surface_Laptop_6_for_Business_2033  
Processor	Intel(R) Core(TM) Ultra 7 165H, 3800 Mhz, 16 Core(s), 22 Logical Processor(s)  
BIOS Version/Date	Microsoft Corporation 22.112.143, 12/24/2025  
SMBIOS Version	3.6  
Embedded Controller Version	255.255  
BIOS Mode	UEFI  
BaseBoard Manufacturer	Microsoft Corporation  
BaseBoard Product	Surface Laptop 6 for Business  
BaseBoard Version	Not Available  
Platform Role	Mobile  
Secure Boot State	Off  
PCR7 Configuration	Elevation Required to View  
Windows Directory	C:\WINDOWS  
System Directory	C:\WINDOWS\system32  
Boot Device	\Device\HarddiskVolume1  
Locale	United States  
Hardware Abstraction Layer	Version = "10.0.26605.1000"  
User Name	LAPTOP-EP01SHCO\DTMLLUAdminUser  
Time Zone	Pacific Daylight Time  
Installed Physical Memory (RAM)	32.0 GB  
Total Physical Memory	31.6 GB  
Available Physical Memory	25.8 GB  
Total Virtual Memory	63.6 GB  
Available Virtual Memory	58.1 GB  
Page File Space	32.0 GB  
Page File	C:\pagefile.sys  
Kernel DMA Protection	On  
Virtualization-based security	Running  
Virtualization-based security Required Security Properties	Base Virtualization Support  
Virtualization-based security Available Security Properties	Base Virtualization Support, DMA Protection, UEFI Code Readonly, SMM Security Mitigations 1.0, Mode Based Execution Control, APIC Virtualization  
Virtualization-based security Services Configured	Secure Launch, Kernel-mode Hardware-enforced Stack Protection  
Virtualization-based security Services Running	Credential Guard  
App Control for Business policy	Enforced  
App Control for Business user mode policy	Audit  
Automatic Device Encryption Support	Elevation Required to View  
A hypervisor has been detected. Features required for Hyper-V will not be displayed.	  
```

### 3. Surface Pro for Business - Surface_Pro_for_Business_12in_1st_Ed_with_Snapdragon_2109 - ARM64
```
OS Name	Microsoft Windows 11 Pro Insider Preview  
Version	10.0.26630 Build 26630  
Other OS Description 	Not Available  
OS Manufacturer	Microsoft Corporation  
System Name	ARMDEVICE  
System Manufacturer	Microsoft Corporation  
System Model	Surface Pro for Business 12in 1st Ed with Snapdragon  
System Type	ARM64-based PC  
System SKU	Surface_Pro_for_Business_12in_1st_Ed_with_Snapdragon_2109  
Processor	Snapdragon X Plus (8-core) @ 3.30 GHz, 3244 Mhz, 8 Core(s), 8 Logical Processor(s)  
BIOS Version/Date	Microsoft Corporation 8.722.235, 7/2/2025  
SMBIOS Version	3.3  
Embedded Controller Version	255.255  
BIOS Mode	UEFI  
BaseBoard Manufacturer	Microsoft Corporation  
BaseBoard Product	Surface Pro for Business 12in 1st Ed with Snapdragon  
BaseBoard Version	Not Available  
Platform Role	Slate  
Secure Boot State	Off  
PCR7 Configuration	Elevation Required to View  
Windows Directory	C:\WINDOWS  
System Directory	C:\WINDOWS\system32  
Boot Device	\Device\HarddiskVolume1  
Locale	United States  
Hardware Abstraction Layer	Version = "10.0.26630.1000"  
User Name	armDevice\arm_dut  
Time Zone	Pacific Daylight Time  
Installed Physical Memory (RAM)	24.0 GB  
Total Physical Memory	23.6 GB  
Available Physical Memory	16.6 GB  
Total Virtual Memory	27.5 GB  
Available Virtual Memory	21.3 GB  
Page File Space	3.88 GB  
Page File	C:\pagefile.sys  
Kernel DMA Protection	On  
Virtualization-based security	Running  
Virtualization-based security Required Security Properties	Base Virtualization Support  
Virtualization-based security Available Security Properties	Base Virtualization Support, DMA Protection, UEFI Code Readonly, Mode Based Execution Control  
Virtualization-based security Services Configured	Hypervisor enforced Code Integrity  
Virtualization-based security Services Running	Credential Guard, Hypervisor enforced Code Integrity, Secure Launch  
App Control for Business policy	Enforced  
App Control for Business user mode policy	Audit  
Security Features Enabled	Return Address Signing (Kernel-mode)  
Automatic Device Encryption Support	Elevation Required to View  
A hypervisor has been detected. Features required for Hyper-V will not be displayed.	  
```
