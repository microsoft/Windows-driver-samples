SpbTestTool
===========

The SpbTestTool sample serves two purposes. First, it demonstrates how to open a handle to the [SPB controller](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698220), use the SPB interface from a KMDF driver, and employ GPIO [passive-level interrupts](http://msdn.microsoft.com/en-us/library/windows/hardware/hh451035). Second, it implements a set of commands for communicating with a peripheral device to aid in debugging.

This sample is incomplete as a driver and merely demonstrates use of the [SPB I/O request interface](http://msdn.microsoft.com/en-us/library/windows/hardware/hh698224) and [GPIO interrupts](http://msdn.microsoft.com/en-us/library/windows/hardware/hh406467). It is not intended for use in a production environment.

### Run the sample

To install the SpbTestTool peripheral driver, follow these steps:

1.  Ensure that the driver builds without errors.

2.  Copy the SYS and INF files to a separate folder.

3.  Run Devcon.exe. You can find this program in the tools\\devcon folder where you installed the WDK. Type the following command in the command window:

    `devcon.exe update SpbTestTool.inf ACPI\<hwid>`

To launch the SpbTestTool application, follow these steps:

1.  Navigate to the directory that contains SpbTestTool.exe.

2.  Type the following command in the command window:

    `SpbTestTool.exe`

3.  By default, the SpbTestTool application uses the SpbTestTool sample driver. However, an alternate peripheral driver can be used instead. To specify an alternate driver, use the following format for the command line:

    `SpbTestTool.exe /p \\.\<alternate_path>`

4.  An input script can used instead of an interactive prompt. The script format requires one command per line. To run the script, use the following format for the command line in the command window:

    `SpbTestTool.exe /i <script.txt>`

### Executing commands

The SpbTestTool application loops indefinitely waiting for one of the following commands. The commands are translated to the appropriate SPB I/O request without any state tracking in the driver. Transfer status, buffer contents, and error codes are returned as necessary. Type `help` at any time to display this command list. Press Ctrl-C at any time to cancel the current command and exit the application.

Command | Description 
--------|------------
open | Open handle to SPB controller.
close | Close handle to SPB controller.
lock | Lock the bus for exclusive access.
unlock | Unlock the bus.
lockconn | Lock the shared connection for exclusive access. This command is used to synchronize bus transfers by the sample driver with op-region accesses by the ACPI firmware.
unlockconn | Unlock the shared connection.
write {} | Write a byte array to the peripheral device. Example: `> write {01, 02, 03}`
read <*numBytes*> | Read <*numBytes*> from the peripheral device. Example: `> read 5`
writeread {} <*numBytes*> | Atomically write a byte array to the peripheral device and read <*numBytes*> back. Example: `> writeread {01, 02, 03} 5`
signal | Inform the SpbTestTool driver that the interrupt has been handled.
help | Display the list of supported commands.
Ctrl-C | Press Ctrl-C at any time to cancel the outstanding command and exit the application.

### Code tour

The following are the relevant functions in the SpbTestTool peripheral driver for using the SPB interface from a KMDF driver.

Function | Description 
---------|------------
OnPrepareHardware | Traverses the driver's start resources and caches the connection ID of the I2C or SPI resource. This ID will be used to open the SPB controller later on.
SpbPeripheralOpen | Opens a handle to the underlying SPB controller via the resource hub. This allows the peripheral driver to be developed without any underlying knowledge of the platform or hardware connections. Instead, the dependency between controller and peripheral is described in ACPI.
SpbPeripheralClose | Sends IOCTL_SPB_LOCK_CONTROLLER to the SPB controller to lock the bus for exclusive access by this peripheral.
SpbPeripheralLock | Sends IOCTL_SPB_LOCK_CONTROLLER to the SPB controller to lock the bus for exclusive access by this peripheral.
SpbPeripheralUnlock | Sends IOCTL_SPB_UNLOCK_CONTROLLER to the SPB controller to unlock the bus from exclusive access by this peripheral.
SpbPeripheralLockConnection | Sends IOCTL_SPB_LOCK_CONNECTION to the SPB controller to lock the shared connection for exclusive access by this target (file handle).
SpbPeripheralUnlockConnection | Sends IOCTL_SPB_UNLOCK_CONNECTION to the SPB controller to unlock the shared connection from exclusive access by this target (file handle).
SpbPeripheralRead | Sends a read request to the SPB controller.
SpbPeripheralWrite | Sends a write request to the SPB controller.
SpbPeripheralWriteRead | Builds a write-read sequence and sends IOCTL_SPB_EXECUTE_SEQUENCE to the SPB controller.
SpbPeripheralOnComplete | Completion callback for all I/O requests.

The following are the relevant functions in the SpbTestTool peripheral driver for managing GPIO passive-level interrupts from a KMDF driver.

Function | Description 
---------|------------
OnPrepareHardware | Traverses the driver's start resources. If "ConnectInterrupt" is set to 1 in the registry, the driver connects the first interrupt resource found and registers an interrupt service routine.
OnInterruptIsr | The interrupt service routine, which has been configured to run at passive-level. Doing so enables the driver to acknowledge or quiesce the interrupt using the SPB interface, which cannot be called at DIRQL. Typically a driver will clear the hardware interrupt and save any volatile information in its ISR, and then it will queue a workitem to continue processing. Our sample driver instead notifies the SpbTestTool app that an interrupt has occurred and calls KeWaitForSingleObject to wait until the interrupt is handled before returning. A "real" driver should never stall in the ISR like this.
SpbPeripheralWaitOnInterrupt | Called to pend a WaitOnInterrupt request in the driver, which will be completed when the next interrupt occurs.
SpbPeripheralInterruptNotify | Completes an outstanding WaitOnInterrupt request to inform the SpbTestTool app that an interrupt has occurred.
SpbPeripheralSignalInterrupt | Notifies the interrupt service routine that the interrupt has been handled and the ISR should return.

### File manifest

The following source files are in the \\SpbTestTool\\sys folder and are used to build the SpbTestTool.sys and SpbTestTool.inf files.

File | Description 
-----|------------
driver.h, driver.cpp | Events on the Device Object, and read, write, and IOCTLs from the SpbTestTool application. Implements the driver's interrupt service routine.
internal.h | Common includes and typedefs
makefile | Redirects to the real makefile that is shared by all components of the WDK.
peripheral.h, peripheral.cpp | Reflection of the SpbTestTool IOCTLs to the SPB API, including opening the controller via the resource hub and using lock, unlock, read, write, and sequence.
resource.rc | Resource descriptor file used for versioning
sources | Lists source files and build options.
sources.dep | Defines build dependencies.
spbtesttool.asl | Sample ASL file for a peripheral device node. It declares I2C and GPIO interrupt resources. Note each macro specifies an ACPI path to describe direct dependencies.
spbtesttool.h | Private SpbTestTool IOCTLs for use between the application and peripheral driver, and driver path names.
spbtesttool.inx | Describes the installation of the driver. The build process converts this into an INF.
trace.h | Sets up WPP tracing.

The following source files are in the \\SpbTestTool\\exe folder and are used to build the SpbTestTool.exe file.

File | Description 
-----|------------
command.h, command.cpp | Classes respresenting each of the SpbTestTool commands. For the list of commands, see Executing commands.
internal.h | Common includes and function definitions
main.cpp | Application entry point, input parsing, and main execution loop. Also contains the interrupt notification thread.
makefile | Redirects to the real makefile that is shared by all components of the WDK.
sources | Lists source files and build options.
util.cpp | Helper functions


