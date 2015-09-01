Windows Biometric Driver Samples (UMDF Version 1)
=================================================

The Windows Biometric Driver Samples contain the Windows Biometric Driver Interface sample and the Windows Biometric Service Adapter samples.

The following table describes the samples contained in this sample set:

Sample | Description
-------|-------------
*Windows Biometric Driver Interface* | This sample implements the Windows Biometric Driver Interface (WBDI). It contains skeleton code for handling the mandatory IOCTLs necessary to interoperate with the Windows Biometric Framework. A WBDI driver can be deployed in conjunction with an engine adapter DLL to allow a sensor to be exposed from the Windows Biometric Framework. This sample has been written to make use of the UMDF framework, which allows for ease of development and system stability.
*Windows Biometric Service Adapters* | These samples provide skeleton code that developers can use as a basis for writing Sensor, Engine, and Storage Adapters for the Windows Biometric Service. Note that the stubs in these samples are non-functional, and Adapter writers will need to follow the programming guidelines in the WinBio Service documentation in order produce a working Adapter component.


Build the sample
----------------

For information on how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

**Note** You can obtain the co-installers by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396).



Installation
------------

### Windows Biometric Driver Interface

The sample requires the use of a suitable fingerprint sensor. It does not capture real data, but it does create a biometric unit in the Windows Biometric Framework.

### Windows Biometric Service Adapters

To write and test an Adapter plug-in, it will be necessary to have a biometric device and a working WBDI driver for the device.

Adapters are generally installed along with the WBDI driver for the corresponding device. Consult the WinBio Service documentation for information on the INF file commands used for installing Adapters. Note that Adapters are trusted plug-in components, so they can only be installed using a privileged account.

Design and Operation
--------------------

### Windows Biometric Driver Interface

This sample is taken from the UMDF FX2 sample and has been modified to expose WBDI. It has the necessary hooks to make this a WBDI driver:

-   Installs WBDI driver, including correct class GUID settings and icons, and registry settings for Windows Biometric Framework configuration.
-   Publishes WBDI device interface.
-   Supports all the mandatory [WBDI IOCTLs](http://msdn.microsoft.com/en-us/library/windows/hardware/ff536414).
-   Supports cancellation.
-   Can be opened with exclusivity.

All of these things are required for the Windows Biometric Framework service to recognize this device as a biometric device and set up a Biometric Unit. It allows the service to properly control the device.

The sample makes use of ATL support for simplified handling of COM objects with UMDF.

The driver makes use of a parallel queue so that multiple requests can be outstanding at once.

It uses device level-locking to simplify internal thread synchronization. This means that only one framework callback can be active at a time.

It supports cancellation of any IOCTL which may be I/O intensive, particularly a capture IOCTL. This sample does not have a real capture mechanism, so it is simulated by a 5 second delay returning a capture IOCTL. Cancellation is supported through the mechanism exposed by WUDF, with a callback for a request object. Cancellation support is required for all IOCTLs.

There are hooks for all [WBDI IOCTLs](http://msdn.microsoft.com/en-us/library/windows/hardware/ff536414), including the optional IOCTLs.

PnP is very simple for this driver. It needs to only implement **OnPrepareHardware** and **OnReleaseHardware** from **IPnpCallbackHardware**.

Some device drivers may need to keep several pending reads to the WinUsb I/O target in order to properly flush all I/O that comes from the device during a capture.

### Windows Biometric Service Adapters

WinBio Adapters are plug-in components that provide a standard interface layer between the Windows Biometric Service and a biometric device. The WinBio Service recognizes three types of Adapters:

-   Sensor Adapters - expose the sample-capture capabilities of the biometric device.
-   Engine Adapters - expose the sample manipulation, template generation, and matching capabilities of the device.
-   Storage Adapters - expose the template storage and retrieval capabilities of the device.

For many simple biometric devices, it will only be necessary to write a WBDI driver for the device plus an Engine Adapter to perform matching operations. Consult the programming guidelines in the WinBio Service documentation for more details.

Each Adapter sample contains a well-known interface-discovery function, whose job is to return the address of a function dispatch table. When the WinBio Service loads an Adapter plug-in, it uses the interface-discovery function to locate the dispatch table, and then calls various methods in the table to communicate with the biometric device. The purpose, arguments, and return codes of each Adapter method are described in the WinBio Service programming guidelines. More information on adapter plug-ins is available at [WBDI Plug-in Reference](http://msdn.microsoft.com/en-us/library/windows/desktop/dd401553(v=vs.85).aspx).

