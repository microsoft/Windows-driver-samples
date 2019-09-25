---
page_type: sample
description: "Provides examples of how to build OEM printer customization plug-ins."
languages:
- cpp
products:
- windows
- windows-wdk
---

# OEM Printer Customization Plug-in Samples

The OEMDLL samples are an illustration of OEM customization plug-ins. The BITMAP, OEMPS, OEMUI, OEMUNI, OEMPREAN, CUSTHLP, SyncSet, ThemeUI, PSUIRep, and Watermark samples do not affect the printer output. They are only examples of how to build OEM Customization DLLs of various types.

The following samples are included as part of this sample set:

| Sample | Description |
| --- | --- |
| BITMAP | A Unidrv-based bitmap rendering plug-in. This sample demonstrates how to write a Unidrv-based rendering plug-in that functions as a bitmap driver. The sample is based on the redesigned version of the OEMUNI sample. |
| CUSTHLP | An OEM help-customization plug-in. This sample demonstrates adding OEM help to your customization plug-in and how to replace standard Microsoft supplied help with customized help. |
| OEMPREAN | Illustrates how you can leverage the pre-analysis feature in Unidrv. This sample is built on the redesigned version of the OEMUNI sample. |
| OEMPS | A PostScript rendering plug-in. This sample demonstrates how to write a PostScript-based rendering plug-in. |
| OEMUI | A user interface plug-in (both PostScript and Unidrv flavors are available). The OEMUI sample demonstrates common UI tasks such as adding additional elements to the UI (that is, items and pages). |
| OEMUNI | A user interface plug-in (both PostScript and Unidrv flavors are available). The OEMUI sample demonstrates common UI tasks such as adding additional elements to the UI (that is, items and pages). |
| PSUIREP | A PostScript UI replacement sample. This sample demonstrates how to completely replace the standard printer driver UI for PostScript driver plug-ins. |
| PTPCPIPR | The Unidrv Plug-in for Print Ticket Provider Interface sample contains a functional Unidrv configuration plug-in that implements the IPrintOemPrintTicketProvider interface. You can use this code as a starting point for developing new driver plug-ins that support PrintTicket functionality. |
| SYNCSET | A synchronization plug-in. This sample demonstrates how to synchronize driver settings between the OEM customization page and the standard device settings page. |
| THEMEUI | A theming plug-in. This sample demonstrates how to use theming (that is, comctl v6) for printer driver UI. |
| UNIUIREP | This Universal Printer Driver (Unidrv) UI plugin sample shows how to fully replace Unidrv's standard UI and how to use the new IPrintCoreHelper interface to enumerate GDP features/options, enumerate constraints, get/set Unidrv settings, and create a GDL snapshot. |
| WATERMARK | Demonstrates how to produce customizable watermark page simulation by controlling PostScript injected in the printing stream. The components of the sample include a PostScript-based rendering module in conjunction with a UI module. The rendering module is responsible for the injection of the modified PostScript into the printing stream. This sample demonstrates the required COM interfaces, required functions with sample code and how to use the OEM's private DEVMODE section to communicate between the UI and rendering modules. |
| WATERMARKUNI | Demonstrates how to produce customizable watermark page simulation by controlling PCL data injected in the printing stream. The components of the sample include a UNIDRV-based rendering module in conjunction with a UI module. The rendering module is responsible for the injection of the PCL data into the printing stream. This sample demonstrates the required COM interfaces, required functions with sample code and how to use the OEM's private DEVMODE section to communicate between the UI and rendering modules. This sample works only if you have a page printer that has its own fonts. It doesn't download any fonts nor convert fonts to bitmap. |

For information about how to develop a plug-in for customizing the print driver UI, see [COM Interfaces for UI Plug-Ins](https://docs.microsoft.com/windows-hardware/drivers/print/com-interfaces-for-ui-plug-ins).

> [!NOTE]
> The OEMDLL samples are for the v3 print driver model only.

To build this sample, you can use Microsoft Visual Studio 2017 (Community, Professional, or Enterprise) and Windows Driver Kit (WDK) 10. You can get Visual Studio 2017 [here](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) and WDK 10 [here](https://docs.microsoft.com/windows-hardware/drivers/download-the-wdk).

You can also build this sample with Visual Studio 2013 (Professional or Ultimate) and Windows Driver Kit (WDK) 8.1.

For Windows Driver Kit (WDK) 8 samples, download the WDK 8 samples pack. The samples in the WDK 8 samples pack will build only with Microsoft Visual Studio Professional 2012 (Professional or Ultimate) and WDK 8.

## Operating system requirements

Client - Windows 7 or later
Server - Windows Server 2008 R2 or later

## Build the sample

To build a driver solution using Windows 10 driver kit (Windows Driver Kit (WDK)) and Visual Studio 2017, perform the following steps:

1. Open the solution file in Visual Studio 2017

1. Add all non-binary files (usually located in the \install directory of the sample) to the Package project

    1. In the **Solution Explorer**, right click **Driver Files**

    1. Select **Add**, then click **Existing Item**

    1. Navigate to the location to which you downloaded the sample, and select all the files in the install directory, or the equivalent set of non-binary files such as INFs, INIs, GPD, PPD files, etc.

    1. Click **Add**

1. Configure these files to be added into the driver package

    1. In the **Solution Explorer**, right click the Package project and select **Properties**

    1. In the left pane, click **Configuration Properties** > **Driver Install** > **Package Files**.

    1. In the right pane, use the ellipsis button (...) to browse to the set of files that needs to be added to the driver package. All the data files that you added in **Step 2-c**, except the INF file, should be added. This configuration is per-architecture, so this configuration must be repeated for each architecture that will be built.

    1. Click **OK**

1. Open the INF file and edit it to match the built output

    1. Open the INF file

    1. In the Version section, add a reference to a catalog file that matches the .INF name like this: CatalogFile=XpsDrvSmpl.cat

    1. In the SourceDisksFiles section, change the location of the DLL files you are building, to =1. This indicates that there is no architecture specific directory in this driver. If you ship multiple architectures simultaneously, you will need to collate the driver INF manually.

At this point, Visual Studio 2017 will be able to build a driver package and output the files to disk. In order to configure driver signing and deployment, see [Developing, Testing, and Deploying Drivers](https://docs.microsoft.com/windows-hardware/drivers/develop/).

For more information about how to build a driver solution using Microsoft Visual Studio, see [Building a Driver with Visual Studio and the WDK](https://docs.microsoft.com/windows-hardware/drivers/develop/building-a-driver).

## Run the sample

### Installation

After building the samples, you can install them by using the Add Printer Wizard. Select the local printer, click Have Disk, and point to the directory that contains the relevant INF (oemdll.inf, bitmap.inf, oemprean.inf, and so on) file.

## Design and Operation

### BITMAP

This sample includes the following features:

- The supported ColorMode options in the bitmap driver's GPD are monochrome, 4bpp, 8bpp, and 24bpp.

- The bitmap driver implements the IPrintOemUni::ImageProcessing callback to access bitmap data one band at a time. The driver's implementation of ImageProcessing buffers the band data every time it is called. The function is also responsible for filling the BITMAPINFOHEADER and COLORTABLE structures that are necessary for dumping the data out to a bitmap file.

- The bitmap driver implements the OEMEndDoc DDI hook to dump the buffered bitmap data to the spooler at the end of the print job. The driver's implementation of OEMEndDoc dumps the headers first and then the buffered data.

- The GrowBuffer helper function is called every time the buffer needs to be enlarged to hold the bitmap data.

- The bitmap driver renders multi-page documents into a single large output bitmap file.

This sample has the following limitations:

- The plug-in does not support landscape orientation although the option exists in the GPD.

- In 24bpp mode, documents larger than three pages will produce an extremely large bitmap (.bmp) file. Although the output is correct, you might not be able to view it because of memory limitations in the bitmap viewer.

### OEMPREAN

When preanalysis is enabled in the GPD (for more details about preanalysis, see the WDK documentation), Unidrv defines the surface as a banding surface but causes the first playback (that is, the first band) to be of the entire page. Unidrv performs this definition by setting the GDI clip window to the entire page. Unidrv enables all drawing commands to be hooked but returns before any drawing can be done. On the subsequent passes, Unidrv resets the clip window back to the original band size and bands normally.

In order for plugins to also take part in preanalysis, they can enable OEM preanalysis by specifying the `"*PreAnalysisOptions: 8"` parameter in the GPD. The OEM rendering plug-in must hook both `DrvStartBanding` and `DrvNextBand` when this mode is enabled in the GPD. The OEM rendering plug-in must test the `pptl` parameter of the `OEMStartBanding` call to determine whether preanalysis has been enabled on this page. If the `pptl` parameter is `NULL`, preanalysis has been enabled, and the OEM rendering plug-in should consider all drawing calls up to the first call to `OEMNextBand` to be part of preanalysis.

During these preanalysis phase drawing calls, the plug-in should not draw on the surface. The plug-in should also not call back into unidrv. You should use this phase only for analysis of the objects on the page. For example, certain printers need to handle black objects that intersect with color objects differently from black objects that appear by themselves. Other printers might need to halftone `StretchBlt` objects differently from `BitBlt` objects.
You should be aware that after preanalysis, you might get more calls than you saw in the normal rendering, because primitives that cross band boundaries yield two or more DDI calls after the preanalysis pass.
The sample uses the `bPreAnalysis` flag in the OEMPDEV structure to denote whether a particular pass is the preanalysis pass or the actual rendering pass. If the `bPreAnalysis` flag is set, it indicates that the current pass is the preanalysis pass. The sample implements all of the drawing function hooks in Ddihook.cpp. The sample uses the `OEMStartBanding` and `OEMNextBand` functions to respectively set and reset the `bPreAnalysis` flag. The sample also uses the `DBG_CLIPOBJ` debug macro function to dump the bounds of the clipping rectangle available for preanalysis in each of the drawing functions during the preanalysis pass. To view all of the debug output in the debugger, you must set the debug level to VERBOSE. The rest of the sample is identical to the Oemuni sample that illustrates a basic OEM rendering plug-in.

> [!NOTE]
> The surface used during preanalysis might differ from the surface passed in during the rendering pass.

### PTPCPIPR

This sample is an educational example that illustrates the basic functionality that a Unidrv plug-in for the `IPrintOemPrintTicketProvider` interface should implement.
The primary functionality of the plug-in is to support the PhotoPrintingIntent PrintSchema keyword. The PhotoPrintingIntent keyword is designed to provide a mechanism for photo printers to support enhancements that are specific to photo printing. An application or the user (through the print UI) can then specify whether a particular print job is a photo-printing job, in which case the device can apply appropriate enhancements.
The intent setting is communicated to the device driver in a PrintTicket. The actual enhancements made are device-specific, and as such, the driver has to map the high-level intent setting into specific output settings, like resolution or bpp. This plug-in sample demonstrates the conversion.
The plug-in implements the `ExpandIntentOptions` method of the `IPrintOemPrintTicketProvider` interface. This method is called with an input print-ticket. The plug-in checks if the print-ticket contains a photo-printing intent setting, and if so, removes the setting, while instead adding to the print-ticket output settings as mentioned above. The code sample uses MSXML6 for all PrintTicket XML handling.

### UINUIREP

This sample shows how a plug-in for a Unidrv-based driver can replace the default UI. It provides new property sheets for the printer property and printing preferences pages.
Use this sample as an example of implementing full UI replacement in a Unidrv-based plugin. It also demonstrates the use of the new `IPrintCoreHelper` interface.

This sample is only provided for educational purposes and should not be used in a production environment.
