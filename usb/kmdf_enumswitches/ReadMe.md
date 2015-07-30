Sample KMDF Bus Driver for OSR USB-FX2
======================================

The kmdf\_enumswitches sample demonstrates how to use Kernel-Mode Driver Framework (KMDF) as a bus driver using the OSR USB-FX2 device.

This sample is written for the OSR USB-FX2 Learning Kit. The specification for the device is at <http://www.osronline.com/hardware/OSRFX2_32.pdf>.

Testing the Device
------------------

To test the device, follow these steps:

1.  If you test signed your driver package, you must enable installation of test signed drivers on the target machine. To do so, either press F8 as the target machine comes up from a reboot, or specify **Bcdedit.exe -set TESTSIGNING ON** and reboot. If you use F8, the change only applies until the next reboot.
2.  Plug in the OSR USB-FX-2 Learning Kit (must be version 2.00 or later).
3.  In Device Manager, select **Update Driver Software**, **Browse my computer for driver software**, **Let me pick from a list of device drivers on my computer**, **Have Disk**. Navigate to the directory that contains your driver package and select the INF file.
4.  After the driver installs, verify that the device appears under the **Sample Device** node in Device Manager.
5.  Flip the switches on the OSR USB-FX-2 hardware board and watch the raw PDO entries appear and disappear under **Sample Device** in Device Manager.
6.  Right-click a raw PDO entry, select **Properties**, and then click the **Events** tab. Under **Information**, examine the hardware ID for the PDO. It should be something like this:

    ```
    6FDE7521-1B65-48ae-B628-80BE62016026}\OsrUsbFxRawPdo\6&227995e2&0&08
    ```

    The last digit matches the number of the switch that you toggled.

Hardware Overview
-----------------

Here is the overview of the device:

-   Device is based on the development board supplied with the Cypress EZ-USB FX2 Development Kit (CY3681).
-   Contains 1 interface and 3 endpoints (Interrupt IN, Bulk Out, Bulk IN).
-   Firmware supports vendor commands to query or set LED Bar graph display, 7-segment LED display and query toggle switch states.
-   Interrupt Endpoint:
    -   Sends an 8-bit value that represents the state of the switches.
    -   Sent on startup, resume from suspend, and whenever the switch pack setting changes.
    -   Firmware does not de-bounce the switch pack.
    -   One switch change can result in multiple bytes being sent.
    -   Bits are in the reverse order of the labels on the pack

        E.g. bit 0x80 is labeled 1 on the pack

-   Bulk Endpoints are configured for loopback:
    -   Device moves data from IN endpoint to OUT endpoint.
    -   Device does not change the values of the data it receives nor does it internally create any data.
    -   Endpoints are always double buffered.
    -   Maximum packet size depends on speed (64 Full speed, 512 High speed).
