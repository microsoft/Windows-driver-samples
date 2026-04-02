# Building Driver Samples Locally

## Prerequisites

### Required tools

Install PowerShell and Git if you don't have them already:

```powershell
winget install --id Microsoft.Powershell --source winget
winget install --id Git.Git --source winget
```

### Install a supported version of the WDK

See [Download the Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) for all available installation options (NuGet packages, MSI installer, EWDK ISO).

### Clone the repository

```powershell
git clone --recurse-submodules "https://github.com/microsoft/Windows-driver-samples.git"
cd ".\Windows-driver-samples"
```

Use `main` for in-market WDK releases, `develop` for WDK Preview / EEAP builds:

```powershell
git checkout main      # stable / in-market
git checkout develop   # preview / EEAP
```

---

## Building the Samples

The script auto-detects which WDK environment is active. Use `-RunMode` to force a specific one if needed:

```powershell
.\Build-Samples.ps1 -RunMode NuGet    # force NuGet
.\Build-Samples.ps1 -RunMode EWDK     # force EWDK
.\Build-Samples.ps1 -RunMode WDK      # force WDK
```

### NuGet Package

Install NuGet if you don't have it:

```powershell
winget install --id Microsoft.NuGet --source winget
```

Restore the WDK packages and build from Powershell:

```powershell
nuget restore -PackagesDirectory ".\packages"
pwsh
.\Build-Samples.ps1
```

### EWDK

Mount the EWDK ISO, open a terminal in the mounted drive, launch the build environment, then build:

```powershell
.\LaunchBuildEnv
pwsh
.\Build-Samples.ps1
```

### WDK MSI/winget

Build the samples from PowerShell:

```powershell
pwsh
.\Build-Samples.ps1
```

---

## Expected Output

```
--- WDK Sample Build Plan ------------------------------------------
  Environment:      NuGet
  Build Number:     26100
  NuGet Version:    10.0.26100.1
  WDK VS Component: 10.0.26100.1882
  InfVerif Options: /samples

  Samples:          132 (0 skipped)
  Configurations:   Debug, Release
  Platforms:        x64, arm64
  Combinations:     528
  Exclusions:       4

  Parallelism:      60 jobs (12 cores x 5)
  Disk Free (GB):   ...
  Wipe Outputs:     False
--------------------------------------------------------------------

Progress legend:
  T=Total  B=Built  R=Running  P=Pending
  S=Succeeded  E=Excluded  U=Unsupported  F=Failed  O=Sporadic

Building all combinations...

--- Build Complete -------------------------------------------------
  Elapsed:          12m 42s
  Disk Free (GB):   ...

  Samples:          132
  Configurations:   Debug, Release
  Platforms:        x64, arm64
  Combinations:     528

  Succeeded:        526
  Excluded:         0
  Unsupported:      2
  Failed:           0
  Sporadic:         0

  Log directory:    .\_logs
  CSV report:       .\_logs\_overview.csv
  HTML report:      .\_logs\_overview.htm
--------------------------------------------------------------------
```

---

## Ways to Run

```powershell
# Show full parameter reference:
Get-Help .\Build-Samples.ps1 -Detailed

# Build everything (all samples, configurations, platforms):
.\Build-Samples.ps1

# Verbose output — prints start/finish of each sample:
.\Build-Samples.ps1 -Verbose

# Limit parallelism (useful for debugging build failures):
.\Build-Samples.ps1 -ThrottleLimit 1

# Build only samples inside the 'tools' folder:
.\Build-Samples.ps1 -Samples 'tools.*'

# Build a specific sample for Debug|x64 only:
.\Build-Samples.ps1 -Samples 'tools.sdv.samples.sampledriver' -Configurations 'Debug' -Platforms 'x64'
```

---

## Additional Notes

### Pre-release WDK: disable strong name validation

Required only when using pre-release WDK versions. Run from an elevated command prompt:

```
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\StrongName\Verification\*,31bf3856ad364e35 /v TestPublicKey /t REG_SZ /d 00240000048000009400000006020000002400005253413100040000010001003f8c902c8fe7ac83af7401b14c1bd103973b26dfafb2b77eda478a2539b979b56ce47f36336741b4ec52bbc51fecd51ba23810cec47070f3e29a2261a2d1d08e4b2b4b457beaa91460055f78cc89f21cd028377af0cc5e6c04699b6856a1e49d5fad3ef16d3c3d6010f40df0a7d6cc2ee11744b5cfb42e0f19a52b8a29dc31b0 /f
```

See [Installing preview versions of the WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/installing-preview-versions-wdk) for more details.

### Building `usb\usbview`: .NET Framework targeting packs

The `usb\usbview` sample requires .NET Framework 4.7.2 and 4.8.1. Choose one option:

- **VS installer** — add the *.NET Framework 4.7.2 targeting pack* and *.NET Framework 4.8.1 SDK* individual components when installing Visual Studio.
- **EWDK** — all required prerequisites are already included.
- **Manual** — download both Developer Packs from https://aka.ms/msbuild/developerpacks.

### NuGet: restoring a specific WDK version

To pin a specific WDK NuGet version before running `nuget restore`:

1. Open `.\packages.config` and update the version in all entries.
2. Open `.\Directory.build.props` and set the same version.
3. Run `nuget restore -PackagesDirectory ".\packages"`.

Useful NuGet commands:

```powershell
# Add an online feed:
nuget sources add -Name "MyFeed" -Source "https://nugetserver.com/_packaging/feedname/nuget/v3/index.json"

# Add a local feed:
nuget sources add -Name "MyFeed" -Source "\\path\to\mylocalrepo"

# Remove a feed:
nuget sources remove -Name "MyFeed"

# List local caches:
nuget locals all -list

# Clear local caches:
nuget locals all -clear
```

