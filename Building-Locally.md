# How to build locally

## Step 1: Install Tools

```powershell
winget install --id Microsoft.Powershell --source winget
winget install --id Git.Git --source winget
```

For using WDK NuGet feed based build additionally:

```powershell
winget install --id Microsoft.NuGet --source winget
```

---

## Step 2: Optional: Disable Strong Name Validation

When: This step is only required if you will be using pre-release versions of the WDK.

As per https://learn.microsoft.com/en-us/windows-hardware/drivers/installing-preview-versions-wdk :

Run the following commands from an elevated command prompt to disable strong name validation:

```
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f
```

---

## Step 3: Optional: Install Microsoft .NET Framework 4.7.2 Targeting Pack and Microsoft .NET Framework 4.8.1 SDK

When: This step is only required to build sample usb\usbview .

### Option A: Install VS Components

Easy: If you will install Visual Studio (see later) you may at that point select to add both of following individual components:
* .NET Framework 4.7.2 targeting pack
* .NET Framework 4.8.1 SDK

### Option B: Use EWDK

Easy: If you use EWDK, then all necessary prequisites are included.

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

---

## Step 4: Clone Windows Driver Samples and checkout relevant branch

```powershell
cd "path\to\your\repos"
git clone --recurse-submodules "https://github.com/microsoft/Windows-driver-samples.git"
cd ".\Windows-driver-samples"
```

If you are planning to use in-market WDK, then you would typically want to use the 'main' branch:

```
git checkout main
```

If you are planning to use a WDK Preview or WDK EEAP release, then you would typically want to use the 'develop' branch:

```
git checkout develop
```

---

## Step 5: Create a "driver build environment"

To build the Windows Driver Samples you need a "driver build environment".  In essence an environment that consist of following prerequisites:
* Visual Studio Build Tools including tools such as for example cl.exe and link.exe .
* The Windows Software Development Kit.
* The Windows Driver Kit.

### Option A: Use WDK NuGet Packages

* See [Download the Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) for instructions on how to install Visual Studio, but only complete `Step 1`.  You do not need to install the SDK or the WDK.
* Launch a "Developer Command Prompt for VS 2022".
* Restore WDK packages from feed :

```powershell
cd "path\to\your\repos\Windows-driver-samples"
nuget restore -PackagesDirectory ".\packages"
```

* When this is done you should have a .\packages folder that looks like example below:

```powershell
cd "path\to\your\repos\Windows-driver-samples"
dir /b packages
Microsoft.Windows.SDK.CPP.10.0.26000.1
Microsoft.Windows.SDK.CPP.x64.10.0.26000.1
Microsoft.Windows.SDK.CPP.arm64.10.0.26000.1
Microsoft.Windows.WDK.x64.10.0.26000.1
Microsoft.Windows.WDK.arm64.10.0.26000.1
```

### Option B: Use the Windows Driver Kit

* Here you will install each of above prerequisites one at a time.
* See [Download the Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) for instructions on how to install Visual Studio, SDK, and WDK.
* Launch a "Developer Command Prompt for VS 2022".

### Option C: Use an Enterprise WDK

* You can also simply use the Enterprise WDK (EWDK), a standalone, self-contained command-line environment for building drivers that contains all prerequisites in one combined ISO.
* See [Download the Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) for instructions on how to download the EWDK.
* Mount ISO image
* Open a terminal
* `.\LaunchBuildEnv`

---

## Step 6: Check all samples builds with expected results for all flavors

```powershell
pwsh
.\Build-AllSamples
```
Above builds all samples for all configurations and platforms.  

You can refine what exact samples to build, what configurations, and platforms to build.  build Here are a few examples:

```powershell
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

Example of expected output:

```
Build Environment:          NuGet
Build Number:               26100
Samples:                    132
Configurations:             2 (Debug Release)
Platforms:                  2 (x64 arm64)
InfVerif_AdditionalOptions: /samples
Combinations:               528
LogicalProcessors:          12
ThrottleFactor:             5
ThrottleLimit:              60
WDS_WipeOutputs:
Disk Remaining (GB):        ...

T: Combinations
B: Built
R: Build is running currently
P: Build is pending an available build slot

S: Built and result was 'Succeeded'
E: Built and result was 'Excluded'
U: Built and result was 'Unsupported' (Platform and Configuration combination)
F: Built and result was 'Failed'
O: Built and result was 'Sporadic'

Building all combinations...

Built all combinations.

Elapsed time:         12 minutes, 42 seconds.
Disk Remaining (GB):  ...
Samples:              132
Configurations:       2 (Debug Release)
Platforms:            2 (x64 arm64)
Combinations:         528
Succeeded:            526
Excluded:             0
Unsupported:          2
Failed:               0
Sporadic:             0
Log files directory:  .\_logs
Overview report:      .\_overview.htm
```

---

## 7: NuGet - Additional Notes

To restore a specific version of our WDK NuGet packages:

Follow these steps before running "nuget restore" command:
* Open the .\packages.config file and update the full version (including the branch if required) in all three entries.
* Open the .\Directory.build.props file and update the version and build of the package with the same values as in previous step.
* Open .\Build-SampleSet and change the NuGet build number (used by .\exclusions.csv and for determining infverif flags)
* Now you can run "nuget restore"

A few examples of how to interact with nuget:

```powershell
# To add an alternative online NuGet source:
nuget sources add -Name "MyNuGetFeed" -Source "https://nugetserver.com/_packaging/feedname/nuget/v3/index.json"

# To add an alternative local NuGet source:
nuget sources add -Name "MyNuGetFeed" -Source "\\path\to\mylocalrepo"

# To remove an alternative NuGet source:
nuget sources remove -Name "MyNuGetFeed"

# To enumerate NuGet locals:
nuget locals all -list

# To clear NuGet locals:
nuget locals all -clear
```
