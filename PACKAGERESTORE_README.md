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
Some combinations were built with errors:
Build errors in Sample prm; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\prm\prmfunc\prmfuncsample.h(29,10): error C1083: Cannot open include file: 'prminterface.h': No such file or directory [C:\Windows-driver-samples\prm\prmfunc\prmfuncsample.vcxproj]
} prm Debug|x64
Build errors in Sample storage.class.classpnp; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\storage\class\classpnp\src\classpnp.vcxproj(317,5): error MSB3073: The command "wmimofck -yx64\Debug\\MOF.MOF -zx64\Debug\\MFL.MFL x64\Debug\\MOFMFL.MOF" exited with code 9009.
} storage.class.classpnp Debug|x64
Build errors in Sample storage.msdsm; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\storage\msdsm\src\SampleDSM.vcxproj(297,5): error MSB3073: The command "wmimofck -yx64\Debug\\MOF.MOF -zx64\Debug\\MFL.MFL x64\Debug\\MOFMFL.MOF" exited with code 9009.
} storage.msdsm Debug|x64
Build errors in Sample usb.ucmcxucsi; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\usb\UcmCxUcsi\Pch.h(27,10): error C1083: Cannot open include file: 'acpiioct.h': No such file or directory [C:\Windows-driver-samples\usb\UcmCxUcsi\UcmCxUcsi.vcxproj]
...
} usb.ucmcxucsi Debug|x64
Build errors in Sample usb.ucmucsiacpisample; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\usb\UcmUcsiAcpiSample\UcmUcsiAcpiSample\Pch.h(29,10): error C1083: Cannot open include file: 'acpiioct.h': No such file or directory [C:\Windows-driver-samples\usb\UcmUcsiAcpiSample\UcmUcsiAcpiSample\UcmUcsiAcpiSample.vcxproj]
...
} usb.ucmucsiacpisample Debug|x64
Build errors in Sample video.kmdod; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\video\KMDOD\BDD.hxx(44,14): error C1083: Cannot open include file: 'd3dkmddi.h': No such file or directory ...
} video.kmdod Debug|x64
Build errors in Sample wmi.wmiacpi; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\wmi\wmiacpi\acpimof.vcxproj(184,5): error MSB3073: The command "wmimofck -yx64\Debug\\MOF.MOF -zx64\Debug\\MFL.MFL x64\Debug\\MOFMFL.MOF" exited with code 9009.
} wmi.wmiacpi Debug|x64
Build errors in Sample wmi.wmisamp; Configuration: Debug; Platform: x64 {
C:\Windows-driver-samples\packages\Microsoft.Windows.WDK.x64.10.0.26052.1000-preview.ge-release\c\build\10.0.26052.0\WindowsDriver.common.targets(707,5): error MSB6006: "mofcomp.exe" exited with code 1. [C:\Windows-driver-samples\wmi\wmisamp\WmiSamp.vcxproj]
} wmi.wmisamp Debug|x64
...
Elapsed time:         9 minutes, 58 seconds.
Disk Remaining (GB):  356.588935852051
Samples:              132
Configurations:       1 (Debug)
Platforms:            1 (x64)
Combinations:         132
Succeeded:            121
Excluded:             2
Unsupported:          1
Failed:               8
Log files directory:  C:\Windows-driver-samples\_logs
Overview report:      C:\Windows-driver-samples\_logs\_overview.htm
>>>
```

So 8 samples still failing.  Let's drive this to 0.  Remaining are:
* 2 x error C1083: Cannot open include file: 'acpiioct.h'
* 1 x error C1083: Cannot open include file: 'prminterface.h'
* 1 x error C1083: Cannot open include file: 'd3dkmddi.h'
* 3 x error MSB3073: The command "wmimofck -yx64\Debug\\MOF.MOF -zx64\Debug\\MFL.MFL x64\Debug\\MOFMFL.MOF" exited with code 9009.
* 1 x error MSB6006: "mofcomp.exe" exited with code 1.

5. There are a number of TODOs in .\Building-Locally.md
