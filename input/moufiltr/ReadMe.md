Mouse Input WDF Filter Driver (Moufiltr)
========================================
The Moufiltr sample is an example of a mouse input filter driver.

This sample is WDF version of the original WDM filter driver sample. The WDM version of this sample has been deprecated.

This driver filters input for a particular mouse on the system. In its current state, it only hooks into the mouse packet report chain and the mouse ISR, and does not do any processing of the data that it sees. (The hooking of the ISR is only available in the i8042prt stack.) With additions to this current filter-only code base, the filter could conceivably add, remove, or modify input as needed.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

Installation
------------

This sample is installed via an .inf file. The .inf file included in this sample is designed to filter a PS/2 mouse.

The .inf file must install the class driver (Mouclass) and the port driver (i8042prt, Mouhid, Sermouse, etc.) by using Msmouse.inf and the INF directives "Needs" and "Include".

The .inf file must add the correct registry values for the class and port driver, as well as using the new directives.

