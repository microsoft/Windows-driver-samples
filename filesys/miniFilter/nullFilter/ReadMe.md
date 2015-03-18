NullFilter File System Minifilter Driver
========================================

The NullFilter minifilter is a sample minifilter that shows how to register a minifilter with the filter manager.

## Universal Compliant
This sample builds a Windows Universal driver. It uses only APIs and DDIs that are included in Windows Core.

Design and Operation
--------------------

The *NullFilter* minifilter is a simple minifilter that registers itself with the filter manager for no callback operations.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

