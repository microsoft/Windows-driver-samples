Synaptics Touch Sample
======================
The Synaptics Touch (KMDF) sample demonstrates how to write a HID miniport driver for the Synaptics 3202 touch controller.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Related technologies
--------------------

[Kernel-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544396)

[Human Interface Devices](http://msdn.microsoft.com/en-us/library/windows/hardware/jj126202)

[Windows Pointer Device](http://msdn.microsoft.com/en-us/library/windows/hardware/jj151570)


System requirements
-------------------
**Client:** Windows 10

**Server:** Windows 10

**Phone:**  Windows 10


File Manifest
-------------
#### driver.h, driver.c
DriverEntry and Events on the Driver Object.

#### device.h, device.c
Events on the Device Object.

#### queue.h, queue.c
Contains Events on the I/O Queue Objects.

#### hid.c, hid.h
Contains the HID descriptor and functions to handle to HID requests.

#### idle.c, idle.h
Contains the declarations for Power Idle specific callbacks and function definitions.

#### internal.h
Contains common types and defintions used internally by the multi touch screen driver.

#### spb.c
Contains all I2C-specific functionality.

#### init.c
Contains Synaptics initialization code.

#### power.c
Contains Synaptics power-on and power-off functionality.

#### registry.c
This module retrieves platform-specific controller configuration from the registry, or assigns default values if no registry configuration is present.

#### report.c
Contains Synaptics specific code for reporting samples.

#### resolutions.c
Contains resolution translation defines and types.

#### rmiinternal.h
Contains common types and defintions used internally by the multi touch screen driver.

#### SynapticsTouch.inx
File that describes the installation of this driver. The build process converts this into an INF file.



