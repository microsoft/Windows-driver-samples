System DMA
==========

This sample demonstrates the usage of V3 System DMA. It shows how a driver could use a system DMA controller supported by Windows to write data to a hardware location using DMA.

The sample consists of a legacy device driver and a Win32 console mode test application. The test application opens a handle to the device exposed by the driver and makes a DeviceIoControl call to initiate the example system DMA. To understand how the V3 system DMA calls are invoked please study SDmaWrite() in SDma.c.

**Note** This sample driver is not a PnP driver. This is a minimal driver meant to demonstrate an OS feature. Neither it nor its sample programs are intended for use in a production environment. Rather, they are intended for educational purposes and as a skeleton driver.

Run the sample
--------------

To test this driver, copy the test app, SystemDmaApp.exe, and the driver to the same directory, and run the application. The application will automatically load the driver if it's not already loaded and interact with the driver. When you exit the app, the driver will be stopped, unloaded and removed. Because no system DMA controller exists for Windows which uses the advertised DRQ, the sample driver will not proceed any further than failing to acquire a system DMA adapter.

