---
page_type: sample
description: "Demonstrates a simulated power policy device."
languages:
- cpp
products:
- windows
- windows-wdk
---

# plpolicy - Simulated Power Limit Policy Driver

This sample is a driver for a simulated power limit policy device.

## Universal Windows Driver Compliant

The plpolicy sample provides the source code for a power limit controller that supplies power limit management to the operating system.

This driver supplies IOCTLs to receive commands from other modules and plumb cooresponding information to the hardware through OS kernel
space APIs.

IOCTL_POWERLIMIT_POLICY_REGISTER: Other module supplies the BIOS name of the target device to control the power limit.

IOCTL_POWERLIMIT_POLICY_QUERY_ATTRIBUTES: Query the target device for supported power limits and attributes.

IOCTL_POWERLIMIT_POLICY_QUERY_VALUES: Query the target device for active power limit values.

IOCTL_POWERLIMIT_POLICY_SET_VALUES: Set power limit values to the target device.

For a production driver, those IOCTLs are not needed. Instead, the policy driver should:
1. During the init phase, subscribes to GUID_DEVINTERFACE_POWER_LIMIT notifications. Then query client's BIOS name in the callback, and
   compares it with the target client device. Then create power limit request through PoCreatePowerLimitRequest, query attributes through
   PoQueryPowerLimitAttributes.

2. At runtime, the policy driver should collect input signals and calcualte power limit target(s), then send targets to the client through
   PoSetPowerLimitValue. If necessary, the policy driver can query the current power limit of client through PoQueryPowerLimitValue.

3. If the power limit control is no longer needed, delete the power limit request through PoDeletePowerLimitRequest.
