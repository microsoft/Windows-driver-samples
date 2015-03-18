Native Wifi IHV Service
=======================

This sample code demonstrates IHV extensibility for Native WiFi.

In particular, this sample contains the following features:

-   IHV profile validation
-   IHV discovery profile creation
-   IHV extension for interactive UI and Profile UI
-   802.1x extension

The sample, after you compile and install it, enables you to connect to an Open WEP Network by using 802.1X through IHV Service extension.

Run the sample
--------------

The fully compiled sample consists of two DLLs: IHVSample.dll and IHVSampleUI.dll. The functions of those DLLs are outlined below:

### IHVSample.dll

The IHVSample DLL supports connecting to a wireless network by using Open authentication and WEP encryption. The sample is capable of connecting to an 802.1X network and a non-802.1X network.

During the discovery phase, the sample generates a temporary profile that the operating system uses to establish a wireless connection. The operating system requests that the IHV provide a temporary profile to use when trying to connect through Discovery. In this case, IHVSample returns a list of usable profiles for connecting by using open-WEP with and without 802.1X. The RC4 algorithm is implemented in a DLL that is provided as part of the sample.

The sample is loaded by the IHV process and uses the public interfaces that are provided by the same process. The IHV process host initializes IHVSample in its process.

The sample gets called to perform pre-associate security and post-associate security. The sample does not implement any of the pre-associate security. For the post-associate security, in the case of open-WEP without 802.1X, IHVSample prompts for UI in case the profile does not exist or does not already have a valid key. In the case of 802.1X networks, the post-associate security portion sets the driver packet exemptions, starts up the Microsoft 802.1X module authentication, and waits for the result that indicates a success or a failure. During this period, IHVSample forwards all 802.1X packets to the IHV process. However, it caches the EAPOL key packets. In either case, after the key is obtained, IHVSample sends it to the driver and establishes the connection.

The operating system validates network profiles before they can be applied and persisted. The operating system performs validation of the non-IHV portion of the profile and passes the rest of data to IHVSample if IHV settings exist. IHVSample does limited XML schema validation.

### UI Sample (IHVSampleUI.dll)

The IHVSampleUI DLL extends the Wireless Profile UI to display IHV connectivity and security information. The security information that is displayed is for both security types based on IHV proprietary security and those based on Microsoft 802.1X. The sample adds custom IHV authentication and encryption types to illustrate the different ways the various types can be embedded in the UI. The configuration UI saves the IHV portion of the profile in accordance with the IHV schema. The sample pages that enable modification of the IHV parameters are displayed from the configuration UI. The sample also displays a wizard-based connection time UI with three sample pages. This connection time UI integrates in both the wizard and non-wizard flows.

Installation
------------

After the sample is compiled, you must copy the binaries on to the target system and associate them with the matching Native Wifi-capable adapter. You can copy the binaries by adding an appropriate **CopyFiles** directive in the **DDInstall** section in the INF file for installing the adapter. You can associate the binaries by adding an appropriate **AddReg** directive in the **DDInstall** section in the INF file for installing the adapter.

### CopyFiles Directive

The CopyFiles Directive should name a File-List-Section. The contents of this section should have the following:

IHVSpecifiedDLLName,,,2

IHVSpecifiedOtherFile,,,2

There should be an associated entry in the DestinationDirs section that specifies the destination to copy the file to. This section should have a directive like one of the following:

File-List-Section= 11 ; \\system32 directory

DefaultDestDir= 11 ; \\system32 directory

### AddReg Directive

The AddReg directive should name an Add-Registry-Section.

The contents of the Miniport INF file must include the following, in order for the correct IHV Service to be started:

-   HKR,Ndi\\IHVExtensions, ExtensibilityDLL,0,"%SystemRoot%\\system32\\IhvExt.dll"

    This registry key is used to determine the location of the IHVSample.dll.

-   HKR,Ndi\\IHVExtensions,UIExtensibilityCLSID,0, "\<CLSID\>"

    This registry key is used to determine the class ID of the COM interface that extends the 802.11 configuration UI.

-   HKR,Ndi\\IHVExtensions,GroupName,0, "IHV provided group name"
-   HKR,Ndi\\IHVExtensions, AdapterOUI, 0x00010001, 0x00123456

    This registry key is used to verify the OUI when the profile is applied to the adapter. If the AdapterOUI value is 0x??123456 in the registry, it needs to look like the following in the profile:

    \<OUIHeader\>

    \<OUI\>123456\</OUI\>

    \<type\>??\</type\>

    \</OUHeader\>

    Note that ?? stands for bits ignored.

-   HKR,Ndi\\IHVExtensions, DiagnosticsID,0, "\<Diagnostics ID GUID\>"

### Uninstallation Instructions

To uninstall this sample, you must undo the AddReg directive and undo the CopyFiles directive.

For more information about creating a Native Wi-Fi package, see [Native 802.11 Wireless LAN](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560690).

