<#

.SYNOPSIS
Checks WDK vsix version and downloads and installs as necessary.

#>

[CmdletBinding()]
param(
    [bool]$optimize = $false
)

$root = Get-Location

# launch developer powershell (if necessary)
if (-not $env:VSCMD_VER) {
    Import-Module (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*\Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    Enter-VsDevShell -VsInstallPath (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*")
    cd $root
}

# source environment variables
. .\Env-Vars.ps1

$version = $env:SAMPLES_VSIX_VERSION
$uri = $env:SAMPLES_VSIX_URI

function PrintWdkVsix {
    $installed = ls "${env:ProgramData}\Microsoft\VisualStudio\Packages\Microsoft.Windows.DriverKit,version=*" | Select -ExpandProperty Name
    "WDK Vsix Version: $installed"
}

function TestWdkVsix {
    Test-Path "${env:ProgramData}\Microsoft\VisualStudio\Packages\Microsoft.Windows.DriverKit,version=$version"
}

if ($optimize) {
    "---> Downloading vsix and configuring build environment..."
    Invoke-WebRequest -Uri "$uri" -OutFile wdk.zip
    Expand-Archive ".\wdk.zip" .\
    cp ".\`$MSBuild\*" (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*\MSBuild\") -Recurse -Force
    "<--- Finished"
}
else {
    PrintWdkVsix
    if (-not (TestWdkVsix)) {
        "The correct WDK vsix is not installed."
        "Will attempt to download and install now..."
        Invoke-WebRequest -Uri "$uri" -OutFile wdk.vsix
        "Finished downloading."
        "Starting install process. This will take some time to complete..."
        Start-Process vsixinstaller -ArgumentList "/f /q /sp .\wdk.vsix" -wait
        "The install process has finished."
        "Checking the WDK.vsix version installed..."
        if (TestWdkVsix) {
            PrintWdkVsix
            "The WDK vsix version is OK"
        }
        else {
            "The WDK vsix install FAILED"
            Write-Host "`u{274C} wdk vsix install had an issue"
            Write-Error "the wdk vsix cannot be installed at this time"
            exit 1
        }
    }
}
