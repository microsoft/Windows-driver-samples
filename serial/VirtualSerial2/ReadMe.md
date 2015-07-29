Virtual serial driver sample
============================

This sample demonstrates these two serial drivers:

-   A simple virtual serial driver (ComPort)
-   A controller-less modem driver (FakeModem).This driver supports sending and receiving AT commands using the ReadFile and WriteFile calls or via a TAPI interface using an application such as, HyperTerminal.

This sample driver is a minimal driver meant to demonstrate the usage of the User-Mode Driver Framework. It is not intended for use in a production environment.

For more information, see [Serial Controller and Device Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff546939) in the WDK documentation.

Code tour
---------

#### comsup.cpp & comsup.h
COM Support code - specifically base classes which provide implementations for the standard COM interfaces **IUnknown** and **IClassFactory** which are used throughout the sample.
The implementation of **IClassFactory** is designed to create instances of the CMyDriver class. If you should change the name of your base driver class, you would also need to modify this file.

#### dllsup.cpp
DLL Support code - provides the DLL's entry point as well as the single required export (**DllGetClassObject**).
These depend on comsup.cpp to perform the necessary class creation.

#### exports.def
This file lists the functions that the driver DLL exports.

#### internal.h
This is the main header file for the sample driver.

#### driver.cpp & driver.h
Definition and implementation of the driver callback class (CMyDriver) for the sample. This includes **DriverEntry** and events on the framework driver object.

#### device.cpp & driver.h
Definition and implementation of the device callback class (CMyDriver) for the sample. This includes events on the framework device object.

#### queue.cpp & queue.h
Definition and implementation of the base queue callback class (CMyQueue). This includes events on the framework I/O queue object.

#### VirtualSerial.rc /FakeModem.rc
This file defines resource information for the sample driver.

#### VirtualSerial.inf / FakeModem.inf
INF file that contains installation information for this driver.

