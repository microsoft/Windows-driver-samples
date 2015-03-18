Serenum sample
==============

Serenum enumerates Plug-n-Play RS-232 devices that are compliant with the current revision of Plug and Play External COM Device. It loads as an upper filter driver to many different RS-232 device drivers that are compliant with its requirements and performs this service for them.

Serenum implements the Serenum service; its executable image is serenum.sys.

Serenum is an upper-level device filter driver that is used with a serial port function driver to enumerate the following types of devices that are connected to an RS-232 port:

-   Plug and Play serial devices that comply with Plug and Play External COM Device Specification, Version 1.00, February 28, 1995.
-   Pointer devices that comply with legacy mouse detection in Windows.

The combined operation of Serial and Serenum provides the function of a Plug and Play bus driver for an RS-232 port. Serenum supports Plug and Play and power management.

Windows provides Serenum to support Serial and other serial port function drivers that need to enumerate an RS-232 port. Hardware vendors do not have to create their own enumerator for RS-232 ports. For example, a device driver can use Serenum to enumerate the devices that are attached to the individual RS-232 ports on a multiport device.

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">File manifest
Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">Enum.c
Functions that enumerate external serial devices—the main purpose of this driver</td>
<td align="left">Pnp.c
Plug and Play support code</td>
</tr>
</tbody>
</table>

For more information, see [Features of Serial and Serenum](http://msdn.microsoft.com/en-us/library/windows/hardware/ff546505).

