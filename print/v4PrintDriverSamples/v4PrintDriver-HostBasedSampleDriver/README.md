USB Host-Based Print Driver Sample
==================================

This driver sample demonstrates how to support host-based devices that use the v4 print driver model, and are connected via USB.

**Note** This sample is for the v4 print driver model.

Windows enables manufacturers to support Bidirectional Communication (Bidi) for USB devices, by using a combination of both a Bidi XML file and a Javascript file known as a USB Bidi extender. The *usb\_host\_based\_sample.js* file that is included with the sample, plays the role of the USB Bidi extender.

The USB Bidi extender allows apps to use Bidi with USB as the transport mechanism. The Javascript implementation does not support any device flow control, or any multiplexing of control information with print jobs during printing.

By default, Bidi queries and status requests are routed over the USB device interface that is used for printing.

In addition to extending Bidi communication, this driver sample also specifies the schema elements that it supports. The *usb\_host\_based\_sample\_extension.xml* file that is included with the sample, provides information about the supported schema elements.

The Bidi schema is a hierarchy of printer attributes, some of which are properties and others that are values (or value entries).

*Property*

A property is a node in the schema hierarchy. A property can have one or more children, and these children can be other properties or values.

*Value*

A value is a leaf in the schema hierarchy that represents either a single data item or a list of related data items. A value has a name, a data type, and a data value. A value cannot have child elements.

For more information, see [USB Bidi Extender](http://msdn.microsoft.com/en-us/library/windows/hardware/jj659903(v=vs.85).aspx) and [Bidi Communication Schema](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545169(v=vs.85).aspx).

Here are the core files that you will find in this sample:

File name

Description

usb\_host\_based\_sample.js

A USB Bidi Extension JavaScript file which includes support for controlling printing for host-based devices. This is the only code in the driver sample. It is invoked by USBMon and it communicates with the device to do the following:

-   Determine if the device is ready to receive data
-   Check to see if there is an error condition
-   Read the device status

usb\_host\_based\_sample\_events.xml

A 'driver events' XML file that specifies an event which detects when the user needs to flip over the paper in the tray.

usb\_host\_based\_sample\_extension.xml

A USB Bidi Extension XML file that specifies the supported Bidi Schema elements for this driver.


Build the sample
----------------

For information and instructions about how to test and deploy drivers, see [Developing, Testing, and Deploying Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554651(v=vs.85).aspx).

Run the sample
--------------

To understand how to run this sample as a Windows driver, see the [v4 Printer Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/hh706306(v=vs.85).aspx) collection of topics.

