<#
.SYNOPSIS
    Builds driver samples from a sample list file with parallel execution and exclusion support.

.DESCRIPTION
    This is the main build orchestrator for driver samples. It performs these steps:

    1. Ensures a developer build environment (VS DevShell or EWDK) is active
    2. Discovers samples via ListAllSamples.ps1 (or uses the -Samples parameter if provided)
    3. Resolves the build environment (auto-detected or forced via -RunMode) and build number
    4. Loads exclusions from exclusions.csv (supports wildcard paths)
    5. Builds all non-excluded sample/configuration/platform combinations in parallel
    6. Generates CSV and HTML overview reports

    Requires PowerShell 7+ (uses ForEach-Object -Parallel).


.PARAMETER Samples
    Optional array of specific sample names or wildcard patterns to build. Supports
    wildcards (e.g. 'tools.*', 'audio.*') which are matched against all discovered
    samples. When omitted, all samples are discovered dynamically via ListAllSamples.ps1.

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
    Additional InfVerif options (e.g. '/samples', '/msft'). If not provided, determined
    automatically based on the WDK build number.

.PARAMETER RunMode
    Selects the build environment mode. Valid values: Auto, WDK, NuGet, EWDK, Github.
    Defaults to 'Auto', which detects the environment automatically (EWDK → WDK → NuGet).
    'Github' behaves identically to 'NuGet' but suppresses opening the HTML report at the end.

.PARAMETER ThrottleLimit
    Maximum parallel build jobs. Defaults to 5 x logical processors.

.EXAMPLE
    .\Build-Samples

    Discovers all samples via ListAllSamples.ps1 and builds them with default settings.

.EXAMPLE
    .\Build-Samples -Samples 'audio.acx.samples.audiocodec.driver','usb.kmdf_fx2' -Configurations 'Debug' -Platforms 'x64'

    Builds specific samples for a single configuration and platform.

.EXAMPLE
    .\Build-Samples -Samples 'tools.*'

    Builds all samples whose name matches the wildcard pattern 'tools.*'.

.EXAMPLE
    .\Build-Samples -ThrottleLimit 8

    Discovers all samples and builds them with limited parallelism.

.EXAMPLE
    .\Build-Samples -RunMode WDK

    Forces WDK mode regardless of environment variables.

.EXAMPLE
    .\Build-Samples -RunMode Github

    Runs as NuGet mode (reads packages\) without opening the HTML report — intended for CI.
#>

#Requires -Version 7.0

[CmdletBinding()]
param(
    [string[]]$Samples,
    [string[]]$Configurations = @(if ([string]::IsNullOrEmpty($env:WDS_Configuration)) { ('Debug', 'Release') } else { $env:WDS_Configuration }),
    [string[]]$Platforms = @(if ([string]::IsNullOrEmpty($env:WDS_Platform)) { ('x64', 'arm64') } else { $env:WDS_Platform }),
    [string]$LogFilesDirectory = (Join-Path (Get-Location) "_logs"),
    [string]$ReportFileName = $(if ([string]::IsNullOrEmpty($env:WDS_ReportFileName)) { "_overview" } else { $env:WDS_ReportFileName }),
    [string]$InfOptions,
    [ValidateSet('Auto', 'WDK', 'NuGet', 'EWDK', 'Github')]
    [string]$RunMode = 'Auto',
    [int]$ThrottleLimit = 0
)

# =============================================================================
#  Helper Functions
# =============================================================================

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')

function Import-SampleExclusions {
    <#
    .SYNOPSIS
        Loads exclusions.csv and returns exclusion objects applicable to the current build.
    .DESCRIPTION
        Each returned exclusion has:
          - Pattern:        dot-separated path (may contain wildcards, e.g. 'general.*')
          - Configurations: semicolon-separated config|platform patterns (or '*' for all)
          - Reason:         human-readable explanation

        Only exclusions whose [MinBuild, MaxBuild] range includes the given build number
        are returned. Exclusions outside the range are silently skipped.
    .NOTES
        CSV format:  Path,Configurations,MinBuild,MaxBuild,Reason
        Example row: network\wlan\wdi,*,,27100,"failure introduced in VS17.14"
    #>
    param(
        [string]$CsvPath,
        [int]$BuildNumber
    )

    if (-not (Test-Path $CsvPath)) {
        Write-Warning "Exclusions file not found: $CsvPath. No exclusions will be applied."
        return @()
    }

    $exclusions = [System.Collections.ArrayList]::new()
    Import-Csv $CsvPath | ForEach-Object {
        $pattern  = $_.Path.Trim('\').Replace('\', '.').ToLower()
        $configs  = if ([string]::IsNullOrWhiteSpace($_.Configurations)) { '*' } else { $_.Configurations }
        $minBuild = if ([string]::IsNullOrWhiteSpace($_.MinBuild)) { 0 }     else { [int]$_.MinBuild }
        $maxBuild = if ([string]::IsNullOrWhiteSpace($_.MaxBuild)) { 99999 } else { [int]$_.MaxBuild }

        if ($minBuild -le $BuildNumber -and $BuildNumber -le $maxBuild) {
            [void]$exclusions.Add([PSCustomObject]@{
                Pattern        = $pattern
                Configurations = $configs
                Reason         = $_.Reason
            })
            Write-Verbose "Exclusion applied: '$pattern' configs='$configs' reason='$($_.Reason)'"
        }
        else {
            Write-Verbose "Exclusion skipped: '$pattern' - build $BuildNumber outside [$minBuild, $maxBuild]"
        }
    }

    return $exclusions.ToArray()
}

function Get-DiskFreeGB {
    <#
    .SYNOPSIS Returns free disk space in GB for the current drive, or 'N/A' on error.
    #>
    try {
        return [math]::Round((Get-Volume (Get-Item '.').PSDrive.Name).SizeRemaining / 1GB, 1)
    }
    catch {
        return 'N/A'
    }
}

function Build-SingleSample {
    <#
    .SYNOPSIS
        Builds a single sample directory for one configuration/platform combination.
    .DESCRIPTION
        Locates the .sln in the given directory, verifies the configuration|platform is
        supported, then invokes msbuild with up to 3 attempts (to detect sporadic failures).
    .OUTPUTS
        Returns an integer exit code:
          0 = succeeded on first attempt
          1 = failed after all retries
          2 = sporadic (failed first, succeeded on retry)
          3 = unsupported configuration/platform
    #>
    param(
        [string]$Directory,
        [string]$SampleName,
        [string]$Configuration = 'Debug',
        [string]$Platform = 'x64',
        [string]$InfVerif_AdditionalOptions = '/samples',
        [string]$LogFilesDirectory = (Get-Location),
        [bool]$Verbose = $false
    )

    if (-not (Test-Path -Path $Directory -PathType Container)) {
        Write-Warning "`u{274C} A valid directory could not be found under $Directory"
        return 1
    }

    New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

    if ([string]::IsNullOrWhiteSpace($SampleName)) {
        $SampleName = (Resolve-Path $Directory).Path.Replace((Get-Location), '').Replace('\', '.').Trim('.').ToLower()
    }

    $solutionFile = Get-ChildItem -Path $Directory -Filter *.sln |
                    Select-Object -ExpandProperty FullName -First 1

    if ($null -eq $solutionFile) {
        Write-Warning "`u{274C} A solution could not be found under $Directory"
        return 1
    }

    # --- Check whether the solution supports the requested configuration|platform ---
    $configurationIsSupported = $false
    $inSolutionConfigurationPlatformsSection = $false
    foreach ($line in Get-Content -Path $solutionFile) {
        if (-not $inSolutionConfigurationPlatformsSection -and
            $line -match '\s*GlobalSection\(SolutionConfigurationPlatforms\).*') {
            $inSolutionConfigurationPlatformsSection = $true
            continue
        }
        elseif ($line -match '\s*EndGlobalSection.*') {
            $inSolutionConfigurationPlatformsSection = $false
            continue
        }

        if ($inSolutionConfigurationPlatformsSection) {
            [regex]$regex = '.*=\s*(?<ConfigString>(?<Configuration>.*)\|(?<Platform>.*))\s*'
            $match = $regex.Match($line)
            if ([string]::IsNullOrWhiteSpace($match.Groups['ConfigString'].Value) -or
                [string]::IsNullOrWhiteSpace($match.Groups['Platform'].Value)) {
                Write-Warning "Could not parse configuration entry $line from file $solutionFile."
                continue
            }
            if ($match.Groups['Configuration'].Value.Trim() -eq $Configuration -and
                $match.Groups['Platform'].Value.Trim() -eq $Platform) {
                $configurationIsSupported = $true
            }
        }
    }

    if (-not $configurationIsSupported) {
        Write-Verbose "[$SampleName] `u{23E9} Skipped. Configuration $Configuration|$Platform not supported."
        return 3
    }

    Write-Verbose "Building Sample: $SampleName; Configuration: $Configuration; Platform: $Platform {"

    $myexit = 1

    # Build up to three times (0th, 1st, and 2nd attempt).
    #   Succeed on 1st  -> success (0)
    #   Fail 1st, succeed on retry -> sporadic (2)
    #   Fail all three  -> failure (1)
    for ($i = 0; $i -lt 3; $i++) {
        $binLogFilePath   = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.binlog"
        $errorLogFilePath = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.err"
        $warnLogFilePath  = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.wrn"
        $outLogFilePath   = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.out"

        msbuild $solutionFile `
            -clp:Verbosity=m -t:rebuild `
            -property:Configuration=$Configuration `
            -property:Platform=$Platform `
            -p:TargetVersion=Windows10 `
            -p:InfVerif_AdditionalOptions="$InfVerif_AdditionalOptions" `
            -warnaserror `
            -binaryLogger:LogFile=$binLogFilePath`;ProjectImports=None `
            -flp1:errorsonly`;logfile=$errorLogFilePath `
            -flp2:WarningsOnly`;logfile=$warnLogFilePath `
            -noLogo > $outLogFilePath

        if ($null -ne $env:WDS_WipeOutputs) {
            Write-Verbose ("WipeOutputs: $Directory " + (((Get-Volume (Get-Item '.').PSDrive.Name).SizeRemaining / 1GB)))
            Get-ChildItem -Path $Directory -Recurse -Include x64   | Remove-Item -Recurse
            Get-ChildItem -Path $Directory -Recurse -Include arm64 | Remove-Item -Recurse
        }

        if ($LASTEXITCODE -eq 0) {
            $myexit = if ($i -eq 0) { 0 } else { 2 }
            # Remove binlog on success to save space; keep otherwise to diagnose issues.
            Remove-Item $binLogFilePath
            break
        }
        else {
            Start-Sleep 1
            if ($Verbose) {
                Write-Warning "`u{274C} Build failed. Retrying to see if sporadic..."
            }
        }
    }

    if ($myexit -eq 1 -and $Verbose) {
        Write-Warning "`u{274C} Build failed. Log available at $errorLogFilePath"
    }
    if ($myexit -eq 2 -and $Verbose) {
        Write-Warning "`u{274C} Build sporadically failed. Log available at $errorLogFilePath"
    }

    Write-Verbose "Building Sample: $SampleName; Configuration: $Configuration; Platform: $Platform }"

    return $myexit
}

# =============================================================================
#  Step 1 - Prepare Build Environment
# =============================================================================

$root = (Get-Location).Path

$selectedVsInstall = Initialize-DevShell -ReturnToDirectory $root
Assert-MsBuildAvailable

# =============================================================================
#  Step 2 - Calculate Parallelism
# =============================================================================

$throttleFactor    = 5
# Sum across all CPU sockets (Get-CimInstance returns an array on multi-socket systems)
$logicalProcessors = ((Get-CimInstance -Class CIM_Processor -Verbose:$false).NumberOfLogicalProcessors | Measure-Object -Sum).Sum

if ($ThrottleLimit -eq 0) {
    $ThrottleLimit = $throttleFactor * $logicalProcessors
}

$verbose = $PSBoundParameters.ContainsKey('Verbose') -and $PSBoundParameters['Verbose']

# =============================================================================
#  Step 3 - Prepare Log Directory
# =============================================================================

Remove-Item -Recurse -Path $LogFilesDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

$reportHtmlPath = Join-Path $LogFilesDirectory "$ReportFileName.htm"
$reportCsvPath  = Join-Path $LogFilesDirectory "$ReportFileName.csv"

# =============================================================================
#  Step 4 - Load Sample List
# =============================================================================

# Always discover the full sample list when patterns contain wildcards,
# or when no -Samples were provided at all.
$hasWildcards = $Samples | Where-Object { $_ -match '[*?]' }

if (-not $Samples) {
    # No filter: discover and build everything.
    Write-Verbose "No -Samples provided. Discovering samples via ListAllSamples.ps1..."
    $sampleNames = & (Join-Path $PSScriptRoot 'ListAllSamples.ps1') -Verbose:$verbose |
                   Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
}
elseif ($hasWildcards) {
    # One or more entries contain wildcards — discover all, then filter with -like.
    Write-Verbose "Wildcard detected in -Samples. Discovering all samples to match patterns..."
    $allSamples = & (Join-Path $PSScriptRoot 'ListAllSamples.ps1') -Verbose:$verbose |
                  Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $sampleNames = @()
    foreach ($pattern in $Samples) {
        $matched = $allSamples | Where-Object { $_ -like $pattern }
        if ($matched) {
            $sampleNames += $matched
        }
        else {
            Write-Warning "Pattern '$pattern' did not match any samples."
        }
    }
    $sampleNames = $sampleNames | Sort-Object -Unique
}
else {
    # Exact list passed by the caller — use as-is, sorted alphabetically.
    $sampleNames = $Samples | Sort-Object
}

# Map sample names to directory paths, validating each exists
$sampleSet = [ordered]@{}
$skippedCount = 0
foreach ($name in $sampleNames) {
    $fullPath = Join-Path $root ($name.Replace('.', '\'))
    if (Test-Path $fullPath -PathType Container) {
        $sampleSet[$name] = $fullPath
    }
    else {
        Write-Warning "Sample directory not found, skipping: $name"
        $skippedCount++
    }
}

if ($sampleSet.Count -eq 0) {
    Write-Error "No valid sample directories found. Ensure ListAllSamples.ps1 is available in the repo root."
    exit 1
}

# =============================================================================
#  Step 5 - Detect Build Environment
# =============================================================================

$buildEnv    = Resolve-BuildEnvironment -RepoRoot $root -RunMode $RunMode -VsInstallation $selectedVsInstall
$buildNumber = $buildEnv.BuildNumber

# =============================================================================
#  Step 6 - Determine InfVerif Options
# =============================================================================
#
# Samples must build cleanly, but certain InfVerif warnings are acceptable because
# they flag issues intentionally present in samples (to be fixed when productizing).
#   <= 22621: suppress individual warnings  /sw1284 /sw1285 /sw1293 /sw2083 /sw2086
#   >  22621: these are grouped under       /samples
#
if ($InfOptions) {
    $infVerifOptions = $InfOptions
}
else {
    $infVerifOptions = if ($buildNumber -le 22621) { '/sw1284 /sw1285 /sw1293 /sw2083 /sw2086' } else { '/samples' }
}

# =============================================================================
#  Step 7 - Load Exclusions
# =============================================================================

$exclusions = Import-SampleExclusions -CsvPath (Join-Path $root 'exclusions.csv') -BuildNumber $buildNumber

# =============================================================================
#  Step 8 - Print Build Plan
# =============================================================================

$combinationsTotal = $sampleSet.Count * $Configurations.Count * $Platforms.Count

Write-Output ""
Write-Output "--- WDK Sample Build Plan ------------------------------------------"
Write-Output "  Environment:      $($buildEnv.Name)"
Write-Output "  Build Number:     $buildNumber"
if ($buildEnv.NuGetVersion) {
    Write-Output "  NuGet Version:    $($buildEnv.NuGetVersion)"
}
Write-Output "  WDK VS Component: $($buildEnv.WdkVsComponentVersion)"
Write-Output "  InfVerif Options: $infVerifOptions"
Write-Output ""
Write-Output "  Samples:          $($sampleSet.Count) ($skippedCount skipped)"
Write-Output "  Configurations:   $($Configurations -join ', ')"
Write-Output "  Platforms:        $($Platforms -join ', ')"
Write-Output "  Combinations:     $combinationsTotal"
Write-Output "  Exclusions:       $($exclusions.Count)"
Write-Output ""
Write-Output "  Parallelism:      $ThrottleLimit jobs ($logicalProcessors cores x $throttleFactor)"
Write-Output "  Disk Free (GB):   $(Get-DiskFreeGB)"
Write-Output "  Wipe Outputs:     $(-not [string]::IsNullOrEmpty($env:WDS_WipeOutputs))"
Write-Output "--------------------------------------------------------------------"
Write-Output ""
Write-Output "Progress legend:"
Write-Output "  T=Total  B=Built  R=Running  P=Pending"
Write-Output "  S=Succeeded  E=Excluded  U=Unsupported  F=Failed  O=Sporadic"
Write-Output ""
Write-Output "Building all combinations..."

# =============================================================================
#  Step 9 - Execute Parallel Builds
# =============================================================================

# Shared mutable state protected by a Mutex. This is required because
# ForEach-Object -Parallel runs each iteration in a separate runspace,
# so standard .NET locks (Monitor/lock) do not work across runspaces.
$buildState = @{
    Built       = 0
    Succeeded   = 0
    Excluded    = 0
    Unsupported = 0
    Failed      = 0
    Sporadic    = 0
    Results     = @()
    FailSet     = @()
    SporadicSet = @()
    Lock        = [System.Threading.Mutex]::new($false)
}

$stopwatch = [Diagnostics.Stopwatch]::StartNew()

# Capture function definition so it can be reconstructed inside each parallel runspace.
$buildSingleSampleDef = ${function:Build-SingleSample}.ToString()

$sampleSet.GetEnumerator() | ForEach-Object -ThrottleLimit $ThrottleLimit -Parallel {
    # --- Import shared state from parent scope ---
    $logDir     = $using:LogFilesDirectory
    $exclusions = $using:exclusions
    $configs    = $using:Configurations
    $platforms  = $using:Platforms
    $infOpts    = $using:infVerifOptions
    $isVerbose  = $using:verbose
    $state      = $using:buildState
    $total      = $using:combinationsTotal
    $throttle   = $using:ThrottleLimit

    # Reconstruct the function inside this parallel runspace
    ${function:Build-SingleSample} = $using:buildSingleSampleDef

    $sampleName = $_.Key
    $directory  = $_.Value

    # Build a result row: one column per configuration|platform combination
    $resultRow = [PSCustomObject]@{ Sample = $sampleName }

    foreach ($configuration in $configs) {
        foreach ($platform in $platforms) {
            $result           = 'Not run'
            $succeededDelta   = 0
            $excludedDelta    = 0
            $unsupportedDelta = 0
            $failedDelta      = 0
            $sporadicDelta    = 0
            $failEntry        = $null
            $sporadicEntry    = $null

            # -- Check exclusions (supports wildcard paths like 'general.*') --
            $exclusionReason = $null
            foreach ($excl in $exclusions) {
                if ($sampleName -like $excl.Pattern) {
                    $configKey = "$configuration|$platform"
                    foreach ($cfgPattern in $excl.Configurations.Split(';')) {
                        if ($configKey -like $cfgPattern) {
                            $exclusionReason = $excl.Reason
                            break
                        }
                    }
                    if ($exclusionReason) { break }
                }
            }

            if ($exclusionReason) {
                Write-Verbose "[$sampleName $configuration|$platform] Excluded: $exclusionReason"
                $excludedDelta = 1
                $result = 'Excluded'
            }
            else {
                # -- Build the sample --
                $buildResult = Build-SingleSample `
                    -Directory $directory -SampleName $sampleName `
                    -LogFilesDirectory $logDir -Configuration $configuration `
                    -Platform $platform -InfVerif_AdditionalOptions $infOpts `
                    -Verbose:$isVerbose

                # Return codes from Build-SingleSample:
                #   0 = succeeded on first attempt
                #   1 = failed after all retries
                #   2 = sporadic (failed first, succeeded on retry)
                #   3 = unsupported configuration/platform
                switch ($buildResult) {
                    0       { $succeededDelta   = 1; $result = 'Succeeded' }
                    1       { $failedDelta      = 1; $result = 'Failed';      $failEntry     = "$sampleName $configuration|$platform" }
                    2       { $sporadicDelta    = 1; $result = 'Sporadic';    $sporadicEntry = "$sampleName $configuration|$platform" }
                    default { $unsupportedDelta = 1; $result = 'Unsupported' }
                }
            }

            $resultRow | Add-Member -MemberType NoteProperty -Name "$configuration|$platform" -Value $result

            # -- Update shared counters (under lock) --
            $null = $state.Lock.WaitOne()
            try {
                $state.Built       += 1
                $state.Succeeded   += $succeededDelta
                $state.Excluded    += $excludedDelta
                $state.Unsupported += $unsupportedDelta
                $state.Failed      += $failedDelta
                $state.Sporadic    += $sporadicDelta
                if ($failEntry)     { $state.FailSet     += $failEntry }
                if ($sporadicEntry) { $state.SporadicSet += $sporadicEntry }

                # Update progress bar
                $built     = $state.Built
                $remaining = $total - $built
                $running   = [Math]::Min($remaining, $throttle)
                $pending   = [Math]::Max($remaining - $throttle, 0)
                $pct       = [Math]::Round(100 * $built / $total)

                $statusLine = "$built of $total combinations built ($pct%) | " +
                    "T:$total; B:$built; R:$running; P:$pending | " +
                    "S:$($state.Succeeded); E:$($state.Excluded); U:$($state.Unsupported); F:$($state.Failed); O:$($state.Sporadic)"

                # Write-Host with carriage return for a single-line progress indicator.
                # Write-Progress does not reliably render from -Parallel runspaces.
                Write-Host "`rBuilding combinations [$statusLine]" -NoNewline
            }
            finally {
                $state.Lock.ReleaseMutex()
            }
        }
    }

    # Append the completed result row
    $null = $state.Lock.WaitOne()
    try {
        $state.Results += $resultRow
    }
    finally {
        $state.Lock.ReleaseMutex()
    }
}

$stopwatch.Stop()

# End the progress line
Write-Host ""

# =============================================================================
#  Step 10 - Report Failures
# =============================================================================

Write-Output ""

if ($buildState.FailSet.Count -gt 0) {
    Write-Output "--- Build Failures -------------------------------------------------"
    foreach ($entry in ($buildState.FailSet | Sort-Object)) {
        if ($entry -match '^(?<name>.*)\s+(?<config>\w+)\|(?<platform>\w+)$') {
            $errLog = Join-Path $LogFilesDirectory "$($Matches.name).$($Matches.config).$($Matches.platform).0.err"
            Write-Output "  [FAIL] $($Matches.name) ($($Matches.config)|$($Matches.platform)):"
            if (Test-Path $errLog) {
                Get-Content $errLog | ForEach-Object { Write-Output "     $_" }
            }
            else {
                Write-Output "     (error log not found: $errLog)"
            }
        }
    }
    Write-Output ""
    Write-Error "Some combinations were built with errors."
}

if ($buildState.SporadicSet.Count -gt 0) {
    Write-Output "--- Sporadic Failures (succeeded on retry) -------------------------"
    foreach ($entry in ($buildState.SporadicSet | Sort-Object)) {
        if ($entry -match '^(?<name>.*)\s+(?<config>\w+)\|(?<platform>\w+)$') {
            $errLog = Join-Path $LogFilesDirectory "$($Matches.name).$($Matches.config).$($Matches.platform).0.err"
            Write-Output "  [SPORADIC] $($Matches.name) ($($Matches.config)|$($Matches.platform)):"
            if (Test-Path $errLog) {
                Get-Content $errLog | ForEach-Object { Write-Output "     $_" }
            }
        }
    }
    Write-Output ""
    Write-Error "Some combinations had sporadic build failures."
}

# =============================================================================
#  Step 11 - Final Summary
# =============================================================================

$elapsed = $stopwatch.Elapsed

Write-Output "--- Build Complete -------------------------------------------------"
Write-Output "  Elapsed:          $($elapsed.Minutes)m $($elapsed.Seconds)s"
Write-Output "  Disk Free (GB):   $(Get-DiskFreeGB)"
Write-Output ""
Write-Output "  Samples:          $($sampleSet.Count)"
Write-Output "  Configurations:   $($Configurations -join ', ')"
Write-Output "  Platforms:        $($Platforms -join ', ')"
Write-Output "  Combinations:     $combinationsTotal"
Write-Output ""
Write-Output "  Succeeded:        $($buildState.Succeeded)"
Write-Output "  Excluded:         $($buildState.Excluded)"
Write-Output "  Unsupported:      $($buildState.Unsupported)"
Write-Output "  Failed:           $($buildState.Failed)"
Write-Output "  Sporadic:         $($buildState.Sporadic)"
Write-Output ""
Write-Output "  Log directory:    $LogFilesDirectory"
Write-Output "  CSV report:       $reportCsvPath"
Write-Output "  HTML report:      $reportHtmlPath"
Write-Output "--------------------------------------------------------------------"

# =============================================================================
#  Step 12 - Generate Reports
# =============================================================================

$sortedResults = $buildState.Results | Sort-Object { $_.Sample }
$sortedResults | ConvertTo-Csv  | Out-File $reportCsvPath
$sortedResults | ConvertTo-Html -Title "WDK Sample Build Overview" | Out-File $reportHtmlPath

# Only open the HTML report interactively (not in CI/automation or Github mode)
if (-not $buildEnv.IsGithubMode -and -not $env:BUILD_BUILDID -and [Environment]::UserInteractive) {
    Invoke-Item $reportHtmlPath
}
