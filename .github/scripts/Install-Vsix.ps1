<#

.SYNOPSIS
Download and install the latest WDK VSIX.

#>

# set uri by resolving amd64 vsix
$uri = "https://marketplace.visualstudio.com$((Invoke-WebRequest -Uri "https://marketplace.visualstudio.com/items?itemName=DriverDeveloperKits-WDK.WDKVsix").Links | Where-Object outerHTML -like '*(amd64)*' | select -expand href)"

# set download version
$uri_version = ([regex]'(\d+\.)(\d+\.)(\d+\.)(\d+)').Matches($uri).Value

# set msbuild path
$msbuild_path = (Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\2022\*\MSBuild\")

# download vsix, expand, and store the downloaded version extracted from the extension manifest
"Downloading WDK VSIX version: $uri_version..."
Invoke-WebRequest -Uri "$uri" -OutFile wdk.zip
"Expanding WDK VSIX archive..."
Expand-Archive ".\wdk.zip" .\
"Extracting version from manifest..."
$downloaded_version = ([xml](Get-Content .\extension.vsixmanifest)).PackageManifest.Metadata.Identity.Version
"Downloaded WDK VSIX version: $downloaded_version"

# copy msbuild files, extension manifest, and check installed version from the extension manifest
"Copying WDK extension files to build path..."
cp (".\`$MSBuild\*", ".\extension.vsixmanifest") "$msbuild_path" -Recurse -Force
"Extracting version from copied manifest..."
$installed_version = ([xml](Get-Content ${msbuild_path}\extension.vsixmanifest)).PackageManifest.Metadata.Identity.Version
"Installed WDK VSIX Version: $installed_version"
if (-not ("$downloaded_version" -eq "$installed_version")) { 
    "WDK VSIX installation failed due to version mismatch"
    exit 1
}

# set github environment variable for vsix version
"SAMPLES_VSIX_VERSION=$installed_version" | Out-File -FilePath "$env:GITHUB_ENV" -Append
