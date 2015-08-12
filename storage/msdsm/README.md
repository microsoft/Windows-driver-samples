Multipath I/O (MPIO) DSM Sample
===============================

The MPIO DSM Sample is intended to serve as an example to follow when building your own vendor specific device specific modules (DSM). This sample DSM supports iSCSI and Fibre Channel devices.


Build the sample
----------------

You can build the sample in two ways: using Microsoft Visual Studio or the command line (*MSBuild*).

Building a Driver Using Visual Studio
-------------------------------------

You build a driver the same way you build any project or solution in Visual Studio. When you create a new driver project using a Windows driver template, the template defines a default (active) project configuration and a default (active) solution build configuration. When you create a project from existing driver sources or convert existing driver code that was built with previous versions of the WDK, the conversion process preserves the target version information (operating systems and platform).

The default Solution build configuration is **Debug** and **Win32**.

### To select a configuration and build a driver or an application

1.  Open the driver project or solution in Visual Studio (find *samplename*.sln or *samplename*.vcxproj).
2.  Right-click the solution in the **Solutions Explorer** and select **Configuration Manager**.
3.  From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Debug or Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
4.  From the Build menu, click **Build Solution** (Ctrl+Shift+B).

Building a Driver Using the Command Line (MSBuild)
--------------------------------------------------

You can build a driver from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe) Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. You can now use the Visual Studio Command Prompt window for all build configurations.

### To select a configuration and build a driver or an application

1.  Open a Visual Studio Command Prompt window at the **Start** screen. From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.
2.  Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called *filtername*.vcxproj, navigate to the project directory and enter the following MSBuild command: 
    
    **msbuild /t:clean /t:build .\\<*samplename*>.vcxproj**.

Installation and Operation
--------------------------

The installation process depends on proper construction of your DSM's INF as well as an installation program provided by you. These are important aspects of complying with the Designed for Windows logo program. The installation was designed to allow for multiple vendors to easily add DSMs and to eliminate rebooting as much as possible. The installation process will require you to update your installation routines and to use the new .INF files. With the new process, you can only modify your DSM's INF file.

The installer sample only needs to be called one time with the INF/driver source path, the name of the DSM INF, and the DSM hardware ID. Typically this would be called from an MSI or setup package, such as one created by InstallShield or other installer technology.

The following annotated DSM INF file illustrates the correct format for your DSM. Replace only those items that are in bold italics. Remember, you must not use "GENDSM" or "MSISCDSM" or "MSDSM" as the name of your DSM. Therefore, you must replace any instances of those strings with the proper name of your DSM.

```
;
; Copyright (c) <YOUR COMPANY NAME HERE>.  All rights reserved.
;
```

In the Version section, make sure the DriverVer is correct for your DSM. Ideally it should match the version in the .rc file. You must specify a different catalog file since the MPIO core drivers now come pre-signed:

```
[Version]
Signature   = "$WINDOWS NT$"
Class       = System
ClassGuid   = {4D36E97D-E325-11CE-BFC1-08002BE10318}
Provider    = %VNDR%
CatalogFile = mydsm.cat
DriverVer   = MM/DD/YYYY,x.x.xxxx

[DestinationDirs]
DefaultDestDir = 12

;
; Multi-Path Device-Specific Module
;

[Manufacturer]
%std_mfg% = std_mfg
```

Substitute all instances of "gendsm" with the proper name for your DSM. For example, "mydsm":

```
[std_mfg]
%mydsm_devicedesc% = mydsm_install, Root\MYDSM

[mydsm_install]
copyfiles = @mydsm.sys

[mydsm_install.Services]
AddService = mydsm, %SPSVCINST_ASSOCSERVICE%, mydsm_service

[mydsm_service]
DisplayName    = %mydsm_desc%
ServiceType    = %SERVICE_KERNEL_DRIVER%
StartType      = %SERVICE_BOOT_START%
ErrorControl   = %SERVICE_ERROR_NORMAL%
ServiceBinary  = %12%\mydsm.sys
LoadOrderGroup = "System Bus Extender"
AddReg         = mydsm_addreg
```

This next section contains the Hardware ID strings for your devices. You can have more than one. Sample format: "VENDOR PRODUCT " - remember to use spaces in a field (vendor, product ID) to pad this to be eight characters for the vendor name (as registered with STA) and sixteen for the product ID (unless the supported devices share a common prefix, in which case the product ID can be less than 16 characters).

**Note** Underscores that are part of the inquiry string (applies to vendor ID as well as product ID fields) must NOT be replaced with spaces.

In this sample, there are two different strings:

```
;
; The following cannot be grouped (as above)
;

HKLM, "SYSTEM\CurrentControlSet\Control\MPDEV", "MPIOSupportedDeviceList", %REG_MULTI_SZ_APPEND%, "VENDOR1 PRODUCT1        "
HKLM, "SYSTEM\CurrentControlSet\Control\MPDEV", "MPIOSupportedDeviceList", %REG_MULTI_SZ_APPEND%, "VENDOR2 PRODUCT2        "
```

These are valid samples:

```
HKLM, "SYSTEM\CurrentControlSet\Control\MPDEV", "MPIOSupportedDeviceList", %REG_MULTI_SZ_APPEND%, "MAXTOR  ATLASU320_18_WLS"

HKLM, "SYSTEM\CurrentControlSet\Control\MPDEV", "MPIOSupportedDeviceList", %REG_MULTI_SZ_APPEND%, "VENDOR3 PROD_PREFIX"
```

(to replace "VENDOR3 PROD\_PREFIX\_A ", "VENDOR3 PROD\_PREFIX\_B " and "VENDOR3 PROD\_PREFIX\_C ")

In the above example, it is assumed that this sample DSM will be used to support three devices from vendor "VENDOR3" with product IDs "PROD\_PREFIX\_A", "PROD\_PREFIX\_B" and "PROD\_PREFIX\_C" respectively. Since all the three devices share the common product ID sub-string "PROD\_PREFIX", we can replace separate entries (in MPIOSupportedDeviceList) for each one of them with just one entry that uses the product ID sub-string that is common to them, without padding it with spaces to make it 16 characters.

It is advisable to use this format if your storage devices generate product IDs on-the-fly using a known product ID prefix. This can significantly reduce the size of your INF file and makes future changes to the INF file less prone to human error. Large INF files can result in very long device installation times and will fill the registry with unnecessary information. Please make sure you take advantage of this new capability as it will improve your customers' experience with MPIO.

Add one entry for each WMI GUID that you use in your DSM. This is required:

```
HKLM, "SYSTEM\CurrentControlSet\Control\WMI\Security", "04517f7e-92bb-4ebe-aed0-54339fa5f544",\%REG_BINARY_NOCLOBBER%,\
        01,00,04,80,14,00,00,00,24,00,00,00,00,00,00,00,\
        34,00,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,02,00,20,00,01,00,00,00,00,00,18,00,\
        1f,00,12,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00
HKLM, "SYSTEM\CurrentControlSet\Control\WMI\Security", "d13373f6-0114-4fe3-b91b-f52c95dfc417",\%REG_BINARY_NOCLOBBER%,\
        01,00,04,80,14,00,00,00,24,00,00,00,00,00,00,00,\
        34,00,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,02,00,48,00,03,00,00,00,00,00,18,00,\
        ff,0f,12,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,00,00,14,00,0d,00,12,00,01,01,00,00,\
        00,00,00,01,00,00,00,00,00,00,14,00,ff,07,12,00,\
        01,01,00,00,00,00,00,05,12,00,00,00
HKLM, "SYSTEM\CurrentControlSet\Control\WMI\Security", "d6dc1bf0-95fa-4246-afd7-40a030458f48",\%REG_BINARY_NOCLOBBER%,\
        01,00,04,80,14,00,00,00,24,00,00,00,00,00,00,00,\
        34,00,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,02,00,48,00,03,00,00,00,00,00,18,00,\
        ff,0f,12,00,01,02,00,00,00,00,00,05,20,00,00,00,\
        20,02,00,00,00,00,14,00,09,00,12,00,01,01,00,00,\
        00,00,00,01,00,00,00,00,00,00,14,00,09,00,12,00,\
        01,01,00,00,00,00,00,05,12,00,00,00

;
; Localizable Strings
;
```

Finally, modify the following strings:

```
[Strings]
VNDR              = "Your Company Name Here"
std_mfg           = "(Standard system devices)"
mydsm_devicedesc = "<Your product> Multi-Path Device Specific Module"
```
The following string is displayed as the friendly name of your DSM:
```
mydsm_desc       = "<Your product name> Multi-Path DSM"

;
; Handy macro substitutions (non-localizable)
;

SERVICE_KERNEL_DRIVER  = 1

SERVICE_BOOT_START     = 0
SERVICE_SYSTEM_START   = 1
SERVICE_DEMAND_START   = 3

SERVICE_ERROR_IGNORE   = 0
SERVICE_ERROR_NORMAL   = 1
SERVICE_ERROR_CRITICAL = 3

SPSVCINST_ASSOCSERVICE = 2

REG_MULTI_SZ           = 0x00010000
REG_MULTI_SZ_APPEND    = 0x00010008
REG_EXPAND_SZ          = 0x00020000
REG_DWORD              = 0x00010001
REG_BINARY_NOCLOBBER   = 0x00030003
```
You should be aware of the following when you install the MPIO DSM sample:

1.  The install sample assumes that all necessary files have already been copied over to a vendor specific directory (preferably a folder under Program Files) and takes that path as one of the parameters. This eliminates requests for the original media when new devices appear.

2.  As the port filter needs to go on top of every adapter that hosts (or might host) a path to the disk, all SCSI adapters are restarted at the end of the install

It is expected that the adapter that hosts the system volumes (boot/paging) will not restart, but that should not be problem if you are not multipathing the boot volume. However, if you are multipathing the boot volume, you will need to restart the system.

**Note** Other filter drivers installed as port filters may interfere with the proper operation of the MPIO port filter. Microsoft does not recommend the use of such filter drivers which may be supplied by HBA miniport vendors.

**Note** Since your DSM binary is not signed, you will get Unsigned Driver Pop-Ups. Ignore these and accept the installation of the new driver. Once your package has been successfully qualified by WHQL, your binaries will get signed and your customers will not get unsigned driver popups.

