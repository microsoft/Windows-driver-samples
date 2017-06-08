Driver package installation toolkit for universal drivers 

Directions:

Each portion of the sample is separated by driver package:

osrfx2_DCHU_base : The driver for the Osr Fx2 Learning Kit.  This includes the device driver,
                   an upper filter driver for the device (a no-op), a Win32 User Service that
                   controls lights on the device, and a console app that can control the device.

osrfx2_DCHU_extension: An extension driver for the Osr Fx2 device.  This extension modifies
                       some registry settings laid down by the base driver (osrfx2_DCHU_base).

osrfx2_DCHU_component: A Software Component that installs on a device that the base driver
                       package's INF (osrfx2_DCHU_base.inf) creates using AddComponent.
                       This Software Component executes a legacy .exe to install a file
                       in the C:\Program Files\... directory that reads the Osr Fx2 device's
                       registry settings applied in the base driver package's INF (osrfx2_DCHU_base.inf)

Each of these solutions can be built with the latest WDK on Visual Studio 2015.  Additionally,
an HSA that can also control the Osr Fx2 Learning Kit's device is available here:

https://github.com/Microsoft/Windows-universal-samples/tree/master/Samples/CustomCapability

The HSA and the contents of this sample can coexist, but currently the HSA can only be built
with Visual Studio 2017 on a machine *WITHOUT* the WDK.  This limitation is reportedly fixed
in RS3.

To install these driver packages, make sure that the target machine is in Test Mode, using:

    bcdedit /set testsigning on

And then use pnputil /i /a <PATHTOINF> to install each of the three driver packages.  They should
be installed in this order:

    osrfx2_DCHU_base
    osrfx2_DCHU_extension
    osrfx2_DCHU_component

Technically the order of osrfx2_DCHU_extension and ...component doesn't matter, but the software
within osrfx2_DCHU_component will read the regisry set by the extension to show that an extension
INF's settings are applied *after* the base INF's.
                       