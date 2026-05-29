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

## Mapping HLK MVHV Tests to underlying MVHV Test Suites

| NO   | HLK Test Name         |                        MVHV Test Command                                                       |         Associated Logfile                |
|:-----|:---------------------:|-----------------------------------------------------------------------------------------------:|:-----------------------------------------:|
| 1    |  MVHV Basic Test      |                        MicrosoftVideoHardwareValidator.exe -v                                  |      MvhvVersionOutput.txt                |
| 2    |  MVHV AV Encoder Test |                        MicrosoftVideoHardwareValidator.exe  execute_test Instantiation -c H264 |      MvhvSanityOutput.txt                 |
| 3    |  MVHV Camera Test     |                        MicrosoftVideoHardwareValidator.exe  execute_test --mode Camera         |      MvhvCameraOutput.txt                 |

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

##### Install appropriate .NET on Client
The underlying Microsoft Teams Test Suite requires a special .NET version.  Unfortunately as of 0.0.1 release this is not automatically installed. As such you need to manually install.

As of release 0.0.1 (MVP) we are supporting AMD64 and ARM64 architectures for clients or DUTs.

For AMD64 devices please download .net from https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-9.0.16-windows-x64-installer?cid=getdotnetcore and run a setup wizard.

For ARM64 devices please download .net from https://aka.ms/dotnet-core-applaunch?missing_runtime=true&arch=arm64&rid=win-arm64&os=win10&apphost_version=9.0.15  and run a setup wizard.



##### Manually deploy MVHV from HLK controller to HLK Client

As a technical (and hopefully very temporary) limitation you must manually deploy the underlying MVHV tests from HLK controller to HLK client.

For x64 based Clients:
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\C$\...\x64 C:\mvhv
```

For arm64 based Clients:
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\C$\...\arm64 C:\mvhv
```

After installing mvhv on client machine please make sure that MicrosoftVideoHardwareValidator.config file is present under c:\mvhv and it has following entries.
<?xml version="1.0" encoding="utf-8" ?>
<configuration>
        <appSettings>
                <add key="resultfolder" value="."/>
                <add key="clipsfolder" value="c:\clips\"/>
                <add key="rundxdiag" value="True" />
        </appSettings>
</configuration>
In case any such lines are missing you can manually edit file to have these entries. 


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
* TODO: Today you have to manually copy MVHV logs to target.   We hope to fix this by release 0.0.3.
* TODO: Today you have only a few rudimentary tests.  We hope to have complete exposure of all of MVHV test by release 0.0.5.
* TODO: Today detailed MVHV logs are left on Client machine. We hope to pull them back into HLK test results by release 0.0.3.
* TODO: Today Clips deployment is a little messy. We hope to clean this up by release TBD.
* TODO: Today detecting prerequisites is not done well.  We hope to remove some prerequisites (say, MVHV robocopy) and hope to add better detection of other prerequisites by release TBD.
