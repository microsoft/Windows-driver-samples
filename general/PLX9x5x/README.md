PLX9x5x PCI Driver
==================

This sample demonstrates how to write driver for a generic PCI device using Windows Driver Framework. The target hardware for this driver is PLX9656/9653RDK-LITE board. The product kit and the hardware specification are available at <http://www.plxtech.com>.

For more information, see [Peripheral Component Interconnect (PCI) Bus Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537451).

The device is a PCI device with port, memory, interrupt and DMA resources. Device can be stopped and started at run-time and also supports low power states. The driver is capable of doing concurrent read and write operations to the device but it can handle only one read or write request at any time. The following lists the driver framework interfaces demonstrated in this sample:

-   Handling PnP & Power Events
-   Registering a Device Interface
-   Hardware resource mapping: Port, Memory & Interrupt
-   DMA Interfaces
-   Serialized Default Queue for Write requests
-   Serialized custom Queue for Read requests
-   Handling Interrupt & DPC

To test the driver, run the PLX.EXE test application.

This sample driver is a minimal driver meant to demonstrate the usage of the Windows Driver Framework. It is not intended for use in a production environment.



