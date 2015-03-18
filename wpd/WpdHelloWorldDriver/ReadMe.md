WPDHelloWorld sample driver for portable devices
================================================

The WpdHelloWorld sample driver supports four objects: a device object, a storage object, a folder object, and a file object. Each object supports corresponding properties. These properties are defined in the file WpdObjectProperties.h.

The sample driver supports a device object that exposes ten read-only properties. These properties, their types, and their values are listed in the following table.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Property name
Property type
Value</td>
<td align="left">DEVICE_PROTOCOL
String
&quot;Hello World Protocol ver 1.00&quot;</td>
<td align="left">DEVICE_FIRMWARE_VERSION
String
&quot;1.0.0.0&quot;</td>
</tr>
</tbody>
</table>

The driver supports a storage object that exposes six read-only properties. These properties, their types, and their values are listed in the following table.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Property name
Property type
Value</td>
<td align="left">STORAGE_CAPACITY
64-bit Integer
1024 * 1024</td>
<td align="left">STORAGE_FREE_SPACE_IN_BYTES
64-bit Integer
(same as above)</td>
</tr>
</tbody>
</table>

The driver supports a folder object that exposes three read-only properties. These properties, their types, and their values are listed in the following table.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Property name
Property type
Value</td>
<td align="left">WPD_OBJECT_DATE_MODIFIED
Date
2006/6/26 5:0:0.0</td>
<td align="left">WPD_OBJECT_DATE_CREATED
Date
2006/1/25 12:0:0.0</td>
</tr>
</tbody>
</table>

The driver supports a file object that exposes three read-only properties. These properties, their types, and their values are listed in the following table.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Property name
Property type
Value</td>
<td align="left">WPD_OBJECT_DATE_MODIFIED
Date
2006/6/26 5:0:0.0</td>
<td align="left">WPD_OBJECT_DATE_CREATED
Date
2006/1/25 12:0:0.0</td>
</tr>
</tbody>
</table>

In addition to the above properties, every object (for example, device, storage, folder, or file) also supports seven common WPD object properties. These are read-only properties that contain object-specific values for the most part. These properties, their types, and their values are listed in the following table.

<table>
<colgroup>
<col width="33%" />
<col width="33%" />
<col width="33%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left">Property name
Property type
Value</td>
<td align="left">WPD_OBJECT_ID
String
Object-specific</td>
<td align="left">WPD_OBJECT_PERSISTENT_UNIQUE_ID
String
Object-specific</td>
</tr>
</tbody>
</table>

For a complete description of this sample and its underlying code and functionality, refer to the [WPD HelloWorld Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/) description in the Windows Driver Kit documentation.

Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/)
