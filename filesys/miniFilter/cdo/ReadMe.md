CDO File System Minifilter Driver
=================================

The CDO minifilter sample is an example if you intend to use a control device object (CDO) with your minifilters.

Although the filter manager infrastructure provides a message interface for communication between applications and minifilters, you might need explicit CDOs while the minifilters interface with legacy software. This sample shows how to create and use a CDO with minifilters.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

When the CDO minifilter is deployed, it creates a CDO object named "FileSystem\\Filters\\CdoSample" in the Microsoft Windows object namespace and enables applications to open it and perform certain operations on it.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.
