---
page_type: sample
description: "A minifilter that demonstrates registration with the filter manager."
languages:
- cpp
products:
- windows
- windows-wdk
---


<!---
    name: NullFilter File System Minifilter Driver
    platform: WDM
    language: cpp
    category: FileSystem
    description: A minifilter that demonstrates registration with the filter manager.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=617653
--->

# NullFilter File System Minifilter Driver

The NullFilter minifilter is a sample minifilter that shows how to register a minifilter with the filter manager.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## Design and Operation

The *NullFilter* minifilter is a simple minifilter that registers itself with the filter manager for no callback operations.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.