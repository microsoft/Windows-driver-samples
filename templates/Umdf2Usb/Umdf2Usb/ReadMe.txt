========================================================================
    Umdf2Usb Project Overview
========================================================================

This file contains a summary of what you will find in each of the files that make up your project.
The project supports UMDF 2.15 on Windows 10.

Umdf2Usb.vcxproj
    This is the main project file for projects generated using an Application Wizard. 
    It contains information about the version of the product that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

Umdf2Usb.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

Umdf2Usb.inf
    Text file used for driver installation. After creating this driver using the template, the user
    *must* update the hardware ID from "USB\VID_vvvv&PID_pppp" to the hardware ID of the USB device
    that the driver should be installed on.

Public.h
    Header file to be shared with applications. 
 
Driver.c & Driver.h
    DriverEntry and WDFDRIVER related functionality and callbacks.
 
Device.cpp & Device.h
    WDFDEVICE and WDFUSBDEVICE related functionality and callbacks.

IoQueue.cpp & IoQueue.h
    WDFQUEUE related functionality and callbacks.

Trace.h
    Definitions for WPP tracing.