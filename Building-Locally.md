# How to build locally

## Step 1: Install Tools

```
winget install --id Microsoft.Powershell --source winget
winget install --id Git.Git --source winget
```

For using WDK NuGet feed based build (experimental) additionally:
```
winget install --id Microsoft.NuGet --source winget
```

## Step 2: Optional: Disable Strong Name Signing

When: This step is required only if you will be using pre-release versions of the WDK.

As per https://learn.microsoft.com/en-us/windows-hardware/drivers/installing-preview-versions-wdk you will need to disable strong 

Run the following commands from an elevated command prompt to disable strong name validation:

```
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f
```

## Step 3: Optional: Install Microsoft .NET Framework 4.7.2 Targeting Pack and Microsoft .NET Framework 4.8.1 SDK

When: This step is required only to build sample usb\usbview .

### Option A: Use EWDK
Easiest: If you use EWDK, then all necessary prequisites are included.

### Option B: Install VS Components
Almost as easy: If you will install Visual Studio (see later) you may at that point select to add both of following individual components:
* .NET Framework 4.7.2 targeting pack
* .NET Framework 4.8.1 SDK

### Option C: Install Developer Pack

Hardest: Install from https://aka.ms/msbuild/developerpacks -> '.NET Framework' -> 'Supported versions' both of following packages:
* .NET Framework 4.7.2 -> Developer Pack 
* .NET Framework 4.8.1 -> Developer Pack 

This will install following Apps:
* Microsoft .NET Framework 4.7.2 SDK
* Microsoft .NET Framework 4.7.2 Targeting Pack
* Microsoft .NET Framework 4.7.2 Targeting Pack (ENU)
* Microsoft .NET Framework 4.8.1 SDK
* Microsoft .NET Framework 4.8.1 Targeting Pack
* Microsoft .NET Framework 4.8.1 Targeting Pack (ENU)

## Step 3: Clone Windows Driver Samples and checkout relevant branch

```
cd path\to\your\repos
git clone --recurse-submodules https://github.com/microsoft/Windows-driver-samples.git
cd Windows-driver-samples
```

If you are planning to use in-market WDK, then you would typically want to use the 'main' branch:
```
git checkout main
```

If you are planning to use a WDK Preview or WDK EEAP release, then you would typically want to use the 'develop' branch:
```
git checkout develop
```

## Step 4: Create a "driver build environment"

To build the Windows Driver Samples you need a "driver build environment".  In essence an environment that consist of following prerequisites:
* Visual Studio Build Tools including tools such as for example cl.exe and link.exe .
* The Windows Software Development Kit.
* The Windows Driver Kit.

### Option A: Use the Windows Driver Kit
* Here you will install each of above prerequisites one at a time.
* See [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk) for instructions. 
* Launch a "Developer Command Prompt for VS 2022".

### Option B: Use an Enterprise WDK
* You can also simply use the Enterprise WDK (EWDK), a standalone, self-contained command-line environment for building drivers that contains all prerequisites in one combined ISO.
* Download the Windows 11, version 22H2 EWDK ISO image from the [official site](https://learn.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022)
* Mount ISO image
* Open a terminal
* `.\LaunchBuildEnv`

### Option C: Use WDK NuGet Packages
* This is an 'experimental' option.
* This option requires you have access to the WDK NuGet EEAP Package (so far only available to EEAP Partners).
* See [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk) for instructions on how to install Visual Studio, but do not install the WDK or download the EWDK.
* Install the Visual Studio Windows Driver Kit Extension (WDK.vsix).  Open Visual Studio -> Extensions -> Manage Extensions... -> Online -> Visual Studio Market Place -> Windows Driver Kit -> 10.0.26090.10
* Launch a "Developer Command Prompt for VS 2022".
* Unzip the WDK NuGet zip (that you have access to as part of EEAP program).  This folder should look as follows:

```
>dir /b C:\my\local\package
Microsoft.Windows.SDK.CPP.10.0.26090.8-preview.ge-release.nupkg
Microsoft.Windows.SDK.CPP.x64.10.0.26090.8-preview.ge-release.nupkg
Microsoft.Windows.SDK.CPP.arm64.10.0.26090.8-preview.ge-release.nupkg
Microsoft.Windows.WDK.x64.10.0.26090.8-preview.ge-release.nupkg
Microsoft.Windows.WDK.arm64.10.0.26090.8-preview.ge-release.nupkg
```
* This is an 'experimental' option.
* Add MSFTNuget feed and restore WDK packages from feed :

```
>cd path\to\your\repos\Windows-driver-samples
>nuget sources add -Name EEAPPackages -Source C:\my\local\package
>nuget restore -PackagesDirectory .\packages
```

* When this is done you should have a .\packages folder that looks exactly as follows:
```
>cd path\to\your\repos\Windows-driver-samples
>dir /b packages
Microsoft.Windows.SDK.CPP.10.0.26090.8-preview.ge-release
Microsoft.Windows.SDK.CPP.x64.10.0.26090.8-preview.ge-release
Microsoft.Windows.SDK.CPP.arm64.10.0.26090.8-preview.ge-release
Microsoft.Windows.WDK.x64.10.0.26090.8-preview.ge-release
Microsoft.Windows.WDK.arm64.10.0.26090.8-preview.ge-release
```


## Step 5: Check all samples builds with expected results for all flavors

```
pwsh
.\Build-AllSamples
```
Above builds all samples for all configurations and platforms.  

You can refine what exact samples to build, what configurations, and platforms to build.  build Here are a few examples:
```
# Get Help:
Get-Help .\Build-AllSamples

# Build all solutions for all flavors with builds running in parallel:
.\Build-AllSamples

# Build with Verbose output (print start and finish of each sample):
.\Build-AllSamples -Verbose

# Build without massive parallism (slow, but good debugging):
.\Build-AllSamples -ThrottleLimit 1

# Build the solutions in the tools folder for all flavors:
.\Build-AllSamples -Samples '^tools.' -Configurations 'Debug','Release' -Platforms 'x64','arm64'

# Build the solutions in the tools folder for only 'Debug|x64':
.\Build-AllSamples -Samples '^tools.' -Configurations 'Debug' -Platforms 'x64'
```

Expected output:
```
Build Environment:          EWDK.ge_release.26052.1000
Build Number:               26052
Samples:                    131
Configurations:             2 (Debug Release)
Platforms:                  2 (x64 arm64)
InfVerif_AdditionalOptions: /samples /sw1402
Combinations:               524
LogicalProcessors:          12
ThrottleFactor:             5
ThrottleLimit:              60
WDS_WipeOutputs:
Disk Remaining (GB):        106.204154968262

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

Elapsed time:         15 minutes, 23 seconds.
Disk Remaining (GB):  100.549266815186
Samples:              131
Configurations:       2 (Debug Release)
Platforms:            2 (x64 arm64)
Combinations:         524
Succeeded:            296
Excluded:             10
Unsupported:          218
Failed:               0
Log files directory:  C:\Users\jakobl\source\repos\Windows-driver-samples\Wds3\_logs
Overview report:      C:\Users\jakobl\source\repos\Windows-driver-samples\Wds3\_logs\_overview.htm
```

# NuGet - Additional Notes

To restore a specific version of our WDK NuGet packages:

Follow following steps before running "nuget restore" command:
* Open the .\packages.config file and update the full version (including the branch if required) in all three entries.
* Open the .\Directory.build.props file and update the version and build of the package with the same values as in previous step.
* Open .\Build-SampleSet and change the NuGet build number (used by .\exclusions.csv and for determining infverif flags)
* Now you can run "nuget restore"
