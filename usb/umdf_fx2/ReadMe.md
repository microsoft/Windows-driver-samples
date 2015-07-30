Sample UMDF Function Driver for OSR USB-FX2 (UMDF Version 1)
============================================================

The umdf\_fx2 sample is a User-Mode Driver Framework (UMDF) driver for the OSR USB-FX2 device. It includes a test app and sample device metadata, and supports impersonation and idle power down.

The sample can also be used with the CustomDeviceAccess MSDK sample. The sample demonstrates how to perform bulk and interrupt data transfers to an USB device. The specification for the device is at <http://www.osronline.com/hardware/OSRFX2_32.pdf>. The driver and sample device metadata also work with the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample.

The osrusbfx2 sample is divided into three samples:

-   **WDF Sample Driver Learning Lab for OSR USB-FX2**: This sample is a series of iterative drivers that demonstrate how to write a "Hello World" driver and adds additional features in each step.

-   **kmdf\_fx2**: This sample is the final version of kernel-mode **wdf\_osrfx2** driver. The sample demonstrates KMDF methods.

-   **umdf\_fx2**: This sample is the final version of the user-mode driver **wdf\_osrfx2**. The sample demonstrates UMDF methods.

Build the sample
----------------

The default Solution build configuration is Debug and Win32. 

**To select a configuration and build a driver**

1.  Open the driver project or solution in Visual Studio 2015 (find *filtername*.sln or *filtername*.vcxproj).
2.  Right-click the solution in the **Solutions Explorer** and select **Configuration Manager**.
3.  From the **Configuration Manager**, select the **Active Solution Configuration** and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.
4.  From the **Build** menu, click **Build Solution** (Ctrl+Shift+B).

Overview
--------

Here is the overview of the device:

-   The device is based on the development board supplied with the Cypress EZ-USB FX2 Development Kit (CY3681).
-   It contains 1 interface and 3 endpoints (Interrupt IN, Bulk Out, Bulk IN).
-   Firmware supports vendor commands to query or set LED Bar graph display and 7-segment LED display, and to query toggle switch states.
-   Interrupt Endpoint:
    -   Sends an 8-bit value that represents the state of the switches.
    -   Sent on startup, resume from suspend, and whenever the switch pack setting changes.
    -   Firmware does not de-bounce the switch pack.
    -   One switch change can result in multiple bytes being sent.
    -   Bits are in the reverse order of the labels on the pack (for example, bit 0x80 is labeled 1 on the pack).
-   Bulk Endpoints are configured for loopback:
    -   The device moves data from IN endpoint to OUT endpoint.
    -   The device does not change the values of the data it receives nor does it internally create any data.
    -   Endpoints are always double buffered.
    -   Maximum packet size depends on speed (64 full speed, 512 high speed).

Testing the driver
------------------

You can use the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample to test the umdf\_fx2 sample.

This sample also includes a test application, osrusbfx2.exe, that you can use to test the device. This console application enumerates the interface registered by the driver and opens the device to send read, write, or IOCTL requests based on the command line options.

Usage for Read/Write test:

-   -r [*n*], where *n* is number of bytes to read.
-   -w [*n*], where *n* is number of bytes to write.
-   -c [*n*], where *n* is number of iterations (default = 1).
-   -v, shows verbose read data.
-   -p, plays with Bar Display, Dip Switch, 7-Segment Display.
-   -a, performs asynchronous I/O operation.
-   -u, dumps USB configuration and pipe information.
-   -f \<*filename*\> [*interval-seconds*], where *interval-seconds* is a delay in milliseconds, to send a text file to the seven-segment display (UMDF only)

**Playing with the 7 segment display, toggle switches, and bar graph display**

Use the command **osrusbfx2.exe -p** with options 1 through 9 to set and clear bar graph display, set and get 7 segment state, and read the toggle switch states. The following shows the function options:

1.  Light Bar
2.  Clear Bar
3.  Light entire Bar graph
4.  Clear entire Bar graph
5.  Get bar graph state
6.  Get Switch state
7.  Get Switch Interrupt Message
8.  Get 7 segment state
9.  Set 7 segment state
10. Reset the device
11. Re-enumerate the device

0. Exit

Selection:

**Reset and re-enumerate the device**

Use the command **osrusbfx2.exe -p** with options 10 and 11 to either reset the device or re-enumerate the device.

**Read and write to bulk endpoints**

The following commands send read and write requests to the device's bulk endpoint.

-   `osrusbfx2.exe -r 64`

    The preceding command reads 64 bytes to the bulk IN endpoint.

-   `osrusbfx2.exe -w 64 `

    The preceding command writes 64 bytes to the bulk OUT endpoint.

-   `osrusbfx2.exe -r 64 -w 64 -c 100 -v`

    The preceding command first writes 64 bytes of data to bulk OUT endpoint (Pipe 1), then reads 64 bytes from bulk IN endpoint (Pipe 2), and then compares the read buffer with write buffer to see if they match. If the buffer contents match, it repeats this operation 100 times.

-   `osrusbfx2.exe -a`

    The preceding command reads and writes to the device asynchronously in an infinite loop.

The bulk endpoints are double buffered. Depending on the operational speed (full or high), the buffer size is either 64 bytes or 512 bytes, respectively. A request to read data does not complete if the buffers are empty. If the buffers are full, a request to write data does not complete until the buffers are emptied. When you are doing a synchronous read, make sure the endpoint buffer has data (for example, when you send 512 bytes write request to the device operating in full speed mode). Because the endpoints are double buffered, the total buffer capacity is 256 bytes. The first 256 bytes fills the buffer and the write request waits in the USB stack until the buffers are emptied. If you run another instance of the application to read 512 bytes of data, both write and read requests complete successfully.

**Displaying descriptors**

The following command displays all the descriptors and endpoint information.

**osrusbfx2.exe -u**

If the device is operating in high speed mode, you get the following information:

`===================`

`USB_CONFIGURATION_DESCRIPTOR`

`bLength = 0x9, decimal 9`

`bDescriptorType = 0x2 ( USB_CONFIGURATION_DESCRIPTOR_TYPE )`

`wTotalLength = 0x27, decimal 39`

`bNumInterfaces = 0x1, decimal 1`

`bConfigurationValue = 0x1, decimal 1`

`iConfiguration = 0x4, decimal 4`

`bmAttributes = 0xa0 ( USB_CONFIG_BUS_POWERED )`

`MaxPower = 0x32, decimal 50`

`-----------------------------`

`USB_INTERFACE_DESCRIPTOR #0`

`bLength = 0x9`

`bDescriptorType = 0x4 ( USB_INTERFACE_DESCRIPTOR_TYPE )`

`bInterfaceNumber = 0x0`

`bAlternateSetting = 0x0`

`bNumEndpoints = 0x3`

`bInterfaceClass = 0xff`

`bInterfaceSubClass = 0x0`

`bInterfaceProtocol = 0x0`

`bInterface = 0x0`

`------------------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe00`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x81 ( INPUT )`

`bmAttributes= 0x3 ( USB_ENDPOINT_TYPE_INTERRUPT )`

`wMaxPacketSize= 0x49, decimal 73`

`bInterval = 0x1, decimal 1`

`------------------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe01`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x6 ( OUTPUT )`

`bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )`

`wMaxPacketSize= 0x200, `

`decimal 512 bInterval = 0x0, `

`decimal 0`

`------------------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe02`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x88 ( INPUT )`

`bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )`

`wMaxPacketSize= 0x200, decimal 512`

`bInterval = 0x0, decimal 0`

If the device is operating in low speed mode, you will get the following information:

`===================`

`USB_CONFIGURATION_DESCRIPTOR`

`bLength = 0x9, decimal 9`

`bDescriptorType = 0x2 ( USB_CONFIGURATION_DESCRIPTOR_TYPE )`

`wTotalLength = 0x27, decimal 39`

`bNumInterfaces = 0x1, decimal 1`

`bConfigurationValue = 0x1, decimal 1`

`iConfiguration = 0x3, decimal 3`

`bmAttributes = 0xa0 ( USB_CONFIG_BUS_POWERED )`

`MaxPower = 0x32, decimal 50 `

`-----------------------------`

`USB_INTERFACE_DESCRIPTOR #0`

`bLength = 0x9`

`bDescriptorType = 0x4 ( USB_INTERFACE_DESCRIPTOR_TYPE )`

`bInterfaceNumber = 0x0 bAlternateSetting = 0x0`

`bNumEndpoints = 0x3`

`bInterfaceClass = 0xff`

`bInterfaceSubClass = 0x0`

`bInterfaceProtocol = 0x0`

`bInterface = 0x0`

`------------------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe00`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x81 ( INPUT )`

`bmAttributes= 0x3 ( USB_ENDPOINT_TYPE_INTERRUPT )`

`wMaxPacketSize= 0x49, decimal 73`

`bInterval = 0x1, decimal 1`

`-------  -----------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe01`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x6 ( OUTPUT )`

`bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )`

`wMaxPacketSize= 0x40, decimal 64`

`bInterval = 0x0, decimal 0`

`------------------------------`

`USB_ENDPOINT_DESCRIPTOR for Pipe02`

`bLength = 0x7`

`bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )`

`bEndpointAddress= 0x88 ( INPUT )`

`bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )`

`wMaxPacketSize= 0x40, decimal 64`

`bInterval = 0x0, decimal 0 `

Sample Contents
---------------

Folder

Description

usb\\umdf\_fx2\\driver

This directory contains driver code that demonstrates the following functionality:

-   Loads the driver and responds to PnP and Power events. You can install, uninstall, disable, enable, suspend, and resume the system.
-   Registers a PnP device interface so that application can open a handle to the device.
-   Implements **IPnpCallbackHardware** interface and initializes USB I/O targets in **IPnpCallbackHardware::OnPrepareHardware** method.
-   Creates a sequential queue for handling IOCTL requests.
-   Adds code to handle the IOCTL to set bar graph display.
-   Creates a parallel queue for handling read and write requests.
-   Retrieves memory from read and write requests, format the requests, and sends them to a USB target.
-   Supports additional IOCTLs to get and set the 7-segment display, get bar graph display, and get config descriptor.
-   Sets power policy for the device.
-   Adds code to indicate that the device is ready by lighting up the period on 7-segment display.
-   Calls **SetupDi** functions to determine the "BusTypeGUID" of the device, and uses impersonation to access resources that only the caller has access to.
-   Shows how to implement idle and wake functionality to make the driver the power policy owner (PPO). The sample achieves this using power-managed queues and UMDF DDIs, AssignS0IdleSettings, and AssignSxWakeSettings.
-   Demonstrates implementation of a continuous reader.
-   Demonstrates the use of impersonation.

usb\\umdf\_fx2\\exe

This directory contains a test application that can be used to drive the UMDF driver and FX2 device. This is a modified version of the test application for the KMDF Fx2 driver.

usb\\umdf\_fx2\\deviceMetadata

This directory contains the device metadata package for the sample. You must copy the device metadata to the system before installing the device. For information on how to update and deploy device metadata, see [Custom driver access sample](http://go.microsoft.com/fwlink/p/?LinkID=248288).

