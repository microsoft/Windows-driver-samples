WPD service sample driver
=========================

The WpdServiceSampleDriver shows how to extend the WpdHelloWorldDriver sample so that it supports a simulated device with a Contacts device service. By using this device service, an application can discover events, methods, and properties that operate on Contacts that are stored on the device. And, the application can use the Contacts device service to handle these events, invoke these methods, or retrieve these properties. For example, the application might invoke methods to synchronize the Contacts that are found on the device with the contacts that are stored on a computer or to read the Name property for a given Contact.

A device service is an extension of a functional object. In addition to logically grouping device capabilities, a device service provides applications that can programmatically discover those capabilities.

**Note** This driver was written in the simplest way to demonstrate concepts. Therefore, the sample driver might perform operations or be structured in a way that are inefficient in a production driver. Additionally, this sample does not use real hardware. Instead, it simulates a device by using data structures in memory. Therefore the driver might be implemented in a way that is unrealistic for production hardware.

For a complete description of this sample and its underlying code and functionality, refer to the [WPD Service Sample Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597714) description in the Windows Driver Kit documentation.


Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](https://msdn.microsoft.com/en-us/library/windows/hardware/ff597898)
