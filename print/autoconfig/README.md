Print auto-configuration sample
===============================

This sample demonstrates how to implement auto-configuration in v4 print drivers. 

**Auto-configuration basics**

Many printers ship with optional components which are not present in all versions of the printer. For these printers, it's important that the driver only shows options which are enabled by the currently installed hardware. For example, if a stapling unit is optional for a particular printer, then the driver shouldn't expose the stapling feature to the end user if that unit is not installed.

Windows auto-configuration allows a print driver to specify a mapping between driver installable options and the state of the printer as expressed through the Bidi Schema. 

For more information on auto-configuration, see [Printer Autoconfiguration](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560774(v=vs.85).aspx).

For more information on the Bidi Schema, see [Bidirectional Communication Schema](https://msdn.microsoft.com/en-us/library/windows/hardware/ff545169(v=vs.85).aspx).


Build the sample
----------------

The auto-configuration sample doesn't have any binaries to be built. It may be installed by using **Add Printer Wizard** and supplying the AutoCnfg.INF as the INF file.

But to build a signed driver package using Windows Driver Kit (WDK) 10 and Visual Studio 2015, for the project file (csproj) that ships with the auto-configuration sample, perform the following steps.

1. Open the solution file in Visual Studio 2015.

2. On the **Build** menu, select **Configuration Manager...**.

3. In **Configuration Manager**, select the **Configuration** and **Platform** that you want to build your driver for.

**Note** When the driver builds, it will be placed in the output folder for the architecture you selected.

At this point, Visual Studio will be able to build a driver package and output the files to disk. In order to configure driver signing and deployment, see [Developing, Testing, and Deploying Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554651(v=vs.85).aspx).

For more information about how to build a driver solution using Microsoft Visual Studio, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

Run the sample
--------------

To run the auto-configuration sample, you must install this driver to support a printer which is installed against either a WSD port, or TCP/IP port. Additionally, USB printers may be supported if the driver is updated to incorporate USB Bidi Javascript, as described here: [Print Driver USB Monitor and Bidi Sample](https://github.com/Microsoft/Windows-driver-samples/tree/master/print/v4PrintDriverSamples/v4PrintDriver-USBMon-Bidi-Extension).

