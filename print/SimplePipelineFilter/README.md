---
page_type: sample
description: "This sample shows how to use the print pipeline's filter interfaces."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Print Pipeline Simple Filter

The printing system supports a print filter pipeline. The pipeline is run when a print job is consumed by the print spooler and sent to the device.

This sample shows how to use the print pipeline's filter interfaces.

The filters in the print pipeline consume a certain data type and produce a certain data type. This information is specified in the pipeline configuration file on a per printer driver basis.

The WDK print filter sample contains two filter samples: one that consumes and produces XPS data type, and the other one consumes and produces opaque byte stream. For more information, see [XPS Printer Driver (XPSDrv)](https://docs.microsoft.com/windows-hardware/drivers/print/xpsdrv-printer-driver).
