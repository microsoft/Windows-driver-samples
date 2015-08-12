Hyper-V Extensible Switch extension filter driver
=================================================

This sample contains a base library used to implement a Hyper-V Extensible Switch extension filter driver. This sample also contains two different extension filter drivers that were developed by using the library.

Hyper-V Extensible Switch extension filter drivers use the NDIS filter model. Every Hyper-V Extensible Switch has a corresponding Extension Protocol and Miniport instance. NDIS OIDs are leveraged to inform filter drivers on the driver stack about switch/port/NIC information. All packets originating from a switch port (External NIC, Synthetic NIC, Emulated NIC and Internal NIC) are first issued as a send from the Extension Protocol, which populates the packet with source information. This corresponds to the ingress data flow. Source based filtering, packet modification, packet queuing, and destination definition can occur at this stage. If the packets arrive at the Extension Miniport, they pass through the built in filtering/forwarding logic. If the packets pass filtering and must be delivered to any destination ports, they are issued as an indication (receive) from the Extension Miniport, which populate the packet with the source and destination information. This corresponds to the egress data flow. Destination based filtering can occur at this stage. If the packets arrive at the Extension Protocol, they are delivered to the defined destinations that were not marked as excluded by filtering logic. The packets then are completed back in the reverse order, as a receive completion first and then a send completion. Extension filter drivers are free to generate new packets on the ingress data path by issuing a send from their filter.

The base library provided, *SxBase.lib*, implements the necessary NDIS functionality common to all types of extension filter drivers. It is not necessary to make any changes to this base library. To implement your own extension filter driver, you only need to define all global variables and implement all functions that are found in *SxApi.h*.

MsPassthroughExt is a basic filtering extension filter driver that is implemented by using SxBase.lib. This demonstrates the bare minimum that must be implemented to use *SxBase.lib*. Installing and enabling MsPassthroughExt on the Hyper-V Extensible Switch does not affect switch behavior.

MsForwardExt is a basic forwarding extension filter driver that is implemented by using *SxBase.lib*. MsForwardExt uses basic MAC forwarding and custom switch policy to allow sends from given MAC addresses. This forwarding sample implements Hybrid Forwarding, which means that the destination table is not populated by this sample if the packet is flagged as a Hyper-V Network Virtualization (HNV) packet. HNV flagged packets' destination tables are computed by the vSwitch HNV policies instead. If this extension filter driver is unconfigured, it will block sends from all VMs, but will maintain connectivity to the host. Each switch policy, which is defined in *MsForwardExtPolicy.mof*, is a MAC address. Applying a switch policy to MsForwardExt allows packets to be sent from the MAC address that is defined in the policy.


Installation
------------

Use the *install.cmd* script provided with each extension filter driver. The *install.cmd* uses **netcfg** to install the extension and **mofcomp** to register any required mof files. The PowerShell cmdlet *Enable-VmSwitchExtension* can then be used to enable the extension filter driver on a Hyper-V Extensible Switch.

For more information on Hyper-V Extensible Switch extensions, see [Hyper-V Extensible Switch](http://msdn.microsoft.com/en-us/library/windows/hardware/hh598161).

