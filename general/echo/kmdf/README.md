KMDF Echo Sample
================

The ECHO (KMDF) sample demonstrates how to use a sequential queue to serialize read and write requests presented to the driver.

It also shows how to synchronize execution of these events with other asynchronous events such as request cancellation and DPC.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Related technologies
--------------------

[Kernel-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544396)

Code Tour
---------

DriverEntry - Creates a framework driver object.

EvtDeviceAdd: Creates a device and registers self managed I/O callbacks so that it can start and stop the periodic timer when the device is entering and leaving D0 state. It registers a device interface so that application can find the device and send I/O. For managing I/O requests, the driver creates a default queue to receive only read & write requests. All other requests sent to the driver will be failed by the framework. Then the driver creates a periodic timer to simulate asynchronous event. The purpose of this timer would be to complete the currently pending request.

In the AutoSync version of the sample, the queue is created with WdfSynchronizationScopeQueue so that I/O callbacks including cancel routine are synchronized with a queue-level lock. Since timer is parented to queue and by default timer objects are created with AutomaticSerialization set to **TRUE**, timer DPC callbacks will be serialized with EvtIoRead, EvtIoWrite and Cancel Routine.

In the DriverSync version of the sample, the queue is created with WdfSynchronizationScopeNone, so that the framework does not provide any synchronization. The driver synchronizes the I/O callbacks, cancel routine and the timer DPC using a spinlock that it creates for this purpose.

EvtIoWrite: Allocates an internal buffer as big as the size of buffer in the write request and copies the data from the request buffer to internal buffer. The internal buffer address is saved in the queue context. If the driver receives another write request, it will free this one and allocate a new buffer to match the size of the incoming request. After copying the data, it will mark the request cancelable and return. The request will be eventually completed either by the timer or by the cancel routine if the application exits.

EvtIoRead: Retrieves request memory buffer and copies the data from the buffer created by the write handler to the request buffer, and marks the request cancelable. The request will be completed by the timer DPC callback.

Since the queue is a sequential queue, only one request is outstanding in the driver.

Testing
-------

**Usage:**

Echoapp.exe --- Send single write and read request synchronously

Echoapp.exe -Async --- Send 100 reads and writes asynchronously

Exit the app anytime by pressing Ctrl-C

File Manifest
-------------

File

Description

Echo.htm

Documentation for this sample (this file).

***(The AutoSync and DriverSync versions of the sample each have their own version of the following files)***

Driver.h, Driver.c

DriverEntry and Events on the Driver Object.

Device.h, Device.c

Events on the Device Object.

Queue.h, Queue.c

Contains Events on the I/O Queue Objects.

Echo.inx

File that describes the installation of this driver. The build process converts this into an INF file.

Makefile.inc

A makefile that defines custom build actions. This includes the conversion of the .INX file into a .INF file

Makefile

This file merely redirects to the real makefile that is shared by all the driver components of the Windows NT DDK.

Sources

Generic file that lists source files and all the build options.

