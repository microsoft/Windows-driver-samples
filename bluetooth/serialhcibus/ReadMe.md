Bluetooth Serial HCI Bus Driver
===============================

The purpose of this sample is to demonstrate how to implement a basic bus driver to support the new [Bluetooth Extensibility transport DDIs](http://msdn.microsoft.com/en-us/library/windows/hardware/ff536585) over the UART transport. Such a serial bus driver can support a multi-radio device over the UART transport and utilize a common Bluetooth HCI packet for communication. The lower edge of this driver interfaces with a UART controller following the Bluetooth SIG's UART (H4) transport protocol.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

**Note** This sample driver is generic. It is not designed for a specific device and allows for a vendor to adopt and enhance it for supporting Bluetooth.

This sample driver, as is, may not properly function for a device until all vendor-specific device requirements (for example, device initialization) have been incorporated.

It is recommended to use the WDK version  that matches the target Windows build version or newer for the development of the serial bus driver.

**FILE MANIFEST**

**WDK header file**

BthXDDI.h - this has the constants, struct, and IOCTL definitions for the Bluetooth extensibility transport. This header file is included in WDK.

**Common code section**

driver.c - driver initialization

driver.h - common header file for driver.c and includes other header files

Fdo.c - functions for function device object (FDO) and BTHX DDI processing

io.c - functions that perform IO read pump via UART controller

Io.h - header for io.c

pdo.c - PDO (Bluetooth function) enumeration and IOCTL processing

public.h - header to share with application to support Radio On/Off ("Airplane mode")

Note: The goal is to keep the common code section the same, so the vendor will only need to update those code sections in the device specific directory.

**Device-specific code section**

Debugdef.h - WPP trace GUID; user should use a new GUID (unique per driver)

device.c - device specific functions to implement:

--DeviceInitialize() - to perform UART and Bluetooth device initialization;

--DeviceEnable() - (optional) to bring serial bus device out of disable/reset state.

--DevicePowerOn() - (optional) to power on the device.

--DeviceEnableWakeControl() - (optional) to arm for device wake signal

--DeviceDisableWakeControl() - (optional) to disarm for device wake signal

device.h - header file for device.c

driver.rc - driver version and name

SerialBusWdk.inx - device specific INF file to install this driver. The vendor will need to add the hardware ID to match the "\_HID" for the Serial Bus Device (Bluetooth) in the DSDT.asl file. For example, in SerialBusWDK.inx, the hardware ID is "ACPI\\<*vendor*\>\_BTH0" where <*vendor*\> could be a 4 digit vendor name
