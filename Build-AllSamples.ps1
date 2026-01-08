<#
.SYNOPSIS
Builds all available sample solutions in the repository (excluding specific solutions).

.DESCRIPTION
This script searches for all available Visual Studio Solutions (.sln files) and attempts to run MSBuild to build them for the specified configurations and platforms.

.PARAMETER Samples
A regular expression matching the samples to be built.  Default is '' that matches all samples.  Examples include '^tools.' or '.dchu'.

.PARAMETER Configurations
A list of configurations to build samples under. Values available are 'Debug' and 'Release'. By default, $env:WDS_Configuration will be used as the sole configuration to build for. If this environment variable is not set the default is 'Debug' and 'Release'.

.PARAMETER Platforms
A list of platforms to build samples under (e.g. 'x64', 'arm64'). By default, $env:WDS_Platform will be used as the sole platform to build for. If this environment variable is not set the default is 'x64' and'arm64'.

.PARAMETER LogFilesDirectory
Path to a directory where the log files will be written to. If not provided, outputs will be logged to the '_logs' directory within the current working directory.

.PARAMETER ThrottleLimit
An integer indicating how many combinations to build in parallel.  If 0 or not provided this defaults to 5 x number of logical processors.

.INPUTS
None.

.OUTPUTS
None.

.EXAMPLE
.\Build-AllSamples

.EXAMPLE
.\Build-AllSamples -Samples '^tools.' -Configurations 'Debug','Release' -Platforms 'x64','arm64'

#>

[CmdletBinding()]
param(
    [string]$Samples = "",
    [string[]]$Configurations = @(if ([string]::IsNullOrEmpty($env:WDS_Configuration)) { ('Debug', 'Release') } else { $env:WDS_Configuration }),
    [string[]]$Platforms = @(if ([string]::IsNullOrEmpty($env:WDS_Platform)) { ('x64', 'arm64') } else { $env:WDS_Platform }),
    [string]$LogFilesDirectory = (Join-Path (Get-Location) "_logs"),
    [int]$ThrottleLimit
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
    if ($dir_norm -match ("^packages.")) {
        Write-Verbose "`u{1F50E} Found and ignored non-sample [$dir_norm] at $dir"
    }
    elseif ($dir_norm -match ($Samples)) {
        Write-Verbose "`u{1F50E} Found and filtered in sample [$dir_norm] at $dir"
        $sampleSet[$dir_norm] = $dir
    }
    else {
        Write-Verbose "`u{1F50E} Found and filtered out sample [$dir_norm] at $dir"
    }
}

.\Build-SampleSet -SampleSet $sampleSet -Configurations $Configurations -Platform $Platforms -LogFilesDirectory $LogFilesDirectory -Verbose:$Verbose -ThrottleLimit $ThrottleLimit
