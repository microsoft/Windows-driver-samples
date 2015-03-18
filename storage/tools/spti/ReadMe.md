SCSI Pass-Through Interface Tool
================================

The SCSI Pass Through Interface sample demonstrates how to communicate with a SCSI device from Microsoft Win32 applications by using the **DeviceIoControl** API.

Installation and Operation
--------------------------

The storage port drivers provide an interface for Win32 applications to send SCSI CBDs (Command Descriptor Block) to SCSI devices. The interfaces are [**IOCTL\_SCSI\_PASS\_THROUGH**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560519) and [**IOCTL\_SCSI\_PASS\_THROUGH\_DIRECT**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560521). Applications can build a pass-through request and send it to the device by using this IOCTL.

Two command line parameters can be used with *SPTI.EXE*. The first parameter is mandatory. It is the name of the device to be opened. Typical values for this are drive letters such as "C:", or device names as defined by a class driver such as Scanner0, or the SCSI port driver name, ScsiN:, where N = 0, 1, 2, etc. The second parameter is optional and is used to set the share mode (note that access mode and share mode are different things) and sector size. The default share mode is (FILE\_SHARE\_READ | FILE\_SHARE\_WRITE) and the default sector size is 512. A parameter of "r" changes the share mode to only FILE\_SHARE\_READ. A parameter of "w" changes the share mode to only FILE\_SHARE\_WRITE. A parameter of "c" changes the share mode to only FILE\_SHARE\_READ and also changes the sector size to 2048. Typically, a CD-ROM device would use the "c" parameter.

