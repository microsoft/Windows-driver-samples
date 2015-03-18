SimThermalClient: Simulated Thermal Client Sample Driver
========================================================

This sample is a driver for a simulated device that is a client of Windows thermal management. This driver publishes a [GUID\_THERMAL\_COOLING\_INTERFACE](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698265) driver interface. Support for this interface is available starting with Windows 8.1. Drivers publish this interface so that they can participate in global thermal management under the coordination of the Windows operating system.

## Universal Compliant
This sample builds a Windows Universal driver. It uses only APIs and DDIs that are included in Windows Core.

For more information, see [Device-Level Thermal Management](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698236).
