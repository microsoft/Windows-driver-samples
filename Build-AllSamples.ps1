<#
.SYNOPSIS
Builds all available sample solutions in the repository (excluding specific solutions).

.DESCRIPTION
This script searches for all available Visual Studio Solutions (.sln files) and attempts to run MSBuild to build them for the specified configurations and platforms.

.PARAMETER Configurations
A list of configurations to build samples under. Values available are "Debug" and "Release". By default, $env:Configuration will be used as the sole configuration to build for. If this value doesn't exist, "Debug" will be used instead.

.PARAMETER Platforms
A list of platforms to build samples under (e.g. "x64", "arm64"). By default, $env:Platform will be used as the sole platform to build for. If this value doesn't exist, "x64" will be used instead.

.PARAMETER LogFilesDirectory
Path to a directory where the log files will be written to. If not provided, outputs will be logged to the "_logs" directory within the current working directory.

.INPUTS
None.

.OUTPUTS
None.

.EXAMPLE
.\Build-AllSamples

.EXAMPLE
.\Build-AllSamples -Configurations 'Debug','Release' -Platforms 'x64','arm64' -LogFilesDirectory .\_logs

#>

[CmdletBinding()]
param(
    [string[]]$Configurations = @([string]::IsNullOrEmpty($env:Configuration) ? "Debug" : $env:Configuration),
    [string[]]$Platforms = @([string]::IsNullOrEmpty($env:Platform) ? "x64" : $env:Platform),
    [string]$LogFilesDirectory = (Join-Path (Get-Location) "_logs")
)

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

$root = Get-Location
$solutionFiles = Get-ChildItem -Path $root -Recurse -Filter *.sln | Select-Object -ExpandProperty FullName

# To include in CI gate
$sampleSet = @{}
foreach ($file in $solutionFiles) {
    $dir = (Get-Item $file).DirectoryName
    $dir_norm = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    Write-Verbose "`u{1F50E} Found sample [$dir_norm] at $dir"
    $sampleSet[$dir_norm] = $dir
}



.\Build-SampleSet -SampleSet $sampleSet -Configurations $Configurations -Platform $Platforms -LogFilesDirectory $LogFilesDirectory -Verbose:$Verbose
