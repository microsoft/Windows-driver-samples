ClassPnP Storage Class Driver Library
=====================================

This library is the library for all storage drivers. It simplifies writing a storage class driver by implementing 90 percent of the code that you need to support Plug and Play (PnP), power management, and so on. This library is used by disk, CDROM, and the tape class drivers.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Installation and Operation
--------------------------

The storage class drivers are used to interact with mass storage devices along with appropriate port driver. The class drivers are layered above the port drivers and manage mass storage devices of a specific class, regardless of their bus type. The classpnp sample contains the common routines that are required for all storage class drivers such as PnP and power management. It also provides I/O and error handling support.

For more information, see [Introduction to Storage Class Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff559215) in the storage technologies design guide.

