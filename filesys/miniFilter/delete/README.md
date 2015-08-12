Delete File System Minifilter Driver
====================================

The Delete minifilter is an example that demonstrates how to detect deletions of files or streams. Deletions are reported as debug output.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The *delete* minifilter illustrates how to detect deletion of files and streams. It monitors IRP\_MJ\_CREATE requests for the FILE\_DELETE\_ON\_CLOSE flag. Also, it detects IRP\_MJ\_SET\_INFORMATION requests for setting FileDispositionInformation. The sample also illustrates how to handle racing deletes (in the form of multiple parallel IRP\_MJ\_SET\_INFORMATION operations), and how to distinguish deletion of an entire file from deletion of just one stream of the file.

**Note** Because of the way in which the Windows operating system deletes files, it is not possible for the minifilter to detect in advance that a file or stream will be deleted. The minifilter can only detect operations that may cause a deletion, and then determine if the deletion took place after the operation completes.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.
