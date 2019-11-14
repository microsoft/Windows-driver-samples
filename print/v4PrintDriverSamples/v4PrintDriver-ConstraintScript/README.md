---
page_type: sample
description: "Demonstrates how to implement advanced constraint handling and PrintTicket/PrintCapabilities handling using JavaScript."
languages:
- javascript
products:
- windows
- windows-wdk
---

# Print Driver Constraints Sample

This sample demonstrates how to implement advanced constraint handling, and also PrintTicket/PrintCapabilities handling using JavaScript.

The Constraints.js file in this sample demonstrates the implementation of JavaScript-based constraints to be used with a v4 print driver. The file implements the following two of the four functions used by JavaScript constraint files, as well as several helper functions:

- **ValidatePrintTicket** takes a given [IPrintSchemaTicket](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/printerextension/nn-printerextension-iprintschematicket) object and validates it for the current printer. The function may determine that the Print Ticket was already valid, modify the Print Ticket to make it valid, or determine that the Print Ticket is invalid and could not be made valid.

- **CompletePrintCapabilities** takes a given **IPrintSchemaTicket** object and the [IPrintSchemaCapabilities](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/printerextension/nn-printerextension-iprintschemacapabilities) object that was produced by the configuration module and augments it as needed. This can be used to establish positive constraint situations.

This sample does not demonstrate **ConvertPrintTicketToDevMode** or **ConvertDevModeToPrintTicket**, which utilize a property bag to store data in the private section of the DEVMODE structure.

> [!NOTE]
> This sample is for the v4 print driver model.

## Related topics

[Building a Driver with Visual Studio and the WDK](https://docs.microsoft.com/windows-hardware/drivers/develop/building-a-driver)

[IPrintSchemaCapabilities](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/printerextension/nn-printerextension-iprintschemacapabilities)

[IPrintSchemaTicket](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/printerextension/nn-printerextension-iprintschematicket)
