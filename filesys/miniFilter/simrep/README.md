SimRep File System Minifilter Driver
====================================

SimRep is a sample filter that demonstrates how a file system filter can simulate file-system like reparse-point behavior to redirect a file open to an alternate path.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Design and Operation
--------------------

Normally, if the file-system sees an open for a file with a reparse-point on it, the filesystem fills out the tag buffer and returns STATUS\_REPARSE. Minifilters see the post-operation callback for this create. As the create travels up file system filter stack in post-create path, each minifilter has the opportunity to interpret the reparse point if they own the tag. If no file system filter claims the tag, IO Manager will attempt to interpret the tag based on tags known to and serviced by IO Manager. If the tag is unknown to IO manager then the create is failed with STATUS\_IO\_REPARSE\_TAG\_NOT\_HANDLED. SimRep does not demonstrate how to handle the case where the file system hits a reparse-point on the file. Instead it "fakes" encountering a reparse point before the create reaches the filesystem. When SimRep detects a create for a path that it is redirecting, SimRep replaces the file name in the file object and completes the open with STATUS\_REPARSE. This means we reparse without actually going to the file system.

SimRep decides to reparse according to a mapping. The mapping is made up of a "New Mapping Path" and an "Old Mapping Path". The old mapping path is the path which SimRep looks for on incoming opens. If the path specified for the create is down the Old Mapping Path, then SimRep will strip off the Old Mapping Path, and replace it with the New Mapping Path. By default, the Old Mapping Path is \\x\\y and the New Mapping Path is \\a\\b. So an open to \\x\\y\\z will be replaced with an open to \\a\\b\\z. These defaults are defined as registry keys at install time and are loaded on DriverEntry. See simrep.inf for details.

It is important to note that SimRep does not take long and short names into account. It literally does a string comparison to detect overlap with the mapping paths. SimRep also handles IRP\_MJ\_NETWORK\_QUERY\_OPEN. Because network query opens are FastIo operations, they cannot be reparsed. This means network query opens which need to be redirected must be failed with FLT\_PREOP\_DISALLOW\_FASTIO. This will cause the Io Manager to reissue the open as a regular IRP based open. To prevent performance regression, SimRep only fails network query opens which need to be reparsed.

For more information on file system minifilter design, start with the [File System Minifilter Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff540402) section in the Installable File Systems Design Guide.

