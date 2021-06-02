---
page_type: sample
description: "Demonstrates how to extend the WpdHelloWorldDriver for a device that supports multiple transports."
languages:
- cpp
products:
- windows
- windows-wdk
---

# WPD multi-transport sample driver

The WpdMultiTransportDriver sample demonstrates how you could extend the WpdHelloWorldDriver for a device that supports multiple transports. A transport is a protocol over which a portable device communicates with a computer. Example transports include Internet Protocol (IP), Bluetooth, and USB.

A number of portable devices now support multiple transports. For example, a number of cell phones support both Bluetooth and USB. Windows supports a multitransport driver model that ensures that only one node appears for each device.

For a complete description of this sample and its underlying code and functionality, refer to the [WPD MultiTransport Driver](https://docs.microsoft.com/windows-hardware/drivers/portable/the-wpdmultitransportdriver-sample) description in the Windows Driver Kit documentation.

## Related topics

[WPD Design Guide](https://docs.microsoft.com/windows-hardware/drivers/portable/wpd-design-guide)

[WPD Driver Development Tools](https://docs.microsoft.com/windows-hardware/drivers/portable/familiarizing-yourself-with-the-sample-driver)

[WPD Programming Guide](https://docs.microsoft.com/windows-hardware/drivers/portable/wpd-programming-guide)
