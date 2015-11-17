Copyright (c) 2011 Microsoft Corporation

Author: Cody Hartwig (chartwig) 9-Jun-2011

------

This document describes the chDMA controller.  chDMA is a 32-channel,
32-request line DMA controller for which a sample HAL extension has been
implemented.  This document also describes a sample peripheral device and the
configuration required for use with the DMA controller.

General Controller Information
==============================

Base Physical Address                     0x70001000
GSI                                       0x27
Number of Channels                        32
Number of Request Lines                   32
Cache Coherent                            No
Supports ReadDmaCounter                   Yes


Register Layout
===============

CH_DMA_CONTROL (Offset: 0x00, RW, Reset: 0x00000000)
--------------

This register sets controller-wide settings such as controller enable.

Bit              Function
31               Controller Enable: 1-enable, 0-disable
30-0             Reserved

CH_DMA_STATUS (Offset: 0x04, RW1C, Reset: 0x00000000)
-------------

This register describes which channels are currently asserting an interrupt.
Interrupts are cleared by writing 1 to the appropriate bit.

Bit                 Function
31                  Channel 31 interrupt status
30                  Channel 30 interrupt status
..
0                   Channel 0 interrupt status

CH_DMA_INTERRUPT_MASK (Offset: 0x08, RW, Reset: 0x00000000)
---------------------

A channel interrupt is unmasked by writing 1 to the corresponding bit in this
register.

Bit                 Function
31                  Channel 31 interrupt mask
30                  Channel 30 interrupt mask
..
0                   Channel 0 interrupt mask

CH_DMA_CHANNEL_i_CONTROL (Offset: 0x10 + (i * 0x10), RW, Reset:0x00000000)
------------------------

This register controls per channel operation of the controller.  A transfer
may begin once the enable bit is set.  When the enable bit is cleared, the
current burst is finished and the transfer will stop.

Bit               Function
31                Enable
30                Interrupt on completion
29-28             Device Width:
                      0b00 - 8-bit
                      0b01 - 16-bit
                      0b10 - 32-bit
                      0b11 - Reserved
27-26             Burst Size:
                      0b00 - 1 Word
                      0b01 - 2 Word
                      0b10 - 4 Word
                      0b11 - 8 Word
25                    Flow Control: 1-enable, 0-disable
24                    Loop: 1-repeat transfer, 0-transfer once
23-19             Request line trigger
18                Transfer Direction: 1-device to mem, 0-mem to device
17-16             Reserved
15-0              Transfer Length

CH_DMA_CHANNEL_i_MEM_PTR (Offset: 0x14 + (i * 0x10), RW, Reset: 0x00000000)
------------------------

This register sets the memory-side address of the transfer.

Bit               Function
31-0              Memory-side address

CH_DMA_CHANNEL_i_DEV_PTR (Offset: 0x18 + (i * 0x10), RW, Reset: 0x00000000)
------------------------

This register sets the device-side address of the transfer.

Bit               Function
31-0              Device-side address

CH_DMA_CHANNEL_i_STATUS (Offset: 0x1c + (i * 0x10), RO, Reset: 0x00000000)
-----------------------

This register reports transfer status.

Bit               Function
31                Channel Busy
30-16             Reserved
15-0              Bytes Transferred



Programming Model
=================
Normal operation of the DMA controller is achieved by the following steps:

1. Enable the controller by writing 0x80000000 to the CH_DMA_CONTROL register.
2. Unmask channel 0's interrupt by or'ing 0x1 with the CH_DMA_INTERRUPT_MASK
   register
3. Program the memory-side transfer address into the CH_DMA_CHANNEL_0_MEM_PTR
   register.
4. Program the device-side transfer address into the CH_DMA_CHANNEL_0_DEV_PTR
   register.
5. Program the CH_DMA_CHANNEL_0_CONTROL register.  This enables the channel.


Peripheral Information
======================

For the purposes of this sample, the chDMA controller is connected to a single
UAART.  This UART requires the following DMA controller configuration to
work properly:

Parameter              Value
Bus Width              8 bits
Burst Size             1 Word
Request Line           0x15
Flow Control           Enabled
