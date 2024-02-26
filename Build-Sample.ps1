<#
.SYNOPSIS
Builds an specific directory containing a sample solution.

.DESCRIPTION
This script attempts to build a directory containing a driver sample Solution for the specified configurations and platforms.

.PARAMETER Directory
Path to a directory containing a valid Visual Studio Solution (.sln file). This is the solution that will be built.

.PARAMETER SampleName
A friendly name to refer to the sample. Is unspecified, a name will be automatically generated one from the sample path.

.PARAMETER Configuration
Configuration name that will be used to build the solution. Common available values are "Debug" and "Release".

.PARAMETER Platform
Platform to build the solution for (e.g. "x64", "arm64").

.PARAMETER InfVerif_AdditionalOptions
Additional options for infverif (e.g. "/samples").

.PARAMETER LogFilesDirectoy
Path to a directory where the log files will be written to. If not provided, outputs will be logged to the current working directory.

.INPUTS
None.

.OUTPUTS
Verbose output about the execution of this script will be provided only if -Verbose is provided. Otherwise, no output will be generated.

.EXAMPLE
.\Build-Sample -Directory .\usb\kmdf_fx2

.EXAMPLE
.\Build-Sample -Directory .\usb\kmdf_fx2 -Configuration 'Release' -Platform 'x64' -Verbose -LogFilesDirectory .\_logs

#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true,
            HelpMessage = 'Enter one directory path',
            Position = 0)]
    [string]$Directory,
    [string]$SampleName,
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$InfVerif_AdditionalOptions = "/samples",
    $LogFilesDirectory = (Get-Location)
)

$Verbose = $false
if ($PSBoundParameters.ContainsKey('Verbose')) {
    $Verbose = $PsBoundParameters.Get_Item('Verbose')
}

$oldPreference = $ErrorActionPreference
$ErrorActionPreference = "stop"
try
{
    # Check that msbuild can be called before trying anything.
    Get-Command "msbuild" | Out-Null
}
catch
{
    Write-Verbose "`u{274C} msbuild cannot be called from current environment. Check that msbuild is set in current path (for example, that it is called from a Visual Studio developer command)."
    Write-Error "msbuild cannot be called from current environment."
    exit 1
}
finally
{
    $ErrorActionPreference = $oldPreference
}

if (-not (Test-Path -Path $Directory -PathType Container))
{
    Write-Warning "`u{274C} A valid directory could not be found under $Directory"
    exit 1
}

New-Item -ItemType Directory -Force -Path $LogFilesDirectory | Out-Null

if (-not (Test-Path -Path $LogFilesDirectory -PathType Container))
{
    Write-Warning "`u{274C} A valid directory for storing log files could not be created under $LogFilesDirectory"
    # No exit here: process will continue but logs won't be available.
}

if ([string]::IsNullOrWhitespace($SampleName))
{
    $SampleName = (Resolve-Path $Directory).Path.Replace((Get-Location), '').Replace('\', '.').Trim('.').ToLower()
}

$solutionFile = Get-ChildItem -Path $Directory -Filter *.sln | Select-Object -ExpandProperty FullName -First 1

if ($null -eq $solutionFile)
{
    Write-Warning "`u{274C} A solution could not be found under $Directory"
    exit 1
}

$configurationIsSupported = $false
$inSolutionConfigurationPlatformsSection = $false
foreach ($line in Get-Content -Path $solutionFile)
{
    if (-not $inSolutionConfigurationPlatformsSection -and $line -match "\s*GlobalSection\(SolutionConfigurationPlatforms\).*")
    {
        $inSolutionConfigurationPlatformsSection = $true;
        continue;
    }
    elseif ($line -match "\s*EndGlobalSection.*")
    {
        $inSolutionConfigurationPlatformsSection = $false;
        continue;
    }

    if ($inSolutionConfigurationPlatformsSection)
    {
        [regex]$regex = ".*=\s*(?<ConfigString>(?<Configuration>.*)\|(?<Platform>.*))\s*"
        $match = $regex.Match($line)
        if ([string]::IsNullOrWhiteSpace($match.Groups["ConfigString"].Value) -or [string]::IsNullOrWhiteSpace($match.Groups["Platform"].Value))
        {
            Write-Warning "Could not parse configuration entry $line from file $solutionFile."
            continue;
        }
        if ($match.Groups["Configuration"].Value.Trim() -eq $Configuration -and $match.Groups["Platform"].Value.Trim() -eq $Platform)
        {
            $configurationIsSupported = $true;
        }
    }
}

if (-not $configurationIsSupported)
{
    Write-Verbose "[$SampleName] `u{23E9} Skipped. Configuration $Configuration|$Platform not supported."
    exit 3
}

Write-Verbose "Building Sample: $SampleName; Configuration: $Configuration; Platform: $Platform {"

$myexit=0

#
# Let us build up to three times (0th, 1st, and 2nd attempt).
# If we succeed at first, then it is a success.
# If we fail at first, but succeed at either of next two attempts, then it is a sporadic failure.
# If we even at third attempt fail, then it is a true failure.
#
for ($i=0; $i -le 2; $i++) {
    $errorLogFilePath = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.err"
    $warnLogFilePath = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.wrn"
    $OutLogFilePath = "$LogFilesDirectory\$SampleName.$Configuration.$Platform.$i.out"

    msbuild $solutionFile -clp:Verbosity=m -t:rebuild -property:Configuration=$Configuration -property:Platform=$Platform -p:TargetVersion=Windows10 -p:InfVerif_AdditionalOptions="$InfVerif_AdditionalOptions" -warnaserror -flp1:errorsonly`;logfile=$errorLogFilePath -flp2:WarningsOnly`;logfile=$warnLogFilePath -noLogo > $OutLogFilePath
    if ($env:WDS_WipeOutputs -ne $null)
    {
        Write-Verbose ("WipeOutputs: "+$Directory+" "+(((Get-Volume ($DriveLetter=(Get-Item ".").PSDrive.Name)).SizeRemaining/1GB)))
        Get-ChildItem -path $Directory -Recurse -Include x64|Remove-Item -Recurse
        Get-ChildItem -path $Directory -Recurse -Include arm64|Remove-Item -Recurse
    }
    if ($LASTEXITCODE -eq 0)
    {
        # We succeeded building. 
        # If at first attempt, then $myexit=0
        # If at later attempt, then $myexit=2
        if ($i -eq 0)
        {
            $myexit=0
        }
        else
        {
            $myexit=2
        }
         break;
    }
    else
    {
        # We failed building. 
        # Let us sleep for a bit.
        # Then let the while loop do its thing and re-run.
        sleep 1
        if ($Verbose)
        {
            Write-Warning "`u{274C} Build failed. Retrying to see if sporadic..."
        }
    }
}

if ($myexit -eq 1)
{
    if ($Verbose)
    {
        Write-Warning "`u{274C} Build failed. Log available at $errorLogFilePath"
    }
    exit 1
}

if ($myexit -eq 2)
{
    if ($Verbose)
    {
        Write-Warning "`u{274C} Build sporadically failed. Log available at $errorLogFilePath"
    }
    exit 2
}

Write-Verbose "Building Sample: $SampleName; Configuration: $Configuration; Platform: $Platform }"
