---
page_type: sample
description: "Illustrates DCHU principles of universal driver design."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Driver package installation toolkit for universal drivers

This sample illustrates the DCHU principles of universal driver design.  The sample uses the [OSR FX2 learning kit](https://osrfx2.sourceforge.net/).  For a detailed code walkthrough, see [Universal Driver Scenarios](https://docs.microsoft.com/windows-hardware/drivers/develop/universal-driver-scenarios).

There are three Visual Studio solutions in this sample.  Each one represents a single submission on the [Windows Hardware Dev Center dashboard](https://developer.microsoft.com/windows/hardware/dashboard-sign-in).  The solutions are split into the following subdirectories:

- `osrfx2_DCHU_base` : The driver for the OSR FX2 Learning Kit.  This includes the device driver, an upper filter driver for the device (a no-op), a Win32 User Service that controls lights on the device, and a console app that can control the device.

- `osrfx2_DCHU_extension_loose`: An extension INF for the OSR FX2 device.  This extension modifies some registry settings originally specified by the base driver (`osrfx2_DCHU_base`) and also uses AddComponent to create a Software Component.  There is also a component INF project that would be a separate submission to DevCenter, which runs some simple software.  These two projects are loosely coupled, and can be installed in any order on the machine.

- `osrfx2_DCHU_extension_tight`: An extension INF for the OSR FX2 device.  This extension mimics the behavior of `osrfx2_DCHU_extension_loose`; however, it does so in a tightly coupled manner.  Using CopyINF, both the extension and component INF are placed into one driver package (and one submission to DevCenter).  Here there is less flexibility with the base/component/extension relationship, but it ensures that the component INF is applied at the same time as the extension.

Both `osrfx2_DCHU_extension_loose` and `osrfx2_DCHU_extension_tight` provide the same functionality, so installing both on the same OSR FX2 device is unnecessary.  They are intended to show a different way to use extension and component INFs depending on a project's needs.

> [!NOTE]
> osrfx2_DCHU_extension_tight will not currently build on Windows 10 version 1703.  You will see an error saying that the directive CopyINF does not work from extension INFs.  This has been fixed for the Windows 10 Fall Creators Update.

Each of these solutions can be built with the latest WDK on Visual Studio 2015.  Additionally, you can also download a [Universal Windows Platform app (UWP)](https://github.com/Microsoft/Windows-universal-samples/tree/master/Samples/CustomCapability) that controls the OSR FX2 Learning Kit's device.  To learn how to pair a UWP app with a device, see [Hardware access for Universal Windows Platform apps](https://docs.microsoft.com/windows-hardware/drivers/devapps/hardware-access-for-universal-windows-platform-apps)

The app and the contents of this sample can coexist, but on Windows 10 version 1703, to build the app with Visual Studio 2017, the computer cannot have the WDK installed.

To install these driver packages, make sure that the target machine is in Test Mode, using `bcdedit /set testsigning on`.

Then, use `pnputil /i /a <PATHTOINF>` to install each of the desired driver packages.  They should be installed in the following order:

- `osrfx2_DCHU_base`

- `osrfx2_DCHU_extension`

- `osrfx2_DCHU_component`

Technically the order of `osrfx2_DCHU_extension` and `osrfx2_DCHU_component` doesn't matter, but the software within `osrfx2_DCHU_component` will read the registry set by the extension to show that an extension's INF settings are applied *after* the base INFs.
