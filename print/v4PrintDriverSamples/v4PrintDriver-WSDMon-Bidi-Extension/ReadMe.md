WSDMon Bidi Extension Sample
============================

This sample demonstrates how to use an XML extension file to support bidirectional (Bidi) communication with a WSD connected printer.

The v4 print driver model continues to employ the WSDMon Bidi Extension file format, as well as the SNMP Bidi Extension file format.

**Note** Third-party port monitors and language monitors are not supported in the v4 driver model or with print class drivers.

The WSDMON port monitor is a printer port monitor that supports printing to network printers that comply with the Web Services for Devices (WSD) technology. The WSDMON port monitor listens for WSD events and updates the printer status accordingly.

A Bidi schema is a hierarchy of printer attributes, some of which are properties and others that are values (or value entries).

A *property* is a node in the schema hierarchy. A property can have one or more children, and these children can be other properties or values.

A *value* is a leaf in the schema hierarchy that represents either a single data item or a list of related data items. A value has a name, a data type, and a data value. A value cannot have child elements.

The WSDMON port monitor can:

-   Discover network printers and install them.

-   Send jobs to WSD printers.

-   Monitor the status and configuration of the WSD printers and update the printer object status accordingly.

-   Respond to bidirectional (bidi) queries for supported bidi schemas.

-   Monitor bidi Schema value changes and send notifications.

WSDMON supports the following Xcv commands:

-   CleanupPort

-   DeviceID

-   PnPXID

-   ResetCommunication

-   ServiceID

**Note** This sample is for the v4 print driver model.

For more information, see [V4 Driver Connectivity Architecture](http://msdn.microsoft.com/en-us/library/windows/hardware/) and [Bidirectional Communication Schema](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545169(v=vs.85).aspx).

