To restore a specific version of a NuGet please follow following steps:

Prerequisites
1. VS 2022 with all workloads required for WDK, you can use automation [winget install --source winget --exact --id Microsoft.VisualStudio.2022.Community --override "--passive --config <vsconfig-folder>\wdk.vsconfig"]
2. Install WDK VSIX
3. Add NuGet Feed to VS 

At this point you are almost there.
1. Open the packages.config file in the root folder and update the full version (including the branch if required) in all three entries.
2. Open the Directory.build.props file and update the version&build of the package with the same values as in step 2.
3. Open developer command prompt in admin mode.
4. msbuild -t:restore -p:RestorePackagesConfig=true RestorePackage.vcxproj

---

Bugs:

1. Let us provide a way to set the NuGet source via NuGet Command Line.  I know how to do, just need to find it..

2. Build-AllSamples.ps1 must detect that it runs in NuGet environment.  I put in a quick hack, but not the right solution.

3. Let's write a script that updates Directory.Build.props and packages.config.

4. This is where we are:
 Repro steps:
 .\Build-AllSamples -Configurations 'Debug' -Platforms 'x64'
 
 Expected:
<<<
...
Failed:               0
 >>>
 
 Actual:
<<<
Build Environment:          NuGet
Build Number:               26045
Samples:                    132
Configurations:             1 (Debug)
Platforms:                  1 (x64)
InfVerif_AdditionalOptions: /samples /sw1402
Combinations:               132
...
Elapsed time:         7 minutes, 36 seconds.
Disk Remaining (GB):  360.743003845215
Samples:              132
Configurations:       1 (Debug)
Platforms:            1 (x64)
Combinations:         132
Succeeded:            110
Excluded:             2
Unsupported:          1
Failed:               19
>>>

Let's drive above to 0!!!
