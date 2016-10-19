# UcmTcpciCx Port Controller Client Driver

This is a skeleton sample driver that shows how to create a Windows USB Type-C port controller driver using the USB Connector Manager Type-C Port Controller Interface class extension driver (UcmTcpciCx). Refer to the UcmTcpciCx documentation for more information.

This sample demonstrates the following:

-   Registration with the UcmTcpci class extension driver.
-   Initializing the port controller's Type-C and Power Delivery capabilities.
-   Initializing the I2C communications channel to the port controller hardware.
-   Performing reads/writes over I2C
-   Handling hardware requests from UcmTcpciCx.
-   Handling alerts from the port controller hardware and notifying UcmTcpciCx of the alert.
-   Power management
-   Platform-level device reset in the case of an unresponsive I2C controller.

## Customizing the sample for your port controller
The sample contains a number of comments prefaced with `// TODO` - review them and modify the code as necessary as you are writing your driver.

## Note regarding Type-C port controller hardware
This sample assumes a device that complies with the USB Type-C Port Controller Interface specification, Revision 1.0 (part of the [USB 3.1 specification download](http://usb.org/developers/docs)). Such a device uses a predefined register layout and an I2C communications channel.
If your port controller hardware is not exactly compliant with the specification, you will need to make additional modifications to the sample.

## Performing read/writes over I2C
The USB Type-C Port Controller Interface specification defines I2C to be the channel by which software communicates with the port controller hardware.
If your port controller hardware is not compliant with the specification and does not use I2C as the communications channel, you will need to make additional modifications to the sample.

## When to write a UcmTcpciCx client driver
UcmTcpciCx is intended for system port controller drivers. If you are bringing up a USB Type-C peripheral, you do not need to write a USB Type-C specific driver; a regular USB client driver will suffice. Refer to [Developing Windows client drivers for USB devices](https://msdn.microsoft.com/en-us/library/windows/hardware/hh406260(v=vs.85).aspx) to determine what type of driver, if any, you need to write to make your USB device work with Windows.