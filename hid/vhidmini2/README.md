HID Minidriver Sample (UMDF V2)
======================================
The *HID minidriver* sample demonstrates how to write a HID minidriver using User-Mode Driver Framework (UMDF).

The sample demonstrates how to communicate with an HID minidriver from an HID client using a custom-feature item in order to control certain features of the HID minidriver. This is needed since other conventional modes for communicating with a driver, like custom IOCTL or WMI, do not work with the HID minidriver. The sample also is useful in testing the correctness of a HID report descriptor without using a physical device. 


Related topics
--------------

[Creating UMDF-based HID Minidrivers](http://msdn.microsoft.com/en-us/library/windows/hardware/hh439579)

[Human Input Devices Design Guide](http://msdn.microsoft.com/en-us/library/windows/hardware/ff539952)

[Human Input Devices Reference](http://msdn.microsoft.com/en-us/library/windows/hardware/ff539956)

[UMDF HID Minidriver IOCTLs](http://msdn.microsoft.com/en-us/library/windows/hardware/hh463977)

