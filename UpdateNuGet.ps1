#Requires -Version 5.1
<#
.SYNOPSIS
    Updates the Windows WDK and SDK NuGet package version referenced by
    Directory.Build.props and packages.config.

.DESCRIPTION
    Rewrites the WDK/SDK NuGet version in both repo-root files so the whole
    sample tree can be moved to a new WDK/SDK build with a single command.

    Only packages whose id starts with 'Microsoft.Windows.WDK' or
    'Microsoft.Windows.SDK' are touched. Any other package reference (for
    example Microsoft.Windows.ImplementationLibrary) is left untouched.

    The files' existing encoding (BOM / no BOM) and line endings are preserved.

.EXAMPLE
    .\UpdateNuGet --package 10.0.28000.2525

.EXAMPLE
    .\UpdateNuGet --package 10.0.28000.2525-preview

.EXAMPLE
    .\UpdateNuGet --package 10.0.28000.2525 --whatif
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Show-Usage {
    @'
Usage: .\UpdateNuGet --package <version> [--whatif]

  --package, -p <version>   New WDK/SDK NuGet version. Examples:
                              10.0.28000.2525
                              10.0.28000.2525-preview
  --whatif                  Show what would change without writing any file.
  --help, -h                Show this help.
'@ | Write-Host
}

# --- parse arguments ---------------------------------------------------------
$NewVersion = $null
$WhatIf     = $false

for ($i = 0; $i -lt $args.Count; $i++) {
    $a = [string]$args[$i]
    if ($a -match '^(?:--package|-package|-p)=(.+)$') {
        $NewVersion = $Matches[1]
    }
    elseif ($a -match '^(?:--package|-package|-p)$') {
        $i++
        if ($i -lt $args.Count) { $NewVersion = [string]$args[$i] }
        else { throw "Missing value after '$a'." }
    }
    elseif ($a -match '^(?:--whatif|-whatif|-WhatIf)$') {
        $WhatIf = $true
    }
    elseif ($a -match '^(?:--help|-help|-h|-\?|/\?)$') {
        Show-Usage
        return
    }
    elseif ($a -notmatch '^-' -and -not $NewVersion) {
        $NewVersion = $a
    }
    else {
        Show-Usage
        throw "Unknown or unexpected argument: '$a'."
    }
}

if (-not $NewVersion) {
    Show-Usage
    throw "No version supplied. Pass --package <version>."
}

# Allow a leading 'v' (v10.0.28000.2525) for convenience.
$NewVersion = $NewVersion.TrimStart('v', 'V')

if ($NewVersion -notmatch '^\d+\.\d+\.\d+\.\d+(?:-[0-9A-Za-z.\-]+)?$') {
    throw "Version '$NewVersion' does not look like a NuGet version " +
          "(expected e.g. 10.0.28000.2525 or 10.0.28000.2525-preview)."
}

# --- files to update ---------------------------------------------------------
$root = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }

$targets = @(
    [pscustomobject]@{
        Path    = Join-Path $root 'Directory.Build.props'
        # packages\Microsoft.Windows.WDK.x64.<version>\build\native\...
        Pattern = [regex]'(Microsoft\.Windows\.(?:WDK|SDK)(?:\.CPP)?(?:\.(?:x64|x86|arm64|arm))?\.)(\d[0-9A-Za-z.\-]*?)(\\build\\native\\)'
    },
    [pscustomobject]@{
        Path    = Join-Path $root 'packages.config'
        # <package id="Microsoft.Windows.WDK.x64" version="<version>" ... />
        Pattern = [regex]'(<package id="Microsoft\.Windows\.(?:WDK|SDK)[^"]*" version=")([^"]*)(")'
    }
)

$replacement = '${1}' + $NewVersion + '${3}'
$anyChange   = $false

Write-Host "Updating WDK/SDK NuGet packages to $NewVersion" -ForegroundColor Cyan
Write-Host ""

foreach ($t in $targets) {
    $name = [System.IO.Path]::GetFileName($t.Path)

    if (-not (Test-Path -LiteralPath $t.Path)) {
        Write-Warning "Skipping (not found): $($t.Path)"
        continue
    }

    $bytes  = [System.IO.File]::ReadAllBytes($t.Path)
    $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    $text   = [System.IO.File]::ReadAllText($t.Path)

    $found = $t.Pattern.Matches($text)
    if ($found.Count -eq 0) {
        Write-Warning "No WDK/SDK package references found in $name."
        continue
    }

    $oldVersions = $found | ForEach-Object { $_.Groups[2].Value } | Sort-Object -Unique
    $newText     = $t.Pattern.Replace($text, $replacement)

    if ($newText -ceq $text) {
        Write-Host ("  {0,-22} already at {1} ({2} reference(s))" -f $name, ($oldVersions -join ', '), $found.Count)
        continue
    }

    Write-Host ("  {0,-22} {1} -> {2} ({3} reference(s))" -f $name, ($oldVersions -join ', '), $NewVersion, $found.Count)

    if (-not $WhatIf) {
        $enc = New-Object System.Text.UTF8Encoding($hasBom)
        [System.IO.File]::WriteAllText($t.Path, $newText, $enc)
    }
    $anyChange = $true
}

Write-Host ""
if ($WhatIf) {
    Write-Host "WhatIf: no files were modified." -ForegroundColor Yellow
}
elseif ($anyChange) {
    Write-Host "Done. WDK/SDK packages set to $NewVersion." -ForegroundColor Green
    Write-Host "Next: restore packages (e.g. 'nuget restore' or 'msbuild -t:restore')."
}
else {
    Write-Host "No changes were necessary." -ForegroundColor Green
}
