Radio Switch Test Driver for OSR USB-FX2 Development Board
==========================================================

This sample demonstrates how to structure a HID driver for radio switches for the OSR USB-FX2 Development Board.

Starting with Windows 8.1, the hardware switch or button to control wireless transmission and the global software switch (Airplane mode switch) in the Radio Management User Interface must be synchronized. To ensure the hardware and software switches that control radio transmission are synchronized, the hardware switch or button must have a HID-compliant driver.


Testing
-------

### Testing Modes

The driver supports five modes representing the valid combinations of HID descriptors that we have defined for the USB forum. Modes are selected using switches 1, 2, and 3 on the switch pack.

### Switch Mapping

<table>
<colgroup>
<col width="25%" />
<col width="25%" />
<col width="25%" />
<col width="25%" />
</colgroup>
<thead>
<tr class="header">
<th align="left">1
2
3
Mode</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td align="left">0
0
0
Mode 1</td>
<td align="left">0
0
1
Mode 1</td>
<td align="left">0
1
0
Mode 2</td>
<td align="left">0
1
1
Mode 3</td>
</tr>
</tbody>
</table>

### Mode 1 Radio Push Button

In this mode the radio switch (switch 8 on the switch pack) represents a momentary push button. A HID report is generated when the switch transitions to the On state (switch down).

### Mode 2 Radio Push Button & LED

In this mode the radio switch (switch 8 on the switch pack) represents a momentary push button. A HID report is generated when the switch transitions to the On state (switch down). In addition, the LED array will reflect the state of the Radio LED: either all on or all off (note that the top 2 LEDs never illuminate as these are not wired in).

### Mode 3 Radio Slider Switch

In this mode the radio switch (switch 8 on the switch pack) represents a momentary push button. A HID report is generated when the switch transitions to the On state (switch down).

### Mode 4 Radio Slider Switch & LED

In this mode the radio switch (switch 8 on the switch pack) represents an A-B slider switch. A HID report is generated in both cases: when the switch transitions to the On state (switch down) and to the Off state (switch up). In addition the LED array will reflect the state of the Radio LED: either all on or all off (note that the top 2 LEDs never illuminate as these are not wired in).

### Mode 5 LED only

In this mode the radio switch (switch 8 on the switch pack) is ignored. The LED array will reflect the state of the Radio LED: either all on or all off (note that the top 2 LEDs never illuminate as these are not wired in).

