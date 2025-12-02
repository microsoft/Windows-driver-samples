---
page_type: sample
description: "A tool to monitor and log any I/O and transaction activity that occurs in the system."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Minispy File System Minifilter Driver

The Minispy sample is a tool to monitor and log any I/O and transaction activity that occurs in the system. Minispy is implemented as a minifilter.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## Design and Operation

Minispy consists of both user-mode and kernel-mode components. The kernel-mode component registers callback functions that correspond to various I/O and transaction operations with the filter manager. These callback functions help Minispy record any I/O and transaction activity occurring in the system. When a user can request the recorded information, the recorded information is passed to the user-mode component, which can either output it on screen or log it to a file on disk.

To observe I/O activity on a device, you must explicitly attach Minispy to that device by using the Minispy user-mode component. Similarly, you can request Minispy to stop logging data for a particular device.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](https://docs.microsoft.com/windows-hardware/drivers/ifs/file-system-minifilter-drivers) section in the Installable File Systems Design Guide.
