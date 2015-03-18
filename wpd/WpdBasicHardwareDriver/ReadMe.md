WPD Basic Hardware Sample Driver (UMDF Version 1)
=================================================

The WpdBasicHardwareDriver is a WPD driver that supports nine devices. These devices were selected because of their simplicity. This simplicity allowed the sample to focus on the tasks that are common to portable devices without getting bogged down in hardware complexities.

This sample driver is based on the WpdHelloWorldDriver that is also included in the Windows Driver Kit (WDK). The "Supporting the WPD Infrastructure” sections for this driver show the changes that were made to the WpdHelloWorldDriver source so that it can communicate with basic hardware devices. Before you work through the topics in this section of the documentation, be familiar with the WpdHelloWorldDriver.

The sensor devices that are supported by the WpdBasicHardwareDriver are described in the following table.

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Device
Description</td>
<td align="left">Memsic 2125 Accelerometer
Senses +/- 2g along the X-axis and Y-axis.</td>
</tr>
</tbody>
</table>

These nine sensors are sold by the Parallax Corporation in Rocklin, California.

To use these sensors with the WpdBasicHardwareDriver, you must purchase the sensors, a programmable microcontroller (Parallax BS2), a test board (like the Parallax BASIC Stamp Homework Board), an RS232 cable, and miscellaneous parts. All of this hardware is available from Parallax and can be ordered through their Web site.

The circuit designs are based on the sample circuits provided by Parallax in their sensor data sheets. These circuits are designed to integrate each sensor with the Parallax BS2 programmable microcontroller .

The microcontroller firmware for each of the nine circuits is included in the src\\wpd\\WpdBasicHardwareDriver\\firmware subdirectory in the Windows Driver Kit (WDK).

For a complete description of this sample and its underlying code and functionality, refer to the [WPD Basic Hardware Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597697) description in the Windows Driver Kit documentation.


Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/)
