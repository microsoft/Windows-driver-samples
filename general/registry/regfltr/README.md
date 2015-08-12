RegFltr Sample Driver
=====================

The RegFltr sample shows how to write a [registry filter driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff545879).In addition to providing some basic examples, this sample demonstrates the following:

-   How to handle transactional registry operations.
-   How and when to capture input parameters.
-   Issues and workarounds for version 1.0 of registry filtering.
-   Changes in version 1.1 of registry filtering.
-   How to use version 1 of the [**REG\_CREATE\_KEY\_INFORMATION**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560920) and [**REG\_OPEN\_KEY\_INFORMATION**](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560957) data structures.

The RegFltr sample contains several examples of user-mode and kernel-mode registry-filtering operations. Each example comes with its own corresponding registry callback routine, and performs the following steps:

1.  Does some setup work.
2.  Registers the callback routine.
3.  Performs one or more registry operations.
4.  Unregisters the callback routine.
5.  Verifies that the sample completed correctly.

The sample driver is a minimal driver that is not intended to be used on production systems. To keep the samples simple, the registry callback routines provided do not check for all possible situations and error conditions. This sample is designed to demonstrate typical scenarios and no other registry filtering driver is expected to be active.

