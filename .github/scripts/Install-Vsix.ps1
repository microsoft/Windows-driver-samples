<#

.SYNOPSIS
Checks WDK vsix version and downloads and installs as necessary.

#>

[CmdletBinding()]
param(
    [bool]$optimize = $false
)

$root = Get-Location

# launch developer powershell
Import-Module (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*\Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*")
cd $root

# Automatically resolve the latest amd64 VSIX
$uri = "https://marketplace.visualstudio.com$((Invoke-WebRequest -Uri "https://marketplace.visualstudio.com/items?itemName=DriverDeveloperKits-WDK.WDKVsix").Links | Where-Object outerHTML -like '*(amd64)*' | select -expand href)"

# Set local version variable
$version = ([regex]'(\d+\.)(\d+\.)(\d+\.)(\d+)').Matches($uri).Value

# Set github environment variable for vsix version
"SAMPLES_VSIX_VERSION=$version" | Out-File -FilePath "$env:GITHUB_ENV" -Append

function PrintWdkVsix {
    "WDK Vsix Version: $(ls "${env:ProgramData}\Microsoft\VisualStudio\Packages\Microsoft.Windows.DriverKit,version=*" | Select -ExpandProperty Name)"
}

function TestWdkVsix {
    Test-Path "${env:ProgramData}\Microsoft\VisualStudio\Packages\Microsoft.Windows.DriverKit,version=$version"
}

# NOTE: The '$optimize' code path examines the '.vsixmanifest' when downloaded and then examines it again (in the 'MSBuild' directory) once 
#       the necessary extension files are copied.
if ($optimize) {
    $msbuild_path = (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*\MSBuild\")
    "---> Downloading vsix version: $version"
    Invoke-WebRequest -Uri "$uri" -OutFile wdk.zip
    Expand-Archive ".\wdk.zip" .\
    "Downloaded VSIX Version: $(([xml](Get-Content .\extension.vsixmanifest)).PackageManifest.Metadata.Identity.Version)"
    "<--- Download complete"
    "---> Configuring build environment..."
    cp ".\`$MSBuild\*" "$msbuild_path" -Recurse -Force
    cp ".\extension.vsixmanifest" "$msbuild_path"
    "Installed VSIX Version: $(([xml](Get-Content ${msbuild_path}\extension.vsixmanifest)).PackageManifest.Metadata.Identity.Version)"
    "<--- Configuration complete"
}
else {
    "Getting installed WDK vsix..."
    PrintWdkVsix
    "Checking the WDK.vsix version installed..."
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
