<#
.SYNOPSIS
    Enumerates all available sample solutions in the repository and writes them to the console.

.DESCRIPTION
    Searches for all .sln files recursively from the repo root, excludes NuGet package directories
    (paths containing 'packages' as a segment), computes a normalized sample name for each, and writes
    the sorted list to stdout (one sample name per line).

    The sample name is derived from the relative directory path: backslashes are replaced with dots
    and the result is lowercased.

.EXAMPLE
    .\ListAllSamples

    Discovers all samples and writes the sorted names to the console.

.OUTPUTS
    Sorted sample names written to stdout, one per line.
#>

[CmdletBinding()]
param()

$root = (Get-Location).Path

# Discover all .sln files
$solutionFiles = Get-ChildItem -Path $root -Recurse -Filter *.sln | Select-Object -ExpandProperty FullName

$sampleNames = @{}

foreach ($file in $solutionFiles) {
    $dir = (Get-Item $file).DirectoryName
    $dirNorm = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()

    if ($dirNorm -match '(^|\.|\b)packages(\.|$)') {
        Write-Verbose "Ignored NuGet package directory: $dirNorm"
        continue
    }

    if (-not $sampleNames.ContainsKey($dirNorm)) {
        $sampleNames[$dirNorm] = $true
    }
}

$sortedNames = $sampleNames.Keys | Sort-Object

Write-Verbose "Found $($sortedNames.Count) samples."
$sortedNames | Write-Output
