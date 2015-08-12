PCMCIA Smart Card Driver
========================

**Warning, this sample and its documentation are known to be out of date. They will be updated soon. Please be aware that some functionality may not work as expected.**

The PCMCIA Smart Card Driver is used for the SCM PCMCIA smart card reader. This driver is written using Kernel-Mode Driver Framework.

This driver in its original form was written in WDM. It was converted to KMDF to take advantage of all the benefits provided by KMDF in terms of reducing complexity and making it robust. Since this driver still needs to work with the existing smartcard library to handle smartcard specific processing, the driver is not restricted to using only KMDF interfaces. Escaping out of KMDF is necessary for processing I/O requests to get the underlying IRPs and provide that to the smartcard library. The driver also uses advanced IRP handling techniques to work around the limitations imposed by the smartcard library. Except for this quirk, the driver is a fully functional KMDF driver. As a sample, it also makes it easier to adapt this driver for USB devices since KMDF has good support for interfacing with USB devices.

Power Management is described in detail in the WDK documentation. There is, however, one situation that is specific to smart card readers: how to deal with smart card insertions and removals while the system is in standby or hibernation mode.

A card reader will not see any card insertion or removal events in these modes, because the bus might not even have power. The card state must be saved before the reader goes into standby or hibernation mode. After the system returns from these modes, it is necessary to determine what the state of the card is. Card tracking calls must complete whenever there was a card in the reader before standby or hibernation mode or whenever there is a card in the reader after these modes. This step is necessary because the user could have changed the card while the system was in a low-power mode.


Build the sample
----------------

For information on how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

**Note** You can obtain the co-installers by downloading the *wdfcoinstaller.msi* package from [WDK 8 Redistributable Components](http://go.microsoft.com/fwlink/p/?LinkID=226396).

Run the sample
--------------

Installation
------------

The PSCR.SYS driver is available from Windows Update. Therefore, when the SCM 488 PCMCIA reader is inserted, the system will automatically install the driver. However, if you want to customize the source code of this driver and replace the driver from Windows Update with your driver, use the supplied INF file.

Tools
-----

Microsoft offers a test tool (Ifdtest.exe) that allows you to use a smart card reader directly from the command line. Normally, the smart card resource manager is connected to a reader. To use Ifdtest.exe, you must stop the smart card resource manager (Scardsvr.exe) by typing net stop scardsvr at the command line. Ifdtest.exe is also used for the smart card reader logo test.

The driver will not unload as long as you have Ifdtest.exe running and connected to the driver.

Resources
---------

ISO 7816 Part 3 describes smart cards and smart card protocols in detail. Refer to the PC99 Handbook for smart card reader requirements.

For more information about Windows smart card reader drivers, see [Smart Card Reader Devices Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/).

