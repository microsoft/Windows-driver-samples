AddFilter Storage Filter Tool
=============================

Addfilter is a command-line application that adds and removes filter drivers for a given drive or volume. This application demonstrates how to insert a filter driver into the driver stack of a device. The sample illustrates how to insert such a filter driver by using the SetupDi API.

Installation and Operation
--------------------------

This initial sample does not check the filter for validity before it is added to the driver stack. If an invalid filter is added, the specified device might no longer be accessible.

The minifilter identifies implicit locks when it sees a non-shared write open request on a volume object. In this scenario, the minifilter closes its metadata file and sets a trigger that corresponds to the volume in its instance object. Later, each close operation is examined to identify if the implicit lock on the volume is being released and, if so, a re-open of the minifilter's metadata file is triggered.

**Important** If you attempt to add a non-existing filter to a boot device and then restart, the system might show the INACCESSIBLE\_BOOT\_DEVICE error message. If this message appears, you will be unable to start the computer. To fix this problem, when the startup menu is displayed when the computer starts up, go to the Advanced Options screen and select **Use Last Known Good Profile**.

The sample is intended for use with upper filter drivers only.

When you add a filter to a device, that device needs to be restarted. Depending on the device, you might also need to restart your computer. The RestartDevice function (in *Addfilter.c*) stops the specified device and then restarts it. If the device has been stopped but not restarted, and the computer is restarted, the restart will not necessarily restart the device. You will need to call the RestartDevice function to restart your device.

Because the sample currently enumerates only disk devices, the sample can operate only on devices of this class. To extend this sample code, you can add another command-line argument that handles other device types.

The following is the command line usage for addfilter:

**addfilter [/listdevices] [/device device\_name] [/add filter] [/remove filter]**

If the device name is not supplied, settings will apply to all devices. If there is no /add or /remove argument, a list of currently installed drivers will be printed.

