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


## Step 2: Microsoft .NET Framework 4.7.2 SDK

This step is required specifically to build sample usb\usbview .

### Option A: Install Developer Pack

Install https://aka.ms/msbuild/developerpacks -> .NET Framework -> Supported versions -> .NET Framework 4.7.2 -> Developer Pack -> https://dotnet.microsoft.com/en-us/download/dotnet-framework/thank-you/net472-developer-pack-offline-installer -> https://go.microsoft.com/fwlink/?linkid=874338 .  

This will install following Apps:
* Microsoft .NET Framework 4.7.2 SDK
* Microsoft .NET Framework 4.7.2 Targeting Pack
* Microsoft .NET Framework 4.7.2 Targeting Pack (ENU)

### Option B: Install VS Components
If you install Visual Studio (see later) you may at that point select either additional Workload '.NET desktop development' or simply individual components 'NET Framework 4.7.2 SDK' and '.NET Framework 4.7.2 targeting pack'.

### Option C: Use EWDK
If you use EWDK, then Microsoft .NET Framework 4.8.1 SDK will be included.  That is sufficient.

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

### Option C: Use WDK NuGet Packages from EEAP Preview Zip (experimental)
* This is an 'experimental' option.
* This option requires you have access to the WDK NuGet packages (so far only available to EEAP Partners).
* See [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk) for instructions on how to install Visual Studio, but do not install the WDK or download the EWDK.
* Install the WDK.vsix (that you have access to as part of EEAP program).
* Launch a "Developer Command Prompt for VS 2022".
* Unzip the WDK NuGet zip (that you have access to as part of EEAP program) to the root of your git enlistment in a folder that must be named .\packages. 
* When this is done you should have a .\packages folder that looks exactly as follows:

```
>cd path\to\your\repos\Windows-driver-samples
>dir /b packages
Microsoft.Windows.SDK.CPP.10.0.26052.1000-preview.ge-release
Microsoft.Windows.SDK.CPP.x64.10.0.26052.1000-preview.ge-release
Microsoft.Windows.WDK.x64.10.0.26052.1000-preview.ge-release
```
TODO: Make sure this actually matches our EEAP build out of GE_RELEASE.

TODO: Make sure above is "officially supported by DevDiv".


### Option D: Use WDK NuGet Packages from "experimental package feed"
* This is an 'experimental' option.
* This option requires you have access to "experimental package feed" MSFTNuget.  TODO:Explain what this is.
* See [install Visual Studio and the Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-and-install-the-windows-11-version-22h2-wdk) for instructions on how to install Visual Studio, but do not install the WDK or download the EWDK.
* Install the WDK.vsix from [Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=DriverDeveloperKits-WDK.WDKVsix) .
* Launch a "Developer Command Prompt for VS 2022".
* Add MSFTNuget feed and restore WDK packages from feed :

```
>cd path\to\your\repos\Windows-driver-samples
>nuget sources Add -Name MSFTNuget -Source https://microsoft.pkgs.visualstudio.com/_packaging/MSFTNuget/nuget/v3/index.json
>nuget restore -PackagesDirectory .\packages
```

* When this is done you should have a .\packages folder that looks exactly as follows:
```
>cd path\to\your\repos\Windows-driver-samples
>dir /b packages
Microsoft.Windows.SDK.CPP.10.0.26052.1000-preview.ge-release
Microsoft.Windows.SDK.CPP.x64.10.0.26052.1000-preview.ge-release
Microsoft.Windows.WDK.x64.10.0.26052.1000-preview.ge-release
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
