# mtabow

## Overview

* What: Focus on improving quality of Microsoft Teams on Windows Platform. 
* Web: https://aka.ms/mtabow
* Email: mtabow@microsoft.com

## Running Microsoft Teams Test Suite as part of the Hardware Lab Kit (HLK)

### Prerequisites:
* Setup HLK Controller, see https://learn.microsoft.com/en-us/windows-hardware/test/hlk/ .
* Setup HLK Client, see https://learn.microsoft.com/en-us/windows-hardware/test/hlk/ .
* Install .NET xyz
* Gather Clips
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

* Manually deploy MVHV from HLK controller to HLK Client
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\C$\...\x64 C:\mvhv
```
### ...
...
