Serial Port Driver
==================

The Serial (16550-based RS-232) sample driver is a WDF version of the inbox Serial.sys driver in %WINDIR%\\system32\\drivers.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

This sample driver is functionally equivalent to the inbox driver, with these two exceptions:

1.  This sample does not support multi-function serial devices.
2.  This sample does not support legacy serial ports. Legacy ports are not detected by the BIOS, and are, therefore, not enumerated by the operating system.

The Serial sample driver runs in kernel mode.

This sample driver supports power management. When a serial port is not in use, the driver places the port hardware in a low-power state. When the port is opened, it receives power and wakes up. The driver supports wake-on-ring for platforms that support this function. The driver can be compiled to run on both 32-bit and 64-bit versions of Windows.

For more information, see [Features of Serial and Serenum](http://msdn.microsoft.com/en-us/library/windows/hardware/ff546505).

This sample can be used for these hardware IDs without any modification to the .inx file included in the project.

-   PNP0501
-   PNP0500

    If you have other hardware such as an add-in card, then you must add the hardware ID in the .inx as shown in this example. Then, you must build the project as per the instructions given in the Building the sample section in this readme.

    ```
    ; For XP and later
    [MSFT.NTamd64]
    ; DisplayName           Section           DeviceId
    ; -----------           -------           --------
    %PNP0500.DevDesc%=       Serial_Inst,      *PNP0500, *PNP0501 ; Communications Port
    %PNP0501.DevDesc%=       Serial_Inst,      *PNP0501, *PNP0500 ; Communications Port
    %PNP0501.DevDesc%=       Serial_Inst,      MF\PCI9710_COM ; Communications Port
    ```
