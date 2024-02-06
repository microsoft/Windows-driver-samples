# To use NuGet:

Use updated instructions in .\Building-Locally.md, specifically section 'Option D: Use WDK NuGet Packages from "experimental package feed"'.

# To restore a specific version of our WDK NuGet packages:

Follow following steps before running "nuget restore" command:
* Open the .\packages.config file and update the full version (including the branch if required) in all three entries.
* Open the .\Directory.build.props file and update the version and build of the package with the same values as in previous step.
* Open .\Build-SampleSet and change the NuGet build number (used by .\exclusions.csv and for determining infverif flags)
* Now you can run "nuget restore"

# Bugs

1. Resolved.

2. Resolved.

3. Let's write a script that updates Directory.Build.props and packages.config.

4. This is where we are:

```
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
```

Let's drive above to 0!!!

5. There are a number of TODOs in .\Building-Locally.md
