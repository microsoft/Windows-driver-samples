---
page_type: sample
urlFragment: virtual-serial-driver-sample-v2
description: "Demonstrates UMDF version 2 serial drivers and includes a simple virtual serial driver (ComPort) and a controller-less modem driver (FakeModem)."
languages:
- c
products:
- windows
- windows-wdk
---

# Virtual serial driver sample (V2)

This sample demonstrates these two serial drivers:

- A simple virtual serial driver (ComPort)

- A controller-less modem driver (FakeModem).This driver supports sending and receiving AT commands using the ReadFile and WriteFile calls or via a TAPI interface using an application such as, HyperTerminal.

This sample driver is a minimal driver meant to demonstrate the usage of the User-Mode Driver Framework. It is not intended for use in a production environment.

For more information, see the [Serial Controller Driver Design Guide](https://docs.microsoft.com/windows-hardware/drivers/serports/).

## Code tour

### internal.h

- This is the main header file for the sample driver.

### driver.c and driver.h

- Definition and implementation of the driver callback function (EVT_WDF_DRIVER_DEVICE_ADD) for the sample. This includes **DriverEntry** and events on the framework driver object.

### device.c and driver.h

- Definition and implementation of the device callback interface for the sample. This includes events on the framework device object.

### queue.c and queue.h

- Definition and implementation of the base queue callback interface. This includes events on the framework I/O queue object.

### ringbuffer.c and ringbuffer.h

- Definition and implement of ring buffer for pending data.

### VirtualSerial.rc /FakeModem.rc

- This file defines resource information for the sample driver.

### VirtualSerial.inf / FakeModem.inf

- INF file that contains installation information for this driver.
