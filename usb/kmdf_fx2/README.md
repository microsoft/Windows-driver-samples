Sample KMDF Function Driver for OSR USB-FX2
===========================================

The kmdf\_fx2 sample is a Kernel-Mode Driver Framework (KMDF) driver for the OSR USB-FX2 device. It includes a test app and sample device metadata.

In the Windows Driver Kit (WDK), the osrusbfx2 sample demonstrated how to perform bulk and interrupt data transfers to an USB device. The sample was written for the OSR USB-FX2 Learning Kit.

The specification for the device is at <http://www.osronline.com/hardware/OSRFX2_32.pdf>. The driver and sample device metadata also work with the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Overview
--------

Here is the overview of the device:

-   The device is based on the development board supplied with the Cypress EZ-USB FX2 Development Kit (CY3681).
-   It contains 1 interface and 3 endpoints (Interrupt IN, Bulk Out, Bulk IN).
-   Firmware supports vendor commands to query or set LED Bar graph display and 7-segment LED display, and to query toggle switch states.
-   Interrupt endpoint:
    -   Sends an 8-bit value that represents the state of the switches.
    -   Sent on startup, resume from suspend, and whenever the switch pack setting changes.
    -   Firmware does not de-bounce the switch pack.
    -   One switch change can result in multiple bytes being sent.
    -   Bits are in the reverse order of the labels on the pack (for example, bit 0x80 is labeled 1 on the pack).
-   Bulk endpoints are configured for loopback:
    -   The device moves data from IN endpoint to OUT endpoint.
    -   The device does not change the values of the data it receives nor does it internally create any data.
    -   Endpoints are always double buffered.
    -   Maximum packet size depends on speed (64 full speed, 512 high speed).
-   Event Tracing for Windows (ETW) events:
    -   Included osrusbfx2.man, which describes events added.
    -   Three events are targeted to the event log:
        -   Failure during the add device routine.
        -   Failure to start the OSR device on a USB 1.1 controller.
        -   Invocation of the "re-enumerate device" IOCTL.
    -   Read/write start/stop events can be used to measure the time taken.
    -   For more information, see Unified Tracing later in this document.

Code tour
---------------

**usb\\kmdf\_fx2\\driver**

This directory contains driver code that demonstrates the following functionality:

-   Loads the driver and responds to PnP and Power events. You can install, uninstall, disable, enable, suspend, and resume the system.
-   Creates a context with the WDFDEVICE object.
-   Initializes the USB device by registering a *EvtPrepareHardware* callback.
-   Marks the interface restricted so that it can be accessed by a privileged Windows Store device app.
-   Creates a default parallel queue to receive an IOCTL request to set bar graph display.
-   Retrieves memory handle from the requests and uses it to send a vendor command to the USB device.
-   Registers read and write events on the default queue.
-   Retrieves memory from read and write requests, formats the requests, and sends it to a USB target.
-   Creates two separate sequential queues and configures them to dispatch read and write requests directly. (*\*kmdf\_fx2 only*)
-   Enables wait-wake and selective suspend support. (*\*kmdf\_fx2 only*)
-   Configures a USB target continuous reader to read toggle switch states asynchronously from the interrupt endpoint. (*\*kmdf\_fx2 only*)
-   Supports additional IOCTLs to get and set the 7-segment display and toggle switches, and to reset and re-enumerate the device. (*\*kmdf\_fx2 only*)
-   Creates ETW provider to log two events to the event log, and read/write start stop events. (*\*kmdf\_fx2 only*)
-   WPP tracing.

**usb\\kmdf\_fx2\\exe**

This directory contains a test application that can be used to drive the KMDF driver and FX2 device.

**usb\\kmdf\_fx2\\deviceMetadata**

This directory contains the device metadata package for the sample. You must copy the device metadata to the system before installing the device. For information on how to update and deploy device metadata, see the [Custom driver access sample](http://go.microsoft.com/fwlink/p/?LinkID=248288).

Testing the driver
------------------

You can use the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample as a testing method.

The sample also includes a test application, osrusbfx2.exe, that you can use to test the device. This console application enumerates the interface registered by the driver and opens the device to send read, write, or IOCTL requests based on the command line options.

Usage for Read/Write test:

-   -r [*n*], where *n* is number of bytes to read.
-   -w [*n*], where *n* is number of bytes to write.
-   -c [*n*], where *n* is number of iterations (default = 1).
-   -v, shows verbose read data.
-   -p, plays with Bar Display, Dip Switch, 7-Segment Display.
-   -a, performs asynchronous I/O operation.
-   -u, dumps USB configuration and pipe information.

**Playing with the 7 segment display, toggle switches, and bar graph display**

Use the command, **osrusbfx2.exe -p** options 1-9, to set and clear bar graph display, set and get 7 segment state, and read the toggle switch states. The following shows the function options:

```
1.  Light bar
2.  Clear bar
3.  Light entire bar graph
4.  Clear entire bar graph
5.  Get bar graph state
6.  Get switch state
7.  Get switch interrupt message
8.  Get 7 segment state
9.  Set 7 segment state
10. Reset the device
11. Re-enumerate the device

12. Exit

Selection:
```

**Reset and re-enumerate the device**

Use the command, osrusbfx2.exe -p with option 10 and 11, to either reset the device or re-enumerate the device.

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

The bulk endpoints are double buffered. Depending on the operational speed (full or high), the buffer size is either 64 bytes or 512 bytes, respectively. A request to read data does not complete if the buffers are empty. If the buffers are full, a request to write data does not complete until the buffers are emptied. When you are doing a synchronous read, make sure the endpoint buffer has data (for example, when you send a 512 bytes write request to the device operating in full speed mode). Because the endpoints are double buffered, the total buffer capacity is 256 bytes. The first 256 bytes fills the buffer, and the write request waits in the USB stack until the buffers are emptied. If you run another instance of the application to read 512 bytes of data, both write and read requests complete successfully.

**Displaying descriptors**

The following command displays all the descriptors and endpoint information.

**osrusbfx2.exe -u**

If the device is operating in high speed mode, you will get the following information:

```
===================

USB_CONFIGURATION_DESCRIPTOR

bLength = 0x9, decimal 9

bDescriptorType = 0x2 ( USB_CONFIGURATION_DESCRIPTOR_TYPE )

wTotalLength = 0x27, decimal 39

bNumInterfaces = 0x1, decimal 1

bConfigurationValue = 0x1, decimal 1

iConfiguration = 0x4, decimal 4

bmAttributes = 0xa0 ( USB_CONFIG_BUS_POWERED )

MaxPower = 0x32, decimal 50

-----------------------------

USB_INTERFACE_DESCRIPTOR #0

bLength = 0x9

bDescriptorType = 0x4 ( USB_INTERFACE_DESCRIPTOR_TYPE )

bInterfaceNumber = 0x0

bAlternateSetting = 0x0

bNumEndpoints = 0x3

bInterfaceClass = 0xff

bInterfaceSubClass = 0x0

bInterfaceProtocol = 0x0

bInterface = 0x0

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe00

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x81 ( INPUT )

bmAttributes= 0x3 ( USB_ENDPOINT_TYPE_INTERRUPT )

wMaxPacketSize= 0x49, decimal 73

bInterval = 0x1, decimal 1

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe01

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x6 ( OUTPUT )

bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )

wMaxPacketSize= 0x200, 

decimal 512 bInterval = 0x0, 

decimal 0

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe02

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x88 ( INPUT )

bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )

wMaxPacketSize= 0x200, decimal 512

bInterval = 0x0, decimal 0

If the device is operating in low speed mode, you will get the following information:

===================

USB_CONFIGURATION_DESCRIPTOR

bLength = 0x9, decimal 9

bDescriptorType = 0x2 ( USB_CONFIGURATION_DESCRIPTOR_TYPE )

wTotalLength = 0x27, decimal 39

bNumInterfaces = 0x1, decimal 1

bConfigurationValue = 0x1, decimal 1

iConfiguration = 0x3, decimal 3

bmAttributes = 0xa0 ( USB_CONFIG_BUS_POWERED )

MaxPower = 0x32, decimal 50 

-----------------------------

USB_INTERFACE_DESCRIPTOR #0

bLength = 0x9

bDescriptorType = 0x4 ( USB_INTERFACE_DESCRIPTOR_TYPE )

bInterfaceNumber = 0x0 bAlternateSetting = 0x0

bNumEndpoints = 0x3

bInterfaceClass = 0xff

bInterfaceSubClass = 0x0

bInterfaceProtocol = 0x0

bInterface = 0x0

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe00

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x81 ( INPUT )

bmAttributes= 0x3 ( USB_ENDPOINT_TYPE_INTERRUPT )

wMaxPacketSize= 0x49, decimal 73

bInterval = 0x1, decimal 1

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe01

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x6 ( OUTPUT )

bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )

wMaxPacketSize= 0x40, decimal 64

bInterval = 0x0, decimal 0

------------------------------

USB_ENDPOINT_DESCRIPTOR for Pipe02

bLength = 0x7

bDescriptorType = 0x5 ( USB_ENDPOINT_DESCRIPTOR_TYPE )

bEndpointAddress= 0x88 ( INPUT )

bmAttributes= 0x2 ( USB_ENDPOINT_TYPE_BULK )

wMaxPacketSize= 0x40, decimal 64

bInterval = 0x0, decimal 0
```

Unified tracing
---------------

To view events the provider manifest must be installed. As part of the installation, from an elevated prompt run the following: `wevtutil im osrusbfx2.man`

Registering the manifest sets up the appropriate paths where the system can find information for decoding the events. The OSR event log can be found in the system event viewer under Event Viewer\\Applications and Services Logs\\OSRUSBFx2\\Operational channel eventlog. Triggering a device re-enumeration through osrusbfx2.exe sends an event to this log.

To trace, you can use the in-box tools, logman and tracerpt, or download XPerf (Windows Performance Toolkit) from Microsoft.

**Using in-box tools**

**To start/stop the trace by using logman:**

1.  Start tracing by using the following command:

    `logman start sample -o osrusbfx2.etl -ets -p OSRUSBFX2`

2.  Generate activity through the osrusbfx2 test application, such as `osrusbfx2.exe -a`.
3.  Stop tracing by using the following command:

    `Logman stop sample`

4.  View the trace file using tracerpt:

    `tracerpt -of csv OSRUSBFX2.etl           `

**To start/stop the trace by using Xperf (Windows Performance Toolkit):**

1.  Start tracing by using the following command:

    `xperf -start sample -f osrusbfx2.etl -on OSRUSBFX2`

2.  Generate activity through the osrusbfx2 test application, such as `osrusbfx2.exe -a`.
3.  Stop tracing by using the following command:

    `xperf -stop sample`

4.  View the trace file using Xperf:

    `xperfview OSRUSBFX2.etl`


