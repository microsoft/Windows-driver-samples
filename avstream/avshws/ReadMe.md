AVStream simulated hardware sample driver (Avshws)
==================================================

The AVStream simulated hardware sample driver (Avshws) provides a pin-centric [AVStream](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554240) capture driver for a simulated piece of hardware. This streaming media driver performs video captures at 320 x 240 pixels in either RGB24 or YUV422 format using direct memory access (DMA) into capture buffers. The purpose of the sample is to demonstrate how to write a pin-centric AVStream minidriver. The sample also shows how to implement DMA by using the related functionality provided by the AVStream class driver.

This sample features enhanced parameter validation and overflow detection.

## Universal Compliant
This sample builds a Windows Universal driver. It uses only APIs and DDIs that are included in Windows Core.

