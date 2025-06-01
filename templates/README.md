# Windows Driver Kit Templates

## What

The Windows Driver Kit WDK.vsix includes 16 templates to easily get started on building drivers.  Once installed you can instantiate these from within Visual Studio using "File -> New -> Project ... -> Type: Driver" .

This folder contains a "snapshot" of these "templates" exported as "samples".  In case you do not have access to the Windows Driver Kit WDK.Vvsix or simply if you prefer the "sample version".

## Version

The exact version of the WDK.vsix used was 10.0.26045.0 and instantiated 2024/02/09.  It is possible the folder here will be a bit "out of sync" with the latest official WDK.vsix.

## How the Samples was Created

After installation of VSIX in Visual Studio the templates can be found under: 
  C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\mjmwh1sj.nyr\ProjectTemplates\Windows Drivers\ . However you cannot just copy and use them as various substituions will have to be done as part of instantiating the template.  As such this copy of the templates is made by manually instantiating each of these templates.
  
  After instantiating them all I ran following to remove .vs folder and *.user files: 
```
  for /F %i in ('dir /s /b /A:DH .vs') do rmdir /s /q %i
  del /s *.user
```

## List of Templates

Here are the templates:

```
  Applications:
    EmptyApplication: Empty Desktop Application for Drivers (Universal)
    EmptyDll: Empty DLL for Drivers (Universal) <-- Bug: This is not marked in VS as "Project Type=Driver".
    EmptyStatic: Empty Static Library for Drivers (Universal)
    WinUsbApp: WinUSB Application (Universal)
  
  Devices:
    KmdfUsb: Kernel Mode Driver, USB (KMDF)
    NdisFilter: Filter Driver: NDIS
    Umdf2Usb: User Mode Driver, USB (UMDF V2)
    v4PrintDriver: Printer Driver V4 <-- This requires filling out fields in Wizard. I left all fields with their default settings.
    XpsRenderFilter: Printer XPS Render Filter
  
  Legacy:
    WDMEmpty: Empty WDM Driver
  
  Package:
    DriverPackage: Driver Install Package
    PrinterDriverPackage: Printer Driver Install Package <-- Bug: This is listed but does not show in VS "File -> New -> Project ..." dialog
    WinUsbInfPackage: WinUSB INF Driver Package
  
  WDF:
    KMDF: Kernel Mode Driver (KMDF)
    KMDFEmpty: Kernel Mode Driver, Empty (KMDF)
    UMDF2: User Mode Driver (UMDF V2)
    UMDF2Empty: User Mode Driver, Empty (UMDF V2)

```

So 16 templates above and a 17th one that is included with WDK.vsix, but  I could not find.

## Do they build?

I tested using:
* Using both EWDK and WDK NuGet (specifically EWDK ge_release_26052_240202-1419 and NuGet 10.0.26052.1000-preview.ge-release ).
* Targeting both Configurations 'Debug' and 'Release'
* Targeting both Platform 'x64' and 'arm64'
* So 16 samples and 8 permutations for each sample.  128 build combinations in total.
* Great news: I got identical results for all permutations. 
* 7 templates builds and 9 does not build.

How:
```
  .\Build-AllSamples -Samples '^templates.' -Configurations 'Debug' -Platforms 'x64'
```

Results:

```
Sample                     Result
templates.driverpackage    Failed     <-- TODO: We should annotate why this template does not build.
templates.emptyapplication Failed     <-- TODO: We should annotate why this template does not build.
templates.emptydll         Failed     <-- TODO: We should annotate why this template does not build.
templates.emptystatic      Succeeded
templates.kmdf             Succeeded
templates.kmdfempty        Failed     <-- TODO: We should annotate why this template does not build.
templates.kmdfusb          Succeeded
templates.ndisfilter       Failed     <-- TODO: We should annotate why this template does not build.
templates.umdf2            Succeeded
templates.umdf2empty       Failed     <-- TODO: We should annotate why this template does not build.
templates.umdf2usb         Succeeded
templates.v4printdriver    Failed     <-- TODO: We should annotate why this template does not build.
templates.wdmempty         Failed     <-- TODO: We should annotate why this template does not build.
templates.winusbapp        Succeeded
templates.winusbinfpackage GE: Succeeded/NI: Failed <-- TODO: We should annotate why this template builds on GE, but not on NI.
templates.xpsrenderfilter  Failed     <-- TODO: We should annotate why this template does not build.
```
