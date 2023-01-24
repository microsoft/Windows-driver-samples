$root = Get-Location
$solutionFiles = Get-ChildItem -Path $root -Recurse -Filter *.sln | Select-Object -ExpandProperty FullName

# To include in CI gate
$index = 0
$ProjectArray = $solutionFiles | foreach-object {
    $dir = (Get-Item $_).DirectoryName
    $dir_norm = (([string]($index)).PadLeft(4,'0'))+"_"+$dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    $dir_norm = $dir.Replace($root, '').Trim('\').Replace('\', '.').ToLower()
    $index++
    #Write-Output "`u{1F50E} Found project [$dir_norm] at $dir"
    new-object psobject -property @{index = $index; dir_norm = $dir_norm; dir =$dir}
}

$env:Platform="x64"

#param(
#    $ProjectArray,
#    [string]$Configuration = $env:Configuration,
#    [string]$Platform = $env:Platform
#)

if ([string]::IsNullOrEmpty($Configuration))
{
    $Configuration = "Debug"
}

if ([string]::IsNullOrEmpty($Platform))
{
    $Platform = "x64"
}

$LogFilesDirectory = "_logfiles"

New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null
$sampleBuilderFilePath = "$LogFilesDirectory\overview.htm"


Remove-Item  -Recurse -Path $LogFilesDirectory 2>&1 | Out-Null
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

$NumberOfLogicalProcessors = (Get-CIMInstance -Class 'CIM_Processor').NumberOfLogicalProcessors
$SolutionsInParallel = 5 * $NumberOfLogicalProcessors

Write-Output "LogFilesDirectory: $LogFilesDirectory"
Write-Output "Overview: $sampleBuilderFilePath"
Write-Output "NumberOfLogicalProcessors: $NumberOfLogicalProcessors"
Write-Output "SolutionsInParallel: $SolutionsInParallel"

$oldPreference = $ErrorActionPreference
$ErrorActionPreference = "stop"
try
{
    # Check that msbuild can be called before trying anything.
    Get-Command "msbuild" | Out-Null
}
catch
{
    Write-Host "`u{274C} msbuild cannot be called from current environment. Check that msbuild is set in current path (for example, that it is called from a Visual Studio developer command)."
    Write-Error "msbuild cannot be called from current environment."
    exit 1
}
finally
{
    $ErrorActionPreference = $oldPreference
}


$exclusionsSet = @{}
$failSet = @()
Import-Csv 'exclusions.csv' | ForEach-Object {
    $exclusionsSet[$_.Path.Replace($root, '').Trim('\').Replace('\', '.').ToLower()] = $_.Reason
}

$jresult = @{
  SolutionsBuilt = 0
  SolutionsExcluded = 0
  SolutionsFailed = 0
  Results = @()
  lock = [System.Threading.Mutex]::new($false)
}

$SolutionsTotal = $ProjectArray.count

Write-Output "SolutionsTotal: $SolutionsTotal"
Write-Output ""
Write-Output "Shorthands"
Write-Output ""
Write-Output "T: Total: $SolutionsTotal"
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

$ProjectArray.GetEnumerator() | ForEach-Object -ThrottleLimit $SolutionsInParallel -Parallel {
    $LogFilesDirectory = $using:LogFilesDirectory
    $exclusionsSet = $using:exclusionsSet
    $Configuration = $using:Configuration
    $Platform = $using:Platform

    $Index = $_.index
    $projectName = $_.dir_norm
    $directory = $_.dir

    $thisunsupported=0
    $thisfailed=0
    $thisexcluded=0
    $thissucceeded=0
    $thisresult="Not run"

    #jjj42: Write-Output "Build-ProjectSet.ps1: .\Build-Project -ProjectName $ProjectName -Configuration $Configuration -Platform $Platform"

    if ($exclusionsSet.ContainsKey($projectName))
    {
        #jjj Write-Output "[$projectName] `u{23E9} Excluded and skipped. Reason: $($exclusionsSet[$projectName])"
        $thisexcluded=1
        $thisresult="Excluded"
    }
    else
    {
        .\Build-Project -Directory $directory -ProjectName $ProjectName -LogFilesDirectory $LogFilesDirectory -Configuration $Configuration -Platform $Platform
        if ($LASTEXITCODE -eq 0)
        {
            $thissucceeded=1
            $thisresult="Succeeded"
        }
        elseif ($LASTEXITCODE -eq 1)
        {
            $failSet += $ProjectName
            $thisfailed=1
            $thisresult="Failed"
        }
        else # ($LASTEXITCODE -eq 2)
        {
            $failSet += $ProjectName
            $thisunsupported=1
            $thisresult="Unsupported"
        }
    }

    $ResultElement=new-object psobject
    Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name Index -Value "$Index"
    Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name Sample -Value "$ProjectName"
    Add-Member -InputObject $ResultElement -MemberType NoteProperty -Name "$Platform" -Value "$thisresult"

    $null = ($using:jresult).lock.WaitOne()
    try {
        ($using:jresult).Results += $ResultElement
        ($using:jresult).SolutionsBuilt += 1
        ($using:jresult).SolutionsSucceeded += $thissucceeded
        ($using:jresult).SolutionsExcluded += $thisexcluded
        ($using:jresult).SolutionsUnsupported += $thisunsupported
        ($using:jresult).SolutionsFailed += $thisfailed
        $SolutionsTotal=$using:SolutionsTotal
        $SolutionsInParallel=$using:SolutionsInParallel
        $SolutionsBuilt = ($using:jresult).SolutionsBuilt
        $SolutionsRemaining = $SolutionsTotal - $SolutionsBuilt
        $SolutionsRunning = $SolutionsRemaining -ge $SolutionsInParallel ? ($SolutionsInParallel) : ($SolutionsRemaining)
        $SolutionsPending = $SolutionsRemaining -ge $SolutionsInParallel ? ($SolutionsRemaining - $SolutionsInParallel) : (0)
        $SolutionsBuiltPercent = [Math]::Round(100*($SolutionsBuilt / $using:SolutionsTotal))
        $TBRP = "T:"+($SolutionsTotal)+"; B:"+(($using:jresult).SolutionsBuilt)+"; R:"+($SolutionsRunning)+"; P:"+($SolutionsPending)
        $rstr = "S:"+(($using:jresult).SolutionsSucceeded)+"; E:"+(($using:jresult).SolutionsExcluded)+"; U:"+(($using:jresult).SolutionsUnsupported)+"; F:"+(($using:jresult).SolutionsFailed)
        # jjj42: 
        Write-Progress -Activity "Building driver solutions" -Status "$SolutionsBuilt of $using:SolutionsTotal solutions built ($SolutionsBuiltPercent%) | $TBRP | $rstr" -PercentComplete $SolutionsBuiltPercent
    }
    finally {
        ($using:jresult).lock.ReleaseMutex()
    }
}

if ($failSet.Count -gt 0)
{
    Write-Output "Some projects were built with errors:"
    foreach ($failedProject in $failSet)
    {
        Write-Output "$failedProject"
    }
    Write-Error "Some projects were built with errors."
}

# Display timer statistics to host
$min = $sw.Elapsed.Minutes
$seconds = $sw.Elapsed.Seconds

$SolutionsSucceeded=$jresult.SolutionsSucceeded
$SolutionsExcluded=$jresult.SolutionsExcluded
$SolutionsUnsupported=$jresult.SolutionsUnsupported
$SolutionsFailed=$jresult.SolutionsFailed
$Results=$jresult.Results

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

$Results | Sort-Object { [int]$_.Index } | ConvertTo-Html | Out-File $sampleBuilderFilePath
Invoke-Item $sampleBuilderFilePath
