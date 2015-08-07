Windows Radio Management Sample
===============================

The Radio Manager sample demonstrates how to structure a Radio Manager for use with the Windows Radio Management APIs.

The operating system contains a set of APIs which are used as a software mechanism to control the various radios found on the machine. The APIs work by communicating with a Radio Manager, which is a COM object that relays commands from the APIs to turn the radio on or off, and reports back radio information to the APIs. This feature is designed in such a way that a separate Radio Manager is required for each radio media type. For example, a WLAN radio will be controlled by a different Radio Manager than a GPS radio. If there are 2 WLAN radios and a GPS radio, the 2 WLAN radios will be controlled by one Radio Manager, and the GPS radio will be controlled by a different Radio Manager. The Radio Manager must be able to run correctly within Local Service Account context. Under this context, the Radio Manager will have the minimum privilege on the local computer.

When the user turns the radio off (either by using the specific radio software switch or the airplane mode switch), radio transmission must be turned off. The device can be powered off as long as the radio switch does not disappear from the UI. It is very important that the radio manager developer ensures that when the device is powered off, the radio switch does not disappear from the UI. If the radio switch disappears from the UI when the radio is turned off by the user, then user has no way to turn the radio back on! If it is desired to conserve power by cutting power to the device when the radio is turned off, but the device cannot be completely powered off because it disappears from the UI, then the solution would be to put the device in a low power state (e.g. D3).

**Important** The radio manager must be given a name. This is the name of the radio switch that is displayed to the user in the Wireless page of PC Settings. The name must be simple, yet descriptive of what the radio is. For example, for NFC radios, the value of the name field should be "NFC", and for GPS radios, the value of the name field should be "GPS" or "GNSS", whichever is more appropriate. The name must not include the word "radio" or the manufacturer's name or some other word related to the functionality of the radio (e.g. "Location" OR "port").


Installation
------------

The sample contains a script, *install.cmd*, which copies the radio manager DLL to the system directory, registers as a COM component, and configures the registry.

Copy the *install.cmd*, *SampleRM.reg* and *SampleRM.dll* files to a directory. Run *install.cmd*.

Code Tour
---------

File | Description
-----|-----
install.cmd | Installation script. Copies and registers the dll and executes SampleRM.reg.
SampleRM.reg | Script to install the Sample Radio Manager into the registry, along with 2 radio instances.
SampleRM.sln | The Visual Studio solution file for building the Sample Radio Manager dll.
sampleRM.idl |The interface definition for the Sample Radio Manager.
RadioMgr.idl | The interface definition for a Windows Radio Manager.
SampleRadioManager.h | Header file for the functions required for a Radio Manager.
SampleRadioInstance.h | Header file for the functions required for a Radio Instance.
SampleInstanceCollection.h | Header file for the functions required for a Collection of Radio Instances.
precomp.h | Common header file.
InternalInterfaces.h | Header file for internal interface used for this sample.
dllmain.cpp | Standard dllmain.
SampleRadioManager.cpp | Implementation details for the Sample Radio Manager. Important concepts include utilizing [IMediaRadioManagerNotifySink](http://msdn.microsoft.com/en-us/library/windows/hardware/hh406534) for radio instance events, adding/Removing radio instances, and queuing and deploying worker jobs for system events.
SampleRadioInstance.cpp | Implementation details for the Sample Radio Instance. Important concepts include accessors and modifiers for radio information, and instance change functions.
SampleInstanceCollection.cpp | Implementation details for the Sample Instance Collection. Important concepts include radio instance discovery and retrieval.
RadioMgr\_interface.cpp | Helper source file to include the MIDL-generated files.

Run the sample
--------------

### Operation ###

This sample Radio Manager does not operate on an actual radio. Instead, it uses registry keys to act as virtual radios.

Each "radio" instance can have the following values:

Name

RadioState

PreviousRadioState

IsMultiComm

IsAssociatingDevice

**Note** It is required that the registry key has AT LEAST a Name value, otherwise the Sample Radio Manager will fail to initialize.

**Important** The radio manager must be given a name and the registry key must have, as a minimum, a Name value. Otherwise, the Sample Radio Manager will fail to initialize. This is the name of the radio switch that is displayed to the user in the Wireless page of PC Settings. The name must be simple, yet descriptive of what the radio is. For example, for NFC radios, the value of the name field should be "NFC", and for GPS radios, the value of the name field should be "GPS" or "GNSS", whichever is more appropriate. The name must not include the word "radio" or the manufacturer's name or some other word related to the functionality of the radio (e.g. "Location" OR "port").

When the Radio Manager is initialized, it uses these registry keys to retrieve the "radio" information. The radio state values can be any of the following enum values:


```c_cpp
typedef enum _DEVICE_RADIO_STATE
{
    DRS_RADIO_ON                    = 0,
    DRS_SW_RADIO_OFF                = 1,
    DRS_HW_RADIO_OFF                = 2,
    DRS_SW_HW_RADIO_OFF             = 3,
    DRS_HW_RADIO_ON_UNCONTROLLABLE  = 4,
    DRS_RADIO_INVALID               = 5,
    DRS_HW_RADIO_OFF_UNCONTROLLABLE = 6,
    DRS_RADIO_MAX                   = DRS_HW_RADIO_OFF_UNCONTROLLABLE
} DEVICE_RADIO_STATE;
```

For IsMultiComm and IsAssociatingDevice, a value of 0 means 'no' and 1 means 'yes'.

### Adding and Setting a Radio Instance ###

To add a new radio instance, add a new instance key to the registry key like the following entry:

```
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\RadioManagement\Misc\SampleRadioManager\SampleRadioX]
"RadioState"=dword:00000000
"Name"="SampleRadioX"
"IsMultiComm"=dword:00000000
```

### Editing a Radio Instance ###

Simply change the values in the registry. For example, change the radio state from DRS\_RADIO\_ON to DRS\_SW\_RADIO\_OFF by changing the 'RadioState' value from 0 to 1.

### Removing a Radio Instance ###

Delete the corresponding registry key.

