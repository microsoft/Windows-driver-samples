---
page_type: sample
description: "Demonstrates how to write a driver for a generic PCI device using Windows Driver Frameworks (WDF)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# PLX9x5x PCI Driver

This sample demonstrates how to write driver for a generic PCI device using Windows Driver Framework. The target hardware for this driver is PLX9656/9653RDK-LITE board. The product kit and the hardware specification are available at the [PLX Technology](http://www.plxtech.com) website.

For more information, see the [PCI driver programming guide](https://docs.microsoft.com/windows-hardware/drivers/pci/).

The device is a PCI device with port, memory, interrupt and DMA resources. The device can be stopped and started at run-time and also supports low power states. The driver is capable of doing concurrent read and write operations to the device but it can handle only one read or write request at any time. The following driver framework interfaces are demonstrated in this sample:

- Handling PnP and Power Events
- Registering a Device Interface
- Hardware resource mapping: Port, Memory, and Interrupt
- DMA Interfaces
- Serialized Default Queue for Write requests
- Serialized custom Queue for Read requests
- Handling Interrupt and DPC

To test the driver, run the PLX.EXE test application.

This sample driver is a minimal driver meant to demonstrate the usage of the Windows Driver Framework. It is not intended for use in a production environment.
