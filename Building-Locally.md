# How to build locally

## Step 1: Install git and pwsh 7.3.0
```
winget install --id Microsoft.Powershell --source winget
winget install --id Git.Git --source winget`
```

## Step 2: Create a "driver build environment"

There are multiple ways to achieve this. For example, [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk). You can just download and mount the EWDK as well.

In the following example that is what we will do:
  * Download the Windows 11, version 22H2 EWDK ISO image from the [official site](https://learn.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022)
  * Mount ISO image
  * From a terminal, run `.\LaunchBuildEnv`

## Step 3: Clone Windows Driver Samples and checkout main branch

```
cd path\to\your\repos
git clone --recurse-submodules https://github.com/microsoft/Windows-driver-samples.git
cd Windows-driver-samples
```

## Step 4: Check all samples builds with expected results for all flavors

```
pwsh
.\Build-AllSamples.ps1 -Configurations 'Debug','Release' -Platforms 'x64','arm64' -LogFilesDirectory '_logs'
```

Expected output:
```
T: Total solutions: 612
B: Built
R: Build is running currently
P: Build is pending an available build slot

S: Built and result was 'Succeeded'
E: Built and result was 'Excluded'
U: Built and result was 'Unsupported' (Platform and Configuration combination)
F: Built and result was 'Failed'

Building driver solutions...

Built solutions.

Total elapsed time:   11 minutes, 18 seconds.
SolutionsTotal:       612
SolutionsSucceeded:   316
SolutionsExcluded:    56
SolutionsUnsupported: 240
SolutionsFailed:      0

Results saved to _logs\overview.htm
```