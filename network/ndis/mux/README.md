NDIS MUX Intermediate Driver and Notify Object
==============================================

The MUX Intermediate Miniport (IM) driver is an NDIS 6.0 driver that demonstrates the operation of an "N:1" MUX driver.The sample demonstrates creating multiple virtual network devices on top of a single lower adapter. Protocols bind to these virtual adapters as if they are real adapters. Examples of Intermediate Miniport drivers that can use this framework are Virtual LAN (VLAN) drivers. Included in the project is a sample Notify Object that demonstrates how to write a notify object for installing and configuring an NDIS MUX intermediate miniport (IM) driver that implements an N:1 relationship between upper and lower bindings, for example, it creates multiple virtual network devices on top of a single lower adapter. Protocols bind to these virtual adapters as if they are real adapters. Examples of Intermediate Miniport drivers that can use this type of notify object are Virtual LAN (VLAN) drivers.

For more information, see [NDIS Intermediate Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff565773) in the network devices design guide.

INSTALLING THE SAMPLE
---------------------

MUX is installed as a protocol (called *Sample Mux-IM Protocol Driver* in the supplied INFs/notification object).

To install, follow the steps below:

1.  Prepare an installation directory that contains these files: muxp.inf, mux\_mp.inf, mux.sys and mux.dll (notification object DLL, built in this DDK at network\\ndis\\mux\\notifyob).
2.  On the desktop, right-click the **My Network Places** icon and choose **Properties**.
3.  Right-click on the relevant **Local Area Connection** icon and choose **Properties**.
4.  Click **Install**, then **Protocol**, then **Add**, then **Have Disk**.
5.  Browse to the drive/directory containing the files listed above. Click **OK**. This should show **Sample Mux-IM Protocol Driver** in a list of Network Protocols. Highlight this and click **OK**. This should install the MUX driver.
6.  Click **OK** or **Yes** each time the system prompts with a warning regarding installation of unsigned files. This is necessary because binaries generated via the DDK build environment are not signed.

Two .INF files are needed rather than one because MUX is installed both as a protocol and a miniport.

NDIS MUX Intermediate Driver
----------------------------

The driver binds to Ethernet (NdisMedium802\_3) adapters as a protocol, and exposes one or more virtual Ethernet devices over each lower adapter, based on its configuration. The term "VELAN" is used to denote a Virtual Ethernet LAN adapter implemented by this driver.

When it binds to a lower adapter, MUX reads the standard "UpperBind" key to obtain a list of VELANs configured over this adapter. For each such VELAN, it calls NdisIMInitializeDeviceInstanceEx() to instantiate the NDIS miniport for the VELAN. NDIS then calls the driver's MiniportInitialize (MPInitialize) routine to start the VELAN miniport.

The MUX driver supports configuring the MAC address for each VELAN miniport using the standard "NetworkAddress" key that it reads from its MiniportInitialize routine. If this is not configured, it computes a "locally significant" MAC address for the VELAN using the MAC address of the lower adapter. The MUX driver sets its lower adapter to promiscuous mode in order to be able to receive frames directed to any of the VELAN MAC addresses. However it does implement packet-filtering (and multicast address filtering) logic for all its VELAN miniports so that it only passes up relevant frames on each VELAN. This aspect of the driver may be modified if, for example, your driver design uses the same MAC address as that of the lower adapter on all VELANs. With such a modification, it is not required to set the lower adapter to promiscuous mode and incur the costs of receiving all packets on the network.

It supports dynamic addition and deletion of VELANs in conjunction with its notify object (related sample). If a VELAN is deleted, the virtual device corresponding to the VELAN is stopped and removed, which in turn results in NDIS halting the miniport instance for the VELAN (see MPHalt). If a VELAN is added, NDIS sends a global reconfiguration event to the protocol edge of this driver. The handler function for this event, PtPNPHandler, goes through all lower adapters to see if any new VELANs have been added, i.e. if any of the "UpperBind" keys have been modified.

Since the driver implements a virtual device, it does not simply pass through most NDIS queries/sets. It keeps its own device view that is reflected in its responses to queries/sets. However it does pass through queries/sets for certain OIDs that are best handled by the lower adapter driver.

The driver supports Power Management in the sense that it allows Wake-On-LAN and related functionality, if supported by the lower adapter, to continue to function. It does so by appropriately forwarding OID\_PNP\_XXX queries/sets to the lower adapter.

### IEEE 802.1Q VLAN Operation

The driver supports configuring a VLAN ID on each VELAN. It then inserts a tag header containing this VLAN ID on all outgoing frames. For incoming frames that contain a tag header, it verifies that a matching VLAN ID is present before indicating it up to protocols. It removes the tag header, if present, from all indicated frames. In all cases, received frames that do not contain tag headers are always handed up to protocols.

With the default configured VLAN ID of zero, the driver does not insert tag header information on sent packets, except for sent packets that contain non-zero Ieee8021QInfo per-packet information, for which the driver does insert corresponding tag headers. Receive-side filtering on VLAN ID is enabled only with a non-zero configured VLAN ID, in which case only received frames containing a matching VLAN ID are passed up. With the default configured VLAN ID of zero, the driver does not check the VLAN ID on received frames.

### Configuring VLANs

The VLAN ID for each VELAN (virtual miniport) can be configured as follows. Right-click on the virtual miniport Local Area Connection icon and choose Properties. Click on the Configure button to bring up the Device Manager UI for the virtual device. Select the Advanced property sheet, this should contain a VLAN ID parameter that is configurable to the desired VLAN ID. Choosing a value of 0 (zero) disables receive-side filtering based on VLAN ID.

### Programming Tour

When it loads, i.e. from its DriverEntry function, the MUX driver registers as an Intermediate miniport driver and as a protocol, in that order.

### Binding and VELAN Creation

NDIS calls MUX's BindAdapter function, `PtBindAdapter`, for each underlying NDIS adapter to which it is configured to bind. This function allocates an `ADAPT` structure to represent the lower adapter, and calls `NdisOpenAdapter` to set up a binding to it. In the context of `BindAdapterHandler`, after successfully opening a binding to the underlying adapter, the driver queries the reserved keyword "UpperBindings" to get a list of device names for the virtual adapters that this particular binding is to expose, see `PtBootStrapVElans` for more details. Note that the MUX driver does not create bindings (for example, call `NdisOpenAdapter`) from any context other than its BindAdapter function. This is recommended behavior for all drivers of this type.

For each device name specified in the "UpperBindings" key, the MUX driver allocates a VELAN data structure to represent the virtual miniport, calls `NdisIMInitializeDeviceInstanceEx`. In response, NDIS eventually calls the MUX miniport's MiniportInitialize entry point, MPInitialize, for each VELAN. After MPInitialize successfully returns, NDIS takes care of getting upper-layer protocols to bind to the newly created virtual adapter(s).

### Unbinding and Halting

NDIS calls MUX's `UnbindAdapter` handler, `PtUnbindAdapter`, to request it to unbind from a lower adapter. In processing this, MUX calls `NdisIMDeInitializeDeviceInstance` for each VELAN instantiated on the indicated adapter, see `PtStopVElan` for details. This call results in NDIS first unbinding any protocols bound to the indicated VELAN, and then calling the MiniportHalt routine, `MPHalt`, for that VELAN. `MPHalt` waits for any outstanding receives/sends on the VELAN to finish before unlinking the VELAN from the ADAPT.

`PtUnbindAdapter` itself blocks until all VELANs associated with the ADAPT structure have been unlinked from it. This is to make sure that no thread running in the context of a miniport-edge entry point for a VELAN will ever access an invalid lower binding handle. Once all VELANs have been unlinked, `PtUnbindAdapter` closes the lower binding by calling `NdisCloseAdapter`. Note that the MUX driver does not close its lower binding from any context other than its `UnbindAdapter` function. This is recommended behavior for all drivers of this type.

`MPHalt` may also be called if the VELAN device is disabled, e.g. from the Network Connections Folder. There is no special code within `MPHalt` to handle this condition. However, `PtUnbindAdapter` takes care to not attempt to deinitialize a VELAN miniport (via `NdisIMDeInitializeDeviceInstance`) that has already been halted.

### Handling Queries

`MPRequest` is the MUX driver's function that handles queries for OID values on VELAN miniports. Most of the "Ethernet" type information for the virtual miniport is stored in the VELAN structure itself, and the driver returns information from this structure. The queries that are forwarded are **OID\_GEN\_MEDIA\_CONNECT\_STATUS**, **OID\_PNP\_CAPABILITIES** and **OID\_PNP\_WAKE\_UP\_PATTERN\_LIST**. See "Handling Power Management" below for more information about the latter two OIDs.

### Handling Sets

`MPRequest` handles setting OID values on VELAN miniports. Data management OIDs handled by the MUX driver are **OID\_802\_3\_MULTICAST\_LIST** and **OID\_GEN\_CURRENT\_PACKET\_FILTER**. The multicast list is handled entirely within the MUX driver, it just stores the set of multicast addresses in the VELAN structure, for reference during receive-side data processing. The packet filter is handled in a different way. The MUX driver combines the packet filter settings (bitwise OR) of all VELANs associated with the same lower adapter. If the combined packet filter is non-zero, MUX sends a Set request with a value of **NDIS\_PACKET\_TYPE\_PROMISCUOUS** for **OID\_GEN\_CURRENT\_PACKET\_FILTER** to start receives on the lower adapter. If the combined packet filter is zero, MUX sets the lower adapter's packet filter to 0 (turns off all receives if there aren't any interested protocols).

Note that setting the lower adapter to promiscuous mode is only done here in order to be able to receive unicast frames directed to multiple MAC addresses. If, for example, all VELANs are assigned the same MAC address (which is identical to the address of the lower adapter), then the MUX driver should only pass down the combined (bitwise OR) setting of packet filter settings of all VELANs.

Some power management OIDs are forwarded to the lower miniport. See "Handling Power Management" below for details.

### Sending Data

Data sent down on a VELAN miniport is forwarded to the lower adapter. The MUX driver itself does not generate any data of its own. The MUX driver clones a `NET_BUFFER_LIST` for each `NetBufferList` passed to its `MPSendNetBufferLists` function, and saves a pointer to the original `NET_BUFFER_LIST` in the reserved area of the `NET_BUFFER_LIST` structure. When the lower adapter completes the send (`PtSendNBLComplete`), MUX picks up the original packet and calls `NdisMSendNetBufferListsComplete` to complete the original send request.

If a non-zero VLAN ID is configured for the VELAN, and/or the packet has non-zero Ieee8021QInfo per-packet information, then the MUX driver inserts an NDIS buffer containing a tag header to the front of the packet before sending it down, see function `MPHandleSendTagging` for details.

### Receiving Data

Data received from a lower adapter is indicated up on zero or more VELANs. The `PtReceiveNBL` function is called for each `NetBufferList` received from the lower adapter. The received data is checked for matches with the packet filter and multicast list for each VELAN associated with the adapter (see `PtMatchPacketToVElan`). Whenever a match is found, a new `NET_BUFFER_LIST` is allocated and set to point to the received data. A pointer to the original received `NET_BUFFER_LIST` (if any) is also stored in the new `NET_BUFFER_LIST`'s reserved area. This packet is indicated up via `NdisMIndicateReceiveNetBufferLists` to all interested protocols on that VELAN.

The driver's `MPReturnNetBufferLists` function is called either by NDIS or by MUX itself when protocols are done with a received `NET_BUFFER_LIST`. This function returns the original `NET_BUFFER_LIST` indicated by the lower driver, if any, by calling `NdisReturnNetBufferLists.`

The driver indicates up received frames that do not have an IEEE 802.1Q tag header in them, see function **PtHandleRcvTagging**. It always strips off tag headers, if present, on received frames. If a non-zero VLAN ID is configured, then it checks received frames that contain tag headers for matching VLAN Ids, only matching frames are indicated up to protocols. Any VLAN/priority information present in incoming frames is copied to per-packet information fields of indicated `NET_BUFFER_LIST` structures.

### Status Indications

The only status indications that are forwarded up by MUX are media connect status indications. See `PtStatus` for more details.

### Handling Power Management

During initialization (`MPInitialize`), the MUX miniport sets the attribute **NDIS\_ATTRIBUTE\_NO\_HALT\_ON\_SUSPEND** in its call to `NdisMSetMiniportAttributes`. When the MUX miniport is requested to report its Plug and Play capabilities (**OID\_PNP\_CAPABILITIES**), the MUX miniport forwards the request to the underlying miniport. If this request succeeds, then the MUX miniport overwrites the following fields before successfully completing the original request:

```
NDIS_DEVICE_POWER_STATE          MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
NDIS_DEVICE_POWER_STATE          MinPatternWakeUp= NdisDeviceStateUnspecified;
NDIS_DEVICE_POWER_STATE          MinLinkChangeWakeUp=NdisDeviceStateUnspecified
```

See `PtPostProcessPnPCapabilities` for details.

**OID\_PNP\_SET\_POWER** and **OID\_PNP\_QUERY\_POWER** are not passed to the lower adapter, since the lower layer miniport will receive independent requests from NDIS.

NDIS calls the MUX driver's `ProtocolPnPEvent` function (`PtPNPHandler`) whenever the underlying adapter is transitioned to a different power state. If the underlying adapter is transitioning to a low power state, the driver waits for all outstanding sends and requests to complete.

Queries/sets received on a VELAN miniport that are to be forwarded to the underlying adapter are queued on the VELAN if the underlying adapter is at a low power state. These are picked up for processing on receiving a notification that the underlying adapter is back to a powered-up state.

### Handling Global Reconfiguration

All modifications to VELAN configuration are accompanied by PnP reconfigure notifications, for example, `NetEventReconfigure` events passed to the MUX's `PnPEventHandler`, `PtPNPHandler`. This driver takes a broad approach to handling reconfiguration, which is to simply re-examine all the "UpperBindings" keys for all currently bound adapters, and start off VELANs for any that do not exit, see `PtBootStrapVElans` for details.

### Canceling Sends

MUX propagates send cancellations from protocols above it to lower miniports.

Sample Notify Object
--------------------

### Preprocessor Flags:

### DISABLE\_PROTOCOLS\_TO\_PHYSICAL

When this flag is defined in the Sources file, the notify object disables the bindings of other protocols such as TCP/IP to the physical adapters during the installation. When all the virtual adapters are removed either through the custom property page or as a result of uninstalling the MUX driver, the notify object re-enables those bindings.

### PASSTHRU\_NOTIFY

This flag is defined to allow the MUX driver to be used in a passthru mode. When this flag is defined, the notify object:

1.  Creates only one virtual miniport for every physical adapter the MUX protocol edge binds to.
2.  Disables the property page to prevent adding of additional virtual miniports.
3.  Stores the device name of the virtual adapter in REG\_SZ registry value under HKLM\\System\\CurrentControlSet\\Services\\muxp\\Parameters\\Adapters\\{PhysicalAdaptersInstanceGuid}\\UpperBindings, because there is one to one binding. In the MUX mode (when this flag is not defined), the notify object stores the device name in a REG\_MULTI\_SZ registry value as there could be more than one virtual miniports.

You can also use this notify object with the Passthru driver by doing the following:

1.  Change the protocol name in file \\ndis\\passthru\\passthru.c from **PASSTHRU** to **MUXP.**
2.  Change the driver name from Passthru to MUX in the sources file.
3.  Rebuild the driver to obtain a mux.sys driver binary.
4.  Build the MUX notify object with **PASSTHRU\_NOTIFY** defined.
5.  Use the MUX inf files, muxp.inf and mux\_mp.inf, to install the driver and dll.

The benefit of using techniques in the MUX notify object for a 1:1 intermediate driver (e.g. Passthru) is to be able to exercise higher level of control over the bindings of MUX with other components in the system, which is not possible with the IM filter driver.

### CUSTOM\_EVENTS

When this macro is defined, the notify object shows how to send custom events to the MUX IM driver when a virtual miniport is added or removed.

### Notify Object Operation

During installation, the notify object performs the following operations:

-   It creates one virtual adapter for each physical adapter the MUX protocol edge binds to.
-   It disables the bindings of other protocols such as TCP/IP to physical adapters if it has been compiled with DISABLE\_PROTOCOLS\_TO\_PHYSICAL defined in the Sources file. This is the most commonly desired behavior for N:1 MUX drivers.
-   It disables the bindings of the protocol edge of the MUX IM driver with all its virtual adapters.

The notify object provides a custom property page for the MUX IM driver. The custom property page allows the user to add one or more virtual adapters on top of a physical adapter or delete an existing virtual adapter.

When the MUX IM driver is uninstalled, or binding is disabled, or the user deletes all the virtual adapters on top of a physical adapter, the notify object restores the bindings of other protocols to the physical adapter if it has been compiled with the preprocessor flag **DISABLE\_PROTOCOLS\_TO\_PHYSICAL** defined in the Sources file.

### File Manifest

File | Description
-----|------------
Miniport.c | Miniport related routines for the MUX driver 
Mux.c | DriverEntry routine and any routines common to the MUX miniport and protocol  
Mux.h | Prototypes of all functions and data structures used by the MUX driver 
Mux.rc | Resource file for the MUX driver 
Muxp.inf | Installation INF for the service (protocol side installation) 
Mux_mp.inf | Installation INF for the miniport (virtual device installation) 
Precomp.h | Precompile header file 
Protocol.c | Protocol related routines for the MUX driver 
Public.h | Contains the common declarations shared by driver and user applications

For more information, see [NDIS Intermediate Drivers](http://msdn.microsoft.com/en-us/library/windows/hardware/ff565773) in the network devices design guide.

