---
page_type: sample
description: "Simulates a device that is a Windows thermal management client."
languages:
- cpp
products:
- windows
- windows-wdk
---


<!---
    name: SimThermalClient - Simulated Thermal Client Sample Driver
    platform: KMDF
    language: cpp
    category: Thermal Power
    description: Simulates a device that is a Windows thermal management client.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=617992
--->

# SimThermalClient - Simulated Thermal Client Sample Driver

This sample is a driver for a simulated device that is a client of Windows thermal management. This driver publishes a [GUID\_THERMAL\_COOLING\_INTERFACE](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698265) driver interface. Drivers publish this interface so that they can participate in global thermal management under the coordination of the Windows operating system.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

For more information, see [Device-Level Thermal Management](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698236).