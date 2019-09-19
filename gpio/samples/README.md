---
page_type: sample
description: "Illustrates how to write a GPIO controller driver that works in conjunction with the GPIO framework extension (GpioClx)."
languages:
- cpp
products:
- windows
- windows-wdk
---

# GPIO Sample Drivers

The GPIO samples contain annotated code to illustrate how to write a [GPIO controller driver](https://docs.microsoft.com/windows-hardware/drivers/gpio/index) that works in conjunction with the [GPIO framework extension](https://docs.microsoft.com/windows-hardware/drivers/gpio/gpio-driver-support-overview) (GpioClx) to handle GPIO I/O control requests, and a peripheral driver that runs in kernel mode and uses GPIO resources.

The GPIO sample set contains the following samples:

| Minifilter | Sample description |
| --- | --- |
| SimGpio | The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx device driver interface (DDI). The GPIO controller driver is written for a hypothetical memory-mapped GPIO controller (simgpio). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware. |
| SimGpio_I2C | The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx DDI. In contrast to the SimGpio sample, the GPIO controller in this sample is not memory-mapped. The GPIO controller driver is written for a hypothetical GPIO controller that resides on an I2C bus (simgpio_i2c). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware. |
| SimDevice\kmdf | The purpose of this sample is to show how a driver opens a device and performs I/O operations on a GPIO controller in kernel mode. Additionally, this sample demonstrates how the driver connects to a GPIO interrupt resource. The ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware. |
| SimDevice\umdf | The purpose of this sample is to show how a driver opens a device and performs I/O operations on a GPIO controller in user mode. Additionally, this sample demonstrates how the driver connects to a GPIO interrupt resource. The ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware. |
