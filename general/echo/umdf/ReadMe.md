Echo Sample (UMDF Version 1)
============================

This sample demonstrates how to use User-Mode Driver Framework (UMDF) version 1 to write a driver and demonstrates best practices.

It also demonstrates the use of a default Serial Dispatch I/O Queue, its request start events, cancellation event, and synchronizing with another thread. The preferred I/O retrieval mode is set to Direct I/O. So, whenever a request is received by the framework, UMDF looks at the size of the buffer and determines, whether it should copy the buffer (if the length is less than 2 full pages) or map it (if the length is greater or equal to 2 full pages).

This sample driver is a minimal driver meant to demonstrate the usage of the User-Mode Driver Framework. It is not intended for use in a production environment.


Related technologies
--------------------

[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)

Testing
-------

To test the Echo driver, you can run echoapp.exe which is built from \\echo\\exe.

First install the device as described above. Then run echoapp.exe.

```
D:\>echoapp /?
Usage:
Echoapp.exe --- Send single write and read request synchronously
Echoapp.exe -Async --- Send 100 reads and writes asynchronously
Exit the app anytime by pressing Ctrl-C
 
D:\>echoapp
DevicePath: \\?\root#sample#0000#{cdc35b6e-0be4-4936-bf5f-5537380a7c1a}
Opened device successfully
512 Pattern Bytes Written successfully
512 Pattern Bytes Read successfully
Pattern Verified successfully
 
D:\>echoapp -Async
DevicePath: \\?\root#sample#0000#{cdc35b6e-0be4-4936-bf5f-5537380a7c1a}
Opened device successfully
Starting AsyncIo
Number of bytes written by request number 0 is 1024
Number of bytes read by request number 0 is 1024
Number of bytes read by request number 1 is 1024
Number of bytes written by request number 2 is 1024
Number of bytes read by request number 2 is 1024
Number of bytes written by request number 3 is 1024
Number of bytes read by request number 3 is 1024
Number of bytes written by request number 4 is 1024
Number of bytes read by request number 4 is 1024
Number of bytes written by request number 5 is 1024
Number of bytes read by request number 5 is 1024
Number of bytes written by request number 6 is 1024
Number of bytes read by request number 6 is 1024
Number of bytes written by request number 7 is 1024
Number of bytes read by request number 7 is 1024
Number of bytes written by request number 8 is 1024
Number of bytes read by request number 8 is 1024
Number of bytes written by request number 9 is 1024
Number of bytes read by request number 9 is 1024
Number of bytes written by request number 10 is 1024
Number of bytes read by request number 10 is 1024
Number of bytes written by request number 11 is 1024
...
```

Note that the reads and writes are performed by independent threads in the echo test application. As a result the order of the output may not exactly match what you see above.

File Manifest
-------------

**comsup.cpp & comsup.h**

- COM Support code - specifically base classes which provide implementations for the standard COM interfaces IUnknown and IClassFactory which are used throughout this sample.
- The implementation of IClassFactory is designed to create instances of the CMyDriver class. If you should change the name of your base driver class, you would also need to modify this file.

**dllsup.cpp**

- DLL Support code - provides the DLL's entry point as well as the single required export (DllGetClassObject).
- These depend on comsup.cpp to perform the necessary class creation.

**exports.def**

- This file lists the functions that the driver DLL exports.

**internal.h**

- This is the main header file for this driver.

**Driver.cpp and Driver.h**

- DriverEntry and events on the driver object.

**Device.cpp and Device.h**

- The Events on the device object.

**Queue.cpp and Queue.h**

- Contains Events on the I/O Queue Objects.

**Echo.rc**

- Resource file for the driver.

**WUDFEchoDriver.inx**

- File that describes the installation of this driver. The build process converts this into an INF file.

**makefile.inc**

- A makefile that defines custom build actions. This includes the conversion of the .INX 

**echodriver.ctl**

- This file lists the WPP trace control GUID(s) for the sample driver. This file can be used with the tracelog command's -guid flag to enable the collection of these trace events within an established trace session.
- These GUIDs must remain in sync with the trace control GUIDs defined in internal.h.

