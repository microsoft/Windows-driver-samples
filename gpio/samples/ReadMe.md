GPIO Sample Drivers
===================

The GPIO samples contain annotated code to illustrate how to write a [GPIO controller driver](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439509) that works in conjunction with the [GPIO framework extension](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439512) (GpioClx) to handle GPIO I/O control requests, and a peripheral driver that runs in kernel mode and uses GPIO resources. For a sample that shows how to write a GPIO peripheral driver that runs in user mode, please refer to the SPB accelerometer sample driver (SPB\\peripherals\\accelerometer).

The GPIO sample set contains the following three samples.

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">Minifilter Sample
Description</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left"><p><em>SimGpio</em></p>
<p>The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx device driver interface (DDI). The GPIO controller driver is written for a hypothetical memory-mapped GPIO controller (simgpio). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.</p></td>
<td align="left"><p><em>SimGpio_I2C</em></p>
<p>The files in this sample contain the source code for a GPIO controller driver that communicates with GpioClx through the GpioClx DDI. In contrast to the SimGpio sample, the GPIO controller in this sample is not memory-mapped. The GPIO controller driver is written for a hypothetical GPIO controller that resides on an I<sup>2</sup>C bus (simgpio_i2c). The code is meant to be purely instructional. An ASL file illustrates how to specify a GPIO interrupt and I/O descriptor in the ACPI firmware.</p></td>
</tr>
</tbody>
</table>

