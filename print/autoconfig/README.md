---
page_type: sample
description: "Demonstrates how to implement auto-configuration in v4 print drivers."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Print auto-configuration sample

This sample demonstrates how to implement auto-configuration in v4 print drivers.

## Auto-configuration basics

Many printers ship with optional components which are not present in all versions of the printer. For these printers, it's important that the driver only shows options which are enabled by the currently installed hardware. For example, if a stapling unit is optional for a particular printer, then the driver shouldn't expose the stapling feature to the end user if that unit is not installed.

Windows auto-configuration allows a print driver to specify a mapping between driver installable options and the state of the printer as expressed through the Bidi Schema.

For more information on auto-configuration, see [Printer Autoconfiguration](https://docs.microsoft.com/windows-hardware/drivers/print/printer-autoconfiguration).

For more information on the Bidi Schema, see [Bidirectional Communication Schema](https://docs.microsoft.com/windows-hardware/drivers/print/bidirectional-communication-schema).

## Build the sample

The auto-configuration sample doesn't have any binaries to be built. It may be installed by using **Add Printer Wizard** and supplying the AutoCnfg.INF as the INF file.

Too build a signed driver package using Windows Driver Kit (WDK) 10 and Visual Studio 2015, for the project file (csproj) that ships with the auto-configuration sample, perform the following steps.

1. Open the solution file in Visual Studio 2015.

1. On the **Build** menu, select **Configuration Manager...**.

1. In **Configuration Manager**, select the **Configuration** and **Platform** that you want to build your driver for.

> [!NOTE]
> When the driver builds, it will be placed in the output folder for the architecture you selected.

At this point, Visual Studio will be able to build a driver package and output the files to disk. In order to configure driver signing and deployment, see [Developing, Testing, and Deploying Drivers](https://docs.microsoft.com/windows-hardware/drivers/develop/).

For more information about how to build a driver solution using Microsoft Visual Studio, see [Building a Driver with Visual Studio and the WDK](https://docs.microsoft.com/windows-hardware/drivers/develop/building-a-driver).

## Run the sample

To run the auto-configuration sample, you must install this driver to support a printer which is installed against either a WSD port, or TCP/IP port. Additionally, USB printers may be supported if the driver is updated to incorporate USB Bidi Javascript, as described here: [Print Driver USB Monitor and Bidi Sample](https://docs.microsoft.com/samples/microsoft/windows-driver-samples/print-driver-usb-monitor-and-bidi-sample/).
