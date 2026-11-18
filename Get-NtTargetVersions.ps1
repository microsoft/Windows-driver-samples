<#
.SYNOPSIS
    Auto-discovers the valid _NT_TARGET_VERSION values from the active WDK.

.DESCRIPTION
    The _NT_TARGET_VERSION property (the OS version of the libraries a driver links against)
    is an enumeration defined by the WDK in its 'DriverGeneral.xml' rule file. This script
    locates that rule file (from the restored NuGet packages, or the installed WDK) and parses
    the enumeration so that nothing in the build needs a hard-coded version list: when a new
    WDK adds a new _NT_TARGET_VERSION it is picked up automatically.

    Returns one object per Windows 10/11 entry, newest-first:
        Version  e.g. 10.0.28000   (use with -NtTargetVersion)
        Tag      e.g. 28000        (short, filename/CI-friendly)
        Code     e.g. 0xA000012    (the NTDDI value passed to msbuild)
        Build    e.g. 28000        (numeric, for sorting/ranges)

.PARAMETER XmlPath
    Optional explicit path to a DriverGeneral.xml. When omitted the newest available rule file
    in the selected RunMode is auto-located. An explicit path overrides RunMode discovery.

.PARAMETER RunMode
    Filters rule discovery to restored NuGet packages, the installed WDK, or the active EWDK
    environment. Valid values: Auto, WDK, NuGet, EWDK. Defaults to Auto, which selects an
    active EWDK first, then restored NuGet packages, then the installed WDK.
    A selected source never falls back to another source if its rule file is missing.

.PARAMETER Newest
    Return only the newest N versions (0 = all). Useful for bounding the CI build matrix.

.PARAMETER AsMatrixJson
    Emit a compact JSON array of { version, tag } objects for a GitHub Actions matrix
    (consumed via fromJSON). Implies a single-line output.

.EXAMPLE
    .\Get-NtTargetVersions.ps1                  # all discovered versions (objects)

.EXAMPLE
    .\Get-NtTargetVersions.ps1 -Newest 4 -AsMatrixJson

.EXAMPLE
    .\Get-NtTargetVersions.ps1 -RunMode NuGet -Newest 1
#>
[CmdletBinding()]
param(
    [string]$XmlPath,
    [int]$Newest = 0,
    [switch]$AsMatrixJson,
    [ValidateSet('Auto', 'WDK', 'NuGet', 'EWDK')]
    [string]$RunMode = 'Auto'
)

function Find-DriverGeneralXml {
    param([string]$RunMode)

    if ($RunMode -eq 'Auto') {
        if ($env:BuildLab -match '^(?<branch>[^.]+)\.(?<build>\d+)\.(?<qfe>[^.]+)$') {
            $RunMode = 'EWDK'
        }
        elseif (Test-Path (Join-Path $PSScriptRoot 'packages\*')) {
            $RunMode = 'NuGet'
        }
        else {
            $RunMode = 'WDK'
        }
    }

    $candidates = @()
    if ($RunMode -eq 'NuGet') {
        $candidates += Get-ChildItem -Path (Join-Path $PSScriptRoot 'packages') -Recurse -Filter 'DriverGeneral.xml' -ErrorAction SilentlyContinue
    }
    elseif ($RunMode -eq 'WDK') {
        foreach ($kitsRoot in @("${env:ProgramFiles(x86)}\Windows Kits\10\build", "${env:ProgramFiles}\Windows Kits\10\build")) {
            if ($kitsRoot -and (Test-Path $kitsRoot)) {
                $candidates += Get-ChildItem -Path $kitsRoot -Recurse -Filter 'DriverGeneral.xml' -ErrorAction SilentlyContinue
            }
        }
    }
    elseif ($RunMode -eq 'EWDK') {
        foreach ($envRoot in @($env:WDKContentRoot, $env:WindowsSdkDir)) {
            if ($envRoot -and (Test-Path $envRoot)) {
                $buildDir = Join-Path $envRoot 'build'
                if (Test-Path $buildDir) {
                    $candidates += Get-ChildItem -Path $buildDir -Recurse -Filter 'DriverGeneral.xml' -ErrorAction SilentlyContinue
                }
            }
        }
    }
    if (-not $candidates) { return $null }
    # Order by the build version embedded in the path (e.g. ...\10.0.28000.0\...), highest first.
    return ($candidates | Sort-Object {
            if ($_.FullName -match '10\.0\.(\d+)\.\d') { [int]$Matches[1] } else { 0 }
        } -Descending | Select-Object -First 1).FullName
}

if (-not $XmlPath) { $XmlPath = Find-DriverGeneralXml -RunMode $RunMode }
if (-not $XmlPath -or -not (Test-Path $XmlPath)) {
    throw "Could not locate DriverGeneral.xml for RunMode '$RunMode'. Restore the NuGet WDK, install the WDK, or initialise the EWDK for the selected mode, or pass -XmlPath."
}

[xml]$xml = Get-Content -Path $XmlPath -Raw
$enum = $xml.ProjectSchemaDefinitions.Rule.EnumProperty | Where-Object { $_.Name -eq '_NT_TARGET_VERSION' }
if (-not $enum) { throw "No _NT_TARGET_VERSION enumeration found in '$XmlPath'." }

$versions =
    $enum.EnumValue |
    ForEach-Object {
        # DisplayName is e.g. "Windows 10.0.28000"; Name is the NTDDI code e.g. "0xA000012".
        if ("$($_.DisplayName)" -match 'Windows\s+(?<v>10\.0\.(?<b>\d+))\s*$') {
            [pscustomobject]@{
                Version = $Matches.v
                Tag     = $Matches.b
                Code    = $_.Name
                Build   = [int]$Matches.b
            }
        }
    } |
    Sort-Object Build -Descending

if ($Newest -gt 0) { $versions = $versions | Select-Object -First $Newest }

if ($AsMatrixJson) {
    # Compact, single-line JSON for a GitHub Actions matrix: [{ "version": "...", "tag": "..." }, ...]
    $matrix = @($versions | ForEach-Object { [ordered]@{ version = $_.Version; tag = $_.Tag } })
    return ($matrix | ConvertTo-Json -Compress -Depth 3)
}

return $versions
