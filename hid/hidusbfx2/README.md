HIDUSBFX2 sample driver
=======================
The HIDUSBFX2 sample driver (hidusbfx2.sys) demonstrates how to map a non-HID USB device to a HID device.

The sample also demonstrates how to write a HID minidriver using Windows Driver Frameworks (WDF). The minidriver is written for the [OSR USB-FX2 Learning Kit](http://www.osronline.com/hardware/OSRFX2_32.pdf). Although the device is not HID-compliant, the sample exposes it as a HID device.

Related topics
--------------

[Human Input Devices Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff539952)
[Human Input Devices Reference](http://msdn.microsoft.com/en-us/library/windows/hardware/ff539956)

Related technologies
--------------------

[Creating Framework-based HID Minidrivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540774) 
[Creating UMDF-based HID Minidrivers](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439579)

Build the sample
----------------

For information on how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

Theory of Operation
-------------------

A HID USB device provides a HID descriptor (through an interface descriptor) that identifies the device as HID-compliant and enables the system-supplied HID minidriver (**hidusb.sys**) and the HID class driver to load, parse the HID descriptor, and enumerate child HID device stacks. The system provides strong support for HID devices, so you do not typically have to write a HID minidriver. However, there are cases in which you might need to write your own HID minidriver (for example, if it is difficult to make desired changes to HID-compliant device firmware or if you need to make a non-HID compliant device into a HID device without updating the firmware).

**Overview of the Device**

You can view the specification for the device in the [Using the OSR USB FX-2 Learning Kit](http://go.microsoft.com/fwlink/p/?linkid=64091) document.

The device is loosely based on the development board that is supplied with the Cypress EZ-USB FX2 Development Kit (CY3681) and contains one interface and three endpoints (Interrupt IN, Bulk Out, and Bulk IN). The firmware supports vendor commands to query or set the LED bar graph display and 7-segment LED display, and to query toggle switch states.

The interrupt endpoint sends an 8-bit value that represents the state of the switches. This value is sent on startup, resume from suspend, and whenever the switch pack setting changes. The firmware does not de-bounce the switch pack. One switch change can cause multiple bytes to be sent. The bits are in the reverse order of the labels on the pack (for example, bit 0x80 is labeled 1 on the pack).

Bulk endpoints are configured for loopback.

**Overview of the Driver Stack**

Kernel-Mode Driver Framework (KMDF) does not support HID minidrivers natively because the HID architecture requires that the HID class driver (**hidclass.sys**) own the driver dispatch table for HID minidrivers. This requirement conflicts with the KMDF requirement that it own the driver dispatch table in order to handle Plug and Play (PnP), power, and I/O requests correctly.

You can resolve this ownership conflict by using a driver stack that consists of a minimal WDM driver as a function driver and a complete KMDF driver as a lower filter driver. The function driver registers with the HID class (so**hidclass.sys** owns its dispatch table) and forwards all of the requests to the lower filter driver. The lower filter driver (KMDF owns the dispatch table) processes all of the requests.

The minimal function driver code is located in the \\hidusbfx2\\hidkmdf directory (the driver binary is named **hidkmdf.sys**), and the lower filter driver code is located in the \\hidusbfx2\\sys folder (the binary is named **hidusbfx2.sys**). The function driver is a minimal WDM driver and you can reuse it without any modification. Remember to rename the driver binary when you reuse it, to avoid a name conflict. You need to modify the KMDF filter driver according to your device's requirements.

**Mapping a Non-HID USB Device to HID**

When the HIDclass driver queries the minidriver, the minidriver returns a hard-coded report descriptor that enables the HID class driver to create child devices as described by the report descriptor. The report descriptor has three top-level application collections:

-   Consumer control

-   System control

-   Vendor-defined

The HID class driver creates a driver stack for each top-level collection. The operating system opens the consumer control and system control collections. These collections have input buttons and obtain data from the interrupt endpoint of the USB device. The vendor-defined collection exposes a feature button to control the 7-segment display and bar graph display. Any client application can open the vendor-defined collection to send feature requests.

**Switch Pack Mapping**

The switch pack on the USB device is mapped as hot keys that are commonly found on modern keyboards. This mapping is possible by exposing the switch pack as two system-supported collections: consumer control and system control. The consumer control collection provides a mapping for some application-launch and application-action keys, as shown in the following table. The system control collection provides a mapping for the power sleep function.

Switch | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 
-------|---|---|---|---|---|---|---|---
Mapping | Sleep | Calculator | Mail | Favorites | Refresh | Forward | Back | Browser

**Segment Display and Bar Graph**

The segment display and bar graph are mapped as HID feature controls that you can manipulate by using the **HidD\_SetFeature** function from a user-mode application. The feature controls are mapped as vendor-defined usage page 0xff00. The SEVEN\_SEGMENT\_REPORT\_ID and BARGRAPH\_REPORT\_ID usages are listed in the following tables. You can also use **Hidclient.exe**, an application that is available in the Windows Driver Kit (WDK), to manipulate the segment display and bar graph. For more information about this mapping, see the following two tables.

**Segment Display Mapping**

Usage ID | 0xD7 | 0x06 | 0xB3 | 0xA7 | 0x66 | 0xE5 | 0xF4 | 0x07 | 0xF7 | 0x67
---------|------|------|------|------|------|------|------|------|------|------
Mapping | Display 0 | Display 1 | Display 2 | Display 3 | Display 4 | Display 5 | Display 6 | Display 7 | Display 8 | Display 9

**Bar Graph Mapping**

Note that you can OR these values to light multiple LEDs.

Usage ID | 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40 | 0x80 | 0xFF | 0x00
---------|------|------|------|------|------|------|------|------|------|------
Mapping | LED 1 ON | LED 2 ON | LED 3 ON | LED 4 ON | LED 5 ON | LED 6 ON | LED 7 ON | LED 8 ON | All LEDS ON | All LEDS OFF

**Support for Selective Suspend**

The HID class driver provides support for selective suspend. The minidriver participates in this feature by handling HID class IOCTLs appropriately. To enable the selective suspend feature for your device, you need to add a "SelectiveSuspend" = 1 value in the registry in the device hardware key through the INF file. For an example, see the**hidusbfx2.inf** file.

Installing the sample
---------------------

If you adapt this driver for your device, update the INF file to match the hardware ID (VID, PID) and the device description text to match your test board/device.

To start installing the sample, you must:

1.  Build the driver and copy the following files to a folder on your hard drive:

    -   **hidusbfx2.inf**
    -   **hidusbfx2.sys**
    -   **Hidkmdf.sys**
    -   The WDF coinstaller from the *\<WDK ROOT\>\\redist\\wdf\\\<platform\>* directory.

    **Note** You can obtain redistributable framework updates by downloading the **wdfcoinstaller.msi** package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396). This package performs a silent install into the directory of your WDK installation. You will see no confirmation that the installation has completed. You can verify that the redistributables have been installed on top of the WDK by ensuring there is a redist\\wdf directory under the root directory of the WDK, %ProgramFiles(x86)%\\Windows Kits\\8.0.

2.  Plug in the device and follow these steps:
    1.  Launch Device Manager by executing command devmgmt.msc in a command window, or from the **Hardware and Sound** program group in **Control Panel**.
    2.  Select **OSR USB-FX2 device** from **Other Devices** category and select **Update Driver Software...** from the right-click menu.
    3.  Select **Browse my computer for software** and provide the location of the driver files.
    4.  Select **Install this driver software anyway** when the Windows Security dialog box appears.
    5.  After the driver is installed, you should see the device in Device Manager under Human Interface Devices.

Testing
-------

**Testing Switches**

-   To open a Web browser, toggle switch number 8 on the device board to the On position (toggle down).

-   To start the calculator application, toggle switch number 2 on the device board to the On position (toggle down).

**Testing Bar Graph and 7-Segment Display**

1.  Start the **hidclient.exe** GUI application from the WDK. The application source code is located in the \\hid\\hclient directory, and you build it by using the appropriate build environment.

2.  From the **HID Device to examine** menu, select the device that contains "UsagePage 0ff00, Usage 01" as a substring.

3.  Click **Modify Features**. The **Feature Data** dialog box opens.

4.  Click **Modify Features**. The **Modify features** dialog box opens.

5.  In the input box, type **7** and click **Send to Device**. You'll see number 7 appear in the 7-segment display.

6.  Type any number from 1-8, and you'll see the respective number displayed in the 7-segment display.

7.  Type any number from (and including) 9-17, and you will see one of the LEDs on the bar graph turn on. For mapping information, see the previous table.

**Report Descriptor**

```
   // Consumer control collection
 
   0x05,0x0C,  // USAGE_PAGE (Consumer Page)
 
   0x09,0x01,  // USAGE (Consumer Control Usage 0x01)
 
   0xA1,0x01,  // COLLECTION (Application)
 
   0x85,0x01,  //  REPORT_ID 
 
   0x0A, 0x23, 0x02,  //  USAGE (Usage Browser)
 
   0x0A, 0x24, 0x02,  //  USAGE (Usage AC Back)
 
   0x0A, 0x25, 0x02,  //  USAGE (Usage AC Forward)
 
   0x0A, 0x27, 0x02,  //  USAGE (Usage AC Refresh)
 
   0x0A, 0x2A, 0x02,  //  USAGE (Usage AC BookMarks)
 
   0x0A, 0x8A, 0x01,  //  USAGE (Usage AL Mail)
 
   0x0A, 0x92, 0x01,  //  USAGE (Usage AL Calculator )
 
   0x15, 0x00,  //  LOGICAL_MINIMUM(0)
 
   0x25, 0x01,  //  LOGICAL_MAXIMUM(1)  
 
   0x75, 0x01,  //  REPORT_SIZE 
 
   0x95, 0x07,  //  REPORT_COUNT 
 
   0x81, 0x02,  //  INPUT (Data, Variable,Abs)
 
   0x75, 0x01,  //  REPORT_SIZE 
 
   0x95, 0x01,  //  REPORT_COUNT
 
   0x81, 0x07,  //  INPUT (const)
 
   0xC0,  // END_COLLECTION
 
   // system control collection
 
   0x05, 0x01,  // Usage Page (Generic Desktop)
 
   0x09, 0x80,  // Usage (System Control)
 
   0xA1, 0x01,  // Collection (Application)
 
   0x85, 0x02,  //  Report ID 
 
   0x95, 0x07,  //  Report Count 
 
   0x81, 0x07,  //  Input (Constant)  
 
   0x09, 0x82,  //  Usage (System Sleep)
 
   0x95, 0x01,  //  Report Count (2)
 
   0x81, 0x06,  //  Input (Data, Variable, Relative, Preferred)
 
   0xC0,  // End Collection
 
   // Feature collection
 
   0x06,0x00, 0xFF,  // USAGE_PAGE (Vender Defined Usage Page)
 
   0x09,0x01,  // USAGE (Vendor Usage 0x01)
 
   0xA1,0x01,  // COLLECTION (Application)
 
   0x85,0x03,  // Report ID 
 
   0x19,0x01,  //  USAGE MINIMUM 
 
   0x29,0x20,  //  USAGE MAXIMUM 
 
   0x15,0x00,  //  LOGICAL_MINIMUM(1)
 
   0x26,0xff, 0x00,  //  LOGICAL_MAXIMUM(255)
 
   0x75,0x08,  //  REPORT_SIZE 
 
   0x95,0x01,  //  REPORT_COUNT 
 
   0xB1,0x00,  //  Feature (Data,Ary,Abs)
 
   0xC0  // END_COLLECTION </code></pre></td>
```

Code Tour
---------

This section includes a file manifest of the files in the \\hidusbfx2 directory.

### File Manifest

**\\hidusbfx2\\hidkmdf**

File | Description 
-----|------------
hidkmdf.c | Contains code for driver entry and dispatch
Sources | WDK sources file
Makefile | WDK build environment makefile
hidkmdf.rc |  Resource file for the driver
 
**\\hidusbfx2\\sys**

File | Description 
-----|------------
Driver.c | Contains code for driver entry and dispatch functions
hid.c | Contains code for handling HID IOCTLS
usb.c | Contains code for communicating with USB stack
Trace.h | Contains trace-related definitions
Hidusbfx2.h | Contains type definitions and function declarations
Hidusbfx2.rc | Resource file for the driver
Hidusbfx2.inx | INX file for the driver
Sources | WDK sources file
Makefile | WDK build environment make file
Makefile.inc | A makefile that defines custom build actions, including the conversion of the .INX file into a .INF file
 
