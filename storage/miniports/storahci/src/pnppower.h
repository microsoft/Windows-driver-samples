/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    pnppower.h

Abstract:

    

Notes:

Revision History:

--*/

#pragma once

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

// ACPI methods
#define ACPI_METHOD_GTF   ((ULONG) 'FTG_') // _GTF
#define ACPI_METHOD_SDD   ((ULONG) 'DDS_') // _SDD
#define ACPI_METHOD_PR3   ((ULONG) '3RP_') // _PR3
#define ACPI_METHOD_DSM   ((ULONG) 'MSD_') // _DSM

// ACPI _DSM method related definition
#define ACPI_METHOD_DSM_LINKPOWER_REVISION              0x01

#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_SUPPORT      0x00
#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_QUERY        0x01
#define ACPI_METHOD_DSM_LINKPOWER_FUNCTION_CONTROL      0x02

#define ACPI_METHOD_DSM_LINKPOWER_REMOVE_POWER          0x00
#define ACPI_METHOD_DSM_LINKPOWER_APPLY_POWER           0x01

#define AHCI_DEVICE_IO_COALESCING_IDLE_TIMEOUT_MS       (1000) // 1s by default

//
// When waiting for the link to change power states, don't wait more
// than 100 microseconds.
//
#define AHCI_LINK_POWER_STATE_CHANGE_TIMEOUT_US   100

typedef union _AHCI_LPM_POWER_SETTINGS {
    struct {
    //LSB
    ULONG HipmEnabled: 1;
    ULONG DipmEnabled: 1;
    ULONG Reserved: 30;
    //MSB
    };

    ULONG AsUlong;

} AHCI_LPM_POWER_SETTINGS, *PAHCI_LPM_POWER_SETTINGS;

BOOLEAN 
AhciPortInitialize(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

BOOLEAN 
AhciAdapterPowerUp(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

BOOLEAN 
AhciAdapterPowerSettingNotification(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTOR_POWER_SETTING_INFO PowerSettingInfo
    );

BOOLEAN 
AhciAdapterPowerDown(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID 
AhciPortPowerUp(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID 
AhciPortPowerDown(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID 
AhciPortStop(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID
AhciPortSmartCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  );

VOID
AhciPortNVCacheCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  );

VOID
AhciPortGetInitCommands(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
  );

VOID
AhciPortEvaluateSDDMethod(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
  );

VOID
AhciAdapterEvaluateDSMMethod(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
  );

VOID
AhciPortAcpiDSMControl(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ ULONG                   PortNumber,
    _In_ BOOLEAN                 Sleep
  );

VOID
IssuePreservedSettingCommands(
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
  );

VOID
IssueInitCommands(
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
  );

VOID
IssueSetDateAndTimeCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN SendStandBy
  );

VOID
IssueReadLogExtCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ UCHAR  LogAddress,
    _In_ USHORT PageNumber,
    _In_ USHORT BlockCount,
    _In_ USHORT FeatureField,
    _In_opt_ PSTOR_PHYSICAL_ADDRESS PhysicalAddress,
    _In_ PVOID DataBuffer,
    _In_opt_ PSRB_COMPLETION_ROUTINE CompletionRoutine
    );

HW_TIMER_EX AhciAutoPartialToSlumber;

BOOLEAN
AhciLpmSettingsModes(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ AHCI_LPM_POWER_SETTINGS LpmMode
    );


#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif
