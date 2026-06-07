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

| No  | HLK Test Name                                 | HLK Test Type | MicrosoftVideoHardwareValidator.exe Command  | Comment                      | Associated Logfile    |
|:----|:---------------------------------------------:|:-------------:|---------------------------------------------:|:----------------------------:|:---------------------:|
|   1 | MVHV Basic Test                               | Device        | -v                                           | HLK_MVHV 0.01 test.           | MvhvVersionOutput.txt |
|   2 | MVHV AV Encoder Test                          | Device        | execute_test Instantiation -c H264           | HLK_MVHV 0.01 test.           | MvhvSanityOutput.txt  |
|   3 | MVHV Camera Test                              | Device        | execute_test --mode Camera                   | HLK_MVHV 0.01 test.           | MvhvCameraOutput.txt  |
|   4 | Microsoft Teams MVHV Test Basic Test          | Device        | execute_testcategory -v                      | Future test. To replace No 1? | mvhvoutput.txt        | 
|   5 | Microsoft Teams MVHV Test AV Encoder Test     | Device/System | execute_testcategory Instantiation -c H264   | Future test. To replace No 2? | mvhvoutput.txt        |
|   6 | Microsoft Teams MVHV Test Camera Test         | Device/System | execute_test --mode Camera                   | Future test. To replace No 3? | mvhvoutput.txt        |
|   7 | Microsoft Teams MVHV Test Instantiation       | Device/System | execute_testcategory Instantiation           | Future test. To replace No 2? | mvhvoutput.txt        |
|   8 | Microsoft Teams MVHV Test DynamicControl      | Device/System | execute_testcategory DynamicControl          | Future test.                  | mvhvoutput.txt        |
|   9 | Microsoft Teams MVHV Test IDR                 | Device/System | execute_testcategory IDR                     | Future test.                  | mvhvoutput.txt        |
|  10 | Microsoft Teams MVHV Test Simulcast           | Device/System | execute_testcategory Simulcast               | Future test.                  | mvhvoutput.txt        |
|  11 | Microsoft Teams MVHV Test QualitySingleStream | Device/System | execute_testcategory QualitySingleStream     | Future test.                  | mvhvoutput.txt        |
|  12 | Microsoft Teams MVHV Test Basic               | Device/System | execute_testcategory Basic                   | Future test.                  | mvhvoutput.txt        |
|  13 | Microsoft Teams MVHV Test Camera              | Device/System | execute_test --mode Camera                   | Future test. To replace No 3? | mvhvoutput.txt        |
|  14 | Microsoft Teams MVHV Test Audio               | Device/System | execute_test --mode Audio                    | Future test.                  | mvhvoutput.txt        |
|  15 | Microsoft Teams MVHV Test Render              | Device/System | execute_test --mode Render                   | Future test.                  | mvhvoutput.txt        |

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
powershell
cd C:\

Start-BitsTransfer `
  -Source "https://go.microsoft.com/fwlink/?linkid=2343402" `
  -Destination "C:\Clips.iso" `
  -DisplayName "Download Clips.ISO" `
  -Description "Downloading Clips.ISO"
$diskImage = Mount-DiskImage -NoDriveLetter -PassThru -ImagePath C:\Clips.iso
$volumeInfo = $diskImage | Get-Disk | Get-Partition | Get-Volume
mountvol M: $volumeInfo.UniqueId
robocopy /mir /nfl /ndl M:\ C:\Clips
Dismount-DiskImage -ImagePath C:\Clips.iso
```

##### Install .NET on Client
The underlying Microsoft Teams Test Suite requires a special .NET version.  This is not automatically installed. As such you need to manually install.
* For AMD64 devices: Download and run https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-9.0.16-windows-x64-installer?cid=getdotnetcore .
* For ARM64 devices: Download and run https://aka.ms/dotnet-core-applaunch?missing_runtime=true&arch=arm64&rid=win-arm64&os=win10&apphost_version=9.0.15 .

##### Manually deploy MVHV from HLK controller to HLK Client

As a technical (and hopefully very temporary) limitation you must manually deploy the underlying MVHV tests from HLK controller to HLK client.

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

## Running HLK MVHV Tests
How to run, how to inspect test ran succesfully, how to analyze results, how to get detailed logs from client
How to Run tests:
----------------
To Run tests from HLK Controller you have to connect controller with a Device that has GPU and a Camera.
In selection screen you have to select your GPU and select Device.Streaming.HMFT.MLVEC by right clicking and adding feature.
To run camera tests you have to do similar step for camera entry in selection screen and picking Device.Streaming.Camera.Videocapture
Afterwards you will see MVHV tests in your Tests tab on your HLK Controller. 
Select the test you wish to run and choose run selected option by right clicking the view 

Test Results:
-------------
If all the tests show green check sign it means tests ran successfully and they passed. Instead if they show red cross sign test failed.
For initial test name and result related log you can check the associated logfile name from table in section "Mapping HLK MVHV Tests to 
underlying MVHV Test Suites" and look for that file under c:\mvhv folder. It is a text file editable in notepad or any other text editor.
For detailed logs there will be a folder called Results under c:\mvhv. For highly technical users please provide these set of logs. 

## Our Test Experiments and Results
TODO: Will showcase the specific hardware we have tested on and results

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
