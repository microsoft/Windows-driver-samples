---
page_type: sample
description: "Demonstrates a KMDF-based implementation of Windows battery driver interfaces."
languages:
- cpp
products:
- windows
- windows-wdk
---

# SimBatt: Simulated Battery Driver Sample

SimBatt is simulated battery device driver.

This source code is intended to demonstrate implementation of Windows battery driver interfaces.

This is a KMDF based sample.

You may use this sample as a starting point to implement a battery miniport specific to your needs.


## How to test

### Driver installation
```
:: Install driver
pnputil /add-driver simbatt.inf /install

:: Simulate two batteries
devgen /add /instanceid 1 /hardwareid "{6B34C467-CE1F-4c0d-A3E4-F98A5718A9D6}\SimBatt"
devgen /add /instanceid 2 /hardwareid "{6B34C467-CE1F-4c0d-A3E4-F98A5718A9D6}\SimBatt"
```

The simulated batteries will have device instance ID `SWD\DEVGEN\1` and `SWD\DEVGEN\2`.

### Modify battery parameters
Parameters for each simulated battery can be changed through `IOCTL_SIMBATT_SET_STATUS` IOCTL calls with [`BATTERY_STATUS`](https://learn.microsoft.com/en-us/windows/win32/power/battery-status-str) input. This enables simulation of many power scenarios, such as low charge and switching between AC and DC power. This can be done without any physical batteries attached. One can in fact use the driver to simulate multi-battery setups in a Virtual Machine (VM).

Example of how to simulate 50% battery charge that is currently being discharged:
```
BATTERY_STATUS status = {};
status.PowerState = BATTERY_DISCHARGING;
status.Capacity = 50; // 50%
status.Rate = BATTERY_UNKNOWN_RATE;
status.Voltage = BATTERY_UNKNOWN_VOLTAGE;
DeviceIoControl(battery, IOCTL_SIMBATT_SET_STATUS, &status, sizeof(status), nullptr, 0, nullptr, nullptr);
```

The `battery` handle can be accessed by first using `CM_Locate_DevNode` to get a device instance handle from the device instance ID. Then, use `CM_Get_DevNode_Property` with `DEVPKEY_Device_PDOName` to get the physical device object (PDO) string. Finally, use `CreateFile` to open the battery based on the PDO string with a `\\?\GLOBALROOT` prefix.

### Driver uninstallation
```
:: Delete simulated batteries
devgen /remove "SWD\DEVGEN\1"
devgen /remove "SWD\DEVGEN\2"

:: Uninstall driver
pnputil /delete-driver simbatt.inf /uninstall /force
```
