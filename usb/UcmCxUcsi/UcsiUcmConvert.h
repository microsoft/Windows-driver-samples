/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcsiUcmConvert.h

Abstract:

    Helpers to convert between UCM and UCSI enums.

Environment:

    Kernel-mode only.

--*/

#pragma once

template<typename A, typename B>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert (
    _In_ A From,
    _Out_ B& To
    );


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_CONNECTOR_PARTNER_TYPE, UCM_TYPEC_PARTNER> (
    _In_ UCSI_CONNECTOR_PARTNER_TYPE PartnerType,
    _Out_ UCM_TYPEC_PARTNER& ConvertedValue
    )
{
    UCM_TYPEC_PARTNER ucmPartner;

    switch (PartnerType)
    {
    case UcsiConnectorPartnerTypeDfp:
        ucmPartner = UcmTypeCPartnerDfp;
        break;
    case UcsiConnectorPartnerTypeUfp:
        ucmPartner = UcmTypeCPartnerUfp;
        break;
    case UcsiConnectorPartnerTypePoweredCableNoUfp:
        ucmPartner = UcmTypeCPartnerPoweredCableNoUfp;
        break;
    case UcsiConnectorPartnerTypePoweredCableWithUfp:
        ucmPartner = UcmTypeCPartnerPoweredCableWithUfp;
        break;
    case UcsiConnectorPartnerTypeDebugAccessory:
        ucmPartner = UcmTypeCPartnerDebugAccessory;
        break;
    case UcsiConnectorPartnerTypeAudioAccessory:
        ucmPartner = UcmTypeCPartnerAudioAccessory;
        break;
    default:
        return false;
    }

    ConvertedValue = ucmPartner;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_BATTERY_CHARGING_STATUS, UCM_CHARGING_STATE> (
    _In_ UCSI_BATTERY_CHARGING_STATUS ChargingState,
    _Out_ UCM_CHARGING_STATE& ConvertedValue
    )
{
    UCM_CHARGING_STATE ucmChargingState;

    switch (ChargingState)
    {
    case UcsiBatteryChargingNotCharging:
        ucmChargingState = UcmChargingStateNotCharging;
        break;
    case UcsiBatteryChargingSlowCharging:
        ucmChargingState = UcmChargingStateSlowCharging;
        break;
    case UcsiBatteryChargingTrickleCharging:
        ucmChargingState = UcmChargingStateTrickleCharging;
        break;
    case UcsiBatteryChargingNominal:
        ucmChargingState = UcmChargingStateNominalCharging;
        break;
    default:
        return false;
    }

    ConvertedValue = ucmChargingState;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCM_DATA_ROLE, UCSI_USB_OPERATION_ROLE> (
    _In_ UCM_DATA_ROLE DataRole,
    _Out_ UCSI_USB_OPERATION_ROLE& UsbRole
    )
{
    UCSI_USB_OPERATION_ROLE usbRole;

    switch (DataRole)
    {
    case UcmDataRoleDfp:
        usbRole = UcsiUsbOperationRoleDfp;
        break;
    case UcmDataRoleUfp:
        usbRole = UcsiUsbOperationRoleUfp;
        break;
    default:
        return false;
    }

    UsbRole = usbRole;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_USB_OPERATION_ROLE, UCM_DATA_ROLE> (
    _In_ UCSI_USB_OPERATION_ROLE UsbRole,
    _Out_ UCM_DATA_ROLE& DataRole
    )
{
    UCM_DATA_ROLE dataRole;

    switch (UsbRole)
    {
    case UcsiUsbOperationRoleDfp:
        dataRole = UcmDataRoleDfp;
        break;
    case UcsiUsbOperationRoleUfp:
        dataRole = UcmDataRoleUfp;
        break;
    default:
        return false;
    }

    DataRole = dataRole;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_POWER_OPERATION_MODE, UCM_TYPEC_CURRENT> (
    _In_ UCSI_POWER_OPERATION_MODE PowerOperationMode,
    _Out_ UCM_TYPEC_CURRENT& ConvertedValue
    )
{
    UCM_TYPEC_CURRENT typeCCurrent;

    switch (PowerOperationMode)
    {
    case UcsiPowerOperationModeTypeC1500:
        typeCCurrent = UcmTypeCCurrent1500mA;
        break;
    case UcsiPowerOperationModeTypeC3000:
        typeCCurrent = UcmTypeCCurrent3000mA;
        break;
    case UcsiPowerOperationModeBc:
    case UcsiPowerOperationModeDefaultUsb:
    case UcsiPowerOperationModePd:
        typeCCurrent = UcmTypeCCurrentDefaultUsb;
        break;
    default:
        return false;
    }

    ConvertedValue = typeCCurrent;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCM_POWER_ROLE, UCSI_POWER_DIRECTION_ROLE> (
    _In_ UCM_POWER_ROLE PowerRole,
    _Out_ UCSI_POWER_DIRECTION_ROLE& ConvertedValue
    )
{
    UCSI_POWER_DIRECTION_ROLE powerRole;

    switch (PowerRole)
    {
    case UcmPowerRoleSource:
        powerRole = UcsiPowerDirectionRoleProvider;
        break;
    case UcmPowerRoleSink:
        powerRole = UcsiPowerDirectionRoleConsumer;
        break;
    default:
        return false;
    }

    ConvertedValue = powerRole;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_POWER_DIRECTION_ROLE, UCM_POWER_ROLE> (
    _In_ UCSI_POWER_DIRECTION_ROLE PowerRole,
    _Out_ UCM_POWER_ROLE& ConvertedValue
    )
{
    UCM_POWER_ROLE powerRole;

    switch (PowerRole)
    {
    case UcsiPowerDirectionRoleProvider:
        powerRole = UcmPowerRoleSource;
        break;
    case UcsiPowerDirectionRoleConsumer:
        powerRole = UcmPowerRoleSink;
        break;
    default:
        return false;
    }

    ConvertedValue = powerRole;
    return true;
}


template<>
_Success_(return != false)
_Must_inspect_result_
bool
FORCEINLINE
Convert<UCSI_POWER_DIRECTION, UCM_POWER_ROLE> (
    _In_ UCSI_POWER_DIRECTION PowerDirection,
    _Out_ UCM_POWER_ROLE& ConvertedValue
    )
{
    UCM_POWER_ROLE powerRole;

    switch (PowerDirection)
    {
    case UcsiPowerDirectionProvider:
        powerRole = UcmPowerRoleSource;
        break;
    case UcsiPowerDirectionConsumer:
        powerRole = UcmPowerRoleSink;
        break;
    default:
        return false;
    }

    ConvertedValue = powerRole;
    return true;
}
