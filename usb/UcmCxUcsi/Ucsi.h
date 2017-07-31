/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Ucsi.h

Abstract:

    UCSI specification structure definitions.
    UCSI version 1.0

Environment:

    Kernel-mode and user-mode.

--*/

#pragma once

#include <pshpack1.h>

#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union


#define UCSI_MAX_DATA_LENGTH 0x10
#define UCSI_MAX_NUM_ALT_MODE 0x80
#define UCSI_MIN_TIME_TO_RESPOND_WITH_BUSY 0x0A
#define UCSI_GET_ERROR_STATUS_DATA_LENGTH 0x10
#define UCSI_MAX_NUM_PDOS 0x7 // PD Rev2.0 Ver1.3: "6.2.1.2 Number of Data Objects" and "6.4.1 Capabilities Message"


typedef enum _UCSI_COMMAND
{
    UcsiCommandPpmReset = 0x01,
    UcsiCommandCancel = 0x02,
    UcsiCommandConnectorReset = 0x03,
    UcsiCommandAckCcCi = 0x04,
    UcsiCommandSetNotificationEnable = 0x05,
    UcsiCommandGetCapability = 0x06,
    UcsiCommandGetConnectorCapability = 0x07,
    UcsiCommandSetUom = 0x08,
    UcsiCommandSetUor = 0x09,
    UcsiCommandSetPdm = 0x0A,
    UcsiCommandSetPdr = 0x0B,
    UcsiCommandGetAlternateModes = 0x0C,
    UcsiCommandGetCamSupported = 0x0D,
    UcsiCommandGetCurrentCam = 0x0E,
    UcsiCommandSetNewCam = 0x0F,
    UcsiCommandGetPdos = 0x10,
    UcsiCommandGetCableProperty = 0x11,
    UcsiCommandGetConnectorStatus = 0x12,
    UcsiCommandGetErrorStatus = 0x13,
    UcsiCommandMax = 0x14
} UCSI_COMMAND;


typedef union _UCSI_VERSION
{
    UINT16 AsUInt16;
    struct
    {
        UINT16 SubMinorVersion: 4;
        UINT16 MinorVersion : 4;
        UINT16 MajorVersion : 8;
    };
} UCSI_VERSION, *PUCSI_VERSION;

static_assert(sizeof(UCSI_VERSION) == 2, "Incorrect size");


typedef union _UCSI_CCI
{
    UINT32 AsUInt32;
    struct
    {
        UINT32 : 1;
        UINT32 ConnectorChangeIndicator : 7;
        UINT32 DataLength : 8;
        UINT32 : 9;
        UINT32 NotSupportedIndicator : 1;
        UINT32 CancelCompletedIndicator : 1;
        UINT32 ResetCompletedIndicator : 1;
        UINT32 BusyIndicator : 1;
        UINT32 AcknowledgeCommandIndicator : 1;
        UINT32 ErrorIndicator : 1;
        UINT32 CommandCompletedIndicator : 1;
    };

} UCSI_CCI, *PUCSI_CCI;

static_assert(sizeof(UCSI_CCI) == 4, "Incorrect size");

BOOLEAN
FORCEINLINE
UCSI_CMD_SUCCEEDED (
    _In_ UCSI_CCI Cci
    )
{
    return Cci.CommandCompletedIndicator &&
           !(Cci.NotSupportedIndicator || Cci.CancelCompletedIndicator || Cci.ErrorIndicator);
}


typedef union _UCSI_CONNECTOR_RESET_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 HardReset : 1;
        UINT64 : 40;
    };
} UCSI_CONNECTOR_RESET_COMMAND, *PUCSI_CONNECTOR_RESET_COMMAND;

static_assert(sizeof(UCSI_CONNECTOR_RESET_COMMAND) == 8, "Incorrect size");


typedef union _UCSI_ACK_CC_CI_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorChangeAcknowledge : 1;
        UINT64 CommandCompletedAcknowledge : 1;
        UINT64 : 46;
    };
} UCSI_ACK_CC_CI_COMMAND, *PUCSI_ACK_CC_CI_COMMAND;

static_assert(sizeof(UCSI_ACK_CC_CI_COMMAND) == 8, "Incorrect size");


typedef union _UCSI_SET_NOTIFICATION_ENABLE_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT8 Command;
        UINT8 DataLength;
        union
        {
            UINT16 NotificationEnable;
            struct
            {
                UINT16 CommandCompleteNotificationEnable : 1;
                UINT16 ExternalSupplyChangeNotificationEnable : 1;
                UINT16 PowerOperationModeChangeNotificationEnable : 1;
                UINT16 : 1;
                UINT16 : 1;
                UINT16 SupportedProviderCapabilitiesChangeNotificationEnable : 1;
                UINT16 NegotiatedPowerLevelChangeNotificationEnable : 1;
                UINT16 PdResetNotificationEnable : 1;
                UINT16 SupportedCamChangeNotificationEnable : 1;
                UINT16 : 1;
                UINT16 : 1;
                UINT16 DataRoleSwapCompletedNotificationEnable : 1;
                UINT16 PowerRoleSwapCompletedNotificationEnable : 1;
                UINT16 : 1;
                UINT16 ConnectChangeNotificationEnable : 1;
                UINT16 ErrorNotificationEnable : 1;
            };
        };
        UINT32 : 32;
    };
} UCSI_SET_NOTIFICATION_ENABLE_COMMAND, *PUCSI_SET_NOTIFICATION_ENABLE_COMMAND;

static_assert(sizeof(UCSI_SET_NOTIFICATION_ENABLE_COMMAND) == 8, "Incorrect size");


typedef union _UCSI_BM_POWER_SOURCE
{
    UINT8 AsUInt8;
    struct
    {
        UINT8 AcSupply : 1;
        UINT8 : 1;
        UINT8 Other : 1;
        UINT8 : 3;
        UINT8 UsesVBus : 1;
        UINT8 : 1;
    };
} UCSI_BM_POWER_SOURCE, *PUCSI_BM_POWER_SOURCE;

static_assert(sizeof(UCSI_BM_POWER_SOURCE) == 1, "Incorrect size");

typedef struct _UCSI_GET_CAPABILITY_IN
{
    union
    {
        UINT32 AsUInt32;
        struct
        {
            UINT32 DisabledStateSupport : 1;
            UINT32 BatteryCharging : 1;
            UINT32 UsbPowerDelivery : 1;
            UINT32 : 3;
            UINT32 UsbTypeCCurrent : 1;
            UINT32 : 1;
            UINT32 bmPowerSource : 8;
            UINT32 : 16;
        };
    } bmAttributes;

    union
    {
        UINT8 bNumConnectors : 7;
        UINT8 : 1;
    };

    union
    {
        struct
        {
            UINT32 SetUomSupported : 1;
            UINT32 SetPdmSupported : 1;
            UINT32 AlternateModeDetailsAvailable : 1;
            UINT32 AlternateModeOverrideSupported : 1;
            UINT32 PdoDetailsAvailable : 1;
            UINT32 CableDetailsAvailable : 1;
            UINT32 ExternalSupplyNotificationSupported : 1;
            UINT32 PdResetNotificationSupported : 1;
            UINT32 : 16;
        } bmOptionalFeatures;

        struct
        {
            UINT32 OptionalFeatures : 24;
            UINT32 bNumAltModes : 8;
        };
    };

    UINT8 : 8;
    UINT16 bcdBcVersion;
    UINT16 bcdPdVersion;
    UINT16 bcdUsbTypeCVersion;

} UCSI_GET_CAPABILITY_IN, *PUCSI_GET_CAPABILITY_IN;

static_assert(sizeof(UCSI_GET_CAPABILITY_IN) == 16, "Incorrect size");


typedef union _UCSI_GET_CONNECTOR_CAPABILITY_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 : 41;
    };
} UCSI_GET_CONNECTOR_CAPABILITY_COMMAND, *PUCSI_GET_CONNECTOR_CAPABILITY_COMMAND;

static_assert(sizeof(UCSI_GET_CONNECTOR_CAPABILITY_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_CONNECTOR_CAPABILITY_IN
{
    union
    {
        UINT8 AsUInt8;
        struct
        {
            UINT8 DfpOnly : 1;
            UINT8 UfpOnly : 1;
            UINT8 Drp : 1;
            UINT8 AudioAccessoryMode : 1;
            UINT8 DebugAccessoryMode : 1;
            UINT8 Usb2 : 1;
            UINT8 Usb3 : 1;
            UINT8 AlternateMode : 1;
        };
    } OperationMode;

    UINT8 Provider : 1;
    UINT8 Consumer : 1;
    UINT8 : 6;
} UCSI_GET_CONNECTOR_CAPABILITY_IN, *PUCSI_GET_CONNECTOR_CAPABILITY_IN;

static_assert(sizeof(UCSI_GET_CONNECTOR_CAPABILITY_IN) == 2, "Incorrect size");


typedef enum _UCSI_USB_OPERATION_MODE
{
    UcsiUsbOperationModeDfp = 0x1,
    UcsiUsbOperationModeUfp = 0x2,
    UcsiUsbOperationModeDrp = 0x4
} UCSI_USB_OPERATION_MODE;

typedef union _UCSI_SET_UOM_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 UsbOperationMode : 3;
        UINT64 : 38;
    };
} UCSI_SET_UOM_COMMAND, *PUCSI_SET_UOM_COMMAND;

static_assert(sizeof(UCSI_SET_UOM_COMMAND) == 8, "Incorrect size");


typedef enum _UCSI_USB_OPERATION_ROLE
{
    UcsiUsbOperationRoleDfp = 0x1,
    UcsiUsbOperationRoleUfp = 0x2,
    UcsiUsbOperationRoleAcceptSwap = 0x4
} UCSI_USB_OPERATION_ROLE;

typedef union _UCSI_SET_UOR_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 UsbOperationRole : 3;
        UINT64 : 38;
    };
} UCSI_SET_UOR_COMMAND, *PUCSI_SET_UOR_COMMAND;

static_assert(sizeof(UCSI_SET_UOR_COMMAND) == 8, "Incorrect size");


typedef enum _UCSI_POWER_DIRECTION_MODE
{
    UcsiPowerDirectionModeProvider = 0x1,
    UcsiPowerDirectionModeConsumer = 0x2,
    UcsiPowerDirectionModeEither = 0x4
} UCSI_POWER_DIRECTION_MODE;

typedef union _UCSI_SET_PDM_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 PowerDirectionMode : 3;
        UINT64 : 38;
    };
} UCSI_SET_PDM_COMMAND, *PUCSI_SET_PDM_COMMAND;

static_assert(sizeof(UCSI_SET_PDM_COMMAND) == 8, "Incorrect size");


typedef enum _UCSI_POWER_DIRECTION_ROLE
{
    UcsiPowerDirectionRoleProvider = 0x1,
    UcsiPowerDirectionRoleConsumer = 0x2,
    UcsiPowerDirectionRoleAcceptSwap = 0x4
} UCSI_POWER_DIRECTION_ROLE;

typedef union _UCSI_SET_PDR_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 PowerDirectionRole : 3;
        UINT64 : 38;
    };
} UCSI_SET_PDR_COMMAND, *PUCSI_SET_PDR_COMMAND;

static_assert(sizeof(UCSI_SET_PDR_COMMAND) == 8, "Incorrect size");


typedef enum _UCSI_GET_ALTERNATE_MODES_RECIPIENT
{
    UcsiGetAlternateModesRecipientConnector = 0,
    UcsiGetAlternateModesRecipientSop = 1,
    UcsiGetAlternateModesRecipientSopP = 2,
    UcsiGetAlternateModesRecipientSopPP = 3
} UCSI_GET_ALTERNATE_MODES_RECIPIENT;

typedef union _UCSI_GET_ALTERNATE_MODES_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 Recipient : 3;
        UINT64 : 5;
        UINT64 ConnectorNumber : 7;
        UINT64 : 1;
        UINT64 AlternateModeOffset : 8;
        UINT64 NumberOfAlternateModes : 2;
        UINT64 : 22;
    };
} UCSI_GET_ALTERNATE_MODES_COMMAND, *PUCSI_GET_ALTERNATE_MODES_COMMAND;

static_assert(sizeof(UCSI_GET_ALTERNATE_MODES_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_ALTERNATE_MODE
{
    UINT16 Svid;
    UINT32 Mode;
} UCSI_ALTERNATE_MODE, *PUCSI_ALTERNATE_MODE;

static_assert(sizeof(UCSI_ALTERNATE_MODE) == 6, "Incorrect size");

typedef struct _UCSI_GET_ALTERNATE_MODES_IN
{
    UCSI_ALTERNATE_MODE AlternateModes[2];
} UCSI_GET_ALTERNATE_MODES_IN, *PUCSI_GET_ALTERNATE_MODES_IN;

static_assert(sizeof(UCSI_GET_ALTERNATE_MODES_IN) == 12, "Incorrect size");


typedef union _UCSI_GET_CAM_SUPPORTED_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 : 41;
    };
} UCSI_GET_CAM_SUPPORTED_COMMAND, *PUCSI_GET_CAM_SUPPORTED_COMMAND;

static_assert(sizeof(UCSI_GET_CAM_SUPPORTED_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_CAM_SUPPORTED_IN
{
    UINT8 bmAlternateModeSupported[16];
} UCSI_GET_CAM_SUPPORTED_IN, *PUCSI_GET_CAM_SUPPORTED_IN;

static_assert(sizeof(UCSI_GET_CAM_SUPPORTED_IN) == UCSI_MAX_DATA_LENGTH, "Incorrect size");


typedef union _UCSI_GET_CURRENT_CAM_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 : 41;
    };
} UCSI_GET_CURRENT_CAM_COMMAND, *PUCSI_GET_CURRENT_CAM_COMMAND;

static_assert(sizeof(UCSI_GET_CURRENT_CAM_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_CURRENT_CAM_IN
{
    UINT8 CurrentAlternateMode;
} UCSI_GET_CURRENT_CAM_IN, *PUCSI_GET_CURRENT_CAM_IN;

static_assert(sizeof(UCSI_GET_CURRENT_CAM_IN) == 1, "Incorrect size");


typedef union _UCSI_SET_NEW_CAM_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 EnterOrExit : 1;
        UINT64 NewCam : 8;
        UINT64 AmSpecific: 32;
    };
} UCSI_SET_NEW_CAM_COMMAND, *PUCSI_SET_NEW_CAM_COMMAND;

static_assert(sizeof(UCSI_SET_NEW_CAM_COMMAND) == 8, "Incorrect size");


typedef enum _UCSI_GET_PDOS_TYPE
{
    UcsiGetPdosTypeSink = 0,
    UcsiGetPdosTypeSource = 1
} UCSI_GET_PDOS_TYPE;

typedef enum _UCSI_GET_PDOS_SOURCE_CAPABILITIES_TYPE
{
    UcsiGetPdosCurrentSourceCapabilities = 0,
    UcsiGetPdosAdvertisedSourceCapabilities = 1,
    UcsiGetPdosMaxSourceCapabilities = 2
} UCSI_GET_PDOS_SOURCE_CAPABILITIES_TYPE;

typedef union _UCSI_GET_PDOS_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 PartnerPdo : 1;
        UINT64 PdoOffset : 8;
        UINT64 NumberOfPdos : 2;
        UINT64 SourceOrSinkPdos : 1;
        UINT64 SourceCapabilitiesType : 2;
        UINT64 : 27;
    };
} UCSI_GET_PDOS_COMMAND, *PUCSI_GET_PDOS_COMMAND;

static_assert(sizeof(UCSI_GET_PDOS_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_PDOS_IN
{
    UINT32 Pdos[4];
} UCSI_GET_PDOS_IN, *PUCSI_GET_PDOS_IN;

static_assert(sizeof(UCSI_GET_PDOS_IN) == 16, "Incorrect size");


typedef union _UCSI_GET_CABLE_PROPERTY_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 : 41;
    };
} UCSI_GET_CABLE_PROPERTY_COMMAND, *PUCSI_GET_CABLE_PROPERTY_COMMAND;

static_assert(sizeof(UCSI_GET_CABLE_PROPERTY_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_CABLE_PROPERTY_IN
{
    union
    {
        UINT16 AsUInt16;
        struct
        {
            UINT16 SpeedExponent : 2;
            UINT16 Mantissa : 14;
        };
    } bmSpeedSupported;

    UINT8 bCurrentCapability;
    UINT16 VBusInCable : 1;
    UINT16 CableType : 1;
    UINT16 Directionality : 1;
    UINT16 PlugEndType : 2;
    UINT16 ModeSupport : 1;
    UINT16 : 2;
    UINT16 Latency : 4;
    UINT16 : 4;
} UCSI_GET_CABLE_PROPERTY_IN, *PUCSI_GET_CABLE_PROPERTY_IN;

static_assert(sizeof(UCSI_GET_CABLE_PROPERTY_IN) == 5, "Incorrect size");


typedef union _UCSI_GET_CONNECTOR_STATUS_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 ConnectorNumber : 7;
        UINT64 : 41;
    };
} UCSI_GET_CONNECTOR_STATUS_COMMAND, *PUCSI_GET_CONNECTOR_STATUS_COMMAND;

static_assert(sizeof(UCSI_GET_CONNECTOR_STATUS_COMMAND) == 8, "Incorrect size");

typedef enum _UCSI_POWER_OPERATION_MODE
{
    UcsiPowerOperationModeNoConsumer = 0,
    UcsiPowerOperationModeDefaultUsb = 1,
    UcsiPowerOperationModeBc = 2,
    UcsiPowerOperationModePd = 3,
    UcsiPowerOperationModeTypeC1500 = 4,
    UcsiPowerOperationModeTypeC3000 = 5
} UCSI_POWER_OPERATION_MODE;

typedef enum _UCSI_POWER_DIRECTION
{
    UcsiPowerDirectionConsumer = 0,
    UcsiPowerDirectionProvider = 1
} UCSI_POWER_DIRECTION;

typedef enum _UCSI_CONNECTOR_PARTNER_FLAGS
{
    UcsiConnectorPartnerFlagUsb = 0x1,
    UcsiConnectorPartnerFlagAlternateMode = 0x2
} UCSI_CONNECTOR_PARTNER_FLAGS;

typedef enum _UCSI_CONNECTOR_PARTNER_TYPE
{
    UcsiConnectorPartnerTypeDfp = 1,
    UcsiConnectorPartnerTypeUfp = 2,
    UcsiConnectorPartnerTypePoweredCableNoUfp = 3,
    UcsiConnectorPartnerTypePoweredCableWithUfp = 4,
    UcsiConnectorPartnerTypeDebugAccessory = 5,
    UcsiConnectorPartnerTypeAudioAccessory = 6
} UCSI_CONNECTOR_PARTNER_TYPE;

typedef enum _UCSI_BATTERY_CHARGING_STATUS
{
    UcsiBatteryChargingNotCharging = 0,
    UcsiBatteryChargingNominal = 1,
    UcsiBatteryChargingSlowCharging = 2,
    UcsiBatteryChargingTrickleCharging = 3
} UCSI_BATTERY_CHARGING_STATUS;

typedef struct _UCSI_GET_CONNECTOR_STATUS_IN
{
    union
    {
        UINT16 AsUInt16;
        struct
        {
            UINT16 : 1;
            UINT16 ExternalSupplyChange : 1;
            UINT16 PowerOperationModeChange : 1;
            UINT16 : 1;
            UINT16 : 1;
            UINT16 SupportedProviderCapabilitiesChange : 1;
            UINT16 NegotiatedPowerLevelChange : 1;
            UINT16 PdResetComplete : 1;
            UINT16 SupportedCamChange : 1;
            UINT16 BatteryChargingStatusChange : 1;
            UINT16 : 1;
            UINT16 ConnectorPartnerChange : 1;
            UINT16 PowerDirectionChange : 1;
            UINT16 : 1;
            UINT16 ConnectChange : 1;
            UINT16 Error : 1;
        };
    } ConnectorStatusChange;

    UINT16 PowerOperationMode : 3;
    UINT16 ConnectStatus : 1;
    UINT16 PowerDirection : 1;
    UINT16 ConnectorPartnerFlags : 8;
    UINT16 ConnectorPartnerType : 3;

    UINT32 RequestDataObject;

    union
    {
        struct
        {
            UINT8 BatteryChargingStatus : 2;
            UINT8 PowerBudgetLimitedReason : 4;
            UINT8 : 2;
        };

        struct
        {
            UINT8 : 2;
            UINT8 PowerBudgetLowered : 1;
            UINT8 ReachingPowerBudgetLimit : 1;
            UINT8 : 1;
            UINT8 : 1;
            UINT8 : 2;
        } bmPowerBudgetLimitedReason;
    };
} UCSI_GET_CONNECTOR_STATUS_IN, *PUCSI_GET_CONNECTOR_STATUS_IN;

static_assert(sizeof(UCSI_GET_CONNECTOR_STATUS_IN) == 9, "Incorrect size");


typedef union _UCSI_GET_ERROR_STATUS_COMMAND
{
    UINT64 AsUInt64;
    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 : 48;
    };
} UCSI_GET_ERROR_STATUS_COMMAND, *PUCSI_GET_ERROR_STATUS_COMMAND;

static_assert(sizeof(UCSI_GET_ERROR_STATUS_COMMAND) == 8, "Incorrect size");

typedef struct _UCSI_GET_ERROR_STATUS_IN
{
    union
    {
        UINT16 AsUInt16;
        struct
        {
            UINT16 UnrecognizedCommandError : 1;
            UINT16 NonExistentConnectorNumberError : 1;
            UINT16 InvalidCommandParametersError : 1;
            UINT16 IncompatibleConnectorPartnerError : 1;
            UINT16 CcCommunicationError : 1;
            UINT16 CommandFailureDueToDeadBattery : 1;
            UINT16 ContractNegotiationFailure : 1;
            UINT16 : 9;
        };
    } ErrorInformation;

    UINT8 VendorDefined[14];

} UCSI_GET_ERROR_STATUS_IN, *PUCSI_GET_ERROR_STATUS_IN;

static_assert(sizeof(UCSI_GET_ERROR_STATUS_IN) == UCSI_MAX_DATA_LENGTH, "Incorrect size");


typedef union _UCSI_CONTROL
{
    UINT64 AsUInt64;

    struct
    {
        UINT64 Command : 8;
        UINT64 DataLength : 8;
        UINT64 CommandSpecific : 48;
    };

    UCSI_CONNECTOR_RESET_COMMAND ConnectorReset;
    UCSI_ACK_CC_CI_COMMAND AckCcCi;
    UCSI_SET_NOTIFICATION_ENABLE_COMMAND SetNotificationEnable;
    UCSI_GET_CONNECTOR_CAPABILITY_COMMAND GetConnectorCapability;
    UCSI_SET_UOM_COMMAND SetUom;
    UCSI_SET_UOR_COMMAND SetUor;
    UCSI_SET_PDM_COMMAND SetPdm;
    UCSI_SET_PDR_COMMAND SetPdr;
    UCSI_GET_ALTERNATE_MODES_COMMAND GetAlternateModes;
    UCSI_GET_CAM_SUPPORTED_COMMAND GetCamSupported;
    UCSI_GET_CURRENT_CAM_COMMAND GetCurrentCam;
    UCSI_SET_NEW_CAM_COMMAND SetNewCam;
    UCSI_GET_PDOS_COMMAND GetPdos;
    UCSI_GET_CABLE_PROPERTY_COMMAND GetCableProperty;
    UCSI_GET_CONNECTOR_STATUS_COMMAND GetConnectorStatus;
    UCSI_GET_ERROR_STATUS_COMMAND GetErrorStatus;

} UCSI_CONTROL, *PUCSI_CONTROL;

static_assert(sizeof(UCSI_CONTROL) == 8, "Incorrect size");


typedef union _UCSI_MESSAGE_IN
{
    UINT8 AsBuffer[UCSI_MAX_DATA_LENGTH];

    UCSI_GET_CAPABILITY_IN Capability;
    UCSI_GET_CONNECTOR_CAPABILITY_IN ConnectorCapability;
    UCSI_GET_ALTERNATE_MODES_IN AlternateModes;
    UCSI_GET_CAM_SUPPORTED_IN CamSupported;
    UCSI_GET_CURRENT_CAM_IN CurrentCam;
    UCSI_GET_PDOS_IN Pdos;
    UCSI_GET_CABLE_PROPERTY_IN CableProperty;
    UCSI_GET_CONNECTOR_STATUS_IN ConnectorStatus;
    UCSI_GET_ERROR_STATUS_IN ErrorStatus;

} UCSI_MESSAGE_IN, *PUCSI_MESSAGE_IN;

static_assert(sizeof(UCSI_MESSAGE_IN) == UCSI_MAX_DATA_LENGTH, "Incorrect size");


typedef union _UCSI_MESSAGE_OUT
{
    UINT8 AsBuffer[UCSI_MAX_DATA_LENGTH];

} UCSI_MESSAGE_OUT, *PUCSI_MESSAGE_OUT;

static_assert(sizeof(UCSI_MESSAGE_OUT) == UCSI_MAX_DATA_LENGTH, "Incorrect size");


typedef struct _UCSI_DATA_BLOCK
{
    UCSI_VERSION UcsiVersion;
    UINT16 : 16;
    UCSI_CCI CCI;
    UCSI_CONTROL Control;
    UCSI_MESSAGE_IN MessageIn;
    UCSI_MESSAGE_OUT MessageOut;
} UCSI_DATA_BLOCK, *PUCSI_DATA_BLOCK;

static_assert(sizeof(UCSI_DATA_BLOCK) == 48, "Incorrect size");


#pragma warning(pop)

#include <poppack.h>
