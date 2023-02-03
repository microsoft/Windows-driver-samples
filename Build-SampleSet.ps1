[CmdletBinding()]
param(
    [hashtable]$SampleSet,
    [string[]]$Configurations = @([string]::IsNullOrEmpty($env:Configuration) ? "Debug" : $env:Configuration),
    [string[]]$Platforms = @([string]::IsNullOrEmpty($env:Platform) ? "x64" : $env:Platform),
    $LogFilesDirectory = (Get-Location)
)

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null
$sampleBuilderFilePath = "$LogFilesDirectory\overview.htm"


Remove-Item  -Recurse -Path $LogFilesDirectory 2>&1 | Out-Null
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

$NumberOfLogicalProcessors = (Get-CIMInstance -Class 'CIM_Processor' -Verbose:$false).NumberOfLogicalProcessors
$SolutionsInParallel = 5 * $NumberOfLogicalProcessors

Write-Verbose "Log files directory: $LogFilesDirectory"
Write-Verbose "Results overview report: $sampleBuilderFilePath"
Write-Verbose "Logical Processors: $NumberOfLogicalProcessors"
Write-Verbose "Solutions in Parallel: $SolutionsInParallel"

$oldPreference = $ErrorActionPreference
$ErrorActionPreference = "stop"
try {
    # Check that msbuild can be called before trying anything.
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

# TODO: Make exclusion more granular; allow for configuration|platform exclusions
$exclusionsSet = @{}
$failSet = @()
Import-Csv 'exclusions.csv' | ForEach-Object {
    $exclusionsSet[$_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()] = $_.Reason
}

$jresult = @{
    SolutionsBuilt    = 0
    SolutionsExcluded = 0
    SolutionsFailed   = 0
    Results           = @()
    lock              = [System.Threading.Mutex]::new($false)
}

$SolutionsTotal = $sampleSet.Count * $Configurations.Count * $Platforms.Count

Write-Output "T: Total solutions: $SolutionsTotal"
Write-Output "B: Built"
Write-Output "R: Build is running currently"
Write-Output "P: Build is pending an available build slot"
Write-Output ""
Write-Output "S: Built and result was 'Succeeded'"
Write-Output "E: Built and result was 'Excluded'"
Write-Output "U: Built and result was 'Unsupported' (Platform and Configuration combination)"
Write-Output "F: Built and result was 'Failed'"
Write-Output ""
Write-Output "Building driver solutions..."

$Results = @()

$sw = [Diagnostics.Stopwatch]::StartNew()

$SampleSet.GetEnumerator() | ForEach-Object -ThrottleLimit $SolutionsInParallel -Parallel {
    $LogFilesDirectory = $using:LogFilesDirectory
    $exclusionsSet = $using:exclusionsSet
    $Configurations = $using:Configurations
    $Platforms = $using:Platforms

    $sampleName = $_.Key
    $directory = $_.Value

    $ResultElement = new-object psobject
    Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name Sample -Value "$sampleName"

    foreach ($configuration in $Configurations) {
        foreach ($platform in $Platforms) {
            $thisunsupported = 0
            $thisfailed = 0
            $thisexcluded = 0
            $thissucceeded = 0
            $thisresult = "Not run"

            if ($exclusionsSet.ContainsKey($sampleName)) {
                # Verbose
                if ($thisexcluded -eq 0) {
                    Write-Verbose "[$sampleName] `u{23E9} Excluded and skipped. Reason: $($exclusionsSet[$sampleName])"
                }
                $thisexcluded += 1
                $thisresult = "Excluded"
            }
            else {
                .\Build-Sample -Directory $directory -SampleName $sampleName -LogFilesDirectory $LogFilesDirectory -Configuration $configuration -Platform $platform -Verbose:$Verbose
                if ($LASTEXITCODE -eq 0) {
                    $thissucceeded += 1
                    $thisresult = "Succeeded"
                }
                elseif ($LASTEXITCODE -eq 1) {
                    $failSet += "$sampleName $configuration|$platform"
                    $thisfailed += 1
                    $thisresult = "Failed"
                }
                else {
                    # ($LASTEXITCODE -eq 2)
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
                $SolutionsTotal = $using:SolutionsTotal
                $SolutionsInParallel = $using:SolutionsInParallel
                $SolutionsBuilt = ($using:jresult).SolutionsBuilt
                $SolutionsRemaining = $SolutionsTotal - $SolutionsBuilt
                $SolutionsRunning = $SolutionsRemaining -ge $SolutionsInParallel ? ($SolutionsInParallel) : ($SolutionsRemaining)
                $SolutionsPending = $SolutionsRemaining -ge $SolutionsInParallel ? ($SolutionsRemaining - $SolutionsInParallel) : (0)
                $SolutionsBuiltPercent = [Math]::Round(100 * ($SolutionsBuilt / $using:SolutionsTotal))
                $TBRP = "T:" + ($SolutionsTotal) + "; B:" + (($using:jresult).SolutionsBuilt) + "; R:" + ($SolutionsRunning) + "; P:" + ($SolutionsPending)
                $rstr = "S:" + (($using:jresult).SolutionsSucceeded) + "; E:" + (($using:jresult).SolutionsExcluded) + "; U:" + (($using:jresult).SolutionsUnsupported) + "; F:" + (($using:jresult).SolutionsFailed)
                Write-Progress -Activity "Building driver solutions" -Status "$SolutionsBuilt of $using:SolutionsTotal solutions built ($SolutionsBuiltPercent%) | $TBRP | $rstr" -PercentComplete $SolutionsBuiltPercent
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

if ($failSet.Count -gt 0) {
    Write-Output "Some samples were built with errors:"
    foreach ($failedSample in $failSet) {
        Write-Output "$failedSample"
    }
    Write-Error "Some samples were built with errors."
}

# Display timer statistics to host
$min = $sw.Elapsed.Minutes
$seconds = $sw.Elapsed.Seconds

$SolutionsSucceeded = $jresult.SolutionsSucceeded
$SolutionsExcluded = $jresult.SolutionsExcluded
$SolutionsUnsupported = $jresult.SolutionsUnsupported
$SolutionsFailed = $jresult.SolutionsFailed
$Results = $jresult.Results

Write-Output ""
Write-Output "Built solutions."
Write-Output ""
Write-Output "Total elapsed time:   $min minutes, $seconds seconds."
Write-Output "SolutionsTotal:       $SolutionsTotal"
Write-Output "SolutionsSucceeded:   $SolutionsSucceeded"
Write-Output "SolutionsExcluded:    $SolutionsExcluded"
Write-Output "SolutionsUnsupported: $SolutionsUnsupported"
Write-Output "SolutionsFailed:      $SolutionsFailed"
Write-Output ""
Write-Output "Results saved to $sampleBuilderFilePath"
Write-Output ""

$Results | Sort-Object { $_.Sample } | ConvertTo-Html -Title "Overview" | Out-File $sampleBuilderFilePath
Invoke-Item $sampleBuilderFilePath
