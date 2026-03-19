<#
.SYNOPSIS
    Builds driver samples from a sample list file with parallel execution and exclusion support.

.DESCRIPTION
    Reads sample names from Samples.txt (or a specified file), processes exclusions from
    exclusions.csv, determines the build environment, and builds all non-excluded samples
    in parallel using Build-Sample.ps1. Generates CSV and HTML overview reports.

    Requires PowerShell 7+ (uses ForEach-Object -Parallel and ternary operators).

.PARAMETER SampleListPath
    Path to the file containing sample names (one per line). Defaults to Samples.txt in the
    current directory.

.PARAMETER Samples
    Optional array of specific sample names to build. When provided, overrides SampleListPath.

.PARAMETER Configurations
    Build configurations (e.g. 'Debug','Release'). Defaults to $env:WDS_Configuration or
    ('Debug','Release').

.PARAMETER Platforms
    Build platforms (e.g. 'x64','arm64'). Defaults to $env:WDS_Platform or ('x64','arm64').

.PARAMETER LogFilesDirectory
    Directory for build log files. Defaults to _logs in the current directory.

.PARAMETER ReportFileName
    Base name for the report files (without extension). Defaults to $env:WDS_ReportFileName
    or '_overview'.

.PARAMETER InfOptions
    Additional InfVerif options. If not provided, determined automatically based on build number.

.PARAMETER ThrottleLimit
    Maximum parallel build jobs. Defaults to 5 x logical processors.

.EXAMPLE
    .\Build-Samples

.EXAMPLE
    .\Build-Samples -Samples 'audio.acx.samples.audiocodec.driver','usb.kmdf_fx2' -Configurations 'Debug' -Platforms 'x64'

.EXAMPLE
    .\Build-Samples -SampleListPath .\MySamples.txt -ThrottleLimit 8
#>

[CmdletBinding()]
param(
    [string]$SampleListPath = (Join-Path (Get-Location) "Samples.txt"),
    [string[]]$Samples,
    [string[]]$Configurations = @(if ([string]::IsNullOrEmpty($env:WDS_Configuration)) { ('Debug', 'Release') } else { $env:WDS_Configuration }),
    [string[]]$Platforms = @(if ([string]::IsNullOrEmpty($env:WDS_Platform)) { ('x64', 'arm64') } else { $env:WDS_Platform }),
    [string]$LogFilesDirectory = (Join-Path (Get-Location) "_logs"),
    [string]$ReportFileName = $(if ([string]::IsNullOrEmpty($env:WDS_ReportFileName)) { "_overview" } else { $env:WDS_ReportFileName }),
    [string]$InfOptions,
    [int]$ThrottleLimit = 0
)

$root = (Get-Location).Path

# Launch developer PowerShell if necessary
if (-not $env:VSCMD_VER) {
    Import-Module (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\*\*\Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    Enter-VsDevShell -VsInstallPath (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\*\*")
    Set-Location $root
}

$ThrottleFactor = 5
$LogicalProcessors = (Get-CIMInstance -Class 'CIM_Processor' -Verbose:$false).NumberOfLogicalProcessors

if ($ThrottleLimit -eq 0) {
    $ThrottleLimit = $ThrottleFactor * $LogicalProcessors
}

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

# Prepare log directory
Remove-Item -Recurse -Path $LogFilesDirectory 2>&1 | Out-Null
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

$reportFilePath = Join-Path $LogFilesDirectory "$ReportFileName.htm"
$reportCsvFilePath = Join-Path $LogFilesDirectory "$ReportFileName.csv"

# Verify msbuild is available
$oldPreference = $ErrorActionPreference
$ErrorActionPreference = "stop"
try {
    Get-Command "msbuild" | Out-Null
}
catch {
    Write-Host "`u{274C} msbuild cannot be called from current environment. Check that msbuild is set in current path (for example, that it is called from a Visual Studio developer command)."
    Write-Error "msbuild cannot be called from current environment."
    exit 1
}
finally {
    $ErrorActionPreference = $oldPreference
}

#
# Load sample list
#
if ($Samples) {
    $sampleNames = $Samples
}
else {
    if (-not (Test-Path $SampleListPath)) {
        Write-Error "Sample list file not found: $SampleListPath. Run .\ListAllSamples.ps1 first to generate Samples.txt."
        exit 1
    }
    $sampleNames = Get-Content $SampleListPath | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}

# Build the sampleSet hashtable: sampleName -> directory path
$sampleSet = @{}
foreach ($name in $sampleNames) {
    $relativePath = $name.Replace('.', '\')
    $fullPath = Join-Path $root $relativePath
    $sampleSet[$name] = $fullPath
}

#
# Determine build environment
#
$build_environment = ""
$build_number = 0
$nuget_package_version = 0

if ($env:GITHUB_REPOSITORY) {
    $build_environment = "GitHub"
    $nuget_package_version = ([regex]'(?<=x64\.)(\d+\.)(\d+\.)(\d+\.)(\d+)').Matches((Get-ChildItem .\packages\*WDK.x64* -Name)).Value
    $build_number = $nuget_package_version.split('.')[2]
}
elseif (Test-Path(".\packages\*")) {
    $build_environment = "NuGet"
    $nuget_package_version = ([regex]'(?<=x64\.)(\d+\.)(\d+\.)(\d+\.)(\d+)').Matches((Get-ChildItem .\packages\*WDK.x64* -Name)).Value
    $build_number = $nuget_package_version.split('.')[2]
}
elseif ($env:BuildLab -match '(?<branch>[^.]*).(?<build>[^.]*).(?<qfe>[^.]*)') {
    $build_environment = "EWDK." + $Matches.branch + "." + $Matches.build + "." + $Matches.qfe
    $build_number = $Matches.build
}
elseif ($env:UCRTVersion -match '10.0.(?<build>.*).0') {
    $build_environment = "WDK"
    $build_number = $Matches.build
}
else {
    Write-Output "Environment variables {"
    Get-ChildItem env:* | Sort-Object name
    Write-Output "Environment variables }"
    Write-Error "Could not determine build environment."
    exit 1
}

# Determine WDK Visual Studio Component version
if ($build_environment -match '^EWDK') {
    $wdk_vs_component_ver = "(WDK Visual Studio Component Version is not included for EWDK builds)"
}
else {
    $wdk_vs_component_ver = Get-ChildItem "${env:ProgramData}\Microsoft\VisualStudio\Packages\Microsoft.Windows.DriverKit,version=*" -ErrorAction SilentlyContinue
    if (-not $wdk_vs_component_ver) {
        Write-Error "WDK Visual Studio Component version not found. Please ensure the WDK Component is installed."
        exit 1
    }
    $wdk_vs_component_ver = [regex]::Match($wdk_vs_component_ver.Name, '(\d+\.){3}\d+').Value
}

#
# InfVerif_AdditionalOptions
#
if ($InfOptions) {
    $InfVerif_AdditionalOptions = $InfOptions
}
else {
    $InfVerif_AdditionalOptions = ($build_number -le 22621 ? "/sw1284 /sw1285 /sw1293 /sw2083 /sw2086" : "/samples")
}

#
# Load exclusions from exclusions.csv
#
$exclusionConfigurations = @{}
$exclusionReasons = @{}
Import-Csv 'exclusions.csv' | ForEach-Object {
    $excluded_driver = $_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    $excluded_configurations = ($_.configurations -eq '' ? '*' : $_.configurations)
    $excluded_minbuild = ($_.MinBuild -eq '' ? 00000 : $_.MinBuild)
    $excluded_maxbuild = ($_.MaxBuild -eq '' ? 99999 : $_.MaxBuild)
    if (($excluded_minbuild -le $build_number) -and ($build_number -le $excluded_maxbuild)) {
        $exclusionConfigurations[$excluded_driver] = $excluded_configurations
        $exclusionReasons[$excluded_driver] = $_.Reason
        Write-Verbose "Exclusion.csv entry applied for '$excluded_driver' for configuration '$excluded_configurations'."
    }
    else {
        Write-Verbose "Exclusion.csv entry not applied for '$excluded_driver' due to build number."
    }
}

#
# Build all samples
#
$jresult = @{
    SolutionsBuilt       = 0
    SolutionsSucceeded   = 0
    SolutionsExcluded    = 0
    SolutionsUnsupported = 0
    SolutionsFailed      = 0
    SolutionsSporadic    = 0
    Results              = @()
    FailSet              = @()
    SporadicSet          = @()
    lock                 = [System.Threading.Mutex]::new($false)
}

$SolutionsTotal = $sampleSet.Count * $Configurations.Count * $Platforms.Count

Write-Output "WDK Build Environment:               $build_environment"
Write-Output "WDK Build Number:                    $build_number"
if (($build_environment -eq "GitHub") -or ($build_environment -eq "NuGet")) {
    Write-Output "WDK Nuget Version:                   $nuget_package_version"
}
Write-Output "WDK Visual Studio Component Version: $wdk_vs_component_ver"
Write-Output "Samples:                             $($sampleSet.Count)"
Write-Output "Configurations:                      $($Configurations.Count) ($Configurations)"
Write-Output "Platforms:                           $($Platforms.Count) ($Platforms)"
Write-Output "InfVerif_AdditionalOptions:          $InfVerif_AdditionalOptions"
Write-Output "Combinations:                        $SolutionsTotal"
Write-Output "LogicalProcessors:                   $LogicalProcessors"
Write-Output "ThrottleFactor:                      $ThrottleFactor"
Write-Output "ThrottleLimit:                       $ThrottleLimit"
Write-Output "WDS_WipeOutputs:                     $env:WDS_WipeOutputs"
Write-Output "Disk Remaining (GB):                 $(((Get-Volume ((Get-Item ".").PSDrive.Name)).SizeRemaining) / 1GB)"
Write-Output ""
Write-Output "T: Combinations"
Write-Output "B: Built"
Write-Output "R: Build is running currently"
Write-Output "P: Build is pending an available build slot"
Write-Output ""
Write-Output "S: Built and result was 'Succeeded'"
Write-Output "E: Built and result was 'Excluded'"
Write-Output "U: Built and result was 'Unsupported' (Platform and Configuration combination)"
Write-Output "F: Built and result was 'Failed'"
Write-Output "O: Built and result was 'Sporadic'"
Write-Output ""
Write-Output "Building all combinations..."

$sw = [Diagnostics.Stopwatch]::StartNew()

$sampleSet.GetEnumerator() | ForEach-Object -ThrottleLimit $ThrottleLimit -Parallel {
    $LogFilesDirectory = $using:LogFilesDirectory
    $exclusionConfigurations = $using:exclusionConfigurations
    $exclusionReasons = $using:exclusionReasons
    $Configurations = $using:Configurations
    $Platforms = $using:Platforms
    $InfVerif_AdditionalOptions = $using:InfVerif_AdditionalOptions
    $Verbose = $using:Verbose

    $sampleName = $_.Key
    $directory = $_.Value

    $ResultElement = New-Object psobject
    Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name Sample -Value "$sampleName"

    foreach ($configuration in $Configurations) {
        foreach ($platform in $Platforms) {
            $thisunsupported = 0
            $thisfailed = 0
            $thissporadic = 0
            $thisexcluded = 0
            $thissucceeded = 0
            $thisresult = "Not run"
            $thisfailset = @()
            $thissporadicset = @()

            if ($exclusionConfigurations.ContainsKey($sampleName) -and ($exclusionConfigurations[$sampleName].Split(';') | Where-Object { "$configuration|$platform" -like $_ })) {
                Write-Verbose "[$sampleName $configuration|$platform] `u{23E9} Excluded and skipped. Reason: $($exclusionReasons[$sampleName])"
                $thisexcluded += 1
                $thisresult = "Excluded"
            }
            else {
                .\Build-Sample -Directory $directory -SampleName $sampleName -LogFilesDirectory $LogFilesDirectory -Configuration $configuration -Platform $platform -InfVerif_AdditionalOptions $InfVerif_AdditionalOptions -Verbose:$Verbose
                if ($LASTEXITCODE -eq 0) {
                    $thissucceeded += 1
                    $thisresult = "Succeeded"
                }
                elseif ($LASTEXITCODE -eq 1) {
                    $thisfailset += "$sampleName $configuration|$platform"
                    $thisfailed += 1
                    $thisresult = "Failed"
                }
                elseif ($LASTEXITCODE -eq 2) {
                    $thissporadicset += "$sampleName $configuration|$platform"
                    $thissporadic += 1
                    $thisresult = "Sporadic"
                }
                else {
                    $thisunsupported += 1
                    $thisresult = "Unsupported"
                }
            }
            Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name "$configuration|$platform" -Value "$thisresult"

            $null = ($using:jresult).lock.WaitOne()
            try {
                ($using:jresult).SolutionsBuilt += 1
                ($using:jresult).SolutionsSucceeded += $thissucceeded
                ($using:jresult).SolutionsExcluded += $thisexcluded
                ($using:jresult).SolutionsUnsupported += $thisunsupported
                ($using:jresult).SolutionsFailed += $thisfailed
                ($using:jresult).SolutionsSporadic += $thissporadic
                ($using:jresult).FailSet += $thisfailset
                ($using:jresult).SporadicSet += $thissporadicset
                $SolutionsTotal = $using:SolutionsTotal
                $ThrottleLimit = $using:ThrottleLimit
                $SolutionsBuilt = ($using:jresult).SolutionsBuilt
                $SolutionsRemaining = $SolutionsTotal - $SolutionsBuilt
                $SolutionsRunning = if ($SolutionsRemaining -ge $ThrottleLimit) { $ThrottleLimit } else { $SolutionsRemaining }
                $SolutionsPending = if ($SolutionsRemaining -ge $ThrottleLimit) { ($SolutionsRemaining - $ThrottleLimit) } else { 0 }
                $SolutionsBuiltPercent = [Math]::Round(100 * ($SolutionsBuilt / $using:SolutionsTotal))
                $TBRP = "T:" + ($SolutionsTotal) + "; B:" + (($using:jresult).SolutionsBuilt) + "; R:" + ($SolutionsRunning) + "; P:" + ($SolutionsPending)
                $rstr = "S:" + (($using:jresult).SolutionsSucceeded) + "; E:" + (($using:jresult).SolutionsExcluded) + "; U:" + (($using:jresult).SolutionsUnsupported) + "; F:" + (($using:jresult).SolutionsFailed) + "; O:" + (($using:jresult).SolutionsSporadic)
                Write-Progress -Activity "Building combinations" -Status "$SolutionsBuilt of $using:SolutionsTotal combinations built ($SolutionsBuiltPercent%) | $TBRP | $rstr" -PercentComplete $SolutionsBuiltPercent
            }
            finally {
                ($using:jresult).lock.ReleaseMutex()
            }
        }
    }
    $null = ($using:jresult).lock.WaitOne()
    try {
        ($using:jresult).Results += $ResultElement
    }
    finally {
        ($using:jresult).lock.ReleaseMutex()
    }
}

$sw.Stop()

Write-Output ""

if ($jresult.FailSet.Count -gt 0) {
    Write-Output "Some combinations were built with errors:"
    $jresult.FailSet = $jresult.FailSet | Sort-Object
    foreach ($failedSample in $jresult.FailSet) {
        $failedSample -match "^(.*) (\w*)\|(\w*)$" | Out-Null
        $failName = $Matches[1]
        $failConfiguration = $Matches[2]
        $failPlatform = $Matches[3]
        Write-Output "Build errors in Sample $failName; Configuration: $failConfiguration; Platform: $failPlatform {"
        Get-Content "$LogFilesDirectory\$failName.$failConfiguration.$failPlatform.0.err" | Write-Output
        Write-Output "} $failedSample"
    }
    Write-Error "Some combinations were built with errors."
    Write-Output ""
}

if ($jresult.SporadicSet.Count -gt 0) {
    Write-Output "Some combinations were built with sporadic error:"
    $jresult.SporadicSet = $jresult.SporadicSet | Sort-Object
    foreach ($sporadicSample in $jresult.SporadicSet) {
        $sporadicSample -match "^(.*) (\w*)\|(\w*)$" | Out-Null
        $sporadicName = $Matches[1]
        $sporadicConfiguration = $Matches[2]
        $sporadicPlatform = $Matches[3]
        Write-Output "Build sporadic errors in Sample $sporadicName; Configuration: $sporadicConfiguration; Platform: $sporadicPlatform {"
        Get-Content "$LogFilesDirectory\$sporadicName.$sporadicConfiguration.$sporadicPlatform.0.err" | Write-Output
        Write-Output "} $sporadicSample"
    }
    Write-Error "Some combinations were built with sporadic errors."
    Write-Output ""
}

# Display timer statistics
$min = $sw.Elapsed.Minutes
$seconds = $sw.Elapsed.Seconds

$SolutionsSucceeded = $jresult.SolutionsSucceeded
$SolutionsExcluded = $jresult.SolutionsExcluded
$SolutionsUnsupported = $jresult.SolutionsUnsupported
$SolutionsFailed = $jresult.SolutionsFailed
$SolutionsSporadic = $jresult.SolutionsSporadic
$Results = $jresult.Results

Write-Output "Built all combinations."
Write-Output ""
Write-Output "Elapsed time:         $min minutes, $seconds seconds."
Write-Output ("Disk Remaining (GB):  " + (((Get-Volume (Get-Item ".").PSDrive.Name).SizeRemaining / 1GB)))
Write-Output ("Samples:              " + $sampleSet.Count)
Write-Output ("Configurations:       " + $Configurations.Count + " (" + $Configurations + ")")
Write-Output ("Platforms:            " + $Platforms.Count + " (" + $Platforms + ")")
Write-Output "Combinations:         $SolutionsTotal"
Write-Output "Succeeded:            $SolutionsSucceeded"
Write-Output "Excluded:             $SolutionsExcluded"
Write-Output "Unsupported:          $SolutionsUnsupported"
Write-Output "Failed:               $SolutionsFailed"
Write-Output "Sporadic:             $SolutionsSporadic"
Write-Output "Log files directory:  $LogFilesDirectory"
Write-Output "Overview report:      $reportFilePath"
Write-Output ""

$Results | Sort-Object { $_.Sample } | ConvertTo-Csv | Out-File $reportCsvFilePath
$Results | Sort-Object { $_.Sample } | ConvertTo-Html -Title "Overview" | Out-File $reportFilePath
Invoke-Item $reportFilePath
