GPIO Sample Drivers
===================

The GPIO samples contain annotated code to illustrate how to write a [GPIO controller driver](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439509) that works in conjunction with the [GPIO framework extension](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439512) (GpioClx) to handle GPIO I/O control requests, and a peripheral driver that runs in kernel mode and uses GPIO resources. For a sample that shows how to write a GPIO peripheral driver that runs in user mode, please refer to the SPB accelerometer sample driver (SPB\\peripherals\\accelerometer).

The GPIO sample set contains the following samples:

Minifilter | Sample Description
-----------|-------------------
SimGpio | The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx device driver interface (DDI). The GPIO controller driver is written for a hypothetical memory-mapped GPIO controller (simgpio). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.
SimGpio_I2C | The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx DDI. In contrast to the SimGpio sample, the GPIO controller in this sample is not memory-mapped. The GPIO controller driver is written for a hypothetical GPIO controller that resides on an I2C bus (simgpio_i2c). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.
SimDevice | The purpose of this sample is to show how a driver opens a device and performs I/O operations on a GPIO controller in kernel mode. Additionally, this sample demonstrates how the driver connects to a GPIO interrupt resource. The ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.
SimDeviceUmdf | The purpose of this sample is to show how a driver opens a device and performs I/O operations on a GPIO controller with UMDF. Additionally, this sample demonstrates how the driver connects to a GPIO interrupt resource. The ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.


