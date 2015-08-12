Super Floppy (sfloppy) Storage Class Driver
===========================================

The sfloppy sample is a super floppy driver. This driver is a class driver for Super Floppy disk drives.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Installation and Operation
--------------------------

This sample sits a level above the port driver (ATAPI, USB, and so on) in the driver stack and controls communication between the application level and the port driver. The floppy driver takes requests from file system drivers and then sends the appropriate [**SCSI\_REQUEST\_BLOCK**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff565393) (SRB) to the port driver.

For more information, see [Introduction to Storage Class Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff559215) in the storage technologies design guide.

