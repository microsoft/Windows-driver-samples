NDIS Virtual Miniport Driver
============================

The NDIS Virtual Miniport Driver sample illustrates the functionality of an NDIS miniport driver without requiring a physical network adapter.

Because the driver does not interact with any hardware, it makes it easier to understand the miniport interface and the usage of various NDIS functions without the clutter of hardware-specific code that is normally found in a fully functional driver. The driver can be installed either manually using the Add Hardware wizard as a root enumerated virtual miniport driver or on a virtual bus (like toaster bus).

This sample driver demonstrates an NDIS virtual miniport driver. If a single instance of the virtual miniport exists, it simply drops the send packets and completes the send operation successfully. If there are multiple virtual miniport instances, the instances behave as if they were multiple network interface cards (NICs) plugged into a single Ethernet hub. This "hub" indicates the incoming send packets to all of the virtual miniport instances.

To test the miniport driver, install more than one miniport driver instance. You can repeat the installation to install more than one instance of the miniport.

**Note** This sample provides an example of minimal driver intended for education purposes. The driver and its sample test programs are not intended for use in a production environment.

For more information on creating NDIS Miniport Drivers, see [NDIS Miniport Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff565949).

