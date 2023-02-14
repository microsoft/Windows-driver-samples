[CmdletBinding()]
param(
    [hashtable]$SampleSet,
    [string[]]$Configurations = @([string]::IsNullOrEmpty($env:WDS_Configuration) ? "Debug" : $env:WDS_Configuration),
    [string[]]$Platforms = @([string]::IsNullOrEmpty($env:WDS_Platform) ? "x64" : $env:WDS_Platform),
    $LogFilesDirectory = (Get-Location),
    [int]$ThrottleLimit
)

$ThrottleFactor = 5
$LogicalProcessors = (Get-CIMInstance -Class 'CIM_Processor' -Verbose:$false).NumberOfLogicalProcessors
$ThrottleLimit = $ThrottleLimit -eq 0 ? ($ThrottleFactor * $LogicalProcessors) : $ThrottleLimit

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null
$sampleBuilderFilePath = "$LogFilesDirectory\overview.htm"


Remove-Item  -Recurse -Path $LogFilesDirectory 2>&1 | Out-Null
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

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
Import-Csv 'exclusions.csv' | ForEach-Object {
    $exclusionsSet[$_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()] = $_.Reason
}

$jresult = @{
    SolutionsBuilt    = 0
    SolutionsExcluded = 0
    SolutionsFailed   = 0
    Results           = @()
    FailSet           = @()
    lock              = [System.Threading.Mutex]::new($false)
}

$SolutionsTotal = $sampleSet.Count * $Configurations.Count * $Platforms.Count

Write-Output ("Samples:              "+$sampleSet.Count)
Write-Output ("Configurations:       "+$Configurations.Count+" ("+$Configurations+")")
Write-Output ("Platforms:            "+$Platforms.Count+" ("+$Platforms+")")
Write-Output "Combinations:         $SolutionsTotal"
Write-Output "LogicalProcessors:    $LogicalProcessors"
Write-Output "ThrottleFactor:       $ThrottleFactor"
Write-Output "ThrottleLimit:        $ThrottleLimit"
Write-Output "WDS_WipeOutputs:      $env:WDS_WipeOutputs"
Write-Output ("Disk Remaining (GB):  "+(((Get-Volume ($DriveLetter=(Get-Item ".").PSDrive.Name)).SizeRemaining/1GB)))
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
Write-Output ""
Write-Output "Building all combinations..."

$Results = @()

$sw = [Diagnostics.Stopwatch]::StartNew()

$SampleSet.GetEnumerator() | ForEach-Object -ThrottleLimit $ThrottleLimit -Parallel {
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
            $thisfailset = @()

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
                    $thisfailset += "$sampleName $configuration|$platform"
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
                ($using:jresult).FailSet += $thisfailset
                $SolutionsTotal = $using:SolutionsTotal
                $ThrottleLimit = $using:ThrottleLimit
                $SolutionsBuilt = ($using:jresult).SolutionsBuilt
                $SolutionsRemaining = $SolutionsTotal - $SolutionsBuilt
                $SolutionsRunning = $SolutionsRemaining -ge $ThrottleLimit ? ($ThrottleLimit) : ($SolutionsRemaining)
                $SolutionsPending = $SolutionsRemaining -ge $ThrottleLimit ? ($SolutionsRemaining - $ThrottleLimit) : (0)
                $SolutionsBuiltPercent = [Math]::Round(100 * ($SolutionsBuilt / $using:SolutionsTotal))
                $TBRP = "T:" + ($SolutionsTotal) + "; B:" + (($using:jresult).SolutionsBuilt) + "; R:" + ($SolutionsRunning) + "; P:" + ($SolutionsPending)
                $rstr = "S:" + (($using:jresult).SolutionsSucceeded) + "; E:" + (($using:jresult).SolutionsExcluded) + "; U:" + (($using:jresult).SolutionsUnsupported) + "; F:" + (($using:jresult).SolutionsFailed)
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

if ($jresult.FailSet.Count -gt 0) {
    Write-Output "Some combinations were built with errors:"
    foreach ($failedSample in $jresult.FailSet) {
        $failedSample -match "^(.*) (\w*)\|(\w*)$" | Out-Null
        $failName = $Matches[1]
        $failConfiguration = $Matches[2]
        $failPlatform = $Matches[3]
        Write-Output "Build errors in Sample $failName; Configuration: $failConfiguration; Platform: $failPlatform {"
        Get-Content "$LogFilesDirectory\$failName.$failConfiguration.$failPlatform.err" | Write-Output
        Write-Output "} $failedSample"
    }
    Write-Error "Some combinations were built with errors."
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
Write-Output "Built all combinations."
Write-Output ""
Write-Output "Elapsed time:         $min minutes, $seconds seconds."
Write-Output ("Disk Remaining (GB):  "+(((Get-Volume ($DriveLetter=(Get-Item ".").PSDrive.Name)).SizeRemaining/1GB)))
Write-Output ("Samples:              "+$sampleSet.Count)
Write-Output ("Configurations:       "+$Configurations.Count+" ("+$Configurations+")")
Write-Output ("Platforms:            "+$Platforms.Count+" ("+$Platforms+")")
Write-Output "Combinations:         $SolutionsTotal"
Write-Output "Succeeded:            $SolutionsSucceeded"
Write-Output "Excluded:             $SolutionsExcluded"
Write-Output "Unsupported:          $SolutionsUnsupported"
Write-Output "Failed:               $SolutionsFailed"
Write-Output "Log files directory:  $LogFilesDirectory"
Write-Output "Overview report:      $sampleBuilderFilePath"
Write-Output ""

$Results | Sort-Object { $_.Sample } | ConvertTo-Html -Title "Overview" | Out-File $sampleBuilderFilePath
Invoke-Item $sampleBuilderFilePath
