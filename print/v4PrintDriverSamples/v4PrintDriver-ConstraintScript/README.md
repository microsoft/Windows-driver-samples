Print Driver Constraints Sample
===============================

This sample demonstrates how to implement advanced constraint handling, and also PrintTicket/PrintCapabilities handling using JavaScript.

The Constraints.js file in this sample demonstrates the implementation of JavaScript-based constraints to be used with a v4 print driver. The file implements the following two of the four functions used by JavaScript constraint files, as well as several helper functions:

-   **ValidatePrintTicket** takes a given [IPrintSchemaTicket](http://msdn.microsoft.com/en-us/library/hh451398(v=vs.85).aspx) object and validates it for the current printer. The function may determine that the Print Ticket was already valid, modify the Print Ticket to make it valid, or determine that the Print Ticket is invalid and could not be made valid.
-   **CompletePrintCapabilities** takes a given **IPrintSchemaTicket** object and the [IPrintSchemaCapabilities](http://msdn.microsoft.com/en-us/library/hh451256(v=vs.85).aspx) object that was produced by the configuration module and augments it as needed. This can be used to establish positive constraint situations.

This sample does not demonstrate **ConvertPrintTicketToDevMode** or **ConvertDevModeToPrintTicket**, which utilize a property bag to store data in the private section of the DEVMODE structure.

**Note** This sample is for the v4 print driver model.

Related topics
--------------

[Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644)

[IPrintSchemaCapabilities](http://msdn.microsoft.com/en-us/library/hh451256(v=vs.85).aspx)

[IPrintSchemaTicket](http://msdn.microsoft.com/en-us/library/hh451398(v=vs.85).aspx)

