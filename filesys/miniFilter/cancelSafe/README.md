CancelSafe File System Minifilter Driver
========================================

The CancelSafe filter is a sample minifilter that you use if you want to use cancel-safe queues.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The *CancelSafe* minifilter initializes a cancel-safe queue when it is attached to a volume. When the minifilter is deployed, it monitors read operations that are passing through the I/O stack. If the read operation is being performed on a file named csqdemo.txt, it is queued onto the cancel-safe queue. Queued operations are completed after a brief pause through a separate worker thread that is running in system context.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.
