WMI ACPI Sample
===============

The WMIACPI sample contains ACPI BIOS and Microsoft Windows Management Instrumentation (WMI) sample code that enables instrumentation of the ACPI BIOS from within ACPI Source Language (ASL) code. ASL code can expose data blocks, methods, and events through WMI by leveraging the ACPI-WMI mapping driver (Wmiacpi.sys).

Operation
---------

The WMIACPI sample contains files which allow an ACPI BIOS developer to add instrumentation from within ASL code. ASL code can expose data blocks, methods, and events through WMI by leveraging the Wmiacpi.sys driver. For more information about the mechanics of writing ASL to expose instrumentation, see the *Windows Instrumentation: WMI and ACPI* white paper included in this sample and available on the Windows Hardware Developer Central (WHDC) Web site.

The following table lists the files included in the sample and their function:

File

Description

Device.asl

ASL code that can be included in the ACPI bios that exposes a set of packages, strings, data, methods and events.

Acpimof.mof

Managed object format (MOF) file that contains a description of the data blocks, methods, and events that are exposed. This description is required so that WMI can access the data blocks, methods, and events.

Acpimof.rc

Acpimof.def

Files that are required to build Acpimof.dll, which is a resource-only DLL.

Wmi-Acpi.htm

The *Windows Instrumentation: WMI and ACPI* whitepaper.

acpimov.vcxproj

Visual Studio project file for the sample.

acpimof.sln

Visual Studio solution file for the sample.

Installation
------------

To add the sample code to your ACPI bios and access through WMI:

1.  Include the contents of *Device.asl* to your ASL source and rebuild the DSDT. Update the operating system with the new DSDT through reflashing.
2.  Build *Acpimof.dll* in the WMIACPI directory. *Acpimof.dll* is a resource-only DLL that contains the compiled MOF in a form that WMI can import into its schema.
3.  Copy *Acpimof.dll* to %windir%\\system32 and add a value named "MofImagePath" under the HKEY\_LOCAL\_MACHINE\\CurrentControlSet\\Services\\WmiAcpi key. The contents of the value should be a path to the *Acpimof.dll* file.
4.  Restart your computer. When Plug and Play (PnP) recognizes the new device with a pnpid of pnp0c14, it will install *Wmiacpi.sys* automatically and make the MOF resource in Acpimof.dll available to the WMI schema.

Note that you do not need an INF file because Windows supplies an INF for the ACPI-WMI mapping driver device as part of the operating system.

