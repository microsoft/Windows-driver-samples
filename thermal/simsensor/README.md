---
page_type: sample
description: "Demonstrates a simulated temperature sensor device."
languages:
- cpp
products:
- windows
- windows-wdk
---

# SimSensor - Simulated Temperature Sensor Sample Driver

This sample is a driver for a simulated temperature sensor device.

## Universal Windows Driver Compliant

This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

A hardware platform designer can strategically place temperature sensors in various thermal zones around the platform. The operating system gets the temperature readings from the temperature sensor drivers and uses these readings to regulate the temperatures across the platform. Regulation can be either passive or active. For more information, see [Device-Level Thermal Management](https://docs.microsoft.com/windows-hardware/drivers/kernel/device-level-thermal-management).

The SimSensor sample provides the source code for a specialized sensor driver that supports platform-wide thermal management by the operating system. This driver does not make the temperature sensor accessible to applications through the [Sensor API](https://docs.microsoft.com/windows/win32/sensorsapi/portal).
