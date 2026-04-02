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
    Write-Output ""
    Write-Output "The following Visual Studio installations were found with the required WDK media:"
    for ($i = 0; $i -lt $Installations.Count; $i++) {
        Write-Output "  [$($i + 1)] $($Installations[$i].DisplayName)  —  $($Installations[$i].InstallationPath)"
    }
    Write-Output ""

    do {
        $choice = Read-Host "Select the installation to use [1-$($Installations.Count)]"
        $index  = [int]$choice - 1
    } while ($index -lt 0 -or $index -ge $Installations.Count)

    return $Installations[$index]
}

function Initialize-DevShell {
    <#
    .SYNOPSIS
        Imports the Visual Studio Developer PowerShell if not already active.
    .OUTPUTS
        Returns the selected VS installation object ({ DisplayName, InstallationPath,
        WdkVsComponentVersion }), or $null if the shell was already active.
        Callers should pass the returned object to Resolve-BuildEnvironment so VS
        selection happens exactly once.
    #>
    param([string]$ReturnToDirectory)

    if ($env:VSCMD_VER) {
        Write-Verbose "VS Developer Shell already active (VSCMD_VER=$env:VSCMD_VER)."
        return $null
    }

    $vsInstall = Select-VsInstallation (Get-VsInstallationsWithWdk)

    $devShellDll = Join-Path $vsInstall.InstallationPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    if (-not (Test-Path $devShellDll)) {
        Write-Error "Visual Studio Developer Shell module not found at '$devShellDll'."
        exit 1
    }

    Import-Module $devShellDll
    Enter-VsDevShell -VsInstallPath $vsInstall.InstallationPath
    Set-Location $ReturnToDirectory
    return $vsInstall
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
        Detects or resolves the active build environment and returns metadata.
    .DESCRIPTION
        When RunMode is 'Auto', checks in priority order: EWDK, NuGet, WDK.
        EWDK is checked first because $env:BuildLab is an active, explicit signal
        whereas the packages\ folder is a passive disk artifact that may linger.
        When RunMode is explicitly set to WDK/NuGet/EWDK, skips detection and uses that mode.
        Pass the VsInstallation returned by Initialize-DevShell to avoid prompting the user
        a second time when multiple VS installations are present.
        Returns a hashtable: Name, BuildNumber (int), NuGetVersion, WdkVsComponentVersion.
    #>
    param(
        [string]$RepoRoot,
        [string]$RunMode = 'Auto',
        [object]$VsInstallation = $null
    )

    $result = @{
        Name                  = ''
        BuildNumber           = [int]0
        NuGetVersion          = ''
        WdkVsComponentVersion = ''
    }

    $effectiveMode = $RunMode

    # --- Resolve build environment ---
    if ($effectiveMode -eq 'EWDK' -or
        ($effectiveMode -eq 'Auto' -and $env:BuildLab -match '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$')) {
        if ($effectiveMode -eq 'EWDK') {
            # Forced EWDK: require BuildLab to be set
            if ($env:BuildLab -notmatch '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$') {
                Write-Error "RunMode is 'EWDK' but the EWDK environment variable BuildLab is not set. Ensure the EWDK is mounted and the environment is initialised."
                exit 1
            }
        }
        $result.Name        = "EWDK.$($Matches.branch).$($Matches.build).$($Matches.qfe)"
        $result.BuildNumber = [int]$Matches.build
    }
    elseif ($effectiveMode -eq 'NuGet' -or
            ($effectiveMode -eq 'Auto' -and (Test-Path "$RepoRoot\packages\*"))) {
        if ($effectiveMode -eq 'NuGet' -and -not (Test-Path "$RepoRoot\packages\*")) {
            Write-Error "RunMode is 'NuGet' but no packages were found under '$RepoRoot\packages\'. Ensure NuGet restore has been run."
            exit 1
        }
        $result.Name = 'NuGet'
        $wdkPackage = Get-ChildItem "$RepoRoot\packages\*WDK.x64*" -Name -ErrorAction SilentlyContinue
        $result.NuGetVersion = ([regex]'(?<=x64\.)(\d+\.){3}\d+').Match($wdkPackage).Value
        $result.BuildNumber  = [int]($result.NuGetVersion.Split('.')[2])
    }
    elseif ($effectiveMode -eq 'WDK' -or
            ($effectiveMode -eq 'Auto' -and $env:UCRTVersion -match '10\.0\.(?<build>\d+)\.0')) {
        if ($effectiveMode -eq 'WDK' -and $env:UCRTVersion -notmatch '10\.0\.(?<build>\d+)\.0') {
            Write-Error "RunMode is 'WDK' but UCRTVersion ('$env:UCRTVersion') is not set or does not match the expected format. Ensure the VS Developer Shell is active."
            exit 1
        }
        $result.Name        = 'WDK'
        $result.BuildNumber = [int]$Matches.build
    }
    else {
        Write-Output "Environment variables {"
        Get-ChildItem env:* | Sort-Object Name
        Write-Output "Environment variables }"
        Write-Error "Could not determine build environment. Ensure EWDK, WDK, or NuGet packages are configured."
        exit 1
    }

    # WDK VS component version (EWDK does not ship this metadata)
    if ($result.Name -match '^EWDK') {
        $result.WdkVsComponentVersion = '(not available for EWDK builds)'
    }
    else {
        # Re-use the installation selected during Initialize-DevShell if available,
        # otherwise query vswhere again (e.g. when the Dev Shell was already active).
        $vsInstall = if ($VsInstallation) { $VsInstallation } else { Select-VsInstallation (Get-VsInstallationsWithWdk) }
        if (-not $vsInstall.WdkVsComponentVersion) {
            Write-Error "Could not determine WDK component version for '$($vsInstall.DisplayName)'. Ensure the WDK Visual Studio component is installed."
            exit 1
        }
        $result.WdkVsComponentVersion = $vsInstall.WdkVsComponentVersion
    }

    return $result
}
