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

### Environment specific requisites

- If you are using the WDK via **NuGet**: install NuGet and restore the packages:

```powershell
winget install --id Microsoft.NuGet --source winget
nuget restore -PackagesDirectory ".\packages"
```

- If you are using the WDK via **EWDK**: mount the EWDK ISO, open a terminal in the mounted drive, and launch the build environment:

```powershell
.\LaunchBuildEnv
```


---

## Building the Samples

The `Build-Samples.ps1` script auto-detects which WDK environment is active and will build all the samples with all the configurations by default. Just run the following command from **PowerShell**:

```powershell
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

# Build every sample targeting an older OS version (default is the latest, Windows10):
.\Build-Samples.ps1 -TargetVersion Windows7
```

The `-TargetVersion` values come from the WDK `DriverGeneral.xml` rule and are listed
newest-first. The default is the latest, `Windows10`:

| Value         | Target OS              |
| ------------- | ---------------------- |
| `Windows10`   | Windows 10 or higher (default) |
| `WindowsV6.3` | Windows 8.1            |
| `Windows8`    | Windows 8              |
| `Windows7`    | Windows 7              |

---

## Excluding samples from the build

Samples that are known not to build for a given environment are listed in `exclusions.csv`
at the repo root. Each row excludes a path (with wildcards) for specific
configuration/platform combinations, an optional WDK build-number range, and an optional
set of target versions:

```
Path,Configurations,TargetVersions,MinBuild,MaxBuild,Reason
```

| Column           | Meaning                                                                                  |
| ---------------- | ---------------------------------------------------------------------------------------- |
| `Path`           | Sample path (backslashes); supports `*`/`?` wildcards.                                    |
| `Configurations` | `;`-separated `Config\|Platform` patterns, or `*` for all (e.g. `*\|ARM64`, `Debug\|x64`). |
| `TargetVersions` | `;`-separated `-like` patterns matched against `-TargetVersion`; blank or `*` = all (e.g. `Windows8`, `Windows7;Windows8`, `Windows*`). |
| `MinBuild`/`MaxBuild` | Inclusive WDK build-number range; blank = unbounded.                                |
| `Reason`         | Human-readable explanation (keep this column last; quote it if it contains commas).      |

A row is applied only when every populated condition matches the current run (path,
configuration/platform, build-number range, and target version are AND-ed together). Leave
`TargetVersions` blank to exclude regardless of target version (the default for most rows).

For example, to exclude all ARM platforms only when building for Windows 8:

```
somepath,*|ARM64,Windows8,,,"ARM not supported when targeting Windows 8"
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

