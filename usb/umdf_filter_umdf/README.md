Sample UMDF Filter above UMDF Function Driver for OSR USB-FX2 (UMDF Version 1)
==============================================================================

The umdf\_filter\_umdf sample demonstrates how to load a User-Mode Driver Framework (UMDF) filter driver as an upper filter driver above the umdf\_fx2 sample driver.

This sample is written for the OSR USB-FX2 Learning Kit. The specification for the device is at <http://www.osronline.com/hardware/OSRFX2_32.pdf>.

### Overview


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

### Testing the driver

You can test this sample either by using the [Custom driver access](http://go.microsoft.com/fwlink/p/?LinkID=248288) sample application, or by using the osrusbfx2.exe test application. For information on how to build and use the osrusbfx2.exe application, see the test instructions for the [umdf\_fx2](http://msdn.microsoft.com/en-us/library/windows/hardware/) sample.

### Sample Contents

Folder | Description 
-------|------------
usb\umdf_filter_umdf\umdf_driver | This directory contains source code for the umdf_fx2 sample driver. 
usb\umdf_filter_umdf\umdf_filter | This directory contains the UMDF filter driver. 


