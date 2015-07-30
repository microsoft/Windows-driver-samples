WPD WUDF sample driver
======================

The comprehensive WPD sample driver (WpdWudfSampleDriver) demonstrates virtually all aspects of the Microsoft Windows Portable Devides (WPD) device driver interface (DDI). This driver is built as a normal User-Mode Driver Framework (UMDF) driver that also processes the WPD command set. Although this driver does not interact with actual hardware, it simulates communicating with a device that supports phone contacts, pictures, music, and video.

This driver was written in the simplest way to demonstrate concepts. Therefore, the sample driver might perform operations or be structured in a way that are inefficient in a production driver. Additionally, this sample does not use real hardware. Instead, it simulates a device by using data structures in memory. Therefore, the driver might be implemented in a way that is unrealistic for production hardware.

Some of the tasks that are accomplished by the WpdWudfSampleDriver are written for the advanced Windows Portable Devices (WPD) driver developer.

For a complete description of this sample and its underlying code and functionality, refer to the [WPD WUDF Sample Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597723) description in the Windows Driver Kit documentation.


Related topics
--------------

[WPD Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597864)

[WPD Driver Development Tools](http://msdn.microsoft.com/en-us/library/windows/hardware/ff597568)

[WPD Programming Guide](https://msdn.microsoft.com/en-us/library/windows/hardware/ff597898)
