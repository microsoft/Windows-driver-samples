Metadata Manager File System Minifilter Driver
==============================================

The Metadata Manager minifilter sample serves as an example if you want to use files for storing metadata that corresponds to your minifilters. The implementation of this sample depicts scenarios in which modifications to the file might have to be blocked or the minifilter might be required to close the file temporarily.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

The Metadata Manager minifilter opens a file when it is first loaded. After that, the minifilter monitors open, close, file control, device control, and Plug and Play (PnP) operations to identify scenarios in which it should close its metadata file or block all writes to it. Applications such as chkdsk obtain implicit or explicit exclusive locks on the volume, and the metadata minifilter demonstrates how to maintain a metadata file without interfering with such lock acquisitions.

The minifilter identifies implicit locks when it sees a non-shared write open request on a volume object. In this scenario, the minifilter closes its metadata file and sets a trigger that corresponds to the volume in its instance object. Later, each close operation is examined to identify if the implicit lock on the volume is being released and, if so, a re-open of the minifilter's metadata file is triggered.

Similarly, the minifilter might close its metadata file if it sees an explicit FSCTL\_DISMOUNT\_VOLUME or FSCTL\_LOCK\_VOLUME file-system control operation. The file is later opened when the minifilter observes the FSCTL\_UNLOCK\_VOLUME control operation. The IRP\_MN\_QUERY\_REMOVE\_DEVICE PnP request can also cause the minifilter to close its metadata file, and the IRP\_MN\_SURPRISE\_REMOVAL PnP request will cause it to detach.

The metadata minifilter also handles the case when a snapshot of its volume object is being taken. In this scenario, the minifilter acquires a shared exclusive lock on the metadata resource object while calling the callback that corresponds to the pre-device control operation for IOCTL\_VOLSNAP\_FLUSH\_AND\_HOLD\_WRITES. The lock is later released in the callback that corresponds to the post-device control operation for IOCTL\_VOLSNAP\_FLUSH\_AND\_HOLD\_WRITES. The lock is acquired to prevent any modifications on the metadata file while the snapshot is being taken.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

