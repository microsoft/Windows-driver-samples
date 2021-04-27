---
page_type: sample
description: "A sample class driver for Super Floppy disk drives."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Super Floppy (sfloppy) Storage Class Driver

The sfloppy sample is a super floppy driver. This driver is a class driver for Super Floppy disk drives.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## Installation and Operation

This sample sits a level above the port driver (ATAPI, USB, and so on) in the driver stack and controls communication between the application level and the port driver. The floppy driver takes requests from file system drivers and then sends the appropriate [**SCSI\_REQUEST\_BLOCK**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/srb/ns-srb-_scsi_request_block) (SRB) to the port driver.

For more information, see [Introduction to Storage Class Drivers](https://docs.microsoft.com/windows-hardware/drivers/storage/introduction-to-storage-class-drivers) in the storage technologies design guide.
