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
Start-BitsTransfer `
  -Source "https://go.microsoft.com/fwlink/?linkid=2343402" `
  -Destination "C:\Clips.iso" `
  -DisplayName "Download Clips.ISO" `
  -Description "Downloading Clips.ISO" `
mount to, say, E:\
robocopy /mir /nfl /ndl E:\ C:\Clips
```

* Manually deploy MVHV from HLK controller to HLK Client
```
powershell
robocopy /mir /nfl /ndl \\hlkcontroller\C$\...\x64 C:\mvhv
```
### ...
...
