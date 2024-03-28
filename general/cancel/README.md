---
page_type: sample
description: "Demonstrates the use of the cancel-safe queue routines."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Cancel-Safe IRP Queue Sample

This sample demonstrates the use of the cancel-safe queue routines [**IoCsqInitialize**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-iocsqinitialize), [**IoCsqInsertIrp**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-iocsqinsertirp), [**IoCsqRemoveIrp**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-iocsqremoveirp), [**IoCsqRemoveNextIrp**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/nf-wdm-iocsqremovenextirp). These routines were introduced in Windows for queuing IRPs in the driver's internal device queue. By using these routines, driver developers do not have to worry about IRP cancellation race conditions. A common problem with cancellation of IRPs in a driver is synchronization between the cancel lock or the InterlockedExchange in the I/O Manager with the driver's queue lock. The **IoCsq*Xxx*** routines abstract the cancel logic while allowing the driver to implement the queue and associated synchronization.

The sample is accompanied by a simple multithreaded Win32 console application to stress-test the driver's cancel and cleanup routines.

This driver is written for an hypothetical data-acquisition device that requires polling at a regular interval. The device has some settling period between two successive reads. On a user request, the driver reads data and records the time. When the next read request comes in, the driver checks the interval to see if it's reading the device too soon. If so, it pends the IRP and sleeps for a while, and then tries again. On arrival, IRPs are queued in a cancel-safe queue and a semaphore is signaled. A polling thread that waits indefinitely on the semaphore wakes up to the signal and processes queued IRPs sequentially.

This sample driver is not a Plug and Play driver. This is a minimal driver meant to demonstrate a feature of the operating system. Neither this driver nor its sample programs are intended for use in a production environment. Instead, they are intended for educational purposes and as a skeleton driver.

Look in the Startio directory for another version of the sample driver that shows how to use cancel-safe IRP queues to implement I/O queuing functionality similar to the [**IoStartPacket**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ntifs/nf-ntifs-iostartpacket) and [**IoStartNextPacket**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/ntifs/nf-ntifs-iostartnextpacket) routines. The same test application works with this driver as well.

For more information, see [Cancel-Safe IRP Queues](https://docs.microsoft.com/windows-hardware/drivers/kernel/cancel-safe-irp-queues).

## Run the sample

To test this driver, run canclapp.exe, which is a simple Win32 multithreaded console application. The driver will automatically load and start. When you exit the application, the driver will stop and be removed.

`Usage: canclapp <NumberOfThreads>`

> [!NOTE]
> The `NumberOfThreads` command-line parameter is limited to a maximum of 10 threads; the default value if no parameter is specified is 1. The main thread waits for user input. If you press Q, the application exits gracefully; otherwise, it exits the process abruptly and forces all the threads to be terminated and all pending I/O operations to be canceled. Other threads perform I/O asynchronously in a loop. After every overlapped read, the thread goes into an alertable sleep and wakes as soon as the completion routine runs, which occurs when the driver completes the read IRP. You should run multiple instances of the application to stress test the driver.
