# Environment variables for script sourcing.
# Note: When a new WDK ships the following need to be updated: 
#       1. Environment variables in .\Env-Vars.ps1 (this script)
#       2. Nuget package versions in .\packages.config 
#       3. Nuget package versions in .\Directory.Build.props
#       4. SDK and WDK versions and WDK vsix link in .\configuration.dsc.yaml
$env:SAMPLES_VSIX_VERSION = "10.0.26100.0"
$env:SAMPLES_VSIX_URI = "https://marketplace.visualstudio.com/_apis/public/gallery/publishers/DriverDeveloperKits-WDK/vsextensions/WDKVsix/10.0.26100.0/vspackage?targetPlatform=5e3e564c-03bb-4499-8ae5-b2b35e9a86dc"
$env:SAMPLES_BUILD_NUMBER = "26100"
