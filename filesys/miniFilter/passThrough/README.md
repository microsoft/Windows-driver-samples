---
page_type: sample
description: "Demonstrates how to specify callback functions for different types of I/O requests."
languages:
- cpp
products:
- windows
- windows-wdk
---

# PassThrough File System Minifilter Driver

The PassThrough minifilter demonstrates how to specify callback functions for different types of I/O requests.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## Design and Operation

The *PassThrough* minifilter does not have any real functionality. For each type of I/O operation, the same pre and post callback functions are called. These callback functions simply forward the I/O request to the next filter on the stack.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](https://docs.microsoft.com/windows-hardware/drivers/ifs/file-system-minifilter-drivers) section in the Installable File Systems Design Guide.
