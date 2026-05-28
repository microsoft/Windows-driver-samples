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
* TODO. Will show: HLK test type (system or device), HLK targeting information, matching MVHV command.

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

##### Install .NET xyz on Client
The underlying Microsoft Teams Test Suite requires a special .NET version.  Unfortunately this is not automatically installed. As such you need to manually install.

```
TODO
```

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

## Running HLK MVHV Tests
TODO. How to run, how to inspect test ran succesfully, how to analyze results, how to get detailed logs from client

## Our Test Experiments and Results
TODO: Will showcase the specific hardware we have tested on and results

## FAQ / Tips and Tricks
TODO

## Known issues and planned changes
* TODO: Today detailed MVHV logs are left on Client machine. We hope to pull them back into HLK test results.
* TODO: Today Clips deployment is a little messy. We hope to clean this up.
* TODO: Today detecting prerequisites is not done well.  We hope to remove some prerequisites (say, MVHV robocopy) and hope to add better detection of other prerequisites.
