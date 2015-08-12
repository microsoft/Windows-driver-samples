Hardware Event Sample
=====================

This sample demonstrates two different ways a Windows kernel-mode driver can notify an application about a hardware event. One way uses an event-based method, and the other uses an IRP-based method. Because the sample driver is not talking to any real hardware, it uses a timer DPC to simulate hardware events. The test application informs the driver whether it wants to be notified by signaling an event or by completing the pending IRP. Additionally, the test application specifies a relative time at which the DPC timer must fire.

*Event-based approach:* The application calls the [**CreateEvent**](http://msdn.microsoft.com/en-us/library/windows/hardware/ms682396) function to create an event. It then passes the event handle to the driver in an I/O control request that uses a private IOCTL code, IOCTL\_REGISTER\_EVENT. Because the driver is a monolithic, top-level driver, its IRP dispatch routines run in the application process context and, as a result, the event handle is still valid in the driver. The driver dereferences the user-mode handle into system space and saves the event object pointer for later use. Next, the driver queues a custom timer DPC. When the DPC fires, the driver signals the event by calling the [**KeSetEvent**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff553253) routine at DISPATCH\_LEVEL, and deletes the references to the event object. You can't use this approach if your driver is not a monolithic, top-level driver; that is because a driver can't guarantee the process context in a multi-level driver stack if the driver is not at the top of the stack.

*Pending IRP-based approach:* The application makes a synchronous IOCTL\_REGISTER\_EVENT request. The driver sets the status of the device I/O control request to IRP pending, queues a timer DPC, and returns STATUS\_PENDING. When the timer fires to indicate a hardware event, the driver completes the pending IRP to notify the application about the hardware event.

There are two advantages of IRP-based approach over the event-based approach. First, the driver can send a message to the application along with the event notification. Second, the driver routines don't have to run in the context of the process that made the request. Instead, the application can send a synchronous or asynchronous (overlapped) I/O control request to the driver.

**Note** This sample driver is not a Plug and Play driver. This is a minimal driver meant to demonstrate a feature of the operating system. Neither this driver nor its sample programs are intended for use in a production environment. Rather, they are intended for educational purposes and as a skeleton driver.


Run the sample
--------------

To test this driver, copy the test application, event.exe, and the driver to the same directory, and run the application. The application will automatically load the driver, if it's not already loaded, and interact with the driver. When you exit the app, the driver will be stopped, unloaded, and removed.

To run the test application, enter the following command in the command window:

`C:\>event.exe <Delay> <0|1>`

The first command-line parameter, `Delay`, equals the time, in seconds, to delay the event signal. For the second command-line parameter, specify 0 for IRP-based notification and 1 for event-based notification.

