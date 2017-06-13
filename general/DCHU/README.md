# Driver package installation toolkit for universal drivers 

This sample illustrates the DCHU principles of universal driver design.  The sample uses the [OSR FX2 learning kit](http://store.osr.com/product/osr-usb-fx2-learning-kit-v2/).  For a detailed code walkthrough, see [Universal Driver Scenarios](https://docs.microsoft.com/windows-hardware/drivers/develop/universal-driver-scenarios).

There are three Visual Studio solutions in this sample.  Each one represents a single submission on the [Windows Hardware Dev Center dashboard](https://developer.microsoft.com/windows/hardware/dashboard-sign-in).  The solutions are split into the following subdirectories:

*  `osrfx2_DCHU_base` : The driver for the OSR FX2 Learning Kit.  This includes the device driver, an upper filter driver for the device (a no-op), a Win32 User Service that controls lights on the device, and a console app that can control the device.

*  `osrfx2_DCHU_extension`: An extension driver for the OSR FX2 device.  This extension modifies some registry settings originally specified by the base driver (`osrfx2_DCHU_base`).

*  `osrfx2_DCHU_component`: A Software Component that installs on a device that the base driver package's INF (`osrfx2_DCHU_base.inf`) creates using the [INF AddComponent Directive](https://docs.microsoft.com/windows-hardware/drivers/install/inf-addcomponent-directive).  This Software Component executes a legacy .exe to install a file in the `C:\Program Files\...` directory that reads the OSR FX2 device's registry settings applied in the base driver package's INF (`osrfx2_DCHU_base.inf`)

Each of these solutions can be built with the latest WDK on Visual Studio 2015.  Additionally, you can also download a [Universal Windows Platform app (UWP)](https://github.com/Microsoft/Windows-universal-samples/tree/master/Samples/CustomCapability) that controls the OSR FX2 Learning Kit's device.  To learn how to pair a UWP app with a device, see [Hardware access for Universal Windows Platform apps](https://docs.microsoft.com/windows-hardware/drivers/devapps/hardware-access-for-universal-windows-platform-apps)

The app and the contents of this sample can coexist, but on Windows 10, version 1703, to build the app with Visual Studio 2017, the computer cannot have the WDK installed.

To install these driver packages, make sure that the target machine is in Test Mode, using `bcdedit /set testsigning on`.

Then, use `pnputil /i /a <PATHTOINF>` to install each of the three driver packages.  They should
be installed in the following order:

    *  `osrfx2_DCHU_base`
    *  `osrfx2_DCHU_extension`
    *  `osrfx2_DCHU_component`

Technically the order of `osrfx2_DCHU_extension` and `osrfx2_DCHU_component` doesn't matter, but the software within `osrfx2_DCHU_component` will read the registry set by the extension to show that an extension INF's settings are applied *after* the base INF's.
                       