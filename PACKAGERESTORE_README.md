To restore a specific version of a NuGet please follow following steps:

Prerequisites
1. VS 2012 with all workloads required for WDK, you can use automation [winget install --source winget --exact --id Microsoft.VisualStudio.2022.Community --override "--passive --config <vsconfig-folder>\wdk.vsconfig"]
2. Install WDK VSIX
3. Add NuGet Feed to VS 

At this point you are almost there.
1. Open the packages.config file in the root folder and update the full version (including the branch if required) in all three entries.
2. Open the Directory.build.props file and update the version&build of the package with the same values as in step 2.
3. Open developer command propmt in admin mode.
4. msbuild -t:restore -p:RestorePackagesConfig=true RestorePackage.vcxproj
