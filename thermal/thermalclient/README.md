---
page_type: sample
description: "Simulates a device that is a Windows thermal management client."
languages:
- cpp
products:
- windows
- windows-wdk
---

# SimThermalClient - Simulated Thermal Client Sample Driver

This sample is a driver for a simulated device that is a client of Windows thermal management. This driver publishes a [GUID\_THERMAL\_COOLING\_INTERFACE](https://docs.microsoft.com/windows-hardware/drivers/kernel/global-thermal-mgmt) driver interface. Drivers publish this interface so that they can participate in global thermal management under the coordination of the Windows operating system.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

## See also

[Device-Level Thermal Management](https://docs.microsoft.com/windows-hardware/drivers/kernel/device-level-thermal-management)
