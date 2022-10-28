#include <wwan.h>
#include <MbbMessages.h>

#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  Table mapping between WWAN type enum to its corresponding MBB type enum
//        payload or other way around. When update the table, please double
//        check wwan.h and MbbMessages.h
//
////////////////////////////////////////////////////////////////////////////////

#pragma region To_WWAN_ENUM
WWAN_STATUS MbbStatusMapTableBasic[MBB_STATUS_BASIC_COUNT()] = {
    // MBB_STATUS_SUCCESS =                    0,
    WWAN_STATUS_SUCCESS,
    // MBB_STATUS_BUSY =                       1,
    WWAN_STATUS_BUSY,
    // MBB_STATUS_FAILURE =                    2,
    WWAN_STATUS_FAILURE,
    // MBB_STATUS_SIM_NOT_INSERTED =           3,
    WWAN_STATUS_SIM_NOT_INSERTED,
    // MBB_STATUS_BAD_SIM =                    4,
    WWAN_STATUS_BAD_SIM,
    // MBB_STATUS_PIN_REQUIRED =               5,
    WWAN_STATUS_PIN_REQUIRED,
    // MBB_STATUS_PIN_DISABLED =               6,
    WWAN_STATUS_PIN_DISABLED,
    // MBB_STATUS_NOT_REGISTERED =             7,
    WWAN_STATUS_NOT_REGISTERED,
    // MBB_STATUS_PROVIDERS_NOT_FOUND =        8,
    WWAN_STATUS_PROVIDERS_NOT_FOUND,
    // MBB_STATUS_NO_DEVICE_SUPPORT =          9,
    WWAN_STATUS_NO_DEVICE_SUPPORT,
    // MBB_STATUS_PROVIDER_NOT_VISIBLE =       10,
    WWAN_STATUS_PROVIDER_NOT_VISIBLE,
    // MBB_STATUS_DATA_CLASS_NOT_AVAILABLE =   11,
    WWAN_STATUS_DATA_CLASS_NOT_AVAILABLE,
    // MBB_STATUS_PACKET_SVC_DETACHED =        12,
    WWAN_STATUS_PACKET_SVC_DETACHED,
    // MBB_STATUS_MAX_ACTIVATED_CONTEXTS =     13,
    WWAN_STATUS_MAX_ACTIVATED_CONTEXTS,
    // MBB_STATUS_NOT_INITIALIZED =            14,
    WWAN_STATUS_NOT_INITIALIZED,
    // MBB_STATUS_VOICE_CALL_IN_PROGRESS =     15,
    WWAN_STATUS_VOICE_CALL_IN_PROGRESS,
    // MBB_STATUS_CONTEXT_NOT_ACTIVATED =      16,
    WWAN_STATUS_CONTEXT_NOT_ACTIVATED,
    // MBB_STATUS_SERVICE_NOT_ACTIVATED =      17,
    WWAN_STATUS_SERVICE_NOT_ACTIVATED,
    // MBB_STATUS_INVALID_ACCESS_STRING =      18,
    WWAN_STATUS_INVALID_ACCESS_STRING,
    // MBB_STATUS_INVALID_USER_NAME_PWD =      19,
    WWAN_STATUS_INVALID_USER_NAME_PWD,
    // MBB_STATUS_RADIO_POWER_OFF =            20,
    WWAN_STATUS_RADIO_POWER_OFF,
    // MBB_STATUS_INVALID_PARAMETERS =         21,
    WWAN_STATUS_INVALID_PARAMETERS,
    // MBB_STATUS_READ_FAILURE =               22,
    WWAN_STATUS_READ_FAILURE,
    // MBB_STATUS_WRITE_FAILURE =              23,
    WWAN_STATUS_WRITE_FAILURE,
    // MBB_STATUS_DENIED_POLICY =              24,
    WWAN_STATUS_DENIED_POLICY,
    // MBIM_STATUS_NO_PHONEBOOK                25,
    WWAN_STATUS_FAILURE,
    // MBIM_STATUS_PARAMETER_TOO_LONG          26,
    WWAN_STATUS_FAILURE,
    // MBIM_STATUS_STK_BUSY                    27,
    WWAN_STATUS_FAILURE,
    // MBIM_STATUS_OPERATION_NOT_ALLOWED       28,
    WWAN_STATUS_NO_DEVICE_SUPPORT,
    // MBIM_STATUS _MEMORY_FAILURE             29,
    WWAN_STATUS_SMS_MEMORY_FAILURE,
    // MBIM_STATUS_INVALID_MEMORY_INDEX        30,
    WWAN_STATUS_SMS_INVALID_MEMORY_INDEX,
    // MBIM_STATUS_MEMORY_FULL                 31,
    WWAN_STATUS_SMS_MEMORY_FULL,
    // MBIM_STATUS _FILTER_NOT_SUPPORTED       32,
    WWAN_STATUS_SMS_FILTER_NOT_SUPPORTED,
    // MBIM_STATUS_DSS_INSTANCE_LIMIT          33,
    WWAN_STATUS_FAILURE,
    // MBIM_STATUS_INVALID_DEVICE_SERVICE_OPERATION 34,
    WWAN_STATUS_FAILURE,
    // MBIM_STATUS_AUTH_INCORRECT_AUTN         35,
    WWAN_STATUS_AUTH_INCORRECT_AUTN,
    // MBIM_STATUS_AUTH_SYNC_FAILURE           36,
    WWAN_STATUS_AUTH_SYNC_FAILURE,
    // MBIM_STATUS_AUTH_AMF_NOT_SET            37,
    WWAN_STATUS_AUTH_AMF_NOT_SET,
    // MBIM_STATUS_CONTEXT_NOT_SUPPORTED       38,
    WWAN_STATUS_MORE_DATA,
    // MBB_STATUS_SHAREABILITY_CONDITION_ERROR 39,
    WWAN_STATUS_SHAREABILITY_CONDITION_ERROR,
    // MBB_STATUS_PIN_FAILURE                  40,
    WWAN_STATUS_PIN_FAILURE,
    // MBB_STATUS_NO_LTE_ATTACH_CONFIG         41,
    WWAN_STATUS_NO_LTE_ATTACH_CONFIG

};

WWAN_STATUS MbbStatusMapTableSms[MBB_STATUS_SMS_COUNT()] = {

    // MBB_STATUS_SMS_UNKNOWN_SMSC_ADDRESS =   100,
    WWAN_STATUS_SMS_UNKNOWN_SMSC_ADDRESS,
    // MBB_STATUS_SMS_NETWORK_TIMEOUT =        101,
    WWAN_STATUS_SMS_NETWORK_TIMEOUT,
    // MBB_STATUS_SMS_LANG_NOT_SUPPORTED =     102,
    WWAN_STATUS_SMS_LANG_NOT_SUPPORTED,
    // MBB_STATUS_SMS_ENCODING_NOT_SUPPORTED = 103,
    WWAN_STATUS_SMS_ENCODING_NOT_SUPPORTED,
    // MBB_STATUS_SMS_FORMAT_NOT_SUPPORTED =   104,
    WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED,
    // MBB_STATUS_SMS_MORE_DATA =              105,
    WWAN_STATUS_SMS_MORE_DATA

};

WWAN_STATUS MbbStatusMapTableUicc[MBB_STATUS_UICC_COUNT()] = {

    // MBB_STATUS_UICC_NO_LOGICAL_CHANNELS =        0x87430001,
    WWAN_STATUS_UICC_NO_LOGICAL_CHANNELS,
    // MBB_STATUS_UICC_SELECT_FAILED =              0x87430002,
    WWAN_STATUS_UICC_SELECT_FAILED,
    // MBB_STATUS_UICC_INVALID_LOGICAL_CHANNEL =    0x87430003,
    WWAN_STATUS_UICC_INVALID_LOGICAL_CHANNEL

};

WWAN_STATUS MbbStatusMapTable4_0[MBB_STATUS_4_0_COUNT()] = {
    // MBB_STATUS_MATCHING_PDU_SESSION_FOUND = 200,
    WWAN_STATUS_SESSION_ALREADY_EXISTS,
    // MBB_STATUS_DISSOCIATION_NEEDED_FOR_APPLICATION = 201,
    WWAN_STATUS_DISSOCIATION_NEEDED_FOR_APPLICATION,
    // MBB_STATUS_ERROR_INVALID_SLOT = 202,
    WWAN_STATUS_ERROR_INVALID_SLOT,
    // MBB_STATUS_NO_MATCHING_URSP_RULE = 203,
    WWAN_STATUS_NO_MATCHING_URSP_RULE,
    // MBB_STATUS_NO_DEFAULT_URSP_RULE = 204,
    WWAN_STATUS_NO_DEFAULT_URSP_RULE
};

// defining the maping table for IpTypes
// this implementation assume that the MBB_CONTEXT_IP_TYPE
// does not have gaps and will end with MbbContextIPTypeMaximum
WWAN_IP_TYPE MbbIpTypesMapTable[MbbContextIPTypeMaximum] = {
    // MbbContextIPTypeDefault = 0,
    WwanIPTypeDefault,
    // MbbContextIPTypeIPv4 = 1,
    WwanIPTypeIPv4,
    // MbbContextIPTypeIPv6 = 2,
    WwanIPTypeIPv6,
    // MbbContextIPTypeIPv4v6 = 3,
    WwanIPTypeIpv4v6,
    // MbbContextIPTypeIPv4AndIPv6 = 4,
    // IPv4AndIPv6 is reported as IPv4v6
    // SHOULD NOT be mapped to WwanIPTypeXlat!!!!
    WwanIPTypeIpv4v6};

WWAN_CONFIGURATION_SOURCE MbbSourcesMapTable[MbbMsContextSourceMaximum] = {
    // MbbMsContextSourceAdmin = 0,
    WwanAdminProvisioned,
    // MbbMsContextSourceUser  = 1,
    WwanUserProvisioned,
    // MbbMsContextSourceOperator  = 2,
    WwanOperatorProvisioned,
    // MbbMsContextSourceModem = 3,
    WwanModemProvisioned,
    // MbbMsContextSourceDevice    = 4,
    WwanDeviceProvisioned,
};

WWAN_CONTEXT_OPERATIONS MbbLteAttachOperationsMapTable[MbbMsLteAttachContextOperationMaximum] = {
    // MbbMsLteAttachContextOperationDefault = 0,
    WwanContextOperationDefault,
    // MbbMsLteAttachContextOperationRestoreFactory  = 1,
    WwanContextOperationRestoreFactory,
    // MbbMsLteAttachContextOperationMaximum = 2,
};
#pragma endregion To_WWAN_ENUM

#pragma region To_MBB_ENUM
MBB_CONTEXT_IP_TYPE WwanIpTypesMapTable[] = {
    // WwanIPTypeDefault = 0,
    MbbContextIPTypeDefault,
    // WwanIPTypeIPv4,
    MbbContextIPTypeIPv4,
    // WwanIPTypeIPv6,
    MbbContextIPTypeIPv6,
    // WwanIPTypeIpv4v6,
    MbbContextIPTypeIPv4v6,
};

MBB_BASICCONNECTEXT_CONTEXT_SOURCE WwanSourcsMapTable[WwanMaxProvisionSource] = {
    // WwanUserProvisioned = 0,        // the source is user(s)
    MbbMsContextSourceUser,
    // WwanAdminProvisioned,           // the source is administrator
    MbbMsContextSourceAdmin,
    // WwanOperatorProvisioned,        // the source is mobile operator
    MbbMsContextSourceOperator,
    // WwanDeviceProvisioned,          // the source is device (such as MultiVariant), but not from modem
    MbbMsContextSourceDevice,
    // WwanModemProvisioned,           // the source is modem (such as pre-configured in modem)
    MbbMsContextSourceModem,
};

MBB_BASICCONNECTEXT_LTEATTACH_CONTEXT_OPERATIONS
WwanLteAttachOperationsMapTable[WwanContextOperationMaximum] = {
    // WwanContextOperationDefault = 0,
    MbbMsLteAttachContextOperationDefault,
    // WwanContextOperationDelete = 1,
    MbbMsLteAttachContextOperationMaximum, // no deletion operation for LTE Attach
                                           // WwanContextOperationRestoreFactory  = 2,
    MbbMsLteAttachContextOperationRestoreFactory,
    // WwanContextOperationMaximum = 3
};
#pragma endregion To_MBB_ENUM