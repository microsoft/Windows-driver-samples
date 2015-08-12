Storage SDIO Driver
===================

This is a sample for a functional Secure Digital (SD) IO driver. The driver is written using the Kernel Mode Driver Framework. It is a driver for a generic mars development board that implements the SDIO protocol without additional functionality.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

The mars board driver exemplifies several different functions that are essential for writing an SDIO driver that leverages KDMF and the SDBUS API. It will show how to:

-   Install and start an SDIO device.

-   Release an SDIO device.

-   Perform data transfers.

-   Alter the settings that the SDIO device uses to communicate with the SD Host Controller.

For more information, see [Secure Digital (SD) Card Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff537945).

**Note** This sample provides an example of a minimal driver. Neither the driver nor the sample programs are intended for use in a production environment. Rather, they are intended for educational purposes and as a skeleton driver.

