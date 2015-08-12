Kernel Counter Sample (Kcs)
===========================

The Kcs sample driver demonstrates the use of the [kernel-mode performance library](http://msdn.microsoft.com/en-us/library/windows/hardware/ff548159). The sample driver does not control any hardware; it simply provides example code that demonstrates how to provide counter data from a kernel-mode driver. The code contains comments to explain what each function does. The sample creates geometric wave and trigonometric wave counter sets.

This module contains sample code to demonstrate how to provide counter data from a kernel driver.

This sample driver should not be used in a production environment.

The Microsoft Windows operating system allows system components and third parties to expose performance metrics in a standard way by using [Performance Counters](http://msdn.microsoft.com/en-us/library/windows/hardware/aa373083). Kernel-mode PCW providers are installed in the system as Performance Counter Library (PERFLIB) (Version 2 providers), which allows their counters to be browsed, and allows for data collection and instance enumeration. Consumers can query KM PCW providers by using PDH and PERFLIB Version 1 without any modification to the consumer code.



