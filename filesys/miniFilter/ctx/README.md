Ctx File System Minifilter Driver
=================================

The Ctx minifilter is an example that demonstrates how to attach contexts to instances, files, streams, and stream handles in your minifilter.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The *Ctx* minifilter demonstrates how to attach and remove contexts from instances, files, steams, and stream handles. *Ctx* attaches a context whenever one of these objects is created. While attaching a context to a file, the sample also creates a stream and stream handle context. All contexts are ultimately deleted by the filter manager using the callback function that the *Ctx* minifilter provides.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.
