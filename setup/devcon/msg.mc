;//
;// Copyright (c) Microsoft Corporation.  All rights reserved.
;//

;//
;// General
;//
MessageId=60000 SymbolicName=MSG_USAGE
Language=English
%1 Usage: %1 [-r] [-m:\\<machine>] <command> [<arg>...]
For more information, type: %1 help
.
MessageId=60001 SymbolicName=MSG_FAILURE
Language=English
%1 failed.
.
MessageId=60002 SymbolicName=MSG_COMMAND_USAGE
Language=English
%1: Invalid use of %2.
For more information, type: %1 help %2
.

;//
;// HELP
;//
MessageId=60100 SymbolicName=MSG_HELP_LONG
Language=English
Device Console Help:
%1 [-r] [-m:\\<machine>] <command> [<arg>...]
-r           Reboots the system only when a restart or reboot is required.
<machine>    Specifies a remote computer. 
<command>    Specifies a Devcon command (see command list below).
<arg>...     One or more arguments that modify a command.
For help with a specific command, type: %1 help <command>
.
MessageId=60101 SymbolicName=MSG_HELP_SHORT
Language=English
%1!-20s! Display Devcon help.
.
MessageId=60102 SymbolicName=MSG_HELP_OTHER
Language=English
Unknown command.
.

;//
;// CLASSES
;//
MessageId=60200 SymbolicName=MSG_CLASSES_LONG
Language=English
Devcon Classes Command
Lists all device setup classes. Valid on local and remote computers.
%1 [-m:\\<machine>] %2
<machine>    Specifies a remote computer.
Class entries have the format <name>: <descr>
where <name> is the class name and <descr> is the class description.
.
MessageId=60201 SymbolicName=MSG_CLASSES_SHORT
Language=English
%1!-20s! List all device setup classes.
.
MessageId=60202 SymbolicName=MSG_CLASSES_HEADER
Language=English
Listing %1!u! setup classes on %2.
.
MessageId=60203 SymbolicName=MSG_CLASSES_HEADER_LOCAL
Language=English
Listing %1!u! setup classes.
.

;//
;// LISTCLASS
;//
MessageId=60300 SymbolicName=MSG_LISTCLASS_LONG
Language=English
Devcon Listclass Command
Lists all devices in the specified setup classes. Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <class> [<class>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Device entries have the format <instance>: <descr>
where <instance> is a unique instance of the device and <descr> is the device description.
.
MessageId=60301 SymbolicName=MSG_LISTCLASS_SHORT
Language=English
%1!-20s! List all devices in a setup class.
.
MessageId=60302 SymbolicName=MSG_LISTCLASS_HEADER
Language=English
Listing %1!u! devices in setup class "%2" (%3) on %4.
.
MessageId=60303 SymbolicName=MSG_LISTCLASS_HEADER_LOCAL
Language=English
Listing %1!u! devices in setup class "%2" (%3).
.
MessageId=60304 SymbolicName=MSG_LISTCLASS_NOCLASS
Language=English
There is no "%1" setup class on %2.
.
MessageId=60305 SymbolicName=MSG_LISTCLASS_NOCLASS_LOCAL
Language=English
There is no "%1" setup class on the local machine.
.
MessageId=60306 SymbolicName=MSG_LISTCLASS_HEADER_NONE
Language=English
There are no devices in setup class "%1" (%2) on %3.
.
MessageId=60307 SymbolicName=MSG_LISTCLASS_HEADER_NONE_LOCAL
Language=English
There are no devices in setup class "%1" (%2).
.

;//
;// FIND
;//
MessageId=60400 SymbolicName=MSG_FIND_LONG
Language=English
Devcon Find Command
Finds devices with the specified hardware or instance ID. Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
Device entries have the format <instance>: <descr>
where <instance> is the unique instance of the device and <descr> is the device description.
.
MessageId=60401 SymbolicName=MSG_FIND_SHORT
Language=English
%1!-20s! Find devices.
.
MessageId=60402 SymbolicName=MSG_FIND_TAIL_NONE
Language=English
No matching devices found on %1.
.
MessageId=60403 SymbolicName=MSG_FIND_TAIL_NONE_LOCAL
Language=English
No matching devices found.
.
MessageId=60404 SymbolicName=MSG_FIND_TAIL
Language=English
%1!u! matching device(s) found on %2.
.
MessageId=60405 SymbolicName=MSG_FIND_TAIL_LOCAL
Language=English
%1!u! matching device(s) found.
.
MessageId=60406 SymbolicName=MSG_FINDALL_LONG
Language=English
Devcon Findall Command
Finds devices with the specified hardware or instance ID, including devices
that are not currently attached. Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
Device entries have the format <instance>: <descr>
where <instance> is the unique instance of the device and <descr> is the description.
.
MessageId=60407 SymbolicName=MSG_FINDALL_SHORT
Language=English
%1!-20s! Find devices, including those that are not currently attached.
.
MessageId=60408 SymbolicName=MSG_STATUS_LONG
Language=English
Devcon Status Command
Lists the running status of devices with the specified hardware or instance ID.
Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60409 SymbolicName=MSG_STATUS_SHORT
Language=English
%1!-20s! List running status of devices.
.
MessageId=60410 SymbolicName=MSG_DRIVERFILES_LONG
Language=English
Devcon Driverfiles Command
List installed driver files for devices with the specified hardware or
instance ID. Valid only on the local computer.
%1 %2 <id> [<id>...]
%1 %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60411 SymbolicName=MSG_DRIVERFILES_SHORT
Language=English
%1!-20s! List installed driver files for devices.
.
MessageId=60412 SymbolicName=MSG_RESOURCES_LONG
Language=English
Devcon Resources Command
Lists hardware resources of devices with the specified hardware or instance ID.
Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer. 
<class>      Specifies a device setup class.
Examples of <id>:
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60413 SymbolicName=MSG_RESOURCES_SHORT
Language=English
%1!-20s! List hardware resources for devices.
.
MessageId=60414 SymbolicName=MSG_HWIDS_LONG
Language=English
Devcon Hwids Command
Lists hardware IDs of all devices with the specified hardware or instance ID.
Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60415 SymbolicName=MSG_HWIDS_SHORT
Language=English
%1!-20s! List hardware IDs of devices.
.
MessageId=60416 SymbolicName=MSG_STACK_LONG
Language=English
Devcon Stack Command
Lists the expected driver stack of devices with the specified hardware
or instance ID. PnP calls each driver's AddDevice routine when building
the device stack. Valid on local and remote computers.
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
<machine>    Specifies a remote computer.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60417 SymbolicName=MSG_STACK_SHORT
Language=English
%1!-20s! List expected driver stack for devices.
.
;//
;// ENABLE
;//
MessageId=60500 SymbolicName=MSG_ENABLE_LONG
Language=English
Devcon Enable Command
Enables devices with the specified hardware or instance ID. Valid only on
the local computer. (To reboot when necessary, include -r.)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
-r           Reboots the system only when a restart or reboot is required.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60501 SymbolicName=MSG_ENABLE_SHORT
Language=English
%1!-20s! Enable devices.
.
MessageId=60502 SymbolicName=MSG_ENABLE_TAIL_NONE
Language=English
No devices were enabled, either because the devices were not found, 
or because the devices could not be enabled.
.
MessageId=60503 SymbolicName=MSG_ENABLE_TAIL_REBOOT
Language=English
The %1!u! device(s) are ready to be enabled. To enable the devices, restart the devices or
reboot the system .
.
MessageId=60504 SymbolicName=MSG_ENABLE_TAIL
Language=English
%1!u! device(s) are enabled.
.

;//
;// DISABLE
;//
MessageId=60600 SymbolicName=MSG_DISABLE_LONG
Language=English
Devcon Disable Command
Disables devices with the specified hardware or instance ID.
Valid only on the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
-r           Reboots the system only when a restart or reboot is required.
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60601 SymbolicName=MSG_DISABLE_SHORT
Language=English
%1!-20s! Disable devices.
.
MessageId=60602 SymbolicName=MSG_DISABLE_TAIL_NONE
Language=English
No devices were disabled, either because the devices were not found,
or because the devices could not be disabled.
.
MessageId=60603 SymbolicName=MSG_DISABLE_TAIL_REBOOT
Language=English
The %1!u! device(s) are ready to be disabled. To disable the devices, restart the
devices or reboot the system .
.
MessageId=60604 SymbolicName=MSG_DISABLE_TAIL
Language=English
%1!u! device(s) disabled.
.


;//
;// RESTART
;//
MessageId=60700 SymbolicName=MSG_RESTART_LONG
Language=English
Devcon Restart Command
Restarts devices with the specified hardware or instance ID.
Valid only on the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=60701 SymbolicName=MSG_RESTART_SHORT
Language=English
%1!-20s! Restart devices.
.
MessageId=60702 SymbolicName=MSG_RESTART_TAIL_NONE
Language=English
No devices were restarted, either because the devices were not found,
or because the devices could not be restarted.
.
MessageId=60703 SymbolicName=MSG_RESTART_TAIL_REBOOT
Language=English
The %1!u! device(s) are ready to be restarted. To restart the devices, reboot the system.
.
MessageId=60704 SymbolicName=MSG_RESTART_TAIL
Language=English
%1!u! device(s) restarted.
.


;//
;// REBOOT
;//
MessageId=60800 SymbolicName=MSG_REBOOT_LONG
Language=English
%1 %2
Reboots the local computer as part of a planned hardware installation.
.
MessageId=60801 SymbolicName=MSG_REBOOT_SHORT
Language=English
%1!-20s! Reboot the local computer.
.
MessageId=60802 SymbolicName=MSG_REBOOT
Language=English
Rebooting the local computer.
.

;//
;// DUMP
;//
MessageId=60904 SymbolicName=MSG_DUMP_PROBLEM
Language=English
The device has the following problem: %1!02u!
.
MessageId=60905 SymbolicName=MSG_DUMP_PRIVATE_PROBLEM
Language=English
The driver reported a problem with the device.
.
MessageId=60906 SymbolicName=MSG_DUMP_STARTED
Language=English
Driver is running.
.
MessageId=60907 SymbolicName=MSG_DUMP_DISABLED
Language=English
Device is disabled.
.
MessageId=60908 SymbolicName=MSG_DUMP_NOTSTARTED
Language=English
Device is currently stopped.
.
MessageId=60909 SymbolicName=MSG_DUMP_NO_RESOURCES
Language=English
Device is not using any resources.
.
MessageId=60910 SymbolicName=MSG_DUMP_NO_RESERVED_RESOURCES
Language=English
Device has no reserved resources.
.
MessageId=60911 SymbolicName=MSG_DUMP_RESOURCES
Language=English
Device is currently using the following resources:
.
MessageId=60912 SymbolicName=MSG_DUMP_RESERVED_RESOURCES
Language=English
Device has the following reserved resources:
.
MessageId=60913 SymbolicName=MSG_DUMP_DRIVER_FILES
Language=English
Driver installed from %2 [%3]. %1!u! file(s) used by driver:
.
MessageId=60914 SymbolicName=MSG_DUMP_NO_DRIVER_FILES
Language=English
Driver installed from %2 [%3]. The driver is not using any files.
.
MessageId=60915 SymbolicName=MSG_DUMP_NO_DRIVER
Language=English
No driver information available for the device.
.
MessageId=60916 SymbolicName=MSG_DUMP_HWIDS
Language=English
Hardware IDs:
.
MessageId=60917 SymbolicName=MSG_DUMP_COMPATIDS
Language=English
Compatible IDs:
.
MessageId=60918 SymbolicName=MSG_DUMP_NO_HWIDS
Language=English
No hardware/compatible IDs found for this device.
.
MessageId=60919 SymbolicName=MSG_DUMP_NO_DRIVERNODES
Language=English
No driver nodes found for this device.
.
MessageId=60920 SymbolicName=MSG_DUMP_DRIVERNODE_HEADER
Language=English
Driver node #%1!u!:
.
MessageId=60921 SymbolicName=MSG_DUMP_DRIVERNODE_INF
Language=English
Inf file is %1
.
MessageId=60922 SymbolicName=MSG_DUMP_DRIVERNODE_SECTION
Language=English
Inf section is %1
.
MessageId=60923 SymbolicName=MSG_DUMP_DRIVERNODE_DESCRIPTION
Language=English
Driver description is %1
.
MessageId=60924 SymbolicName=MSG_DUMP_DRIVERNODE_MFGNAME
Language=English
Manufacturer name is %1
.
MessageId=60925 SymbolicName=MSG_DUMP_DRIVERNODE_PROVIDERNAME
Language=English
Provider name is %1
.
MessageId=60926 SymbolicName=MSG_DUMP_DRIVERNODE_DRIVERDATE
Language=English
Driver date is %1
.
MessageId=60927 SymbolicName=MSG_DUMP_DRIVERNODE_DRIVERVERSION
Language=English
Driver version is %1!u!.%2!u!.%3!u!.%4!u!
.
MessageId=60928 SymbolicName=MSG_DUMP_DRIVERNODE_RANK
Language=English
Driver node rank is %1!u!
.
MessageId=60929 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS
Language=English
Driver node flags are %1!08X!
.
MessageId=60930 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_OLD_INET_DRIVER
Language=English
Inf came from the Internet
.
MessageId=60931 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_BAD_DRIVER
Language=English
Driver node is marked "BAD"
.
MessageId=60932 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_INF_IS_SIGNED
Language=English
Inf is digitally signed
.
MessageId=60933 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_OEM_F6_INF
Language=English
Inf was installed by using F6 during text mode setup
.
MessageId=60934 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_BASIC_DRIVER
Language=English
Driver provides basic functionality when no signed driver is available.
.
MessageId=60935 SymbolicName=MSG_DUMP_DEVICESTACK_UPPERCLASSFILTERS
Language=English
Upper class filters:
.
MessageId=60936 SymbolicName=MSG_DUMP_DEVICESTACK_UPPERFILTERS
Language=English
Upper filters:
.
MessageId=60937 SymbolicName=MSG_DUMP_DEVICESTACK_SERVICE
Language=English
Controlling service:
.
MessageId=60938 SymbolicName=MSG_DUMP_DEVICESTACK_NOSERVICE
Language=English
(none)
.
MessageId=60939 SymbolicName=MSG_DUMP_DEVICESTACK_LOWERCLASSFILTERS
Language=English
Class lower filters:
.
MessageId=60940 SymbolicName=MSG_DUMP_DEVICESTACK_LOWERFILTERS
Language=English
Lower filters:
.
MessageId=60941 SymbolicName=MSG_DUMP_SETUPCLASS
Language=English
Setup Class: %1 %2
.
MessageId=60942 SymbolicName=MSG_DUMP_NOSETUPCLASS
Language=English
Device is not set up.
.
MessageId=60943 SymbolicName=MSG_DUMP_DESCRIPTION
Language=English
Name: %1
.
MessageId=60944 SymbolicName=MSG_DUMP_PHANTOM
Language=English
Device is not present.
.
MessageId=60945 SymbolicName=MSG_DUMP_STATUS_ERROR
Language=English
Error retrieving the device's status.
.

;//
;// INSTALL
;//
MessageId=61000 SymbolicName=MSG_INSTALL_LONG
Language=English
Devcon Install Command
Installs the specified device manually. Valid only on the local computer. 
(To reboot when necesary, Include -r .)
%1 [-r] %2 <inf> <hwid>
<inf>        Specifies an INF file with installation information for the device.
<hwid>       Specifies a hardware ID for the device.
-r           Reboots the system only when a restart or reboot is required.
.
MessageId=61001 SymbolicName=MSG_INSTALL_SHORT
Language=English
%1!-20s! Install a device manually.
.
MessageId=61002 SymbolicName=MSG_INSTALL_UPDATE
Language=English
Device node created. Install is complete when drivers are installed...
.

;//
;// UPDATE
;//
MessageId=61100 SymbolicName=MSG_UPDATE_LONG
Language=English
Devcon Update Command
Updates drivers for all devices with the specified hardware ID (<hwid>). 
Valid only on the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <inf> <hwid>
-r           Reboots the system only when a restart or reboot is required.
<inf>        Specifies an INF file with installation information for the devices.
<hwid>       Specifies the hardware ID of the devices.
.
MessageId=61101 SymbolicName=MSG_UPDATE_SHORT
Language=English
%1!-20s! Update a device manually.
.
MessageId=61102 SymbolicName=MSG_UPDATE_INF
Language=English
Updating drivers for %1 from %2.
.
MessageId=61103 SymbolicName=MSG_UPDATE
Language=English
Updating drivers for %1.
.
MessageId=61104 SymbolicName=MSG_UPDATENI_LONG
Language=English
%1 [-r] %2 <inf> <hwid>
Update drivers for devices (Non Interactive).
This command will only work for local machine.
Specify -r to reboot automatically if needed.
<inf> is an INF to use to install the device.
All devices that match <hwid> are updated.
Unsigned installs will fail. No UI will be
presented.
.
MessageId=61105 SymbolicName=MSG_UPDATENI_SHORT
Language=English
%1!-20s! Manually update a device (non interactive).
.
MessageId=61106 SymbolicName=MSG_UPDATE_OK
Language=English
Drivers installed successfully.
.
;//
;// Driver Package (add/remove/enum)
;//
MessageId=61107 SymbolicName=MSG_DPADD_LONG
Language=English
%1 %2 <inf>
Adds (installs) a third-party (OEM) driver package.
This command will only work on the local machine.
<inf> is a full path to the INF of the Driver
Package that will be installed on this machine.
.
MessageId=61108 SymbolicName=MSG_DPADD_SHORT
Language=English
%1!-20s! Adds (installs) a third-party (OEM) driver package.
.
MessageId=61109 SymbolicName=MSG_DPDELETE_LONG
Language=English
%1 [-f] %2 <inf>
Deletes a third-party (OEM) driver package.
This command will only work on the local machine.
[-f] will force delete the driver package, even
if it is in use by a device.
<inf> is the name of a published INF on the local
machine.  This is the value returned from dp_add
and dp_enum.
.
MessageId=61110 SymbolicName=MSG_DPDELETE_SHORT
Language=English
%1!-20s! Deletes a third-party (OEM) driver package.
.
MessageId=61111 SymbolicName=MSG_DPENUM_LONG
Language=English
%1 %2
Lists the third-party (OEM) driver packages installed on this machine.
This command will only work on the local machine.
Values returned from dp_enum can be sent to dp_delete 
to be removed from the machine.
.
MessageId=61112 SymbolicName=MSG_DPENUM_SHORT
Language=English
%1!-20s! Lists the third-party (OEM) driver packages installed on this machine.
.
MessageId=61113 SymbolicName=MSG_DPADD_INVALID_INF
Language=English
The specified INF path is not valid.
.
MessageId=61114 SymbolicName=MSG_DPADD_FAILED
Language=English
Adding the specified driver package to the machine failed.
.
MessageId=61115 SymbolicName=MSG_DPADD_SUCCESS
Language=English
Driver package '%1' added.
.
MessageId=61116 SymbolicName=MSG_DPDELETE_FAILED
Language=English
Deleting the specified driver package from the machine failed.
.
MessageId=61117 SymbolicName=MSG_DPDELETE_FAILED_IN_USE
Language=English
Deleting the specified driver package from the machine failed
because it is in use by a device.
.
MessageId=61118 SymbolicName=MSG_DPDELETE_FAILED_NOT_OEM_INF
Language=English
Deleting the specified driver package from the machine failed
because it is not an third-party package.
.
MessageId=61119 SymbolicName=MSG_DPDELETE_SUCCESS
Language=English
Driver package '%1' deleted.
.
MessageId=61120 SymbolicName=MSG_DPENUM_NO_OEM_INF
Language=English
There are no third-party driver packages on this machine.
.
MessageId=61121 SymbolicName=MSG_DPENUM_LIST_HEADER
Language=English
The following third-party driver packages are installed on this computer:
.
MessageId=61122 SymbolicName=MSG_DPENUM_LIST_ENTRY
Language=English
%1
.
MessageId=61123 SymbolicName=MSG_DPENUM_DUMP_PROVIDER
Language=English
    Provider: %1
.
MessageId=61124 SymbolicName=MSG_DPENUM_DUMP_PROVIDER_UNKNOWN
Language=English
    Provider: unknown
.
MessageId=61125 SymbolicName=MSG_DPENUM_DUMP_CLASS
Language=English
    Class: %1
.
MessageId=61126 SymbolicName=MSG_DPENUM_DUMP_CLASS_UNKNOWN
Language=English
    Class: unknown
.
MessageId=61127 SymbolicName=MSG_DPENUM_DUMP_VERSION
Language=English
    Version: %1
.
MessageId=61128 SymbolicName=MSG_DPENUM_DUMP_VERSION_UNKNOWN
Language=English
    Version: unknown
.
MessageId=61129 SymbolicName=MSG_DPENUM_DUMP_DATE
Language=English
    Date: %1
.
MessageId=61130 SymbolicName=MSG_DPENUM_DUMP_DATE_UNKNOWN
Language=English
    Date: unknown
.
MessageId=61131 SymbolicName=MSG_DPENUM_DUMP_SIGNER
Language=English
    Signer: %1
.
MessageId=61132 SymbolicName=MSG_DPENUM_DUMP_SIGNER_UNKNOWN
Language=English
    Signer: unknown
.
;//
;// REMOVE
;//
MessageId=61200 SymbolicName=MSG_REMOVE_LONG
Language=English
Devcon Remove Command
Removes devices with the specified hardware or instance ID. Valid only on
the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=61201 SymbolicName=MSG_REMOVE_SHORT
Language=English
%1!-20s! Remove devices.
.
MessageId=61202 SymbolicName=MSG_REMOVE_TAIL_NONE
Language=English
No devices were removed.
.
MessageId=61203 SymbolicName=MSG_REMOVE_TAIL_REBOOT
Language=English
The %1!u! device(s) are ready to be removed. To remove the devices, reboot the system.
.
MessageId=61204 SymbolicName=MSG_REMOVE_TAIL
Language=English
%1!u! device(s) were removed.
.

;//
;// RESCAN
;//
MessageId=61300 SymbolicName=MSG_RESCAN_LONG
Language=English
Devcon Rescan Command
Directs Plug and Play to scan for new hardware. Valid on a local or remote computer.
%1 [-m:\\<machine>] %2
<machine>    Specifies a remote computer. 
.
MessageId=61301 SymbolicName=MSG_RESCAN_SHORT
Language=English
%1!-20s! Scan for new hardware.
.
MessageId=61302 SymbolicName=MSG_RESCAN_LOCAL
Language=English
Scanning for new hardware.
.
MessageId=61303 SymbolicName=MSG_RESCAN
Language=English
Scanning for new hardware on %1.
.
MessageId=61304 SymbolicName=MSG_RESCAN_OK
Language=English
Scanning completed.
.

;//
;// DRIVERNODES
;//
MessageId=61400 SymbolicName=MSG_DRIVERNODES_LONG
Language=English
Devcon Drivernodes Command
Lists driver nodes for devices with the specified hardware or instance ID.
Valid only on the local computer.
%1 %2 <id> [<id>...]
%1 %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)
.
MessageId=61401 SymbolicName=MSG_DRIVERNODES_SHORT
Language=English
%1!-20s! List driver nodes of devices.
.

;//
;// CLASSFILTER
;//
MessageId=61500 SymbolicName=MSG_CLASSFILTER_LONG
Language=English
Devcon Classfilter Command

Lists, adds, deletes, and reorders upper and lower filter drivers for a device
setup class. Changes do not take effect until the affected devices are restarted
or the machine is rebooted.

%1 %2 [-r] <class> {upper | lower} [<operator><filter> [<operator><filter>...]]
<class>      Specifies a device setup class.
<operator>   Specifies an operation (listed below).
<filter>     Specifies a class filter driver.
upper        Identifies an upper filter driver.
lower        Identifies a lower filter driver.

To list the upper/lower filter drivers for a class, 
type:  devcon classfilter <class> {upper | lower}

The Devcon classfilter command uses subcommands, which consist of an 
operator (=, @, -, +, !) and a filter driver name.

The Devcon classfilter command uses a virtual cursor to move through
the list of filter drivers. The cursor starts at the beginning of the 
list (before the first filter). Unless returned to the starting position,
the cursor always moves forward.

Operators
 =       Move the cursor to the beginning of the filter driver list (before the
         first filter driver).

 @       Position the cursor on the next instance of the specified filter.

 -       Add before. Insert the specified filter before the filter on which the cursor
         is positioned. If the cursor is not positioned on a filter, insert the
         new filter at the beginning of the list. When the subcommand completes, the
         cursor is positioned on the newly-added filter.

 +       Add after. Insert the specified filter after the filter on which the cursor
         is positioned. If the cursor is not positioned on a filter, Devcon inserts the
         new filter at the end of the list. When the subcommand completes, the cursor
         cursor is positioned on the newly-added filter.       

 !       Deletes the next occurrence of the specified filter. When the subcommand 
         completes, the cursor occupies the position of the deleted filter. 
         Subsequent - or + subcommands insert a new filter at the cursor position.


Examples:
If the upper filters for setup class "foo" are A,B,C,B,D,B,E:
%1 %2 foo upper @D !B    - deletes the third 'B'.
%1 %2 foo upper !B !B !B - deletes all three instances of 'B'.
%1 %2 foo upper =!B =!A  - deletes the first 'B' and the first 'A'.
%1 %2 foo upper !C +CC   - replaces 'C' with 'CC'.
%1 %2 foo upper @D -CC   - inserts 'CC' before 'D'.
%1 %2 foo upper @D +CC   - inserts 'CC' after 'D'.
%1 %2 foo upper -CC      - inserts 'CC' before 'A'.
%1 %2 foo upper +CC      - inserts 'CC' after 'E'.
%1 %2 foo upper @D +X +Y - inserts 'X' after 'D' and 'Y' after 'X'.
%1 %2 foo upper @D -X -Y - inserts 'X' before 'D' and 'Y' before 'X'.
%1 %2 foo upper @D -X +Y - inserts 'X' before 'D' and 'Y' between 'X' and 'D'.
.

MessageId=61501 SymbolicName=MSG_CLASSFILTER_SHORT
Language=English
%1!-20s! Add, delete, and reorder class filters.
.
MessageId=61502 SymbolicName=MSG_CLASSFILTER_CHANGED
Language=English
Class filters changed. Restart the devices or reboot the system to make the change effective.
.
MessageId=61503 SymbolicName=MSG_CLASSFILTER_UNCHANGED
Language=English
Class filters unchanged.
.
;//
;// SETHWID
;//
MessageId=61600 SymbolicName=MSG_SETHWID_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...] := <subcmds>
%1 [-m:\\<machine>] %2 =<class> [<id>...] := <subcmds>
Modifies the hardware ID's of the listed devices. This command will only work for root-enumerated devices.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices (not recommended)
ISAPNP\PNP0601     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ROOT\*\*          - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.

<subcmds> consists of one or more:
=hwid              - Clear hardware ID list and set it to hwid.
+hwid              - Add or move hardware ID to head of list (better match).
-hwid              - Add or move hardware ID to end of list (worse match).
!hwid              - Remove hardware ID from list.
hwid               - each additional hardware id is inserted after the previous.
.
MessageId=61601 SymbolicName=MSG_SETHWID_SHORT
Language=English
%1!-20s! Modify Hardware ID's of listed root-enumerated devices.
.

MessageId=61602 SymbolicName=MSG_SETHWID_TAIL_NONE
Language=English
No hardware ID's modified.
.
MessageId=61603 SymbolicName=MSG_SETHWID_TAIL_SKIPPED
Language=English
Skipped %1!u! non-root device(s), modified the hardware ID on %2!u! device(s).
.
MessageId=61604 SymbolicName=MSG_SETHWID_TAIL_MODIFIED
Language=English
Modified the Hardware ID on %1!u! device(s).
.
MessageId=61605 SymbolicName=MSG_SETHWID_NOTROOT
Language=English
Skipping (Not root-enumerated).
.


