# BuildEnvironment.ps1
# Helper functions for detecting and initialising the build environment.
# Dot-sourced by Build-Samples.ps1.

function Get-VsInstallationsWithWdk {
    <#
    .SYNOPSIS
        Returns all Visual Studio installations that have the WDK component installed.
    .DESCRIPTION
        Uses vswhere.exe (from its fixed install location under Program Files (x86)) to
        enumerate VS installations that carry the Microsoft.Windows.DriverKit component.
        If vswhere.exe is not found at the expected path the installed VS version is too
        old to be supported and the script exits with an error.
    #>
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        Write-Error "vswhere.exe was not found at '$vswhere'. Visual Studio 2017 or later is required."
        exit 1
    }

    $json = & $vswhere -all -format json -requires Microsoft.Windows.DriverKit -include packages 2>$null
    $installations = $json | ConvertFrom-Json
    return $installations | ForEach-Object {
        $wdkPackage = $_.packages | Where-Object { $_.id -eq 'Microsoft.Windows.DriverKit' } | Select-Object -First 1
        [PSCustomObject]@{
            DisplayName          = $_.displayName
            InstallationPath     = $_.installationPath
            WdkVsComponentVersion = $wdkPackage.version
        }
    }
}

function Select-VsInstallation {
    <#
    .SYNOPSIS
        Chooses a Visual Studio installation from the list returned by Get-VsInstallationsWithWdk.
    .DESCRIPTION
        - 0 found  : error + exit
        - 1 found  : verbose log, return it
        - 2+ found : display a numbered menu and prompt the user to choose
    #>
    param([object[]]$Installations)

    if (-not $Installations -or $Installations.Count -eq 0) {
        Write-Error "No Visual Studio installation with the required WDK media was found. Ensure the WDK Visual Studio component is installed."
        exit 1
    }

    if ($Installations.Count -eq 1) {
        Write-Verbose "Found Visual Studio installation with required WDK media: $($Installations[0].DisplayName) at $($Installations[0].InstallationPath)"
        return $Installations[0]
    }

    # Multiple installations — let the user choose
    Write-Host ""
    Write-Host "The following Visual Studio installations were found with the required WDK media:"
    for ($i = 0; $i -lt $Installations.Count; $i++) {
        Write-Host "  [$($i + 1)] $($Installations[$i].DisplayName)  —  $($Installations[$i].InstallationPath)"
    }
    Write-Host ""

    do {
        $choice = Read-Host "Select the installation to use [1-$($Installations.Count)]"
        $index  = [int]$choice - 1
    } while ($index -lt 0 -or $index -ge $Installations.Count)

    return $Installations[$index]
}

function Assert-MsBuildAvailable {
    <#
    .SYNOPSIS Verifies msbuild.exe is on PATH. Exits with error if not found.
    #>
    $savedPref = $ErrorActionPreference
    $ErrorActionPreference = 'Stop'
    try {
        Get-Command 'msbuild' | Out-Null
    }
    catch {
        Write-Error "msbuild cannot be called from current environment. Ensure it is on PATH (run from VS Developer Command Prompt or EWDK)."
        exit 1
    }
    finally {
        $ErrorActionPreference = $savedPref
    }
}

function Resolve-BuildEnvironment {
    <#
    .SYNOPSIS
        Detects the active build environment, opens a VS Developer Shell when needed,
        and returns metadata about the environment.
    .DESCRIPTION
        Handles the full setup sequence in one place:
          1. Detect mode: EWDK → NuGet → WDK (Auto), or use the explicitly supplied RunMode.
          2. For NuGet / WDK: open a VS Developer Shell if one is not already active,
             prompting the user to choose if multiple VS installations with the required
             WDK media are found. If the shell is already active, the matching installation
             is located via $env:VSINSTALLDIR.
          3. For EWDK: skip VS detection entirely ($env:BuildLab is the authoritative signal).
        Returns a hashtable: Name, BuildNumber (int), NuGetVersion, WdkVsComponentVersion.
    #>
    param(
        [string]$RepoRoot,
        [string]$RunMode = 'Auto'
    )

    $result = @{
        Name                  = ''
        BuildNumber           = [int]0
        NuGetVersion          = ''
        WdkVsComponentVersion = ''
    }

    # -------------------------------------------------------------------------
    # Step 1 – Detect / validate build mode
    # -------------------------------------------------------------------------

    # EWDK: checked first. $env:BuildLab is an active, explicit signal that
    # disappears when you close the EWDK prompt, unlike the packages\ folder.
    if ($RunMode -eq 'EWDK' -or
        ($RunMode -eq 'Auto' -and $env:BuildLab -match '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$')) {

        if ($RunMode -eq 'EWDK' -and
            $env:BuildLab -notmatch '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$') {
            Write-Error "RunMode is 'EWDK' but the EWDK environment variable BuildLab is not set. Ensure the EWDK is mounted and the environment is initialised."
            exit 1
        }
        # Re-run the match to populate $Matches (the Auto branch already matched above;
        # the forced-EWDK branch needs an explicit match after the validation guard).
        $null = $env:BuildLab -match '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$'
        $result.Name                  = "EWDK.$($Matches.branch).$($Matches.build).$($Matches.qfe)"
        $result.BuildNumber           = [int]$Matches.build
        $result.WdkVsComponentVersion = '(not available for EWDK builds)'
        return $result
    }

    $isNuGet = ($RunMode -eq 'NuGet') -or
               ($RunMode -eq 'Auto'   -and (Test-Path "$RepoRoot\packages\*"))

    if ($RunMode -eq 'NuGet' -and -not (Test-Path "$RepoRoot\packages\*")) {
        Write-Error "RunMode is 'NuGet' but no packages were found under '$RepoRoot\packages\'. Ensure NuGet restore has been run."
        exit 1
    }

    # If not EWDK and not NuGet, assume WDK. VS Dev Shell setup below will validate
    # the environment; if no VS with WDK media is found, Select-VsInstallation errors out.

    # -------------------------------------------------------------------------
    # Step 2 – Set up VS Developer Shell
    # -------------------------------------------------------------------------

    $vsInstall = $null

    if (-not $env:VSCMD_VER) {
        # Dev Shell not active – open one now.
        $vsInstall   = Select-VsInstallation (Get-VsInstallationsWithWdk)
        $devShellDll = Join-Path $vsInstall.InstallationPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
        if (-not (Test-Path $devShellDll)) {
            Write-Error "Visual Studio Developer Shell module not found at '$devShellDll'."
            exit 1
        }
        Import-Module $devShellDll
        Enter-VsDevShell -VsInstallPath $vsInstall.InstallationPath
        Set-Location $RepoRoot
    }
    else {
        Write-Verbose "VS Developer Shell already active (VSCMD_VER=$env:VSCMD_VER)."
        # Locate the matching installation via VSINSTALLDIR so we can read its
        # WdkVsComponentVersion without prompting the user again.
        # Normalize trailing backslash: VSINSTALLDIR ends with '\', vswhere paths do not.
        $normalizedVsInstallDir = $env:VSINSTALLDIR.TrimEnd('\')
        $vsInstall = Get-VsInstallationsWithWdk |
                     Where-Object { $_.InstallationPath.TrimEnd('\') -eq $normalizedVsInstallDir } |
                     Select-Object -First 1
        if (-not $vsInstall) {
            Write-Error "The active Visual Studio Developer Shell ('$env:VSINSTALLDIR') does not have the required WDK media installed. Ensure the WDK Visual Studio component is installed."
            exit 1
        }
    }

    # -------------------------------------------------------------------------
    # Step 3 – Fill mode-specific fields (Dev Shell is now guaranteed active)
    # -------------------------------------------------------------------------

    if ($isNuGet) {
        $result.Name        = 'NuGet'
        $wdkPackage         = Get-ChildItem "$RepoRoot\packages\*WDK.x64*" -Name -ErrorAction SilentlyContinue
        $result.NuGetVersion = ([regex]'(?<=x64\.)(\d+\.){3}\d+').Match($wdkPackage).Value
        $result.BuildNumber  = [int]($result.NuGetVersion.Split('.')[2])
    }
    else {
        # WDK – Dev Shell is now active, UCRTVersion must be set.
        if ($env:UCRTVersion -notmatch '10\.0\.(?<build>\d+)\.0') {
            Write-Error "UCRTVersion ('$env:UCRTVersion') is not set or does not match the expected format. Ensure the VS Developer Shell is active."
            exit 1
        }
        $result.Name        = 'WDK'
        $result.BuildNumber = [int]$Matches.build
    }

    if (-not $vsInstall.WdkVsComponentVersion) {
        Write-Error "Could not determine WDK component version for '$($vsInstall.DisplayName)'. Ensure the WDK Visual Studio component is installed."
        exit 1
    }
    $result.WdkVsComponentVersion = $vsInstall.WdkVsComponentVersion

    return $result
}
