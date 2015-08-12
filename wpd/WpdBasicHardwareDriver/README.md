WPD Basic Hardware Sample Driver (UMDF Version 1)
=================================================

The WpdBasicHardwareDriver is a WPD driver that supports nine devices. These devices were selected because of their simplicity. This simplicity allowed the sample to focus on the tasks that are common to portable devices without getting bogged down in hardware complexities.

This sample driver is based on the WpdHelloWorldDriver that is also included in the Windows Driver Kit (WDK). The "Supporting the WPD Infrastructure" sections for this driver show the changes that were made to the WpdHelloWorldDriver source so that it can communicate with basic hardware devices. Before you work through the topics in this section of the documentation, be familiar with the WpdHelloWorldDriver.

The sensor devices that are supported by the WpdBasicHardwareDriver, such as the Memsic 2125 Accelerometer, are sold by the Parallax Corporation in Rocklin, California.

To use these sensors with the WpdBasicHardwareDriver, you must purchase the sensors, a programmable microcontroller (Parallax BS2), a test board (like the Parallax BASIC Stamp Homework Board), an RS232 cable, and miscellaneous parts. All of this hardware is available from Parallax and can be ordered through their Web site.

The circuit designs are based on the sample circuits provided by Parallax in their sensor data sheets. These circuits are designed to integrate each sensor with the Parallax BS2 programmable microcontroller .

The microcontroller firmware for each of the nine circuits is included in the **\\firmware** subdirectory of this sample.

For a complete description of this sample and its underlying code and functionality, refer to the [WPD Basic Hardware Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597697) description in the Windows Driver Kit documentation.


Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/)


Installation
------------

To test this sample, you must have a test computer. This can be a second computer or, if necessary, your development computer.

To install the WpdBasicHardwareDriver sample, do the following:

1.  Copy the driver binary and the wpdbasichardwaredriver.inf file to a directory on your test computer (for example, C:\\wpdbasichardwaredriver.)

2.  Copy the UMDF coinstaller, WUDFUpdate\_*MMmmmm*.dll, from the \\redist\\wdf\\\<architecture\> directory to the same directory (for example, C:\\wpdbasichardwaredriver).

    **Note** You can obtain the co-installers by downloading and installing the "Windows Driver Framework (WDF)" package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396).

3.  Navigate to the directory that contains the INF file and binaries (for example, cd /d c:\\wpdbasichardwaredriver), and run DevCon.exe as follows:
    **devcon.exe install wpdbasichardwaredriver.inf WUDF\\WpdBasicHardware**
    You can find DevCon.exe in the \\tools directory of the WDK (for example, \\tools\\devcon\\i386\\devcon.exe).

