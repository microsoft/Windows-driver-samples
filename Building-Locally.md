# How to build locally

## Step 1: Install git and pwsh 7.3.0
```
winget install --id Microsoft.Powershell --source winget
winget install --id Git.Git --source winget`
```

## Step 2: Create a "driver build environment"

There are multiple ways to achieve this. For example, [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk). 

You can also just download and mount the EWDK as well and in the following example that is what we will do:
  * Download the Windows 11, version 22H2 EWDK ISO image from the [official site](https://learn.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022)
  * Mount ISO image
  * Open a terminal
  * `.\LaunchBuildEnv`
  
## Step 3: Clone Windows Driver Samples and checkout main branch

```
cd path\to\your\repos
git clone --recurse-submodules https://github.com/microsoft/Windows-driver-samples.git
cd Windows-driver-samples
```

## Step 4: Check all samples builds with expected results for all flavors

```
pwsh
.\Build-AllSamples
```
Above builds all samples for all configurations and platforms.  

You can refine, for example as follows:
```
pwsh
.\Build-AllSamples -Samples '^tools.' -Configurations 'Debug','Release' -Platforms 'x64','arm64'
```

Expected output:
```
Samples:              153
Configurations:       2 (Debug Release)
Platforms:            2 (x64 arm64)
Combinations:         612
Logical Processors:   12
Throttle factor:      5
Throttle limit:       60

T: Combinations
B: Built
R: Build is running currently
P: Build is pending an available build slot

S: Built and result was 'Succeeded'
E: Built and result was 'Excluded'
U: Built and result was 'Unsupported' (Platform and Configuration combination)
F: Built and result was 'Failed'

Building all combinations...

Built all combinations.

Elapsed time:         12 minutes, 34 seconds.
Samples:              153
Configurations:       2 (Debug Release)
Platforms:            2 (x64 arm64)
Combinations:         612
Succeeded:            326
Excluded:             56
Unsupported:          230
Failed:               0
Log files directory:  .\_logs
Overview report:      .\_logs\overview.htm
```
