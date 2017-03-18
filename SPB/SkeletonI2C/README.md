Skeleton I2C Sample Driver
=========================

The SkeletonI2C sample demonstrates how to design a KMDF controller driver for Windows that conforms to the [simple peripheral bus](http://msdn.microsoft.com/en-us/library/windows/hardware/hh450903) (SPB) device driver interface (DDI). SPB is an abstraction for low-speed serial buses (for example, I<sup>2</sup>C and SPI) that allows peripheral drivers to be developed for cross-platform use without any knowledge of the underlying bus hardware or device connections. While this sample implements an empty I<sup>2</sup>C driver, it could just as easily be the starting point for an SPI driver with only minor modifications.

Note that the SkeletonI2C sample is simplified to show the overall structure of an SPB controller, but contains only the code that the driver requires to communicate with the [SPB framework extension (SpbCx)](http://msdn.microsoft.com/en-us/library/windows/hardware/hh406203) and KMDF. The SkeletonI2C sample driver omits all hardware-specific code. It does not simulate data transfers or implement request completion asynchronously. Pay close attention to code comments marked with "TODO" that refer to blocks of code that must be removed or updated.

The simplified structure of the SkeletonI2C sample driver makes it a convenient starting point for development of a real SPB controller driver that manages the hardware functions in an SPB controller.

Modifying the sample
--------------------

Here are some high-level points to consider when modifying the SkeletonI2C sample for use on real hardware:

-   Edit (and likely rename) Skeletoni2c.h to describe your hardware's register set.
-   Modify Controller.cpp and Device.cpp to translate the SPB DDI and primitives into I<sup>2</sup>C or SPI protocol for your hardware. This includes initialization, I/O configuration, and interrupt processing.
-   Address any comments marked with "TODO" in the sample, especially those that short circuit the I/O path to complete requests synchronously.
-   Modify the HWID (`ACPI\skeletoni2c`) in Skeletoni2c.inf to match the device node in your firmware.
-   Generate and specify a unique trace GUID in I2ctrace.h.
-   Refactor the driver name, functions, comments, etc., to better describe your implementation.

Code tour
---------

The following are relevant functions in the SkeletonI2C driver for implementing the SPB DDI.

Function

Description

INITIALIZATION

`OnDeviceAdd`

Within `OnDeviceAdd`, the driver makes several configuration calls for SPB.

[**SpbDeviceInitConfig**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh450918) must be called before creating the WDFDEVICE. Note that SpbCx sets a default security descriptor on the device object, but the controller driver can override it by calling [**WdfDeviceInitAssignSDDLString**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff546035) after **SpbDeviceInitConfig**.

After creating the WDFDEVICE, the driver configures it appropriately for SPB by calling [**SpbDeviceInitialize**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh450919). Here the driver also sets the target and request attributes.

Finally the driver configures a WDF system-managed idle time-out.

TARGET CONNECTION

`OnTargetConnect`

Invoked when a client opens a handle to the specified SPB target. Queries the I<sup>2</sup>C connection parameters from the resource hub (via SPB) and initializes the target context.

SPB I/O CALLBACKS

`OnRead`

SPB read callback. Invokes the `PbcConfigureForNonSequence` function to set up the transfer.

`OnWrite`

SPB write callback. Invokes the `PbcConfigureForNonSequence` function to set up the transfer.

`OnSequence`

SPB sequence callback. Configures the controller for an atomic transfer\*.

`OnControllerLock`

SPB lock controller callback. Configures to handle subsequent I/O as an atomic transfer\*. For I<sup>2</sup>C the controller should place a start bit on the bus. For SPI the controller should assert the chip-select line. The driver may choose to carry this out as part of this callback or defer until the first I/O operation is received (the next call to `OnRead` or `OnWrite`).

`OnControllerUnlock`

SPB unlock controller callback. Marks the end of an atomic transfer\*. For I<sup>2</sup>C, the controller should place a stop bit on the bus. For SPI, the controller should de-assert the chip-select line.

SPB HELPER METHODS

`PbcConfigureForIndex`

Configures the request context for the specified transfer index. This could be a single I/O or part of a sequence.

`PbcRequestComplete`

Sets the number of bytes completed for a request and invokes the [**SpbRequestComplete**](http://msdn.microsoft.com/en-us/library/windows/hardware/hh450920) method.

\*An atomic transfer in SPB is implemented using Sequence or a Lock/Unlock pair. For I<sup>2</sup>C, this means a set of reads and writes with restarts in between. For SPI, this means a set of reads and writes with the chip select-line asserted throughout.

The following are relevant functions in the SkeletonI2C driver for implementing controller-specific I2C protocol. For the most part, these are placeholders and must be filled in appropriately.

Function

Description

INITIALIZATION

`ControllerInitialize`

One-time controller initialization. Prepare FIFOs, clocks, interrupts, etc.

`ControllerConfigureForTransfer`

Per-I/O controller configuration. Depending on the type of I/O (and whether its part of an ongoing atomic transfer), the driver may need to configure direction, set interrupts, etc.

Additionally, for I<sup>2</sup>C, the driver may need to insert a start, restart, or stop bit as necessary, and for SPI the driver may need to assert or de-assert the chip select line.

I/O PROCESSING

`OnInterruptIsr`

Interrupt callback. Acknowledges interrupts and saves state as necessary. Queues a DPC for processing.

`OnInterruptDpc`

DPC callback. Processes saved interrupts. If necessary the request is completed.

`ControllerProcessInterrupts`

Handles processing for both normal and error condition interrupts. Invokes `ControllerCompleteTransfer`() as appropriate.

`ControllerCompleteTransfer`

Invoked when an I/O completes or an error is detected. If this I/O is part of a sequence, `PbcRequestConfigureForIndex`() is called to prepare the next I/O; otherwise, the request is marked for completion.
