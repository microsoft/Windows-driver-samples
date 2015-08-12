USB Function Client Driver
==========================

This is a skeleton sample driver that shows how to create a Windows USB function controller driver using the USB function class extension driver (UFX).

This sample demonstrates the following:

-   Registration with the UFX class extension driver
-   Handling USB transfers
-   Handling function controller events
-   Handling attach and detach notifications
-   Handling charger/port detection
-   Power management

Operating system requirements
-----------------------------

Windows 10 Mobile 

Customizing the sample for your device
--------------------------------------

This sample is not a functional driver. It is a skeleton driver intended to illustrate the general design of a UFX client driver.  The sample contains a number of comments prefaced with " #### TODO ", which indicates where code will need to be added to perform the controller operation as described in the comment.

Installation Note
-----------------

Installation on Windows 10 Mobile requires the creation of a package.  To properly interact with the USB UI on Windows 10 Mobile, the package must include a Security Element that specifies the ID_CAP_USB capability with DEVICE_READ and DEVICE_WRITE rights.
