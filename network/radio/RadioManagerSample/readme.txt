Sample Radio Manager
====================
Demonstrates how to structure a Radio Manager for use with the Windows 8 Radio Management APIs.

On Windows 8 and later, the operating system contains a set of APIs which are used as a software mechanism
to control the various radios found on the machine. The APIs work by communicating with a Radio Manager, 
which is a COM object that relays commands from the APIs to turn the radio on or off, and reports back
radio information to the APIs. This feature is designed in such a way that a separate Radio Manager is 
required for each radio media type. For example, a WLAN radio will be controlled by a different Radio
Manager than a GPS radio. If there are 2 WLAN radios and a GPS radio, the 2 WLAN radios will be controlled
by one Radio Manager, and the GPS radio will be controlled by a different Radio Manager. The Radio Manager 
must be able to run correctly within Local Service Account context. Under this context, the Radio Manager 
will have the minimum privilege on the local computer.

When the user turns the radio off (either by using the specific radio software switch or the airplane mode switch), 
radio transmission must be turned off. The device can be powered off as long as the radio switch does not disappear 
from the UI. It is very important that the radio manager developer ensures that when the device is powered off, the 
radio switch does not disappear from the UI. If the radio switch disappears from the UI when the radio is turned off 
by the user, then user has no way to turn the radio back on! If it is desired to conserve power by cutting power to 
the device when the radio is turned off, but the device cannot be completely powered off because it disappears from 
the UI, then the solution would be to put the device in a low power state (e.g. D3). 

IMPORTANT! The radio manager must be given a name – this is the name of the radio switch that is displayed to the 
user in the Wireless page of PC Settings. The name must be simple, yet descriptive of what the radio is. For example, 
for NFC radios, the value of the name field should be "NFC", and for GPS radios, the value of the name field should 
be "GPS" or "GNSS", whichever is more appropriate. The name must not include the word "radio" or the manufacturer's 
name or some other word related to the functionality of the radio (e.g. "Location" OR "port").



Sample Language Implementations
===============================
This sample is available in the following language implementations:
     C++


Installation Files
==================
install.cmd
- Installation script. Copies and registers the dll and executes SampleRM.reg

SampleRM.reg
- script to install the Sample Radio Manager into the registry, along with 2 radio instances


Source Files
============
SampleRM.sln 
- The Visual Studio 2010 solution file for building the Sample Radio Manager dll

sampleRM.idl
- The interface definition for the Sample Radio Manager

RadioMgr.idl
- The interface definition for a Windows Radio Manager

SampleRadioManager.h
- Header file for the functions required for a Radio Manager

SampleRadioInstance.h
- Header file for the functions required for a Radio Instance

SampleInstanceCollection.h
- Header file for the functions required for a Collection of Radio Instances

precomp.h
- Common header file

InternalInterfaces.h
- Header file for internal interface used for this sample

dllmain.cpp
- Standard dllmain

SampleRadioManager.cpp
- Implementation details for the Sample Radio Manager. Important concepts include:
	- Utilizing IMediaRadioManagerNotifySink for radio instance events
	- Adding/Removing radio instances
	- Queuing and deploying worker jobs for system events

SampleRadioInstance.cpp
- Implementation details for the Sample Radio Instance. Important concepts include:
	- Accessors & Modifiers for radio information
	- Instance change functions

SampleInstanceCollection.cpp
- Implementation details for the Sample Instance Collection. Important concepts include:
	- Radio Instance discovery and retrieval

RadioMgr_interface.cpp
- Helper source file to include the MIDL-generated files.


How the sample works
====================
This sample Radio Manager does not operate on an actual radio. Instead, it uses registry keys to act as 
virtual radios. Each "radio" instance can have the following values: 
	-Name
	-RadioState
	-PreviousRadioState
	-IsMultiComm
	-IsAssociatingDevice

***It is required that the registry key has AT LEAST a Name value, otherwise the Sample Radio Manager will
fail to initialize***

IMPORTANT! The radio manager must be given a name – this is the name of the radio switch that is displayed to the 
user in the Wireless page of PC Settings. The name must be simple, yet descriptive of what the radio is. For example, 
for NFC radios, the value of the name field should be "NFC", and for GPS radios, the value of the name field should 
be "GPS" or "GNSS", whichever is more appropriate. The name must not include the word "radio" or the manufacturer's 
name or some other word related to the functionality of the radio (e.g. "Location" OR "port").
 

When the Radio Manager is initialized, it uses these registry keys to retrieve the "radio" information. 
The radio state values can be any of the following enum values:

	typedef enum _DEVICE_RADIO_STATE
	{
	    DRS_RADIO_ON = 0,
	    DRS_SW_RADIO_OFF = 1,
	    DRS_HW_RADIO_OFF = 2,
	    DRS_SW_HW_RADIO_OFF = 3,
	    DRS_HW_RADIO_ON_UNCONTROLLABLE = 4,
	    DRS_RADIO_INVALID = 5,
	    DRS_HW_RADIO_OFF_UNCONTROLLABLE = 6,
	    DRS_RADIO_MAX = DRS_HW_RADIO_OFF_UNCONTROLLABLE
	} DEVICE_RADIO_STATE;

For IsMultiComm and IsAssociatingDevice, a value of 0 means 'no' and 1 means 'yes.


To Add a Radio Instance
=======================
Add a new item to the registry:
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\RadioManagement\Misc\SampleRadioManager\SampleRadioX]
"RadioState"=dword:00000000
"Name"="SampleRadioX"
"IsMultiComm"=dword:00000000

To Edit a Radio Instance
========================
Simply change the values in the registry. For example, change the radio state from DRS_RADIO_ON to DRS_SW_RADIO_OFF
by changing the 'RadioState' value from 0 to 1.


To Remove a Radio Instance
==========================
Delete the corresponding registry key.


Prerequisites for viewing and building solution
===============================================
1. Windows 7 
2. Visual Studio 2010


To build the sample using Visual Studio 2010 (preferred method):
===============================================================
1. Open File Explorer and navigate to the directory.
2. Double-click the icon for the SampleRM.sln (solution) file to open the file in Visual Studio.
3. In the Build menu, select Build Solution. The dll will be built in the default \Debug or \Release directory


Prerequisites for installing Radio Manager
==========================================
1. Windows 8
2. The VC++ 2010 Redistributable package must be installed for this dll to load.


Installing Radio Manager
========================
1. Copy the install.cmd, SampleRM.reg and SampleRM.dll files to a directory
2. Run install.cmd



















