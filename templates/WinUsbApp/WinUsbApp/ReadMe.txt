========================================================================
    WinUsbApp Project Overview
========================================================================

This file contains a summary of what you will find in each of the files that make up your solution.

WinUsbApp.inf
    Text file used for WinUSB driver installation. After creating this driver using the template,
    you *must* update the hardware ID from "USB\VID_vvvv&PID_pppp" to the hardware ID of the USB
    device that the driver should be installed on.

    When the solution is built, a driver package is generated around the INF, separate from the
    compiled application. The driver package must be installed in order for the application to find
    a device.

WinUsbApp.vcxproj
    This is the main project file for projects generated using an Application Wizard. 
    It contains information about the version of the product that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

WinUsbApp.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

pch.h
    Header file for project
 
device.cpp & device.h
    Basic code to find the WinUSB device, if connected. Defines the device interface GUID for the
    application to look for (same value as defined in the INF).

main.cpp
    Simple console application demonstrating finding the device using device.h, and communicating
    with the device.

/////////////////////////////////////////////////////////////////////////////

Learn more about WinUSB here:

http://msdn.microsoft.com/library/windows/hardware/ff540196.aspx
http://msdn.microsoft.com/en-us/windows/hardware/gg487341

/////////////////////////////////////////////////////////////////////////////
