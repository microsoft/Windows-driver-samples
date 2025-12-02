//
//    Copyright (C) Microsoft.  All rights reserved.
//

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0625 */
/* at Mon Jan 18 19:14:07 2038
 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#if 0
#include <rpc.h>
#include <rpcndr.h>
#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */
#endif 

#ifndef __MbbMessages_h__
#define __MbbMessages_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if _CONTROL_FLOW_GUARD_XFG
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

/* header files for imported files */
// #include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_MbbMessages_0000_0000 */
/* [local] */ 

//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include <initguid.h>
//
// Service Ids
//
// Invald GUID
// 00000000-0000-0000-0000-000000000000
DEFINE_GUID( MBB_UUID_INVALID, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
#define MBB_UUID_INVALID_CONSTANT { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }
// Basic IP Connectivity
// a289cc33-bcbb-8b4f-b6b0-133ec2aae6df
DEFINE_GUID( MBB_UUID_BASIC_CONNECT, 0xa289cc33, 0xbcbb, 0x8b4f, 0xb6, 0xb0, 0x13, 0x3e, 0xc2, 0xaa, 0xe6, 0xdf );
#define MBB_UUID_BASIC_CONNECT_CONSTANT { 0xa289cc33, 0xbcbb, 0x8b4f, { 0xb6, 0xb0, 0x13, 0x3e, 0xc2, 0xaa, 0xe6, 0xdf } }
// SMS
// 533fbeeb-14fe-4467-9f90-33a223e56c3f
DEFINE_GUID( MBB_UUID_SMS, 0x533fbeeb, 0x14fe, 0x4467, 0x9f, 0x90, 0x33, 0xa2, 0x23, 0xe5, 0x6c, 0x3f );
#define MBB_UUID_SMS_CONSTANT { 0x533fbeeb, 0x14fe, 0x4467, { 0x9f, 0x90, 0x33, 0xa2, 0x23, 0xe5, 0x6c, 0x3f } }
// USSD
// e550a0c8-5e82-479e-82f7-10abf4c3351f
DEFINE_GUID( MBB_UUID_USSD, 0xe550a0c8, 0x5e82, 0x479e, 0x82, 0xf7, 0x10, 0xab, 0xf4, 0xc3, 0x35, 0x1f );
#define MBB_UUID_USSD_CONSTANT { 0xe550a0c8, 0x5e82, 0x479e, { 0x82, 0xf7, 0x10, 0xab, 0xf4, 0xc3, 0x35, 0x1f } }
// Phonebook
// 4bf38476-1e6a-41db-b1d8-bed289c25bdb
DEFINE_GUID( MBB_UUID_PHONEBOOK, 0x4bf38476, 0x1e6a, 0x41db, 0xb1, 0xd8, 0xbe, 0xd2, 0x89, 0xc2, 0x5b, 0xdb );
#define MBB_UUID_PHONEBOOK_CONSTANT { 0x4bf38476, 0x1e6a, 0x41db, { 0xb1, 0xd8, 0xbe, 0xd2, 0x89, 0xc2, 0x5b, 0xdb } }
// SIM Application Toolkit
// d8f20131-fcb5-4e17-8602-d6ed3816164c
DEFINE_GUID( MBB_UUID_SAT, 0xd8f20131, 0xfcb5, 0x4e17, 0x86, 0x02, 0xd6, 0xed, 0x38, 0x16, 0x16, 0x4c );
#define MBB_UUID_SAT_CONSTANT { 0xd8f20131, 0xfcb5, 0x4e17, { 0x86, 0x02, 0xd6, 0xed, 0x38, 0x16, 0x16, 0x4c } }
// Windows 7 Vendor Extension
// b492e7e2-adba-499d-8401-7794bb913c1c
DEFINE_GUID( MBB_UUID_MS_VENDOR_EXTENSION, 0xb492e7e2, 0xadba, 0x499d, 0x84, 0x01, 0x77, 0x94, 0xbb, 0x91, 0x3c, 0x1c );
#define MBB_UUID_MS_VENDOR_EXTENSION_CONSTANT { 0xb492e7e2, 0xadba, 0x499d, { 0x84, 0x01, 0x77, 0x94, 0xbb, 0x91, 0x3c, 0x1c } }
// Authentication
// 1d2b5ff7-0aa1-48b2-aa52-50f15767174e
DEFINE_GUID( MBB_UUID_AUTH, 0x1d2b5ff7, 0x0aa1, 0x48b2, 0xaa, 0x52, 0x50, 0xf1, 0x57,0x67, 0x17, 0x4e );
#define MBB_UUID_AUTH_CONSTANT { 0x1d2b5ff7, 0x0aa1, 0x48b2, { 0xaa, 0x52, 0x50, 0xf1, 0x57,0x67, 0x17, 0x4e } }
// Device Service Stream
// c08a26dd-7718-4382-8482-6e0d583c4d0e
DEFINE_GUID( MBB_UUID_DSS, 0xc08a26dd, 0x7718, 0x4382, 0x84, 0x82, 0x6e, 0x0d, 0x58, 0x3c, 0x4d, 0x0e );
#define MBB_UUID_DSS_CONSTANT { 0xc08a26dd, 0x7718, 0x4382, { 0x84, 0x82, 0x6e, 0x0d, 0x58, 0x3c, 0x4d, 0x0e } }
// Multi-carrier device service
// 8b569648-628d-4653-9b9f-1025404424e1
DEFINE_GUID( MBB_UUID_MULTICARRIER, 0x8b569648, 0x628d, 0x4653, 0x9b, 0x9f, 0x10, 0x25, 0x40, 0x44, 0x24, 0xe1 );
#define MBB_UUID_MULTICARRIER_CONSTANT { 0x8b569648, 0x628d, 0x4653, { 0x9b, 0x9f, 0x10, 0x25, 0x40, 0x44, 0x24, 0xe1 } }
// Host Shutdown device service
// 883b7c26-985f-43fa-9804-27d7fb80959c
DEFINE_GUID( MBB_UUID_HOSTSHUTDOWN, 0x883b7c26, 0x985f, 0x43fa, 0x98, 0x04, 0x27, 0xd7, 0xfb, 0x80, 0x95, 0x9c );
#define MBB_UUID_HOSTSHUTDOWN_CONSTANT { 0x883b7c26, 0x985f, 0x43fa, { 0x98, 0x04, 0x27, 0xd7, 0xfb, 0x80, 0x95, 0x9c } }
// MBIM 2.0 voice extensions
// 8d8b9eba-37be-449b-8f1e-61cb034a702e
DEFINE_GUID( MBB_UUID_VOICEEXTENSIONS, 0x8d8b9eba, 0x37be, 0x449b, 0x8f, 0x1e, 0x61, 0xcb, 0x03, 0x4a, 0x70, 0x2e );
#define MBB_UUID_VOICEEXTENSIONS_CONSTANT { 0x8d8b9eba, 0x37be, 0x449b, { 0x8f, 0x1e, 0x61, 0xcb, 0x03, 0x4a, 0x70, 0x2e } }
// Low-Level UICC Access
// c2f6588e-f037-4bc9-8665-f4d44bd09367
DEFINE_GUID( MBB_UUID_UICC_LOW_LEVEL, 0xc2f6588e, 0xf037, 0x4bc9, 0x86, 0x65, 0xf4, 0xd4, 0x4b, 0xd0, 0x93, 0x67 );
#define MBB_UUID_UICC_LOW_LEVEL_CONSTANT { 0xc2f6588e, 0xf037, 0x4bc9, { 0x86, 0x65, 0xf4, 0xd4, 0x4b, 0xd0, 0x93, 0x67 } }
// Selective Absorption Rate (SAR) control
// 68223d04-9f6c-4e0f-822d-28441fb72340
DEFINE_GUID( MBB_UUID_SARCONTROL, 0x68223d04, 0x9f6c, 0x4e0f, 0x82, 0x2d, 0x28, 0x44, 0x1f, 0xb7, 0x23, 0x40 );
#define MBB_UUID_SARCONTROL_CONSTANT { 0x68223d04, 0x9f6c, 0x4e0f, { 0x82, 0x2d, 0x28, 0x44, 0x1f, 0xb7, 0x23, 0x40 } }
// Basic IP Connectivity Extensions (a private Microsoft service not approved by MBIM USB forum as of 2016/11/09)
// 3d01dcc5-fef5-4d05-0d3a-bef7058e9aaf
DEFINE_GUID( MBB_UUID_BASIC_CONNECT_EXTENSIONS, 0x3d01dcc5, 0xfef5, 0x4d05, 0x0d, 0x3a, 0xbe, 0xf7, 0x05, 0x8e, 0x9a, 0xaf );
#define MBB_UUID_BASIC_CONNECT_EXT_CONSTANT { 0x3d01dcc5, 0xfef5, 0x4d05, { 0x0d, 0x3a, 0xbe, 0xf7, 0x05, 0x8e, 0x9a, 0xaf } }
// DeviceService GUID for Modem Log Transfer (parameter for DSS connect/disconnect request to open/close DSS data channel)
// 0ebb1ceb-af2d-484d-8df3-53bc51fd162c
DEFINE_GUID( MBB_UUID_MODEM_LOG_TRANSFER, 0x0ebb1ceb, 0xaf2d, 0x484d, 0x8d, 0xf3, 0x53, 0xbc, 0x51, 0xfd, 0x16, 0x2c );
#define MBB_UUID_MODEM_LOG_TRANSFER_CONSTANT { 0x0ebb1ceb, 0xaf2d, 0x484d, { 0x8d, 0xf3, 0x53, 0xbc, 0x51, 0xfd, 0x16, 0x2c } }
//
// Context Type GUIDs
//
// MBIMContextTypeNone
// B43F758C-A560-4B46-B35E-C5869641FB54
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_NONE, 0xB43F758C, 0xA560, 0x4B46, 0xB3, 0x5E, 0xC5, 0x86, 0x96, 0x41, 0xFB, 0x54 );
// MBIMContextTypeInternet
// 7E5E2A7E-4E6F-7272-736B-656E7E5E2A7E
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_INTERNET, 0x7E5E2A7E, 0x4E6F, 0x7272, 0x73, 0x6B, 0x65, 0x6E, 0x7E, 0x5E, 0x2A, 0x7E );
// MBIMContextTypeVpn
// 9B9F7BBE-8952-44B7-83AC-CA41318DF7A0
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_VPN, 0x9B9F7BBE, 0x8952, 0x44B7, 0x83, 0xAC, 0xCA, 0x41, 0x31, 0x8D, 0xF7, 0xA0 );
// MBIMContextTypeVoice
// 88918294-0EF4-4396-8CCA-A8588FBC02B2
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_VOICE, 0x88918294, 0x0EF4, 0x4396, 0x8C, 0xCA, 0xA8, 0x58, 0x8F, 0xBC, 0x02, 0xB2 );
// MBIMContextTypeVideoShare
// 05A2A716-7C34-4B4D-9A91-C5EF0C7AAACC
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_VIDEO_SHARE, 0x05A2A716,0x7C34, 0x4B4D, 0x9A, 0x91, 0xC5, 0xEF, 0x0C, 0x7A, 0xAA, 0xCC );
// MBIMContextTypePurchase
// B3272496-AC6C-422B-A8C0-ACF687A27217
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_PURCHASE, 0xB3272496, 0xAC6C, 0x422B, 0xA8, 0xC0, 0xAC, 0xF6, 0x87, 0xA2, 0x72, 0x17 );
// MBIMContextTypeIMS
// 21610D01-3074-4BCE-9425-B53A07D697D6
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_IMS, 0x21610D01, 0x3074, 0x4BCE, 0x94, 0x25, 0xB5, 0x3A, 0x07, 0xD6, 0x97, 0xD6 );
// MBIMContextTypeMMS
// 46726664-7269-6BC6-9624-D1D35389ACA9
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MMS, 0x46726664, 0x7269, 0x6BC6, 0x96, 0x24, 0xD1, 0xD3, 0x53, 0x89, 0xAC, 0xA9 );
// MBIMContextTypeLocal
// A57A9AFC-B09F-45D7-BB40-033C39F60DB9
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_LOCAL, 0xA57A9AFC, 0xB09F, 0x45D7, 0xBB, 0x40, 0x03, 0x3C, 0x39, 0xF6, 0x0D, 0xB9 );
// MBIMMsContextTypeAdmin
// 5f7e4c2e-e80b-40a9-a239-f0abcfd11f4b
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MS_ADMIN, 0x5f7e4c2e, 0xe80b, 0x40a9, 0xa2, 0x39, 0xf0, 0xab, 0xcf, 0xd1, 0x1f, 0x4b );
// MBIMMSContextTypeApp
// 74d88a3d-dfbd-4799-9a8c-7310a37bb2ee
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MS_APP, 0x74d88a3d, 0xdfbd, 0x4799, 0x9a, 0x8c, 0x73, 0x10, 0xa3, 0x7b, 0xb2, 0xee );
// MBIMMsContextTypeXcap
// 50d378a7-baa5-4a50-b872-3fe5bb463411
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MS_XCAP, 0x50d378a7, 0xbaa5, 0x4a50, 0xb8, 0x72, 0x3f, 0xe5, 0xbb, 0x46, 0x34, 0x11 );
// MBIMMsContextTypeTethering
// 5e4e0601-48dc-4e2b-acb8-08b4016bbaac
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MS_TETHERING, 0x5e4e0601, 0x48dc, 0x4e2b, 0xac, 0xb8, 0x08, 0xb4, 0x01, 0x6b, 0xba, 0xac );
// MBIMMsContextTypeEmergencyCalling
// 5f41adb8-204e-4d31-9da8-b3c970e360f2
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_MS_EMERGENCYCALL, 0x5f41adb8, 0x204e, 0x4d31, 0x9d, 0xa8, 0xb3, 0xc9, 0x70, 0xe3, 0x60, 0xf2 );
// 9C494542-A43B-4EA5-B8B7-53310E71DF10
DEFINE_GUID( MBB_UUID_CONTEXT_TYPE_CUSTOM, 0x9C494542, 0xA43B, 0x4EA5, 0xB8, 0xB7, 0x53, 0x31, 0x0E, 0x71, 0xDF, 0x10);
// 146FED19-EB71-42A7-B3EB-3875DEA61D92
DEFINE_GUID(GUID_USB_CAPABILITY_DEVICE_TYPE, 0x146FED19, 0xEB71, 0x42A7, 0xB3, 0xEB, 0x38, 0x75, 0xDE, 0xA6, 0x1D, 0x92);
#define MBB_STATUS_SUCCESS( STATUS ) \
    ( \
        ((STATUS) == MBB_STATUS_SUCCESS) || \
        ((STATUS) == MBB_STATUS_SMS_MORE_DATA) \
    )
#define MBB_STATUS_IS_BASIC(MBBSTATUS)      (((MBBSTATUS) >= MBB_STATUS_BASIC_START) && ((MBBSTATUS) < MBB_STATUS_BASIC_END))
#define MBB_STATUS_BASIC_INDEX(MBBSTATUS)   ((MBBSTATUS)-MBB_STATUS_BASIC_START)
#define MBB_STATUS_BASIC_COUNT()            (MBB_STATUS_BASIC_END-MBB_STATUS_BASIC_START)
#define MBB_STATUS_IS_SMS(MBBSTATUS)        (((MBBSTATUS) >= MBB_STATUS_SMS_START) && ((MBBSTATUS) < MBB_STATUS_SMS_END))
#define MBB_STATUS_SMS_INDEX(MBBSTATUS)     ((MBBSTATUS)-MBB_STATUS_SMS_START)
#define MBB_STATUS_SMS_COUNT()              (MBB_STATUS_SMS_END-MBB_STATUS_SMS_START)
#define MBB_STATUS_IS_UICC(MBBSTATUS)       (((MBBSTATUS) >= MBB_STATUS_UICC_START) && ((MBBSTATUS) < MBB_STATUS_UICC_END))
#define MBB_STATUS_UICC_INDEX(MBBSTATUS)    ((MBBSTATUS)-MBB_STATUS_UICC_START)
#define MBB_STATUS_UICC_COUNT()             (MBB_STATUS_UICC_END-MBB_STATUS_UICC_START)
#define MBB_UUID_TO_NET(_dest, _src) \
{                                                  \
    (_dest)->Data1 = RtlUlongByteSwap((_src)->Data1);  \
    (_dest)->Data2 = RtlUshortByteSwap((_src)->Data2); \
    (_dest)->Data3 = RtlUshortByteSwap((_src)->Data3); \
    RtlCopyMemory((_dest)->Data4, (_src)->Data4, sizeof((_src)->Data4)); \
                                                   \
}
#define MBB_UUID_TO_HOST(_dest, _src) MBB_UUID_TO_NET(_dest, _src)
#define MBB_IS_DEVICE_REGISTERED( _State_ ) \
( \
    ( (_State_) == MbbRegisterStateHome ) || \
    ( (_State_) == MbbRegisterStateRoaming ) || \
    ( (_State_) == MbbRegisterStatePartner ) \
)
#include <packon.h>
#define MBB_MAXIMUM_DATA_CLASS_NAME_LENGTH ( 11 )

#define MBB_MAXIMUM_DEVICE_ID_LENGTH ( 17 )

#define MBB_MAXIMUM_FIRMWARE_INFO_LENGTH ( 31 )

#define MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH ( 15 )

#define MBB_MAXIMUM_SIM_ICC_ID_LENGTH ( 20 )

#define MBB_MAXIMUM_TELEPHONE_NUMBER_LENGTH ( 15 )

#define MBB_MAXIMUM_TELEPHONE_NUMBER_ERRATA_LENGTH ( 22 )

#define MBB_MAXIMUM_PIN_LENGTH ( 16 )

#define MBB_MAXIMUM_PROVIDER_ID_LENGTH ( 6 )

#define MBB_MAXIMUM_PROVIDER_NAME_LENGTH ( 20 )

#define MBB_MAXIMUM_ROAMING_TEXT_LENGTH ( 63 )

#define MBB_MAXIMUM_ACCESS_STRING_LENGTH ( 100 )

#define MBB_MAXIMUM_USERNAME_LENGTH ( 255 )

#define MBB_MAXIMUM_PASSWORD_LENGTH ( 255 )

#define MBB_MAXIMUM_SMS_ADDRESS_LENGTH ( 20 )

#define MBB_MAXIMUM_SMS_CDMA_ADDRESS_LENGTH ( 49 )

#define MBB_MAXIMUM_SMS_CDMA_TIMESTAMP_LENGTH ( 21 )

#define MBB_MAXIMUM_SMS_CDMA_BUFFER_LENGTH ( 160 )

#define MBB_MAXIMUM_SMS_GSM_PDU_BUFFER_LENGTH ( 183 )

#define MBB_MAXIMUM_SMS_CDMA_PDU_BUFFER_LENGTH ( 255 )

#define MBB_USSD_STRING_LEN_MAX ( 160 )

#define MBB_RSSI_DISABLE ( 0xffffffff )

#define MBB_RSSI_DEFAULT ( 0 )

#define MBB_SET_CONTEXT_STATE_EX3_MIN_TLV_CNT ( 3 )

#define MBB_CONTEXT_STATE_EX3_MIN_TLV_CNT ( 1 )

#define MBIM_VERSION_1_0 ( 0x100 )

#define MBIM_MS_EXTENDED_VER_1_0 ( 0x100 )

#define MBIM_MS_EXTENDED_VER_2_0 ( 0x200 )

#define MBIM_MS_EXTENDED_VER_3_0 ( 0x300 )

#define MBB_MS_DEVICE_CAPS_INFO_V3_MIN_TLV_CNT ( 6 )

#define MBB_TLV_TYPE_RESERVED ( 0 )

#define MBB_TLV_TYPE_UE_POLICIES ( 1 )

#define MBB_TLV_TYPE_SINGLE_NSSAI ( 2 )

#define MBB_TLV_TYPE_ALLOWED_NSSAI ( 3 )

#define MBB_TLV_TYPE_CFG_NSSAI ( 4 )

#define MBB_TLV_TYPE_DFLT_CFG_NSSAI ( 5 )

#define MBB_TLV_TYPE_PRECFG_DFLT_CFG_NSSAI ( 6 )

#define MBB_TLV_TYPE_REJ_NSSAI ( 7 )

#define MBB_TLV_TYPE_LADN ( 8 )

#define MBB_TLV_TYPE_TAI ( 9 )

#define MBB_TLV_TYPE_WCHAR_STR ( 10 )

#define MBB_TLV_TYPE_UINT16_TBL ( 11 )

#define MBB_TLV_TYPE_EAP_PACKET ( 12 )

#define MBB_TLV_TYPE_PCO ( 13 )

#define MBB_TLV_TYPE_ROUTE_SELECTION_DESCRIPTORS ( 14 )

#define MBB_TLV_TYPE_TRAFFIC_PARAMETERS ( 15 )

#define MBB_TLV_TYPE_WAKE_COMMAND ( 16 )

#define MBB_TLV_TYPE_WAKE_PACKET ( 17 )

#define MBB_NW_PARAMS_INFO_CONFIG_TLV_CNT ( 6 )

#define MBB_NW_PARAMS_INFO_UE_POLICIES_TLV_CNT ( 1 )

typedef unsigned short MBB_TLV_TYPE;

typedef 
enum _USB_CAP_DEVICE_TYPE
    {
        USB_CAP_DEVICE_TYPE_USB = 0,
        USB_CAP_DEVICE_TYPE_UDE_MBIM = 1,
        USB_CAP_DEVICE_TYPE_UDE_MBIM_FASTIO = 2,
        USB_CAP_DEVICE_TYPE_MAXIMUM = ( USB_CAP_DEVICE_TYPE_UDE_MBIM_FASTIO + 1 ) 
    } USB_CAP_DEVICE_TYPE;

typedef struct _USB_CAP_DEVICE_INFO_HEADER
    {
    USB_CAP_DEVICE_TYPE DeviceType;
    UCHAR DeviceMinorVersion;
    UCHAR DeviceMajorVersion;
    ULONG Reserved;
    } USB_CAP_DEVICE_INFO_HEADER;

typedef struct _USB_CAP_DEVICE_INFO_HEADER *PUSB_CAP_DEVICE_INFO_HEADER;

typedef struct _USB_CAP_DEVICE_INFO
    {
    USB_CAP_DEVICE_INFO_HEADER DeviceInfoHeader;
    } USB_CAP_DEVICE_INFO;

typedef struct _USB_CAP_DEVICE_INFO *PUSB_CAP_DEVICE_INFO;

typedef 
enum _MBB_NBL_TYPE
    {
        MBB_NBL_TYPE_IP = 0,
        MBB_NBL_TYPE_DSS = 1,
        MBB_NBL_TYPE_MAXIMUM = ( MBB_NBL_TYPE_DSS + 1 ) 
    } MBB_NBL_TYPE;

typedef 
enum _MBB_STATUS
    {
        MBB_STATUS_SUCCESS = 0,
        MBB_STATUS_BASIC_START = 0,
        MBB_STATUS_BUSY = 1,
        MBB_STATUS_FAILURE = 2,
        MBB_STATUS_SIM_NOT_INSERTED = 3,
        MBB_STATUS_BAD_SIM = 4,
        MBB_STATUS_PIN_REQUIRED = 5,
        MBB_STATUS_PIN_DISABLED = 6,
        MBB_STATUS_NOT_REGISTERED = 7,
        MBB_STATUS_PROVIDERS_NOT_FOUND = 8,
        MBB_STATUS_NO_DEVICE_SUPPORT = 9,
        MBB_STATUS_PROVIDER_NOT_VISIBLE = 10,
        MBB_STATUS_DATA_CLASS_NOT_AVAILABLE = 11,
        MBB_STATUS_PACKET_SVC_DETACHED = 12,
        MBB_STATUS_MAX_ACTIVATED_CONTEXTS = 13,
        MBB_STATUS_NOT_INITIALIZED = 14,
        MBB_STATUS_VOICE_CALL_IN_PROGRESS = 15,
        MBB_STATUS_CONTEXT_NOT_ACTIVATED = 16,
        MBB_STATUS_SERVICE_NOT_ACTIVATED = 17,
        MBB_STATUS_INVALID_ACCESS_STRING = 18,
        MBB_STATUS_INVALID_USER_NAME_PWD = 19,
        MBB_STATUS_RADIO_POWER_OFF = 20,
        MBB_STATUS_INVALID_PARAMETERS = 21,
        MBB_STATUS_READ_FAILURE = 22,
        MBB_STATUS_WRITE_FAILURE = 23,
        MBB_STATUS_DENIED_POLICY = 24,
        MBB_STATUS_NO_PHONEBOOK = 25,
        MBB_STATUS_PARAMETER_TOO_LONG = 26,
        MBB_STATUS_STK_BUSY = 27,
        MBB_STATUS_OPERATION_NOT_ALLOWED = 28,
        MBB_STATUS_MEMORY_FAILURE = 29,
        MBB_STATUS_INVALID_MEMORY_INDEX = 30,
        MBB_STATUS_MEMORY_FULL = 31,
        MBB_STATUS_FILTER_NOT_SUPPORTED = 32,
        MBB_STATUS_DSS_INSTANCE_LIMIT = 33,
        MBB_STATUS_INVALID_DEVICE_SERVICE_OPERATION = 34,
        MBB_STATUS_AUTH_INCORRECT_AUTN = 35,
        MBB_STATUS_AUTH_SYNC_FAILURE = 36,
        MBB_STATUS_AUTH_AMF_NOT_SET = 37,
        MBB_STATUS_CONTEXT_NOT_SUPPORTED = 38,
        MBB_STATUS_SHAREABILITY_CONDITION_ERROR = 39,
        MBB_STATUS_PIN_FAILURE = 40,
        MBB_STATUS_NO_LTE_ATTACH_CONFIG = 41,
        MBB_STATUS_SESSION_ALREADY_EXISTS = 42,
        MBB_STATUS_BASIC_END = ( MBB_STATUS_SESSION_ALREADY_EXISTS + 1 ) ,
        MBB_STATUS_SMS_UNKNOWN_SMSC_ADDRESS = 100,
        MBB_STATUS_SMS_START = 100,
        MBB_STATUS_SMS_NETWORK_TIMEOUT = 101,
        MBB_STATUS_SMS_LANG_NOT_SUPPORTED = 102,
        MBB_STATUS_SMS_ENCODING_NOT_SUPPORTED = 103,
        MBB_STATUS_SMS_FORMAT_NOT_SUPPORTED = 104,
        MBB_STATUS_SMS_MORE_DATA = 105,
        MBB_STATUS_SMS_END = ( MBB_STATUS_SMS_MORE_DATA + 1 ) ,
        MBB_STATUS_UICC_NO_LOGICAL_CHANNELS = 0x87430001,
        MBB_STATUS_UICC_START = 0x87430001,
        MBB_STATUS_UICC_SELECT_FAILED = 0x87430002,
        MBB_STATUS_UICC_INVALID_LOGICAL_CHANNEL = 0x87430003,
        MBB_STATUS_UICC_END = ( MBB_STATUS_UICC_INVALID_LOGICAL_CHANNEL + 1 ) 
    } MBB_STATUS;

typedef 
enum _MBB_ERROR
    {
        MBB_ERROR_NONE = 0,
        MBB_ERROR_TIMEOUT_FRAGMENT = 1,
        MBB_ERROR_FRAGMENT_OUT_OF_SEQUENCE = 2,
        MBB_ERROR_LENGTH_MISMATCH = 3,
        MBB_ERROR_DUPLICATE_TID = 4,
        MBB_ERROR_NOT_OPENED = 5,
        MBB_ERROR_UNKNOWN = 6,
        MBB_ERROR_CANCEL = 7,
        MBB_ERROR_MAX_TRANSFER = 8
    } MBB_ERROR;

typedef 
enum _MBB_MESSAGE_TYPE
    {
        MBB_MESSAGE_TYPE_NONE = 0,
        MBB_MESSAGE_TYPE_OPEN = 0x1,
        MBB_MESSAGE_TYPE_CLOSE = 0x2,
        MBB_MESSAGE_TYPE_COMMAND = 0x3,
        MBB_MESSAGE_TYPE_HOST_ERROR = 0x4,
        MBB_MESSAGE_TYPE_OPEN_DONE = 0x80000001,
        MBB_MESSAGE_TYPE_CLOSE_DONE = 0x80000002,
        MBB_MESSAGE_TYPE_COMMAND_DONE = 0x80000003,
        MBB_MESSAGE_TYPE_FUNCTION_ERROR = 0x80000004,
        MBB_MESSAGE_TYPE_INDICATE_STATUS = 0x80000007
    } MBB_MESSAGE_TYPE;

typedef 
enum _MBB_COMMAND_TYPE
    {
        MBB_COMMAND_TYPE_QUERY = 0,
        MBB_COMMAND_TYPE_SET = 1
    } MBB_COMMAND_TYPE;

typedef struct _MBB_MESSAGE_HEADER
    {
    MBB_MESSAGE_TYPE MessageType;
    ULONG MessageLength;
    ULONG MessageTransactionId;
    } MBB_MESSAGE_HEADER;

typedef struct _MBB_MESSAGE_HEADER *PMBB_MESSAGE_HEADER;

typedef struct _MBB_FRAGMENT_HEADER
    {
    ULONG TotalFragments;
    ULONG CurrentFragment;
    } MBB_FRAGMENT_HEADER;

typedef struct _MBB_FRAGMENT_HEADER *PMBB_FRAGMENT_HEADER;

typedef struct _MBB_COMMAND
    {
    GUID ServiceId;
    ULONG CommandId;
    } MBB_COMMAND;

typedef struct _MBB_COMMAND *PMBB_COMMAND;

typedef struct _MBB_COMMAND_HEADER
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_FRAGMENT_HEADER FragmentHeader;
    MBB_COMMAND Command;
    MBB_COMMAND_TYPE CommandType;
    ULONG InformationBufferLength;
    } MBB_COMMAND_HEADER;

typedef struct _MBB_COMMAND_HEADER *PMBB_COMMAND_HEADER;

typedef struct _MBB_COMMAND_FRAGMENT_HEADER
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_FRAGMENT_HEADER FragmentHeader;
    } MBB_COMMAND_FRAGMENT_HEADER;

typedef struct _MBB_COMMAND_FRAGMENT_HEADER *PMBB_COMMAND_FRAGMENT_HEADER;

typedef struct _MBB_COMMAND_DONE_HEADER
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_FRAGMENT_HEADER FragmentHeader;
    MBB_COMMAND Command;
    MBB_STATUS MbbStatus;
    ULONG InformationBufferLength;
    } MBB_COMMAND_DONE_HEADER;

typedef struct _MBB_COMMAND_DONE_HEADER *PMBB_COMMAND_DONE_HEADER;

typedef struct _MBB_INDICATE_STATUS_HEADER
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_FRAGMENT_HEADER FragmentHeader;
    MBB_COMMAND Command;
    ULONG InformationBufferLength;
    } MBB_INDICATE_STATUS_HEADER;

typedef struct _MBB_INDICATE_STATUS_HEADER *PMBB_INDICATE_STATUS_HEADER;

typedef struct _MBB_OPEN_MESSAGE
    {
    MBB_MESSAGE_HEADER MessageHeader;
    ULONG MaximumControlTransfer;
    } MBB_OPEN_MESSAGE;

typedef struct _MBB_OPEN_MESSAGE *PMBB_OPEN_MESSAGE;

typedef struct _MBB_OPEN_MESSAGE_FASTIO
    {
    MBB_MESSAGE_HEADER MessageHeader;
    ULONG MaximumControlTransfer;
    PVOID AdapterContext;
    PVOID SendNetBufferListsCompleteHandler;
    PVOID ReceiveNetBufferListsHandler;
    } MBB_OPEN_MESSAGE_FASTIO;

typedef struct _MBB_OPEN_MESSAGE_FASTIO *PMBB_OPEN_MESSAGE_FASTIO;

typedef struct _MBB_CLOSE_MESSAGE
    {
    MBB_MESSAGE_HEADER MessageHeader;
    } MBB_CLOSE_MESSAGE;

typedef struct _MBB_CLOSE_MESSAGE *PMBB_CLOSE_MESSAGE;

typedef struct _MBB_OPEN_DONE
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_STATUS MbbStatus;
    } MBB_OPEN_DONE;

typedef struct _MBB_OPEN_DONE *PMBB_OPEN_DONE;

typedef struct _MBB_OPEN_DONE_FASTIO
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_STATUS MbbStatus;
    PVOID ModemContext;
    PVOID SendNetBufferListsHandler;
    PVOID ReturnNetBufferListsHandler;
    PVOID CancelSendHandler;
    PVOID HaltHandler;
    PVOID PauseHandler;
    PVOID ShutdownHandler;
    PVOID ResetHandler;
    PVOID RestartHandler;
    } MBB_OPEN_DONE_FASTIO;

typedef struct _MBB_OPEN_DONE_FASTIO *PMBB_OPEN_DONE_FASTIO;

typedef struct _MBB_CLOSE_DONE
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_STATUS MbbStatus;
    } MBB_CLOSE_DONE;

typedef struct _MBB_CLOSE_DONE *PMBB_CLOSE_DONE;

typedef struct _MBB_ERROR_MESSAGE
    {
    MBB_MESSAGE_HEADER MessageHeader;
    MBB_ERROR ErrorCode;
    } MBB_ERROR_MESSAGE;

typedef struct _MBB_ERROR_MESSAGE *PMBB_ERROR_MESSAGE;

typedef 
enum _MBB_BASIC_CID
    {
        MBB_BASIC_CID_DEVICE_CAPS = 1,
        MBB_BASIC_CID_SUBSCRIBER_READY_INFO = 2,
        MBB_BASIC_CID_RADIO_STATE = 3,
        MBB_BASIC_CID_PIN_INFO = 4,
        MBB_BASIC_CID_PIN_LIST = 5,
        MBB_BASIC_CID_HOME_PROVIDER = 6,
        MBB_BASIC_CID_PREFERRED_PROVIDERS = 7,
        MBB_BASIC_CID_VISIBLE_PROVIDERS = 8,
        MBB_BASIC_CID_REGISTER_STATE = 9,
        MBB_BASIC_CID_PACKET_SERVICE = 10,
        MBB_BASIC_CID_SIGNAL_STATE = 11,
        MBB_BASIC_CID_CONNECT = 12,
        MBB_BASIC_CID_PROVISIONED_CONTEXTS = 13,
        MBB_BASIC_CID_SERVICE_ACTIVATION = 14,
        MBB_BASIC_CID_IP_ADDRESS_INFO = 15,
        MBB_BASIC_CID_DEVICE_SERVICES = 16,
        MBB_BASIC_CID_NOTIFY_DEVICE_SERVICE_UPDATES = 19,
        MBB_BASIC_CID_PACKET_STATISTICS = 20,
        MBB_BASIC_CID_NETWORK_IDLE_HINT = 21,
        MBB_BASIC_CID_EMERGENCY_MODE = 22,
        MBB_BASIC_CID_PACKET_FILTERS = 23,
        MBB_BASIC_CID_MULTICARRIER_PROVIDERS = 24,
        MBB_BASIC_CID_MAXIMUM = ( MBB_BASIC_CID_MULTICARRIER_PROVIDERS + 1 ) 
    } MBB_BASIC_CID;

typedef 
enum _MBB_SMS_CID
    {
        MBB_SMS_CID_CONFIGURATION = 1,
        MBB_SMS_CID_READ = 2,
        MBB_SMS_CID_SEND = 3,
        MBB_SMS_CID_DELETE = 4,
        MBB_SMS_CID_STATUS = 5,
        MBB_SMS_CID_MAXIMUM = ( MBB_SMS_CID_STATUS + 1 ) 
    } MBB_SMS_CID;

typedef 
enum _MBB_VENDOR_CID
    {
        MBB_VENDOR_CID_MS_VENDOR_SPECIFIC = 1,
        MBB_VENDOR_CID_MAXIMUM = ( MBB_VENDOR_CID_MS_VENDOR_SPECIFIC + 1 ) 
    } MBB_VENDOR_CID;

typedef 
enum _MBB_USSD_CID
    {
        MBB_USSD_CID_USSD = 1,
        MBB_USSD_CID_MAXIMUM = ( MBB_USSD_CID_USSD + 1 ) 
    } MBB_USSD_CID;

typedef 
enum _MBB_PHONEBOOK_CID
    {
        MBB_PHONEBOOK_CID_CONFIGURATION = 1,
        MBB_PHONEBOOK_CID_READ = 2,
        MBB_PHONEBOOK_CID_DELETE = 3,
        MBB_PHONEBOOK_CID_SAVE = 4,
        MBB_PHONEBOOK_CID_MAXIMUM = ( MBB_PHONEBOOK_CID_SAVE + 1 ) 
    } MBB_PHONEBOOK_CID;

typedef 
enum _MBB_SAT_CID
    {
        MBB_SAT_CID_PAC = 1,
        MBB_SAT_CID_TERMINAL_RESPONSE = 2,
        MBB_SAT_CID_ENVELOPE = 3,
        MBB_SAT_CID_MAXIMUM = ( MBB_SAT_CID_ENVELOPE + 1 ) 
    } MBB_SAT_CID;

typedef 
enum _MBB_AUTH_CID
    {
        MBB_AUTH_CID_AKA = 1,
        MBB_AUTH_CID_AKAP = 2,
        MBB_AUTH_CID_SIM = 3,
        MBB_AUTH_CID_MAXIUM = ( MBB_AUTH_CID_SIM + 1 ) 
    } MBB_AUTH_CID;

typedef 
enum _MBB_DSS_CID
    {
        MBB_DSS_CID_CONNECT = 1,
        MBB_DSS_CID_MAXIUM = ( MBB_DSS_CID_CONNECT + 1 ) 
    } MBB_DSS_CID;

typedef 
enum _MBB_MULTICARRIER_CID
    {
        MBB_MULTICARRIER_CID_CAPABILITIES = 1,
        MBB_MULTICARRIER_CID_LOCATION_INFO = 2,
        MBB_MULTICARRIER_CID_CURRENT_CID_LIST = 3,
        MBB_MULTICARRIER_CID_MAXIMUM = ( MBB_MULTICARRIER_CID_CURRENT_CID_LIST + 1 ) 
    } MBB_MULTICARRIER_CID;

typedef 
enum _MBB_HOSTSHUTDOWN_CID
    {
        MBB_HOSTSHUTDOWN_CID_ONE = 1,
        MBB_HOSTSHUTDOWN_CID_PRESHUTDOWN = 2,
        MBB_HOSTSHUTDOWN_CID_MAX = ( MBB_HOSTSHUTDOWN_CID_PRESHUTDOWN + 1 ) 
    } MBB_HOSTSHUTDOWN_CID;

typedef 
enum _MBB_VOICEEXTENSIONS_CID
    {
        MBB_VOICEEXTENSIONS_CID_SYS_CAPS = 1,
        MBB_VOICEEXTENSIONS_CID_DEVICE_CAPS_V2 = 2,
        MBB_VOICEEXTENSIONS_CID_SYS_SLOTMAPPINGS = 3,
        MBB_VOICEEXTENSIONS_CID_SLOT_INFO_STATUS = 4,
        MBB_VOICEEXTENSIONS_CID_DEVICE_BINDINGS = 5,
        MBB_VOICEEXTENSIONS_CID_REGISTER_STATE_V2 = 6,
        MBB_VOICEEXTENSIONS_CID_IMS_VOICE_STATE = 7,
        MBB_VOICEEXTENSIONS_CID_SIGNAL_STATE_V2 = 8,
        MBB_VOICEEXTENSIONS_CID_LOCATION_STATE = 9,
        MBB_VOICEEXTENSIONS_CID_NITZ = 10,
        MBB_VOICEEXTENSIONS_CID_MAX = ( MBB_VOICEEXTENSIONS_CID_NITZ + 1 ) 
    } MBB_VOICEEXTENSIONS_CID;

typedef 
enum _MBB_UICC_CID
    {
        MBB_UICC_CID_ATR = 1,
        MBB_UICC_CID_OPEN_CHANNEL = 2,
        MBB_UICC_CID_CLOSE_CHANNEL = 3,
        MBB_UICC_CID_APDU = 4,
        MBB_UICC_CID_TERMINAL_CAPABILITY = 5,
        MBB_UICC_CID_RESET = 6,
        MBB_UICC_CID_APP_LIST = 7,
        MBB_UICC_CID_FILE_STATUS = 8,
        MBB_UICC_CID_ACCESS_BINARY = 9,
        MBB_UICC_CID_ACCESS_RECORD = 10,
        MBB_UICC_CID_MAXIMUM = ( MBB_UICC_CID_ACCESS_RECORD + 1 ) 
    } MBB_UICC_CID;

typedef 
enum _MBB_SAR_CID
    {
        MBB_SAR_CID_CONFIG = 1,
        MBB_SAR_CID_TRANSMISSION_STATUS = 2,
        MBB_SAR_CID_MAXIMUM = ( MBB_SAR_CID_TRANSMISSION_STATUS + 1 ) 
    } MBB_SAR_CID;

typedef 
enum _MBB_BASICCONNECTEXT_CID
    {
        MBB_BASICCONNECTEXT_CID_PROVISIONED_CONTEXT_V2 = 1,
        MBB_BASICCONNECTEXT_CID_NETWORK_BLACKLIST = 2,
        MBB_BASICCONNECTEXT_CID_LTE_ATTACH_CONFIG = 3,
        MBB_BASICCONNECTEXT_CID_LTE_ATTACH_STATUS = 4,
        MBB_BASICCONNECTEXT_CID_SYS_CAPS = 5,
        MBB_BASICCONNECTEXT_CID_DEVICE_CAPS_V2 = 6,
        MBB_BASICCONNECTEXT_CID_DEVICE_SLOT_MAPPINGS = 7,
        MBB_BASICCONNECTEXT_CID_SLOT_INFO_STATUS = 8,
        MBB_BASICCONNECTEXT_CID_PCO = 9,
        MBB_BASICCONNECTEXT_CID_DEVICE_RESET = 10,
        MBB_BASICCONNECTEXT_CID_BASE_STATIONS_INFO = 11,
        MBB_BASICCONNECTEXT_CID_LOCATION_INFO_STATUS = 12,
        MBB_BASICCONNECTEXT_CID_MODEM_LOGGING_CONFIG = 13,
        MBB_BASICCONNECTEXT_CID_PIN_INFO_EX2 = 14,
        MBB_BASICCONNECTEXT_CID_VERSION = 15,
        MBB_BASICCONNECTEXT_CID_MODEM_CONFIG = 16,
        MBB_BASICCONNECTEXT_CID_REGISTRATION_PARAMS = 17,
        MBB_BASICCONNECTEXT_CID_NETWORK_PARAMS = 18,
        MBB_BASICCONNECTEXT_CID_WAKE_REASON = 19,
        MBB_BASICCONNECTEXT_CID_MAXIMUM = ( MBB_BASICCONNECTEXT_CID_WAKE_REASON + 1 ) 
    } MBB_BASICCONNECTEXT_CID;

typedef 
enum _MBB_DEVICE_TYPE
    {
        MbbDeviceTypeUnknown = 0,
        MbbDeviceTypeEmbedded = 1,
        MbbDeviceTypeRemovable = 2,
        MbbDeviceTypeRemote = 3,
        MbbDeviceTypeMaximum = ( MbbDeviceTypeRemote + 1 ) 
    } MBB_DEVICE_TYPE;

typedef 
enum _MBB_CELLULAR_CLASS
    {
        MbbCellularClassInvalid = 0,
        MbbCellularClassGsm = 1,
        MbbCellularClassCdma = 2,
        MbbCellularClassMaximum = ( MbbCellularClassCdma + 1 ) 
    } MBB_CELLULAR_CLASS;

typedef 
enum _MBB_VOICE_CLASS
    {
        MbbVoiceClassInvalid = 0,
        MbbVoiceClassNoVoice = 1,
        MbbVoiceClassSeparateVoiceData = 2,
        MbbVoiceClassSimultaneousVoiceData = 3,
        MbbVoiceClassMaximum = ( MbbVoiceClassSimultaneousVoiceData + 1 ) 
    } MBB_VOICE_CLASS;

typedef 
enum _MBB_SIM_CLASS
    {
        MbbSimClassInvalid = 0,
        MbbSimClassSimLogical = 1,
        MbbSimClassSimRemovable = 2,
        MbbSimClassMaximum = ( MbbSimClassSimRemovable + 1 ) 
    } MBB_SIM_CLASS;

typedef 
enum _MBB_DATA_CLASS_VALUE
    {
        MbbDataClassNone = 0,
        MbbDataClassGprs = 0x1,
        MbbDataClassEdge = 0x2,
        MbbDataClassUmts = 0x4,
        MbbDataClassHsdpa = 0x8,
        MbbDataClassHsupa = 0x10,
        MbbDataClassLte = 0x20,
        MbbDataClass5G = 0x40,
        MbbDataClass_UNUSED = 0x80,
        MbbDataClassTdScdma = 0x1000,
        MbbDataClass1xRtt = 0x10000,
        MbbDataClass1xEvdo = 0x20000,
        MbbDataClass1xEvdoReva = 0x40000,
        MbbDataClass1xEvdv = 0x80000,
        MbbDataClass3xRtt = 0x100000,
        MbbDataClass1xEvdoRevb = 0x200000,
        MbbDataClassUmb = 0x400000,
        MbbDataClassCustom = 0x80000000
    } MBB_DATA_CLASS_VALUE;

typedef 
enum _MBB_DATA_SUBCLASS_VALUE
    {
        MbbDataSubClassNone = 0,
        MbbDataSubClass5GENDC = 0x1,
        MbbDataSubClass5GNR = 0x2,
        MbbDataSubClass5GNEDC = 0x4,
        MbbDataSubClass5GELTE = 0x8,
        MbbDataSubClassNGENDC = 0x10
    } MBB_DATA_SUBCLASS_VALUE;

typedef 
enum _MBB_FREQUENCY_RANGE
    {
        MbbFrequencyRangeUnknown = 0,
        MbbFrequencyRange1 = 1,
        MbbFrequencyRange2 = 2,
        MbbFrequencyRange1AndRange2 = 3,
        MbbFrequencyRangeMaximum = ( MbbFrequencyRange1AndRange2 + 1 ) 
    } MBB_FREQUENCY_RANGE;

typedef 
enum _MBB_BAND_CLASS_VALUE
    {
        MbbBandClassUnknown = 0,
        MbbBandClass0 = 0x1,
        MbbBandClassI = 0x2,
        MbbBandClassII = 0x4,
        MbbBandClassIII = 0x8,
        MbbBandClassIV = 0x10,
        MbbBandClassV = 0x20,
        MbbBandClassVI = 0x40,
        MbbBandClassVII = 0x80,
        MbbBandClassVIII = 0x100,
        MbbBandClassIX = 0x200,
        MbbBandClassX = 0x400,
        MbbBandClassXI = 0x800,
        MbbBandClassXII = 0x1000,
        MbbBandClassXIII = 0x2000,
        MbbBandClassXIV = 0x4000,
        MbbBandClassXV = 0x8000,
        MbbBandClassXVI = 0x10000,
        MbbBandClassXVII = 0x20000,
        MbbBandClassCustom = 0x80000000
    } MBB_BAND_CLASS_VALUE;

typedef 
enum _MBB_SMS_CAPS_VALUE
    {
        MbbSmsCapsNone = 0,
        MbbSmsCapsPduReceive = 0x1,
        MbbSmsCapsPduSend = 0x2,
        MbbSmsCapsTextReceive = 0x4,
        MbbSmsCapsTextSend = 0x8
    } MBB_SMS_CAPS_VALUE;

typedef 
enum _MBB_CONTROL_CAPS_VALUE
    {
        MbbControlCapsNone = 0,
        MbbControlCapsRegManual = 0x1,
        MbbControlCapsHwRadioSwitch = 0x2,
        MbbControlCapsCdmaMobileIp = 0x4,
        MbbControlCapsCdmaSimpleIp = 0x8,
        MbbControlCapsMultiCarrier = 0x10,
        MbbControlCapsESIM = 0x20,
        MbbControlCapsUEPolicyRouteSelection = 0x40,
        MbbControlCapsSIMHotSwapCapable = 0x80
    } MBB_CONTROL_CAPS_VALUE;

typedef 
enum _MBB_READY_STATE
    {
        MbbReadyStateOff = 0,
        MbbReadyStateInitialized = 1,
        MbbReadyStateSimNotInserted = 2,
        MbbReadyStateBadSim = 3,
        MbbReadyStateFailure = 4,
        MbbReadyStateNotActivated = 5,
        MbbReadyStateDeviceLocked = 6,
        MbbReadyStateNoEsimProfile = 7,
        MbbReadyStateMaximum = ( MbbReadyStateNoEsimProfile + 1 ) 
    } MBB_READY_STATE;

typedef 
enum _MBB_READY_INFO_FLAGS
    {
        MbbReadyInfoFlagsNone = 0,
        MbbReadyInfoFlagsUniqueId = 1
    } MBB_READY_INFO_FLAGS;

typedef 
enum _MBB_EMERGENCY_MODE
    {
        MbbEmergencyModeOff = 0,
        MbbEmergencyModeOn = 1,
        MbbEmergencyModeMaximum = ( MbbEmergencyModeOn + 1 ) 
    } MBB_EMERGENCY_MODE;

typedef 
enum _MBB_RADIO_STATE
    {
        MbbRadioStateOff = 0,
        MbbRadioStateOn = 1,
        MbbRadioStateMaximum = ( MbbRadioStateOn + 1 ) 
    } MBB_RADIO_STATE;

typedef 
enum _MBB_PIN_TYPE
    {
        MbbPinTypeNone = 0,
        MbbPinTypeCustom = 1,
        MbbPinTypePin1 = 2,
        MbbPinTypePin2 = 3,
        MbbPinTypeDeviceSimPin = 4,
        MbbPinTypeDeviceFirstSimPin = 5,
        MbbPinTypeNetworkPin = 6,
        MbbPinTypeNetworkSubsetPin = 7,
        MbbPinTypeSvcProviderPin = 8,
        MbbPinTypeCorporatePin = 9,
        MbbPinTypeSubsidyLock = 10,
        MbbPinTypePuk1 = 11,
        MbbPinTypePuk2 = 12,
        MbbPinTypeDeviceFirstSimPuk = 13,
        MbbPinTypeNetworkPuk = 14,
        MbbPinTypeNetworkSubsetPuk = 15,
        MbbPinTypeSvcProviderPuk = 16,
        MbbPinTypeCorporatePuk = 17,
        MbbPinTypeNev = 18,
        MbbPinTypeAdm = 19,
        MbbPinTypeMaximum = ( MbbPinTypeAdm + 1 ) 
    } MBB_PIN_TYPE;

typedef 
enum _MBB_PIN_STATE
    {
        MbbPinStateUnlocked = 0,
        MbbPinStateLocked = 1,
        MbbPinStateMaximum = ( MbbPinStateLocked + 1 ) 
    } MBB_PIN_STATE;

typedef 
enum _MBB_PIN_OPERATION
    {
        MbbPinOperationEnter = 0,
        MbbPinOperationEnable = 1,
        MbbPinOperationDisable = 2,
        MbbPinOperationChange = 3,
        MbbPinOperationMaximum = ( MbbPinOperationChange + 1 ) 
    } MBB_PIN_OPERATION;

typedef 
enum _MBB_PIN_MODE
    {
        MbbPinModeNotSupported = 0,
        MbbPinModeEnabled = 1,
        MbbPinModeDisabled = 2,
        MbbPinModeMaximum = ( MbbPinModeDisabled + 1 ) 
    } MBB_PIN_MODE;

typedef 
enum _MBB_PIN_FORMAT
    {
        MbbPinFormatUnknown = 0,
        MbbPinFormatNumeric = 1,
        MbbPinFormatAlphaNumeric = 2,
        MbbPinFormatMaximum = ( MbbPinFormatAlphaNumeric + 1 ) 
    } MBB_PIN_FORMAT;

typedef 
enum _MBB_PROVIDER_STATE_VALUE
    {
        MbbProviderStateHome = 0x1,
        MbbProviderStateForbidden = 0x2,
        MbbProviderStatePreferred = 0x4,
        MbbProviderStateVisible = 0x8,
        MbbProviderStateRegistered = 0x10,
        MbbProviderStatePreferredMulticarrier = 0x20
    } MBB_PROVIDER_STATE_VALUE;

typedef 
enum _MBB_REGISTER_ACTION
    {
        MbbRegisterActionAutomatic = 0,
        MbbRegisterActionManual = 1,
        MbbRegisterActionMaximum = ( MbbRegisterActionManual + 1 ) 
    } MBB_REGISTER_ACTION;

typedef 
enum _MBB_REGISTER_STATE
    {
        MbbRegisterStateUnknown = 0,
        MbbRegisterStateDeregistered = 1,
        MbbRegisterStateSearching = 2,
        MbbRegisterStateHome = 3,
        MbbRegisterStateRoaming = 4,
        MbbRegisterStatePartner = 5,
        MbbRegisterStateDenied = 6,
        MbbRegisterStateMaximum = ( MbbRegisterStateDenied + 1 ) 
    } MBB_REGISTER_STATE;

typedef 
enum _MBB_REGISTER_MODE
    {
        MbbRegisterModeUnknown = 0,
        MbbRegisterModeAutomatic = 1,
        MbbRegisterModeManual = 2,
        MbbRegisterModeMaximum = ( MbbRegisterModeManual + 1 ) 
    } MBB_REGISTER_MODE;

typedef 
enum _MBB_PACKET_SERVICE_ACTION
    {
        MbbPacketServiceActionAttach = 0,
        MbbPacketServiceActionDetach = 1,
        MbbPacketServiceActionMaximum = ( MbbPacketServiceActionDetach + 1 ) 
    } MBB_PACKET_SERVICE_ACTION;

typedef 
enum _MBB_PACKET_SERVICE_STATE
    {
        MbbPacketServiceStateUnknown = 0,
        MbbPacketServiceStateAttaching = 1,
        MbbPacketServiceStateAttached = 2,
        MbbPacketServiceStateDetaching = 3,
        MbbPacketServiceStateDetached = 4,
        MbbPacketServiceStateMaximum = ( MbbPacketServiceStateDetached + 1 ) 
    } MBB_PACKET_SERVICE_STATE;

typedef 
enum _MBB_CONTEXT_IP_TYPE
    {
        MbbContextIPTypeDefault = 0,
        MbbContextIPTypeIPv4 = 1,
        MbbContextIPTypeIPv6 = 2,
        MbbContextIPTypeIPv4v6 = 3,
        MbbContextIPTypeIPv4AndIPv6 = 4,
        MbbContextIPTypeMaximum = ( MbbContextIPTypeIPv4AndIPv6 + 1 ) 
    } MBB_CONTEXT_IP_TYPE;

typedef 
enum _MBB_ACTIVATION_COMMAND
    {
        MbbActivationCommandDeactivate = 0,
        MbbActivationCommandActivate = 1,
        MbbActivationCommandCancel = 2,
        MbbActivationCommandMaximum = ( MbbActivationCommandCancel + 1 ) 
    } MBB_ACTIVATION_COMMAND;

typedef 
enum _MBB_COMPRESSION
    {
        MbbCompressionNone = 0,
        MbbCompressionEnable = 1,
        MbbCompressionMaximum = ( MbbCompressionEnable + 1 ) 
    } MBB_COMPRESSION;

typedef 
enum _MBB_AUTH_PROTOCOL
    {
        MbbAuthProtocolNone = 0,
        MbbAuthProtocolPap = 1,
        MbbAuthProtocolChap = 2,
        MbbAuthProtocolMsChapV2 = 3,
        MbbAuthProtocolMaximum = ( MbbAuthProtocolMsChapV2 + 1 ) 
    } MBB_AUTH_PROTOCOL;

typedef 
enum _MBB_ACTIVATION_STATE
    {
        MbbActivationStateUnknown = 0,
        MbbActivationStateActivated = 1,
        MbbActivationStateActivating = 2,
        MbbActivationStateDeactivated = 3,
        MbbActivationStateDeactivating = 4,
        MbbActivationStateMaximum = ( MbbActivationStateDeactivating + 1 ) 
    } MBB_ACTIVATION_STATE;

typedef 
enum _MBB_VOICE_CALL_STATE
    {
        MbbVoiceCallStateNone = 0,
        MbbVoiceCallStateInProgress = 1,
        MbbVoiceCallStateHangUp = 2,
        MbbVoiceCallStateMaximum = ( MbbVoiceCallStateHangUp + 1 ) 
    } MBB_VOICE_CALL_STATE;

typedef 
enum _MBB_SMS_FORMAT
    {
        MbbSmsFormatPdu = 0,
        MbbSmsFormatCdma = 1,
        MbbSmsFormatMaximum = ( MbbSmsFormatCdma + 1 ) 
    } MBB_SMS_FORMAT;

typedef 
enum _MBB_SMS_FLAG
    {
        MbbSmsFlagAll = 0,
        MbbSmsFlagIndex = 1,
        MbbSmsFlagNew = 2,
        MbbSmsFlagOld = 3,
        MbbSmsFlagSent = 4,
        MbbSmsFlagDraft = 5,
        MbbSmsFlagMaximum = ( MbbSmsFlagDraft + 1 ) 
    } MBB_SMS_FLAG;

typedef 
enum _MBB_SMS_CDMA_LANGUAGE
    {
        MbbSmsCdmaLanguageUnknown = 0,
        MbbSmsCdmaLanguageEnglish = 1,
        MbbSmsCdmaLanguageFrench = 2,
        MbbSmsCdmaLanguageSpanish = 3,
        MbbSmsCdmaLanguageJapanese = 4,
        MbbSmsCdmaLanguageKorean = 5,
        MbbSmsCdmaLanguageChinese = 6,
        MbbSmsCdmaLanguageHebrew = 7,
        MbbSmsCdmaLanguageMaximum = ( MbbSmsCdmaLanguageHebrew + 1 ) 
    } MBB_SMS_CDMA_LANGUAGE;

typedef 
enum _MBB_SMS_CDMA_ENCODING
    {
        MbbSmsCdmaEncodingOctet = 0,
        MbbSmsCdmaEncodingEpm = 1,
        MbbSmsCdmaEncoding7BitAscii = 2,
        MbbSmsCdmaEncodingIa5 = 3,
        MbbSmsCdmaEncodingUnicode = 4,
        MbbSmsCdmaEncodingShiftJis = 5,
        MbbSmsCdmaEncodingKorean = 6,
        MbbSmsCdmaEncodingLatinHebrew = 7,
        MbbSmsCdmaEncodingLatin = 8,
        MbbSmsCdmaEncodingGsm7Bit = 9,
        MbbSmsCdmaEncodingMaximum = ( MbbSmsCdmaEncodingGsm7Bit + 1 ) 
    } MBB_SMS_CDMA_ENCODING;

typedef 
enum _MBB_SMS_MESSAGE_STATUS
    {
        MbbSmsMessageStatusNew = 0,
        MbbSmsMessageStatusOld = 1,
        MbbSmsMessageStatusDraft = 2,
        MbbSmsMessageStatusSent = 3,
        MbbSmsMessageStatusMaximum = ( MbbSmsMessageStatusSent + 1 ) 
    } MBB_SMS_MESSAGE_STATUS;

typedef 
enum _MBB_REG_FLAGS_VALUE
    {
        MbbRegFlagsNone = 0,
        MbbRegFlagsNoManualReg = 0x1,
        MbbRegFlagsPSAutoAttach = 0x2
    } MBB_REG_FLAGS_VALUE;

typedef 
enum _MBB_SMS_STATUS_FLAGS_VALUE
    {
        MbbSmsFlagNone = 0,
        MbbSmsFlagMessageStoreFull = 1,
        MbbSmsFlagNewMessage = 2
    } MBB_SMS_STATUS_FLAGS_VALUE;

typedef struct _MBB_STRING
    {
    ULONG Offset;
    ULONG Size;
    } MBB_STRING;

typedef struct _MBB_STRING *PMBB_STRING;

typedef struct _MBB_ARRAY_ELEMENT
    {
    ULONG Offset;
    ULONG Size;
    } MBB_ARRAY_ELEMENT;

typedef struct _MBB_ARRAY_ELEMENT *PMBB_ARRAY_ELEMENT;

typedef struct _MBB_ARRAY_ELEMENT2
    {
    ULONG Size;
    ULONG Offset;
    } MBB_ARRAY_ELEMENT2;

typedef struct _MBB_ARRAY_ELEMENT2 *PMBB_ARRAY_ELEMENT2;

typedef 
enum _MBB_DSS_PAYLOAD_SUPPORT
    {
        MbbDssPayloadNone = 0,
        MbbDssPayloadHostToDevice = 0x1,
        MbbDssPayloadDeviceToHost = 0x2
    } MBB_DSS_PAYLOAD_SUPPORT;

typedef 
enum _MBB_DSS_LINK_STATE
    {
        MbbDssLinkDeactivate = 0,
        MbbDssLinkActivate = 0x1
    } MBB_DSS_LINK_STATE;

typedef 
enum _MBB_IP_CONFIGURATION_FLAGS_VALUE
    {
        MbbIpFlagsNone = 0,
        MbbIpFlagsAddressAvailable = 0x1,
        MbbIpFlagsGatewayAvailable = 0x2,
        MbbIpFlagsDnsServerAvailable = 0x4,
        MbbIpFlagsMTUAvailable = 0x8
    } MBB_IP_CONFIGURATION_FLAGS_VALUE;

typedef 
enum _MBB_NETWORK_IDLE_HINT_STATE
    {
        MbbNetworkIdleHintDisabled = 0,
        MbbNetworkIdleHintEnabled = 1
    } MBB_NETWORK_IDLE_HINT_STATE;

typedef 
enum _MBB_SAR_BACKOFF_STATE
    {
        MbbSarBackoffStatusDisabled = 0,
        MbbSarBackoffStatusEnabled = 1,
        MbbSarBackoffStatusMaximum = ( MbbSarBackoffStatusEnabled + 1 ) 
    } MBB_SAR_BACKOFF_STATE;

typedef 
enum _MBB_SAR_CONTROL_MODE
    {
        MbbSarControlModeDevice = 0,
        MbbSarControlModeOS = 1,
        MbbSarControlModeMaximum = ( MbbSarControlModeOS + 1 ) 
    } MBB_SAR_CONTROL_MODE;

typedef 
enum _MBB_SAR_WIFI_HARDWARE_INTEGRATION
    {
        MbbSarWifiHardwareNotIntegrated = 0,
        MbbSarWifiHardwareIntegrated = 1,
        MbbSarWifiHardwareIntegrationMaximum = ( MbbSarWifiHardwareIntegrated + 1 ) 
    } MBB_SAR_WIFI_HARDWARE_INTEGRATION;

typedef 
enum _MBB_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE
    {
        MbbTransmissionNotificationDisabled = 0,
        MbbTransmissionNotificationEnabled = 1,
        MbbTransmissionNotificationMaximum = ( MbbTransmissionNotificationEnabled + 1 ) 
    } MBB_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE;

typedef 
enum _MBB_SAR_TRANSMISSION_STATUS
    {
        MbbTransmissionStateInactive = 0,
        MbbTransmissionStateActive = 1,
        MbbTransmissionStateMaximum = ( MbbTransmissionStateActive + 1 ) 
    } MBB_SAR_TRANSMISSION_STATUS;

typedef 
enum _MBB_BASICCONNECTEXT_CONTEXT_ROAMING_CONTROL
    {
        MbbMsContextRoamingControlHomeOnly = 0,
        MbbMsContextRoamingControlPartnerOnly = 1,
        MbbMsContextRoamingControlNonPartnerOnly = 2,
        MbbMsContextRoamingControlHomeAndPartner = 3,
        MbbMsContextRoamingControlHomeAndNonPartner = 4,
        MbbMsContextRoamingControlPartnerAndNonPartner = 5,
        MbbMsContextRoamingControlAllowAll = 6,
        MbbMsContextRoamingControlMaximum = ( MbbMsContextRoamingControlAllowAll + 1 ) 
    } MBB_BASICCONNECTEXT_CONTEXT_ROAMING_CONTROL;

typedef 
enum _MBB_BASICCONNECTEXT_CONTEXT_MEDIA_TYPE
    {
        MbbMsContextMediaTypeCellularOnly = 0,
        MbbMsContextMediaTypeWifiOnly = 1,
        MbbMsContextMediaTypeAll = 2,
        MbbMsContextMediaTypeMaximum = ( MbbMsContextMediaTypeAll + 1 ) 
    } MBB_BASICCONNECTEXT_CONTEXT_MEDIA_TYPE;

typedef 
enum _MBB_BASICCONNECTEXT_CONTEXT_ENABLE
    {
        MbbMsContextDisabled = 0,
        MbbMsContextEnabled = 1,
        MbbMsContextEnableMaximum = ( MbbMsContextEnabled + 1 ) 
    } MBB_BASICCONNECTEXT_CONTEXT_ENABLE;

typedef 
enum _MBB_BASICCONNECTEXT_CONTEXT_SOURCE
    {
        MbbMsContextSourceAdmin = 0,
        MbbMsContextSourceUser = 1,
        MbbMsContextSourceOperator = 2,
        MbbMsContextSourceModem = 3,
        MbbMsContextSourceDevice = 4,
        MbbMsContextSourceMaximum = ( MbbMsContextSourceDevice + 1 ) 
    } MBB_BASICCONNECTEXT_CONTEXT_SOURCE;

typedef 
enum _MBB_BASICCONNECTEXT_CONTEXT_OPERATIONS
    {
        MbbMsContextOperationDefault = 0,
        MbbMsContextOperationDelete = 1,
        MbbMsContextOperationRestoreFactory = 2,
        MbbMsContextOperationMaximum = ( MbbMsContextOperationRestoreFactory + 1 ) 
    } MBB_BASICCONNECTEXT_CONTEXT_OPERATIONS;

typedef 
enum _MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_STATE
    {
        MbbMsNetworkBlacklistStateNotActuated = 0,
        MbbMsNetworkBlacklistSIMProviderActuated = 1,
        MbbMsNetworkBlacklistNetworkProviderActuated = 2,
        MbbMsNetworkBlacklistStateMaximum = ( MbbMsNetworkBlacklistNetworkProviderActuated + 1 ) 
    } MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_STATE;

typedef 
enum _MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_TYPE
    {
        MbbMsNetworkBlacklistTypeSIM = 0,
        MbbMsNetworkBlacklistTypeNetwork = 1,
        MbbMsNetworkBlacklistTypeMaximum = ( MbbMsNetworkBlacklistTypeNetwork + 1 ) 
    } MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_TYPE;

typedef 
enum _MBB_BASICCONNECTEXT_LTEATTACH_ROAMING_CONTROL
    {
        MbbMsLteAttachContextRoamingControlHome = 0,
        MbbMsLteAttachContextRoamingControlPartner = 1,
        MbbMsLteAttachContextRoamingControlNonPartner = 2,
        MbbMsLteAttachContextRoamingControlMaximum = ( MbbMsLteAttachContextRoamingControlNonPartner + 1 ) 
    } MBB_BASICCONNECTEXT_LTEATTACH_ROAMING_CONTROL;

typedef 
enum _MBB_BASICCONNECTEXT_LTEATTACH_CONTEXT_OPERATIONS
    {
        MbbMsLteAttachContextOperationDefault = 0,
        MbbMsLteAttachContextOperationRestoreFactory = 1,
        MbbMsLteAttachContextOperationMaximum = ( MbbMsLteAttachContextOperationRestoreFactory + 1 ) 
    } MBB_BASICCONNECTEXT_LTEATTACH_CONTEXT_OPERATIONS;

typedef 
enum _MBB_BASICCONNECTEXT_LTEATTACH_STATE
    {
        MbbMsLteAttachStateDetached = 0,
        MbbMsLteAttachStateAttached = 1,
        MbbMsLteAttachStateMaximum = ( MbbMsLteAttachStateAttached + 1 ) 
    } MBB_BASICCONNECTEXT_LTEATTACH_STATE;

typedef 
enum _MBB_BASICCONNECTEXT_UICCSLOT_STATE
    {
        MbbMsUiccSlotStateUnknown = 0,
        MbbMsUiccSlotStateOffEmpty = 1,
        MbbMsUiccSlotStateOff = 2,
        MbbMsUiccSlotStateEmpty = 3,
        MbbMsUiccSlotStateNotReady = 4,
        MbbMsUiccSlotStateActive = 5,
        MbbMsUiccSlotStateError = 6,
        MbbMsUiccSlotStateActiveEsim = 7,
        MbbMsUiccSlotStateActiveEsimNoProfiles = 8
    } MBB_BASICCONNECTEXT_UICCSLOT_STATE;

typedef 
enum _MBB_BASICCONNECTEXT_MODEM_LOGGING_LEVEL_CONFIG
    {
        MbbMsModemLoggingLevelProd = 0,
        MbbMsModemLoggingLevelLabVerbose = 1,
        MbbMsModemLoggingLevelLabMedium = 2,
        MbbMsModemLoggingLevelLabLow = 3,
        MbbMsModemLoggingLevelOem = 4,
        MbbMsModemLoggingLevelMaximum = ( MbbMsModemLoggingLevelOem + 1 ) 
    } MBB_BASICCONNECTEXT_MODEM_LOGGING_LEVEL_CONFIG;

typedef 
enum _MBB_PCO_TYPE
    {
        MbbPcoTypeComplete = 0,
        MbbPcoTypePartial = 1,
        MbbPcoTypeMaximum = ( MbbPcoTypePartial + 1 ) 
    } MBB_PCO_TYPE;

typedef struct _MBB_DEVICE_CAPS
    {
    MBB_DEVICE_TYPE DeviceType;
    MBB_CELLULAR_CLASS CellularClass;
    MBB_VOICE_CLASS VoiceClass;
    MBB_SIM_CLASS SimClass;
    ULONG DataClass;
    ULONG SmsCaps;
    ULONG ControlCaps;
    ULONG dwMaxSessions;
    MBB_STRING CustomDataClass;
    MBB_STRING DeviceIdString;
    MBB_STRING FirmwareInfo;
    MBB_STRING HardwareInfo;
    UCHAR DataBuffer[ 1 ];
    } MBB_DEVICE_CAPS;

typedef struct _MBB_DEVICE_CAPS *PMBB_DEVICE_CAPS;

typedef struct _MBB_SUBSCRIBER_READY_INFO
    {
    MBB_READY_STATE ReadyState;
    MBB_STRING SubscriberId;
    MBB_STRING SimIccId;
    MBB_READY_INFO_FLAGS ReadyInfoFlags;
    ULONG TelephoneNumberCount;
    MBB_STRING TelephoneNumbers[ 1 ];
    } MBB_SUBSCRIBER_READY_INFO;

typedef struct _MBB_SUBSCRIBER_READY_INFO *PMBB_SUBSCRIBER_READY_INFO;

typedef 
enum _MBB_SUBSCRIBER_READY_STATUS_FLAGS
    {
        MbbSubscriberReadyStatusFlagNone = 0,
        MbbSubscriberReadyStatusFlagESim = 0x1,
        MbbSubscriberReadyStatusFlagSIMRemovabilityKnown = 0x2,
        MbbSubscriberReadyStatusFlagSIMRemovable = 0x4
    } MBB_SUBSCRIBER_READY_STATUS_FLAGS;

typedef struct _MBB_SUBSCRIBER_READY_INFO_EX3
    {
    MBB_READY_STATE ReadyState;
    MBB_SUBSCRIBER_READY_STATUS_FLAGS StatusFlags;
    MBB_STRING SubscriberId;
    MBB_STRING SimIccId;
    MBB_READY_INFO_FLAGS ReadyInfoFlags;
    ULONG TelephoneNumberCount;
    MBB_STRING TelephoneNumbers[ 1 ];
    } MBB_SUBSCRIBER_READY_INFO_EX3;

typedef struct _MBB_SUBSCRIBER_READY_INFO_EX3 *PMBB_SUBSCRIBER_READY_INFO_EX3;

typedef struct _MBB_QUERY_RADIO_STATE
    {
    MBB_RADIO_STATE HwRadioState;
    MBB_RADIO_STATE SwRadioState;
    } MBB_QUERY_RADIO_STATE;

typedef struct _MBB_QUERY_RADIO_STATE *PMBB_QUERY_RADIO_STATE;

typedef struct _MBB_PIN_INFO
    {
    MBB_PIN_TYPE PinType;
    MBB_PIN_STATE PinState;
    ULONG AttemptsRemaining;
    } MBB_PIN_INFO;

typedef struct _MBB_PIN_INFO *PMBB_PIN_INFO;

typedef struct _MBB_PIN_ACTION
    {
    MBB_PIN_TYPE PinType;
    MBB_PIN_OPERATION PinOperation;
    MBB_STRING Pin;
    MBB_STRING NewPin;
    UCHAR DataBuffer[ 1 ];
    } MBB_PIN_ACTION;

typedef struct _MBB_PIN_ACTION *PMBB_PIN_ACTION;

typedef struct _MBB_PIN_DESCRIPTION
    {
    MBB_PIN_MODE PinMode;
    MBB_PIN_FORMAT PinFormat;
    ULONG PinLengthMin;
    ULONG PinLengthMax;
    } MBB_PIN_DESCRIPTION;

typedef struct _MBB_PIN_DESCRIPTION *PMBB_PIN_DESCRIPTION;

typedef struct _MBB_PIN_LIST
    {
    MBB_PIN_DESCRIPTION PinDescPin1;
    MBB_PIN_DESCRIPTION PinDescPin2;
    MBB_PIN_DESCRIPTION PinDescDeviceSimPin;
    MBB_PIN_DESCRIPTION PinDescDeviceFirstSimPin;
    MBB_PIN_DESCRIPTION PinDescNetworkPin;
    MBB_PIN_DESCRIPTION PinDescNetworkSubsetPin;
    MBB_PIN_DESCRIPTION PinDescSvcProviderPin;
    MBB_PIN_DESCRIPTION PinDescCorporatePin;
    MBB_PIN_DESCRIPTION PinDescSubsidyLock;
    MBB_PIN_DESCRIPTION PinDescCustom;
    } MBB_PIN_LIST;

typedef struct _MBB_PIN_LIST *PMBB_PIN_LIST;

typedef struct _MBB_PROVIDER
    {
    MBB_STRING ProviderId;
    ULONG ProviderState;
    MBB_STRING ProviderName;
    MBB_CELLULAR_CLASS CellularClass;
    ULONG Rssi;
    ULONG ErrorRate;
    } MBB_PROVIDER;

typedef struct _MBB_PROVIDER *PMBB_PROVIDER;

typedef struct _MBB_PROVIDER_LIST
    {
    ULONG ProviderCount;
    MBB_ARRAY_ELEMENT Providers[ 1 ];
    } MBB_PROVIDER_LIST;

typedef struct _MBB_PROVIDER_LIST *PMBB_PROVIDER_LIST;

typedef struct _MBB_REGISTRATION_STATE
    {
    ULONG NetworkError;
    MBB_REGISTER_STATE RegisterState;
    MBB_REGISTER_MODE RegisterMode;
    ULONG AvailableDataClasses;
    MBB_CELLULAR_CLASS CurrentCellularClass;
    MBB_STRING ProviderId;
    MBB_STRING ProviderName;
    MBB_STRING RoamingText;
    ULONG RegFlags;
    UCHAR DataBuffer[ 1 ];
    } MBB_REGISTRATION_STATE;

typedef struct _MBB_REGISTRATION_STATE *PMBB_REGISTRATION_STATE;

typedef struct _MBB_SET_REGISTER_STATE
    {
    MBB_STRING ProviderId;
    MBB_REGISTER_ACTION RegisterAction;
    ULONG DataClass;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_REGISTER_STATE;

typedef struct _MBB_SET_REGISTER_STATE *PMBB_SET_REGISTER_STATE;

typedef struct _MBB_SET_PACKET_SERVICE
    {
    MBB_PACKET_SERVICE_ACTION PacketServiceAction;
    } MBB_SET_PACKET_SERVICE;

typedef struct _MBB_SET_PACKET_SERVICE *PMBB_SET_PACKET_SERVICE;

typedef struct _MBB_PACKET_SERVICE
    {
    ULONG NetworkError;
    MBB_PACKET_SERVICE_STATE PacketServiceState;
    ULONG HighestAvailableDataClass;
    ULONGLONG UplinkSpeed;
    ULONGLONG DownlinkSpeed;
    } MBB_PACKET_SERVICE;

typedef struct _MBB_PACKET_SERVICE *PMBB_PACKET_SERVICE;

typedef struct _MBB_PACKET_SERVICE_INFO_V2
    {
    ULONG NetworkError;
    MBB_PACKET_SERVICE_STATE PacketServiceState;
    MBB_DATA_CLASS_VALUE CurrentDataClass;
    ULONGLONG UplinkSpeed;
    ULONGLONG DownlinkSpeed;
    MBB_FREQUENCY_RANGE FrequencyRange;
    } MBB_PACKET_SERVICE_INFO_V2;

typedef struct _MBB_PACKET_SERVICE_INFO_V2 *PMBB_PACKET_SERVICE_INFO_V2;

typedef struct _MBB_PLMN
    {
    USHORT Mcc;
    USHORT Mnc;
    } MBB_PLMN;

typedef struct _MBB_PLMN *PMBB_PLMN;

typedef struct _MBB_TAI
    {
    MBB_PLMN Plmn;
    ULONG Tac;
    } MBB_TAI;

typedef struct _MBB_TAI *PMBB_TAI;

typedef struct _MBB_PACKET_SERVICE_INFO_V3
    {
    ULONG NetworkError;
    MBB_PACKET_SERVICE_STATE PacketServiceState;
    MBB_DATA_CLASS_VALUE CurrentDataClass;
    ULONGLONG UplinkSpeed;
    ULONGLONG DownlinkSpeed;
    MBB_FREQUENCY_RANGE FrequencyRange;
    MBB_DATA_SUBCLASS_VALUE CurrentDataSubClass;
    MBB_TAI TrackingAreaId;
    } MBB_PACKET_SERVICE_INFO_V3;

typedef struct _MBB_PACKET_SERVICE_INFO_V3 *PMBB_PACKET_SERVICE_INFO_V3;

typedef struct _MBB_SET_SIGNAL_INDICATION
    {
    ULONG RssiInterval;
    ULONG RssiThreshold;
    ULONG ErrorRateThreshold;
    } MBB_SET_SIGNAL_INDICATION;

typedef struct _MBB_SET_SIGNAL_INDICATION *PMBB_SET_SIGNAL_INDICATION;

typedef struct _MBB_SIGNAL_STATE
    {
    ULONG Rssi;
    ULONG ErrorRate;
    ULONG RssiInterval;
    ULONG RssiThreshold;
    ULONG ErrorRateThreshold;
    } MBB_SIGNAL_STATE;

typedef struct _MBB_SIGNAL_STATE *PMBB_SIGNAL_STATE;

typedef struct _MBB_SET_CONTEXT_STATE
    {
    ULONG SessionId;
    MBB_ACTIVATION_COMMAND ActivationCommand;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    MBB_CONTEXT_IP_TYPE IPType;
    GUID ContextType;
    CHAR DataBuffer[ 1 ];
    } MBB_SET_CONTEXT_STATE;

typedef struct _MBB_SET_CONTEXT_STATE *PMBB_SET_CONTEXT_STATE;

typedef 
enum _MBB_ACCESS_MEDIA_PREF
    {
        MbbAccessMediaPrefNone = 0,
        MbbAccessMediaPref3GPP = 1,
        MbbAccessMediaPref3GPPPreferred = 2,
        MbbAccessMediaPrefMaximum = ( MbbAccessMediaPref3GPPPreferred + 1 ) 
    } MBB_ACCESS_MEDIA_PREF;

typedef struct _MBB_SET_CONTEXT_STATE_EX3
    {
    ULONG SessionId;
    MBB_ACTIVATION_COMMAND ActivationCommand;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    MBB_CONTEXT_IP_TYPE IPType;
    GUID ContextType;
    MBB_ACCESS_MEDIA_PREF MediaPreference;
    } MBB_SET_CONTEXT_STATE_EX3;

typedef struct _MBB_SET_CONTEXT_STATE_EX3 *PMBB_SET_CONTEXT_STATE_EX3;

typedef struct _MBB_QUERY_CONTEXT_EX3
    {
    ULONG SessionId;
    } MBB_QUERY_CONTEXT_EX3;

typedef struct _MBB_QUERY_CONTEXT_EX3 *PMBB_QUERY_CONTEXT_EX3;

typedef struct _MBB_CONTEXT_STATE
    {
    ULONG SessionId;
    MBB_ACTIVATION_STATE ActivationState;
    MBB_VOICE_CALL_STATE VoiceCallState;
    MBB_CONTEXT_IP_TYPE IPType;
    GUID ContextType;
    ULONG NetworkError;
    } MBB_CONTEXT_STATE;

typedef struct _MBB_CONTEXT_STATE *PMBB_CONTEXT_STATE;

typedef struct _MBB_CONTEXT_STATE_EX3
    {
    ULONG SessionId;
    MBB_ACTIVATION_STATE ActivationState;
    MBB_VOICE_CALL_STATE VoiceCallState;
    MBB_CONTEXT_IP_TYPE IPType;
    GUID ContextType;
    ULONG NetworkError;
    MBB_ACCESS_MEDIA_PREF MediaPreference;
    } MBB_CONTEXT_STATE_EX3;

typedef struct _MBB_CONTEXT_STATE_EX3 *PMBB_CONTEXT_STATE_EX3;

typedef struct _MBB_SET_CONTEXT
    {
    ULONG ContextId;
    GUID ContextType;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    MBB_STRING ProviderId;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_CONTEXT;

typedef struct _MBB_SET_CONTEXT *PMBB_SET_CONTEXT;

typedef struct _MBB_CONTEXT
    {
    ULONG ContextId;
    GUID ContextType;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_CONTEXT;

typedef struct _MBB_CONTEXT *PMBB_CONTEXT;

typedef struct _MBB_CONTEXT_LIST
    {
    ULONG ContextCount;
    MBB_ARRAY_ELEMENT Contexts[ 1 ];
    } MBB_CONTEXT_LIST;

typedef struct _MBB_CONTEXT_LIST *PMBB_CONTEXT_LIST;

typedef struct _MBB_SERVICE_ACTIVATION
    {
    CHAR VendorSpecificBuffer[ 1 ];
    } MBB_SERVICE_ACTIVATION;

typedef struct _MBB_SERVICE_ACTIVATION *PMBB_SERVICE_ACTIVATION;

typedef struct _MBB_SERVICE_ACTIVATION_STATUS
    {
    ULONG NetworkError;
    CHAR VendorSpecificBuffer[ 1 ];
    } MBB_SERVICE_ACTIVATION_STATUS;

typedef struct _MBB_SERVICE_ACTIVATION_STATUS *PMBB_SERVICE_ACTIVATION_STATUS;

typedef struct _MBB_IPV4_ADDRESS
    {
    ULONG OnLinkPrefixLength;
    UCHAR IPV4Address[ 4 ];
    } MBB_IPV4_ADDRESS;

typedef struct _MBB_IPV4_ADDRESS *PMBB_IPV4_ADDRESS;

typedef struct _MBB_IPV6_ADDRESS
    {
    ULONG OnLinkPrefixLength;
    UCHAR IPV6Address[ 16 ];
    } MBB_IPV6_ADDRESS;

typedef struct _MBB_IPV6_ADDRESS *PMBB_IPV6_ADDRESS;

typedef struct _MBB_IP_ADDRESS_INFO
    {
    ULONG SessionId;
    ULONG IPv4Flags;
    ULONG IPv6Flags;
    ULONG IPv4AddressCount;
    ULONG IPv4AddressOffset;
    ULONG IPv6AddressCount;
    ULONG IPv6AddressOffset;
    ULONG IPv4GatewayOffset;
    ULONG IPv6GatewayOffset;
    ULONG IPv4DnsServerCount;
    ULONG IPv4DnsServerOffset;
    ULONG IPv6DnsServerCount;
    ULONG IPv6DnsServerOffset;
    ULONG IPv4MTU;
    ULONG IPv6MTU;
    } MBB_IP_ADDRESS_INFO;

typedef struct _MBB_IP_ADDRESS_INFO *PMBB_IP_ADDRESS_INFO;

typedef struct _MBB_PACKET_STATISTICS
    {
    ULONG InDiscards;
    ULONG InErrors;
    ULONGLONG InOctets;
    ULONGLONG InPackets;
    ULONGLONG OutOctets;
    ULONGLONG OutPackets;
    ULONG OutErrors;
    ULONG OutDiscards;
    } MBB_PACKET_STATISTICS;

typedef struct _MBB_PACKET_STATISTICS *PMBB_PACKET_STATISTICS;

typedef 
enum _MBB_VISIBLE_PROVIDERS_ACTION_VALUE
    {
        MbbVisibleProvidersActionFull = 0,
        MbbVisibleProvidersActionMulticarrier = 0x1,
        MbbVisibleProvidersActionMax = ( MbbVisibleProvidersActionMulticarrier + 1 ) 
    } MBB_VISIBLE_PROVIDERS_ACTION_VALUE;

typedef struct _MBB_GET_VISIBLE_PROVIDERS
    {
    ULONG Action;
    } MBB_GET_VISIBLE_PROVIDERS;

typedef struct _MBB_GET_VISIBLE_PROVIDERS *PMBB_GET_VISIBLE_PROVIDERS;

typedef 
enum _MBB_SMS_STORAGE_STATE
    {
        MbbSmsStorageNotInitialized = 0,
        MbbSmsStorageInitialized = 1
    } MBB_SMS_STORAGE_STATE;

typedef struct _MBB_SET_SMS_CONFIGURATION
    {
    MBB_SMS_FORMAT SmsFormat;
    MBB_STRING ScAddress;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_SMS_CONFIGURATION;

typedef struct _MBB_SET_SMS_CONFIGURATION *PMBB_SET_SMS_CONFIGURATION;

typedef struct _MBB_SMS_CONFIGURATION
    {
    MBB_SMS_STORAGE_STATE SmsStorageState;
    MBB_SMS_FORMAT SmsFormat;
    ULONG MaxMessages;
    ULONG CdmaShortMessageSize;
    MBB_STRING ScAddress;
    UCHAR DataBuffer[ 1 ];
    } MBB_SMS_CONFIGURATION;

typedef struct _MBB_SMS_CONFIGURATION *PMBB_SMS_CONFIGURATION;

typedef struct _MBB_SMS_READ
    {
    MBB_SMS_FORMAT SmsFormat;
    MBB_SMS_FLAG SmsFlag;
    ULONG MessageIndex;
    } MBB_SMS_READ;

typedef struct _MBB_SMS_READ *PMBB_SMS_READ;

typedef struct _MBB_SMS_CDMA_RECORD
    {
    ULONG MessageIndex;
    MBB_SMS_MESSAGE_STATUS MessageStatus;
    MBB_STRING Address;
    MBB_STRING TimeStamp;
    MBB_SMS_CDMA_ENCODING EncodingId;
    MBB_SMS_CDMA_LANGUAGE LanguageId;
    MBB_ARRAY_ELEMENT EncodedMessage;
    ULONG SizeInCharacters;
    UCHAR DataBuffer[ 1 ];
    } MBB_SMS_CDMA_RECORD;

typedef struct _MBB_SMS_CDMA_RECORD *PMBB_SMS_CDMA_RECORD;

typedef struct _MBB_SMS_PDU_RECORD
    {
    ULONG MessageIndex;
    MBB_SMS_MESSAGE_STATUS MessageStatus;
    MBB_ARRAY_ELEMENT PduData;
    UCHAR DataBuffer[ 1 ];
    } MBB_SMS_PDU_RECORD;

typedef struct _MBB_SMS_PDU_RECORD *PMBB_SMS_PDU_RECORD;

typedef struct _MBB_SMS_RECEIVE
    {
    MBB_SMS_FORMAT SmsFormat;
    ULONG MessageCount;
    MBB_ARRAY_ELEMENT MessageElement[ 1 ];
    } MBB_SMS_RECEIVE;

typedef struct _MBB_SMS_RECEIVE *PMBB_SMS_RECEIVE;

typedef struct _MBB_SMS_SEND_PDU
    {
    MBB_ARRAY_ELEMENT PduData;
    UCHAR DataBuffer[ 1 ];
    } MBB_SMS_SEND_PDU;

typedef struct _MBB_SMS_SEND_PDU *PMBB_SMS_SEND_PDU;

typedef struct _MBB_SMS_SEND_CDMA
    {
    MBB_SMS_CDMA_ENCODING EncodingId;
    MBB_SMS_CDMA_LANGUAGE LanguageId;
    MBB_STRING Address;
    MBB_ARRAY_ELEMENT EncodedMessage;
    ULONG SizeInCharacters;
    UCHAR DataBuffer[ 1 ];
    } MBB_SMS_SEND_CDMA;

typedef struct _MBB_SMS_SEND_CDMA *PMBB_SMS_SEND_CDMA;

typedef struct _MBB_SMS_SEND
    {
    MBB_SMS_FORMAT SmsFormat;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_MbbMessages_0000_0000_0001
        {
        /* [case()] */ MBB_SMS_SEND_PDU Pdu;
        /* [case()] */ MBB_SMS_SEND_CDMA Cdma;
        } u;
    } MBB_SMS_SEND;

typedef struct _MBB_SMS_SEND *PMBB_SMS_SEND;

typedef struct _MBB_SMS_SEND_STATUS
    {
    ULONG MessageReference;
    } MBB_SMS_SEND_STATUS;

typedef struct _MBB_SMS_SEND_STATUS *PMBB_SMS_SEND_STATUS;

typedef struct _MBB_SMS_DELETE
    {
    MBB_SMS_FLAG SmsFlags;
    ULONG MessageIndex;
    } MBB_SMS_DELETE;

typedef struct _MBB_SMS_DELETE *PMBB_SMS_DELETE;

typedef struct _MBB_SMS_STATUS
    {
    ULONG StatusFlags;
    ULONG MessageIndex;
    } MBB_SMS_STATUS;

typedef struct _MBB_SMS_STATUS *PMBB_SMS_STATUS;

typedef 
enum _MBB_USSD_ACTION
    {
        MbbUSSDInitiate = 0,
        MbbUSSDContinue = 1,
        MbbUSSDCancel = 2
    } MBB_USSD_ACTION;

typedef 
enum _MBB_USSD_RESPONSE
    {
        MbbUSSDNoActionRequired = 0,
        MbbUSSDActionRequired = 1,
        MbbUSSDTerminated = 2,
        MbbUSSDOtherLocalClient = 3,
        MbbUSSDOperationNotSupported = 4,
        MbbUSSDNetworkTimeOut = 5
    } MBB_USSD_RESPONSE;

typedef 
enum _MBB_USSD_SESSION_STATE
    {
        MbbUSSDNewSession = 0,
        MbbUSSDExistingSession = 1
    } MBB_USSD_SESSION_STATE;

typedef struct _MBB_SET_USSD
    {
    MBB_USSD_ACTION USSDAction;
    ULONG USSDDataCodingScheme;
    MBB_ARRAY_ELEMENT USSDPayload;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_USSD;

typedef struct _MBB_SET_USSD *PMBB_SET_USSD;

typedef struct _MBB_USSD
    {
    MBB_USSD_RESPONSE USSDResponse;
    MBB_USSD_SESSION_STATE USSDSessionState;
    ULONG USSDDataCodingScheme;
    MBB_ARRAY_ELEMENT USSDPayload;
    UCHAR DataBuffer[ 1 ];
    } MBB_USSD;

typedef struct _MBB_USSD *PMBB_USSD;

typedef struct _MBB_AKA_AUTH_CHALLENGE
    {
    UCHAR Rand[ 16 ];
    UCHAR Autn[ 16 ];
    } MBB_AKA_AUTH_CHALLENGE;

typedef struct _MBB_AKA_AUTH_CHALLENGE *PMBB_AKA_AUTH_CHALLENGE;

typedef struct _MBB_AKA_AUTH_RESPONSE
    {
    UCHAR Res[ 16 ];
    ULONG ResLength;
    UCHAR IK[ 16 ];
    UCHAR CK[ 16 ];
    UCHAR Auts[ 14 ];
    } MBB_AKA_AUTH_RESPONSE;

typedef struct _MBB_AKA_AUTH_RESPONSE *PMBB_AKA_AUTH_RESPONSE;

typedef struct _MBB_AKAP_AUTH_CHALLENGE
    {
    UCHAR Rand[ 16 ];
    UCHAR Autn[ 16 ];
    MBB_STRING NetworkName;
    UCHAR DataBuffer[ 1 ];
    } MBB_AKAP_AUTH_CHALLENGE;

typedef struct _MBB_AKAP_AUTH_CHALLENGE *PMBB_AKAP_AUTH_CHALLENGE;

typedef struct _MBB_AKAP_AUTH_RESPONSE
    {
    UCHAR Res[ 16 ];
    ULONG ResLength;
    UCHAR IK[ 16 ];
    UCHAR CK[ 16 ];
    UCHAR Auts[ 14 ];
    } MBB_AKAP_AUTH_RESPONSE;

typedef struct _MBB_AKAP_AUTH_RESPONSE *PMBB_AKAP_AUTH_RESPONSE;

typedef struct _MBB_SIM_AUTH_CHALLENGE
    {
    UCHAR Rand1[ 16 ];
    UCHAR Rand2[ 16 ];
    UCHAR Rand3[ 16 ];
    ULONG n;
    } MBB_SIM_AUTH_CHALLENGE;

typedef struct _MBB_SIM_AUTH_CHALLENGE *PMBB_SIM_AUTH_CHALLENGE;

typedef struct _MBB_SIM_AUTH_RESPONSE
    {
    UCHAR Sres1[ 4 ];
    UCHAR Kc1[ 8 ];
    UCHAR Sres2[ 4 ];
    UCHAR Kc2[ 8 ];
    UCHAR Sres3[ 4 ];
    UCHAR Kc3[ 8 ];
    ULONG n;
    } MBB_SIM_AUTH_RESPONSE;

typedef struct _MBB_SIM_AUTH_RESPONSE *PMBB_SIM_AUTH_RESPONSE;

typedef struct _MBB_PACKET_FILTERS
    {
    ULONG SessionId;
    ULONG PacketFiltersCount;
    MBB_ARRAY_ELEMENT ArrayElement[ 1 ];
    } MBB_PACKET_FILTERS;

typedef struct _MBB_PACKET_FILTERS *PMBB_PACKET_FILTERS;

typedef struct _MBB_SINGLE_PACKET_FILTER
    {
    ULONG FilterSize;
    ULONG PacketFilterOffset;
    ULONG PacketMaskOffset;
    UCHAR DataBuffer[ 1 ];
    } MBB_SINGLE_PACKET_FILTER;

typedef struct _MBB_SINGLE_PACKET_FILTER *PMBB_SINGLE_PACKET_FILTER;

typedef struct _MBB_SINGLE_PACKET_FILTER_V2
    {
    ULONG FilterSize;
    ULONG PacketFilterOffset;
    ULONG PacketMaskOffset;
    ULONG FilterId;
    UCHAR DataBuffer[ 1 ];
    } MBB_SINGLE_PACKET_FILTER_V2;

typedef struct _MBB_SINGLE_PACKET_FILTER_V2 *PMBB_SINGLE_PACKET_FILTER_V2;

typedef struct _MBB_NETWORK_IDLE_HINT
    {
    MBB_NETWORK_IDLE_HINT_STATE NetworkIdleHintState;
    } MBB_NETWORK_IDLE_HINT;

typedef struct _MBB_NETWORK_IDLE_HINT *PMBB_NETWORK_IDLE_HINT;

typedef struct _MBB_DEVICE_SERVICE_ELEMENT
    {
    GUID DeviceServiceId;
    MBB_DSS_PAYLOAD_SUPPORT DSSPayload;
    ULONG MaxDSSInstances;
    ULONG CIDCount;
    ULONG CIDList[ 1 ];
    } MBB_DEVICE_SERVICE_ELEMENT;

typedef struct _MBB_DEVICE_SERVICE_ELEMENT *PMBB_DEVICE_SERVICE_ELEMENT;

typedef struct _MBB_DEVICE_SERVICES_HEADER
    {
    ULONG DeviceServicesCount;
    ULONG MaxDSSSessions;
    MBB_ARRAY_ELEMENT ArrayElement[ 1 ];
    } MBB_DEVICE_SERVICES_HEADER;

typedef struct _MBB_DEVICE_SERVICES_HEADER *PMBB_DEVICE_SERVICES_HEADER;

typedef struct _MBB_SUBSCRIBE_EVENT_ENTRY
    {
    GUID DeviceServiceId;
    ULONG CIDCount;
    ULONG CIDList[ 1 ];
    } MBB_SUBSCRIBE_EVENT_ENTRY;

typedef struct _MBB_SUBSCRIBE_EVENT_ENTRY *PMBB_SUBSCRIBE_EVENT_ENTRY;

typedef struct _MBB_SUBSCRIBE_EVENT_LIST
    {
    ULONG Count;
    MBB_ARRAY_ELEMENT ArrayElement[ 1 ];
    } MBB_SUBSCRIBE_EVENT_LIST;

typedef struct _MBB_SUBSCRIBE_EVENT_LIST *PMBB_SUBSCRIBE_EVENT_LIST;

typedef struct _MBB_SET_DSS_CONNECT
    {
    GUID DeviceServiceId;
    ULONG DssSessionId;
    MBB_DSS_LINK_STATE DssLinkState;
    } MBB_SET_DSS_CONNECT;

typedef struct _MBB_SET_DSS_CONNECT *PMBB_SET_DSS_CONNECT;

typedef struct _MBB_SET_DSS_CLOSE
    {
    ULONG DssSessionId;
    } MBB_SET_DSS_CLOSE;

typedef struct _MBB_SET_DSS_CLOSE *PMBB_SET_DSS_CLOSE;

typedef struct _MBB_MULTICARRIER_CURRENT_CID_LIST
    {
    ULONG CIDCount;
    ULONG CIDList[ 1 ];
    } MBB_MULTICARRIER_CURRENT_CID_LIST;

typedef struct _MBB_MULTICARRIER_CURRENT_CID_LIST *PMBB_MULTICARRIER_CURRENT_CID_LIST;

typedef 
enum _MBB_UICC_APP_TYPE
    {
        MbbUiccAppTypeUnknown = 0,
        MbbUiccAppTypeMf = 1,
        MbbUiccAppTypeMfSIM = 2,
        MbbUiccAppTypeMfRUIM = 3,
        MbbUiccAppTypeUSIM = 4,
        MbbUiccAppTypeCSIM = 5,
        MbbUiccAppTypeISIM = 6,
        MbbUiccAppTypeMax = ( MbbUiccAppTypeISIM + 1 ) 
    } MBB_UICC_APP_TYPE;

typedef struct _MBB_UICC_APP_INFO
    {
    MBB_UICC_APP_TYPE AppType;
    MBB_ARRAY_ELEMENT AppId;
    MBB_ARRAY_ELEMENT AppName;
    ULONG NumPins;
    MBB_ARRAY_ELEMENT PinRef;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_APP_INFO;

typedef struct _MBB_UICC_APP_INFO *PMBB_UICC_APP_INFO;

typedef struct _MBB_UICC_APP_LIST
    {
    ULONG Version;
    ULONG AppCount;
    ULONG ActiveAppIndex;
    ULONG AppListSize;
    MBB_ARRAY_ELEMENT DataBuffer[ 1 ];
    } MBB_UICC_APP_LIST;

typedef struct _MBB_UICC_APP_LIST *PMBB_UICC_APP_LIST;

typedef 
enum _MBB_UICC_FILE_ACCESSIBILITY
    {
        MbbUiccFileAccessibilityUnknown = 0,
        MbbUiccFileAccessibilityNotShareable = 1,
        MbbUiccFileAccessibilityShareable = 2,
        MbbUiccFileAccessibilityMax = ( MbbUiccFileAccessibilityShareable + 1 ) 
    } MBB_UICC_FILE_ACCESSIBILITY;

typedef 
enum _MBB_UICC_FILE_TYPE
    {
        MbbUiccFileTypeUnknown = 0,
        MbbUiccFileTypeWorkingEf = 1,
        MbbUiccFileTypeInternalEf = 2,
        MbbUiccFileTypeDfOrAdf = 3,
        MbbUiccFileTypeMax = ( MbbUiccFileTypeDfOrAdf + 1 ) 
    } MBB_UICC_FILE_TYPE;

typedef 
enum _MBB_UICC_FILE_STRUCTURE
    {
        MbbUiccFileStructureUnknown = 0,
        MbbUiccFileStructureTransparent = 1,
        MbbUiccFileStructureCyclic = 2,
        MbbUiccFileStructureLinear = 3,
        MbbUiccFileStructureBertlv = 4,
        MbbUiccFileStructureMax = ( MbbUiccFileStructureBertlv + 1 ) 
    } MBB_UICC_FILE_STRUCTURE;

typedef struct _MBB_UICC_FILE_PATH
    {
    ULONG Version;
    MBB_ARRAY_ELEMENT AppId;
    MBB_ARRAY_ELEMENT FilePath;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_FILE_PATH;

typedef struct _MBB_UICC_FILE_PATH *PMBB_UICC_FILE_PATH;

typedef struct _MBB_UICC_FILE_STATUS
    {
    ULONG Version;
    ULONG StatusWord1;
    ULONG StatusWord2;
    MBB_UICC_FILE_ACCESSIBILITY FileAccessibility;
    MBB_UICC_FILE_TYPE FileType;
    MBB_UICC_FILE_STRUCTURE FileStructure;
    ULONG ItemCount;
    ULONG ItemSize;
    MBB_PIN_TYPE FileLockStatus[ 4 ];
    } MBB_UICC_FILE_STATUS;

typedef struct _MBB_UICC_FILE_STATUS *PMBB_UICC_FILE_STATUS;

typedef struct _MBB_UICC_ACCESS_BINARY
    {
    ULONG Version;
    MBB_ARRAY_ELEMENT AppId;
    MBB_ARRAY_ELEMENT UiccFilePath;
    ULONG FileOffset;
    ULONG NumberOfBytes;
    MBB_ARRAY_ELEMENT LocalPin;
    MBB_ARRAY_ELEMENT BinaryData;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_ACCESS_BINARY;

typedef struct _MBB_UICC_ACCESS_BINARY *PMBB_UICC_ACCESS_BINARY;

typedef struct _MBB_UICC_ACCESS_RECORD
    {
    ULONG Version;
    MBB_ARRAY_ELEMENT AppId;
    MBB_ARRAY_ELEMENT UiccFilePath;
    ULONG RecordNumber;
    MBB_ARRAY_ELEMENT LocalPin;
    MBB_ARRAY_ELEMENT RecordData;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_ACCESS_RECORD;

typedef struct _MBB_UICC_ACCESS_RECORD *PMBB_UICC_ACCESS_RECORD;

typedef struct _MBB_UICC_RESPONSE
    {
    ULONG Version;
    ULONG StatusWord1;
    ULONG StatusWord2;
    MBB_ARRAY_ELEMENT ResponseData;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_RESPONSE;

typedef struct _MBB_UICC_RESPONSE *PMBB_UICC_RESPONSE;

typedef struct _MBB_PIN_APP
    {
    ULONG Version;
    MBB_ARRAY_ELEMENT AppId;
    UCHAR DataBuffer[ 1 ];
    } MBB_PIN_APP;

typedef struct _MBB_PIN_APP *PMBB_PIN_APP;

typedef struct _MBB_PIN_ACTION_EX2
    {
    MBB_PIN_TYPE PinType;
    MBB_PIN_OPERATION PinOperation;
    MBB_STRING Pin;
    MBB_STRING NewPin;
    MBB_ARRAY_ELEMENT AppId;
    UCHAR DataBuffer[ 1 ];
    } MBB_PIN_ACTION_EX2;

typedef struct _MBB_PIN_ACTION_EX2 *PMBB_PIN_ACTION_EX2;

typedef struct _MBB_SYS_CAPS_INFO
    {
    ULONG NumberOfExecutors;
    ULONG NumberOfSlots;
    ULONG Concurrency;
    } MBB_SYS_CAPS_INFO;

typedef struct _MBB_SYS_CAPS_INFO *PMBB_SYS_CAPS_INFO;

typedef struct _MBB_DEVICE_CAPS_V2
    {
    MBB_DEVICE_TYPE DeviceType;
    MBB_CELLULAR_CLASS CellularClass;
    MBB_VOICE_CLASS VoiceClass;
    MBB_SIM_CLASS SimClass;
    ULONG DataClass;
    ULONG SmsCaps;
    ULONG ControlCaps;
    ULONG dwMaxSessions;
    MBB_STRING CustomDataClass;
    MBB_STRING DeviceIdString;
    MBB_STRING FirmwareInfo;
    MBB_STRING HardwareInfo;
    ULONG DeviceIndex;
    UCHAR DataBuffer[ 1 ];
    } MBB_DEVICE_CAPS_V2;

typedef struct _MBB_DEVICE_CAPS_V2 *PMBB_DEVICE_CAPS_V2;

typedef struct _MBB_DEVICE_SLOT_MAPPING_INFO
    {
    ULONG MapCount;
    MBB_ARRAY_ELEMENT SlotMapList[ 1 ];
    } MBB_DEVICE_SLOT_MAPPING_INFO;

typedef struct _MBB_DEVICE_SLOT_MAPPING_INFO *PMBB_DEVICE_SLOT_MAPPING_INFO;

typedef 
enum _MBB_UICCSLOT_STATE
    {
        MbbUiccSlotStateOffEmpty = 0,
        MbbUiccSlotStateOff = 1,
        MbbUiccSlotStateEmpty = 2,
        MbbUiccSlotStateNotReady = 3,
        MbbUiccSlotStateActive = 4,
        MbbUiccSlotStateError = 5
    } MBB_UICCSLOT_STATE;

typedef struct _MBB_SLOT_INFO_REQ
    {
    ULONG SlotIndex;
    } MBB_SLOT_INFO_REQ;

typedef struct _MBB_SLOT_INFO_REQ *PMBB_SLOT_INFO_REQ;

typedef struct _MBB_SLOT_INFO
    {
    ULONG SlotIndex;
    MBB_UICCSLOT_STATE State;
    } MBB_SLOT_INFO;

typedef struct _MBB_SLOT_INFO *PMBB_SLOT_INFO;

typedef struct _MBB_DEVICE_BINDINGS_INFO
    {
    ULONG ApplicationCount;
    MBB_ARRAY_ELEMENT ApplicationList[ 1 ];
    } MBB_DEVICE_BINDINGS_INFO;

typedef struct _MBB_DEVICE_BINDINGS_INFO *PMBB_DEVICE_BINDINGS_INFO;

typedef 
enum _MBB_REGISTRATION_VOICE_CLASS
    {
        MbbRegistrationVoiceClassVoiceCentric = 0,
        MbbRegistrationVoiceClassDataCentric = 1
    } MBB_REGISTRATION_VOICE_CLASS;

typedef 
enum _MBB_REGISTRATION_VOICE_DOMAIN_PREFERENCE
    {
        MbbRegistrationVoiceDomainPreferenceCsOnly = 0,
        MbbRegistrationVoiceDomainPreferenceCsPreferred = 1,
        MbbRegistrationVoiceDomainPreferenceImsPreferred = 2,
        MbbRegistrationVoiceDomainPreferenceImsOnly = 3
    } MBB_REGISTRATION_VOICE_DOMAIN_PREFERENCE;

typedef 
enum _MBB_REGISTRATION_VOICE_SUPPORT
    {
        MbbRegistrationVoiceSupportNone = 0,
        MbbRegistrationVoiceSupportIms = 0x1,
        MbbRegistrationVoiceSupportEmergencyAttach = 0x2,
        MbbRegistrationVoiceSupportCs = 0x4,
        MbbRegistrationVoiceSupportCsfb = 0x8,
        MbbRegistrationVoiceSupport1xCsfb = 0x10,
        MbbRegistrationVoiceSupportCsEmergency = 0x20
    } MBB_REGISTRATION_VOICE_SUPPORT;

typedef 
enum _MBB_REGISTRATION_CDMA_ROAM_MODE
    {
        MbbRegistrationCdmaRoamModeAutomatic = 0,
        MbbRegistrationCdmaRoamModeHomeOnly = 1
    } MBB_REGISTRATION_CDMA_ROAM_MODE;

typedef struct _MBB_SET_REGISTER_STATE_V2
    {
    MBB_STRING ProviderId;
    MBB_REGISTER_ACTION RegisterAction;
    ULONG DataClass;
    MBB_REGISTRATION_VOICE_CLASS VoiceClass;
    MBB_REGISTRATION_VOICE_DOMAIN_PREFERENCE VoiceDomain;
    MBB_REGISTRATION_CDMA_ROAM_MODE CdmaRoamMode;
    MBB_ARRAY_ELEMENT AcquisitionOrder[ 1 ];
    } MBB_SET_REGISTER_STATE_V2;

typedef struct _MBB_SET_REGISTER_STATE_V2 *PMBB_SET_REGISTER_STATE_V2;

typedef struct _MBB_REGISTRATION_STATE_V2
    {
    ULONG NetworkError;
    MBB_REGISTER_STATE RegisterState;
    MBB_REGISTER_MODE RegisterMode;
    ULONG AvailableDataClasses;
    MBB_CELLULAR_CLASS CurrentCellularClass;
    MBB_STRING ProviderId;
    MBB_STRING ProviderName;
    MBB_STRING RoamingText;
    ULONG RegFlags;
    MBB_REGISTRATION_VOICE_SUPPORT VoiceSupport;
    ULONG CurrentRATClass;
    UCHAR DataBuffer[ 1 ];
    } MBB_REGISTRATION_STATE_V2;

typedef struct _MBB_REGISTRATION_STATE_V2 *PMBB_REGISTRATION_STATE_V2;

typedef struct _MBB_REGISTRATION_STATE_INFOS_V2
    {
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT RegistrationStateList[ 1 ];
    } MBB_REGISTRATION_STATE_INFOS_V2;

typedef struct _MBB_REGISTRATION_STATE_INFOS_V2 *PMBB_REGISTRATION_STATE_INFOS_V2;

typedef struct _MBB_REGISTRATION_STATE_INFO_V2
    {
    ULONG NetworkError;
    MBB_REGISTER_STATE RegisterState;
    MBB_REGISTER_MODE RegisterMode;
    ULONG AvailableDataClasses;
    MBB_CELLULAR_CLASS CurrentCellularClass;
    MBB_STRING ProviderId;
    MBB_STRING ProviderName;
    MBB_STRING RoamingText;
    ULONG RegistrationFlag;
    ULONG PreferredDataClasses;
    UCHAR DataBuffer[ 1 ];
    } MBB_REGISTRATION_STATE_INFO_V2;

typedef struct _MBB_REGISTRATION_STATE_INFO_V2 *PMBB_REGISTRATION_STATE_INFO_V2;

typedef 
enum _MBB_IMS_VOICE_STATUS
    {
        MbbImsVoiceStatusUnknown = 0,
        MbbImsVoiceStatusUnregistered = 1,
        MbbImsVoiceStatusRegistered = 2
    } MBB_IMS_VOICE_STATUS;

typedef struct _MBB_SET_IMS_VOICE_STATE
    {
    MBB_IMS_VOICE_STATUS ImsVoiceStatus;
    } MBB_SET_IMS_VOICE_STATE;

typedef struct _MBB_SET_IMS_VOICE_STATE *PMBB_SET_IMS_VOICE_STATE;

typedef struct _MBB_IMS_VOICE_STATE
    {
    MBB_IMS_VOICE_STATUS ImsVoiceStatus;
    } MBB_IMS_VOICE_STATE;

typedef struct _MBB_IMS_VOICE_STATE *PMBB_IMS_VOICE_STATE;

typedef struct _MBB_SET_SIGNAL_INDICATION_V2
    {
    ULONG RssiInterval;
    ULONG RssiThreshold;
    ULONG ErrorRateThreshold;
    ULONG SnrThreshold;
    } MBB_SET_SIGNAL_INDICATION_V2;

typedef struct _MBB_SET_SIGNAL_INDICATION_V2 *PMBB_SET_SIGNAL_INDICATION_V2;

typedef struct _MBB_SIGNAL_STATE_V2
    {
    ULONG Rssi;
    ULONG ErrorRate;
    ULONG RssiInterval;
    ULONG RssiThreshold;
    ULONG ErrorRateThreshold;
    ULONG Snr;
    ULONG SnrThreshold;
    ULONG DataClass;
    } MBB_SIGNAL_STATE_V2;

typedef struct _MBB_SIGNAL_STATE_V2 *PMBB_SIGNAL_STATE_V2;

typedef struct _MBB_RSRP_SNR_INFO
    {
    ULONG RSRP;
    ULONG SNR;
    ULONG RSRPThreshold;
    ULONG SNRThreshold;
    MBB_DATA_CLASS_VALUE SystemType;
    } MBB_RSRP_SNR_INFO;

typedef struct _MBB_RSRP_SNR_INFO *PMBB_RSRP_SNR_INFO;

typedef struct _MBB_RSRP_SNR
    {
    ULONG ElementCount;
    MBB_RSRP_SNR_INFO DataBuffer[ 1 ];
    } MBB_RSRP_SNR;

typedef struct _MBB_RSRP_SNR *PMBB_RSRP_SNR;

typedef struct _MBB_SIGNAL_STATE_INFO_V2
    {
    ULONG Rssi;
    ULONG ErrorRate;
    ULONG SignalStrengthInterval;
    ULONG RssiThreshold;
    ULONG ErrorRateThreshold;
    ULONG RsrpSnrOffset;
    ULONG RsrpSnrSize;
    MBB_RSRP_SNR DataBuffer;
    } MBB_SIGNAL_STATE_INFO_V2;

typedef struct _MBB_SIGNAL_STATE_INFO_V2 *PMBB_SIGNAL_STATE_INFO_V2;

typedef struct _MBB_SIGNAL_STATE_INFOS_V2
    {
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT SignalStateList[ 1 ];
    } MBB_SIGNAL_STATE_INFOS_V2;

typedef struct _MBB_SIGNAL_STATE_INFOS_V2 *PMBB_SIGNAL_STATE_INFOS_V2;

typedef 
enum _MBB_LOCATION_TRIGGER_MODE
    {
        MbbLocationTriggerNone = 0,
        MbbLocationTriggerAreaCode = 1,
        MbbLocationTriggerAreaCellId = 2
    } MBB_LOCATION_TRIGGER_MODE;

typedef struct _MBB_SET_LOCATION_STATE
    {
    MBB_LOCATION_TRIGGER_MODE Trigger;
    } MBB_SET_LOCATION_STATE;

typedef struct _MBB_SET_LOCATION_STATE *PMBB_SET_LOCATION_STATE;

typedef struct _MBB_LOCATION_STATE_INFO
    {
    MBB_LOCATION_TRIGGER_MODE Trigger;
    ULONG DataClass;
    ULONG AreaCode;
    ULONG CellId;
    } MBB_LOCATION_STATE_INFO;

typedef struct _MBB_LOCATION_STATE_INFO *PMBB_LOCATION_STATE_INFO;

typedef struct _MBB_NITZ_INFO
    {
    ULONG Year;
    ULONG Month;
    ULONG Day;
    ULONG Hour;
    ULONG Minute;
    ULONG Second;
    ULONG TimeZoneOffsetMinutes;
    ULONG DaylightSavingTimeOffsetMinutes;
    ULONG DataClasses;
    } MBB_NITZ_INFO;

typedef struct _MBB_NITZ_INFO *PMBB_NITZ_INFO;

typedef struct _MBB_ATR_INFO
    {
    MBB_ARRAY_ELEMENT2 Atr;
    UCHAR DataBuffer[ 1 ];
    } MBB_ATR_INFO;

typedef struct _MBB_ATR_INFO *PMBB_ATR_INFO;

typedef struct _MBB_SET_UICC_OPEN_CHANNEL
    {
    MBB_ARRAY_ELEMENT2 AppId;
    ULONG SelectP2Arg;
    ULONG ChannelGroup;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_UICC_OPEN_CHANNEL;

typedef struct _MBB_SET_UICC_OPEN_CHANNEL *PMBB_SET_UICC_OPEN_CHANNEL;

typedef struct _MBB_UICC_OPEN_CHANNEL_INFO
    {
    UCHAR Status[ 4 ];
    ULONG Channel;
    MBB_ARRAY_ELEMENT2 Response;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_OPEN_CHANNEL_INFO;

typedef struct _MBB_UICC_OPEN_CHANNEL_INFO *PMBB_UICC_OPEN_CHANNEL_INFO;

typedef struct _MBB_SET_UICC_CLOSE_CHANNEL
    {
    ULONG Channel;
    ULONG ChannelGroup;
    ULONG SelectP2Arg;
    } MBB_SET_UICC_CLOSE_CHANNEL;

typedef struct _MBB_SET_UICC_CLOSE_CHANNEL *PMBB_SET_UICC_CLOSE_CHANNEL;

typedef struct _MBB_UICC_CLOSE_CHANNEL_INFO
    {
    UCHAR Status[ 4 ];
    } MBB_UICC_CLOSE_CHANNEL_INFO;

typedef struct _MBB_UICC_CLOSE_CHANNEL_INFO *PMBB_UICC_CLOSE_CHANNEL_INFO;

typedef 
enum _MBB_UICC_SECURE_MESSAGING
    {
        MbbUiccSecureMessagingNone = 0,
        MbbUiccSecureMessagingNoHdrAuth = 1
    } MBB_UICC_SECURE_MESSAGING;

typedef 
enum _MBB_UICC_CLASS_BYTE_TYPE
    {
        MbbUiccClassByteTypeInterindustry = 0,
        MbbUiccClassByteTypeExtended = 1
    } MBB_UICC_CLASS_BYTE_TYPE;

typedef struct _MBB_SET_UICC_APDU
    {
    ULONG Channel;
    MBB_UICC_SECURE_MESSAGING SecureMessaging;
    MBB_UICC_CLASS_BYTE_TYPE Type;
    MBB_ARRAY_ELEMENT2 Command;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_UICC_APDU;

typedef struct _MBB_SET_UICC_APDU *PMBB_SET_UICC_APDU;

typedef struct _MBB_UICC_APDU_INFO
    {
    UCHAR Status[ 4 ];
    MBB_ARRAY_ELEMENT2 Response;
    UCHAR DataBuffer[ 1 ];
    } MBB_UICC_APDU_INFO;

typedef struct _MBB_UICC_APDU_INFO *PMBB_UICC_APDU_INFO;

typedef struct _MBB_UICC_TERMINAL_CAPABILITY_TLV
    {
    UCHAR Data[ 3 ];
    } MBB_UICC_TERMINAL_CAPABILITY_TLV;

typedef struct _MBB_UICC_TERMINAL_CAPABILITY_TLV *PMBB_UICC_TERMINAL_CAPABILITY_TLV;

typedef struct _MBB_SET_UICC_TERMINAL_CAPABILITY
    {
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT CapabilityList[ 1 ];
    } MBB_SET_UICC_TERMINAL_CAPABILITY;

typedef struct _MBB_SET_UICC_TERMINAL_CAPABILITY *PMBB_SET_UICC_TERMINAL_CAPABILITY;

typedef struct _MBB_UICC_TERMINAL_CAPABILITY_INFO
    {
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT CapabilityList[ 1 ];
    } MBB_UICC_TERMINAL_CAPABILITY_INFO;

typedef struct _MBB_UICC_TERMINAL_CAPABILITY_INFO *PMBB_UICC_TERMINAL_CAPABILITY_INFO;

typedef 
enum _MBB_UICC_PASSTHROUGH_ACTION
    {
        MbbUiccPassThroughDisable = 0,
        MbbUiccPassThroughEnable = 1
    } MBB_UICC_PASSTHROUGH_ACTION;

typedef 
enum _MBB_UICC_PASSTHROUGH_STATUS
    {
        MbbUiccPassThroughDisabled = 0,
        MbbUiccPassThroughEnabled = 1
    } MBB_UICC_PASSTHROUGH_STATUS;

typedef struct _MBB_SET_UICC_RESET
    {
    MBB_UICC_PASSTHROUGH_ACTION PassThroughAction;
    } MBB_SET_UICC_RESET;

typedef struct _MBB_SET_UICC_RESET *PMBB_SET_UICC_RESET;

typedef struct _MBB_UICC_RESET_INFO
    {
    MBB_UICC_PASSTHROUGH_STATUS PassThroughStatus;
    } MBB_UICC_RESET_INFO;

typedef struct _MBB_UICC_RESET_INFO *PMBB_UICC_RESET_INFO;

typedef struct _MBB_SAR_CONFIG_INDICES
    {
    ULONG SarAntennaIndex;
    ULONG SarBackoffIndex;
    } MBB_SAR_CONFIG_INDICES;

typedef struct _MBB_SAR_CONFIG_INDICES *PMBB_SAR_CONFIG_INDICES;

typedef struct _MBB_SAR_CONFIG_INFO
    {
    MBB_SAR_CONTROL_MODE SarMode;
    MBB_SAR_BACKOFF_STATE SarBackoffStatus;
    MBB_SAR_WIFI_HARDWARE_INTEGRATION SarWifiIntegration;
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT Configurations[ 1 ];
    } MBB_SAR_CONFIG_INFO;

typedef struct _MBB_SAR_CONFIG_INFO *PMBB_SAR_CONFIG_INFO;

typedef struct _MBB_SET_SAR_CONFIG
    {
    MBB_SAR_CONTROL_MODE SarMode;
    MBB_SAR_BACKOFF_STATE SarBackoffStatus;
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT Configurations[ 1 ];
    } MBB_SET_SAR_CONFIG;

typedef struct _MBB_SET_SAR_CONFIG *PMBB_SET_SAR_CONFIG;

typedef struct _MBB_SAR_TRANSMISSION_STATUS_INFO
    {
    MBB_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE ChannelNotification;
    MBB_SAR_TRANSMISSION_STATUS TransmissionStatus;
    ULONG HysteresisTimer;
    } MBB_SAR_TRANSMISSION_STATUS_INFO;

typedef struct _MBB_SAR_TRANSMISSION_STATUS_INFO *PMBB_SAR_TRANSMISSION_STATUS_INFO;

typedef struct _MBB_SET_SAR_TRANSMISSION_STATUS
    {
    MBB_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE ChannelNotification;
    ULONG HysteresisTimer;
    } MBB_SET_SAR_TRANSMISSION_STATUS;

typedef struct _MBB_SET_SAR_TRANSMISSION_STATUS *PMBB_SET_SAR_TRANSMISSION_STATUS;

typedef struct _MBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2
    {
    MBB_BASICCONNECTEXT_CONTEXT_OPERATIONS Operation;
    GUID ContextType;
    MBB_CONTEXT_IP_TYPE IPType;
    MBB_BASICCONNECTEXT_CONTEXT_ENABLE Enable;
    MBB_BASICCONNECTEXT_CONTEXT_ROAMING_CONTROL Roaming;
    MBB_BASICCONNECTEXT_CONTEXT_MEDIA_TYPE MediaType;
    MBB_BASICCONNECTEXT_CONTEXT_SOURCE Source;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2;

typedef struct _MBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2 *PMBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2;

typedef struct _MBB_MS_CONTEXT_V2
    {
    ULONG ContextId;
    GUID ContextType;
    MBB_CONTEXT_IP_TYPE IPType;
    MBB_BASICCONNECTEXT_CONTEXT_ENABLE Enable;
    MBB_BASICCONNECTEXT_CONTEXT_ROAMING_CONTROL Roaming;
    MBB_BASICCONNECTEXT_CONTEXT_MEDIA_TYPE MediaType;
    MBB_BASICCONNECTEXT_CONTEXT_SOURCE Source;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_CONTEXT_V2;

typedef struct _MBB_MS_CONTEXT_V2 *PMBB_MS_CONTEXT_V2;

typedef struct _MBB_MS_NETWORK_BLACKLIST_PROVIDER
    {
    ULONG MCC;
    ULONG MNC;
    MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_TYPE NetworkBlacklistType;
    } MBB_MS_NETWORK_BLACKLIST_PROVIDER;

typedef struct _MBB_MS_NETWORK_BLACKLIST_PROVIDER *PMBB_MS_NETWORK_BLACKLIST_PROVIDER;

typedef struct _MBB_MS_NETWORK_BLACKLIST_INFO
    {
    MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_STATE BlacklistState;
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT Contexts[ 1 ];
    } MBB_MS_NETWORK_BLACKLIST_INFO;

typedef struct _MBB_MS_NETWORK_BLACKLIST_INFO *PMBB_MS_NETWORK_BLACKLIST_INFO;

typedef struct _MBB_MS_VERSION_INFO
    {
    USHORT bcdMBIMVersion;
    USHORT bcdMBIMExtendedVersion;
    } MBB_MS_VERSION_INFO;

typedef struct _MBB_MS_VERSION_INFO *PMBB_MS_VERSION_INFO;

typedef struct _MBB_MS_LTE_ATTACH_CONTEXT
    {
    MBB_CONTEXT_IP_TYPE IPType;
    MBB_BASICCONNECTEXT_LTEATTACH_ROAMING_CONTROL Roaming;
    MBB_BASICCONNECTEXT_CONTEXT_SOURCE Source;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_LTE_ATTACH_CONTEXT;

typedef struct _MBB_MS_LTE_ATTACH_CONTEXT *PMBB_MS_LTE_ATTACH_CONTEXT;

typedef struct _MBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG
    {
    MBB_BASICCONNECTEXT_LTEATTACH_CONTEXT_OPERATIONS Operation;
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT Contexts[ 1 ];
    } MBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG;

typedef struct _MBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG *PMBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG;

typedef struct _MBB_MS_LTE_ATTACH_CONFIG_INFO
    {
    ULONG ElementCount;
    MBB_ARRAY_ELEMENT Contexts[ 1 ];
    } MBB_MS_LTE_ATTACH_CONFIG_INFO;

typedef struct _MBB_MS_LTE_ATTACH_CONFIG_INFO *PMBB_MS_LTE_ATTACH_CONFIG_INFO;

typedef struct _MBB_MS_LTE_ATTACH_STATUS
    {
    MBB_BASICCONNECTEXT_LTEATTACH_STATE LteAttachState;
    MBB_CONTEXT_IP_TYPE IPType;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_LTE_ATTACH_STATUS;

typedef struct _MBB_MS_LTE_ATTACH_STATUS *PMBB_MS_LTE_ATTACH_STATUS;

typedef struct _MBB_MS_LTE_ATTACH_STATUS_V2
    {
    MBB_BASICCONNECTEXT_LTEATTACH_STATE LteAttachState;
    ULONG NetworkError;
    MBB_CONTEXT_IP_TYPE IPType;
    MBB_STRING AccessString;
    MBB_STRING UserName;
    MBB_STRING Password;
    MBB_COMPRESSION Compression;
    MBB_AUTH_PROTOCOL AuthProtocol;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_LTE_ATTACH_STATUS_V2;

typedef struct _MBB_MS_LTE_ATTACH_STATUS_V2 *PMBB_MS_LTE_ATTACH_STATUS_V2;

typedef struct _MBB_MS_SYS_CAPS_INFO
    {
    ULONG NumberOfExecutors;
    ULONG NumberOfSlots;
    ULONG Concurrency;
    ULONG64 ModemId;
    } MBB_MS_SYS_CAPS_INFO;

typedef struct _MBB_MS_SYS_CAPS_INFO *PMBB_MS_SYS_CAPS_INFO;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V2
    {
    MBB_DEVICE_TYPE DeviceType;
    MBB_CELLULAR_CLASS CellularClass;
    MBB_VOICE_CLASS VoiceClass;
    MBB_SIM_CLASS SimClass;
    ULONG DataClass;
    ULONG SmsCaps;
    ULONG ControlCaps;
    ULONG dwMaxSessions;
    MBB_STRING CustomDataClass;
    MBB_STRING DeviceIdString;
    MBB_STRING FirmwareInfo;
    MBB_STRING HardwareInfo;
    ULONG ExecutorIndex;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_DEVICE_CAPS_INFO_V2;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V2 *PMBB_MS_DEVICE_CAPS_INFO_V2;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V3_OBS
    {
    MBB_DEVICE_TYPE DeviceType;
    MBB_CELLULAR_CLASS CellularClass;
    MBB_VOICE_CLASS VoiceClass;
    ULONG SimClassBitMasks;
    ULONG DataClass;
    ULONG SmsCaps;
    ULONG ControlCaps;
    ULONGLONG MiscCaps;
    ULONG dwMaxSessions;
    ULONG ExecutorIndex;
    ULONG WcdmaBandClass;
    } MBB_MS_DEVICE_CAPS_INFO_V3_OBS;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V3_OBS *PMBB_MS_DEVICE_CAPS_INFO_V3_OBS;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V3
    {
    MBB_DEVICE_TYPE DeviceType;
    MBB_CELLULAR_CLASS CellularClass;
    MBB_VOICE_CLASS VoiceClass;
    ULONG SimClassBitMasks;
    ULONG DataClass;
    ULONG SmsCaps;
    ULONG ControlCaps;
    ULONGLONG DataSubClass;
    ULONG dwMaxSessions;
    ULONG ExecutorIndex;
    ULONG WcdmaBandClass;
    } MBB_MS_DEVICE_CAPS_INFO_V3;

typedef struct _MBB_MS_DEVICE_CAPS_INFO_V3 *PMBB_MS_DEVICE_CAPS_INFO_V3;

typedef struct _MBB_MS_DEVICE_SLOT_MAPPING_INFO
    {
    ULONG MapCount;
    MBB_ARRAY_ELEMENT SlotMapList[ 1 ];
    } MBB_MS_DEVICE_SLOT_MAPPING_INFO;

typedef struct _MBB_MS_DEVICE_SLOT_MAPPING_INFO *PMBB_MS_DEVICE_SLOT_MAPPING_INFO;

typedef struct _MBB_MS_SLOT_INFO_REQ
    {
    ULONG SlotIndex;
    } MBB_MS_SLOT_INFO_REQ;

typedef struct _MBB_MS_SLOT_INFO_REQ *PMBB_MS_SLOT_INFO_REQ;

typedef struct _MBB_MS_SLOT_INFO
    {
    ULONG SlotIndex;
    MBB_BASICCONNECTEXT_UICCSLOT_STATE State;
    } MBB_MS_SLOT_INFO;

typedef struct _MBB_MS_SLOT_INFO *PMBB_MS_SLOT_INFO;

typedef struct _MBB_MS_PCO_VALUE
    {
    ULONG SessionId;
    ULONG PcoDataSize;
    MBB_PCO_TYPE PcoDataType;
    UCHAR PcoDataBuffer[ 1 ];
    } MBB_MS_PCO_VALUE;

typedef struct _MBB_MS_PCO_VALUE *PMBB_MS_PCO_VALUE;

typedef struct _MBB_MS_BASE_STATIONS_INFO_REQ
    {
    ULONG MaxGSMCount;
    ULONG MaxUMTSCount;
    ULONG MaxTDSCDMACount;
    ULONG MaxLTECount;
    ULONG MaxCDMACount;
    } MBB_MS_BASE_STATIONS_INFO_REQ;

typedef struct _MBB_MS_BASE_STATIONS_INFO_REQ *PMBB_MS_BASE_STATIONS_INFO_REQ;

typedef struct _MBB_MS_BASE_STATIONS_INFO_REQ_V2
    {
    ULONG MaxGSMCount;
    ULONG MaxUMTSCount;
    ULONG MaxTDSCDMACount;
    ULONG MaxLTECount;
    ULONG MaxCDMACount;
    ULONG MaxNRCount;
    } MBB_MS_BASE_STATIONS_INFO_REQ_V2;

typedef struct _MBB_MS_BASE_STATIONS_INFO_REQ_V2 *PMBB_MS_BASE_STATIONS_INFO_REQ_V2;

typedef struct _MBB_MS_BASE_STATIONS_INFO
    {
    MBB_DATA_CLASS_VALUE SystemType;
    MBB_ARRAY_ELEMENT GSMServingCell;
    MBB_ARRAY_ELEMENT UMTSServingCell;
    MBB_ARRAY_ELEMENT TDSCDMAServingCell;
    MBB_ARRAY_ELEMENT LTEServingCell;
    MBB_ARRAY_ELEMENT GSMNmr;
    MBB_ARRAY_ELEMENT UMTSMrl;
    MBB_ARRAY_ELEMENT TDSCDMAMrl;
    MBB_ARRAY_ELEMENT LTEMrl;
    MBB_ARRAY_ELEMENT CDMAMrl;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_BASE_STATIONS_INFO;

typedef struct _MBB_MS_BASE_STATIONS_INFO *PMBB_MS_BASE_STATIONS_INFO;

typedef struct _MBB_MS_BASE_STATIONS_INFO_V2
    {
    MBB_DATA_CLASS_VALUE SystemType;
    MBB_DATA_SUBCLASS_VALUE SystemSubType;
    MBB_ARRAY_ELEMENT GSMServingCell;
    MBB_ARRAY_ELEMENT UMTSServingCell;
    MBB_ARRAY_ELEMENT TDSCDMAServingCell;
    MBB_ARRAY_ELEMENT LTEServingCell;
    MBB_ARRAY_ELEMENT GSMNmr;
    MBB_ARRAY_ELEMENT UMTSMrl;
    MBB_ARRAY_ELEMENT TDSCDMAMrl;
    MBB_ARRAY_ELEMENT LTEMrl;
    MBB_ARRAY_ELEMENT CDMAMrl;
    MBB_ARRAY_ELEMENT NRServingCells;
    MBB_ARRAY_ELEMENT NRNeighborCells;
    UCHAR DataBuffer[ 1 ];
    } MBB_MS_BASE_STATIONS_INFO_V2;

typedef struct _MBB_MS_BASE_STATIONS_INFO_V2 *PMBB_MS_BASE_STATIONS_INFO_V2;

typedef struct _MBB_MODEM_LOGGING_CONFIG
    {
    ULONG Version;
    ULONG MaxSegmentSize;
    ULONG MaxFlushTime;
    MBB_BASICCONNECTEXT_MODEM_LOGGING_LEVEL_CONFIG LevelConfig;
    } MBB_MODEM_LOGGING_CONFIG;

typedef struct _MBB_MODEM_LOGGING_CONFIG *PMBB_MODEM_LOGGING_CONFIG;

typedef struct _MBB_TLV_IE
    {
    MBB_TLV_TYPE Type;
    UCHAR Reserved;
    UCHAR PaddingLength;
    ULONG DataLength;
    } MBB_TLV_IE;

typedef struct _MBB_TLV_IE *PMBB_TLV_IE;

typedef 
enum _MBB_ACCESS_TYPE
    {
        MbbAccessUnknown = 0,
        MbbAccess3GPP = 1,
        MbbAccessNon3GPP = 2,
        MbbAccessMaximum = ( MbbAccessNon3GPP + 1 ) 
    } MBB_ACCESS_TYPE;

typedef struct _MBB_PRE_DFLT_NSSAI_INFO
    {
    MBB_ACCESS_TYPE AccessType;
    MBB_TLV_IE PreferredNSSAI;
    } MBB_PRE_DFLT_NSSAI_INFO;

typedef struct _MBB_PRE_DFLT_NSSAI_INFO *PMBB_PRE_DFLT_NSSAI_INFO;

typedef struct _MBB_SNSSAI_INFO
    {
    UCHAR SnssaiLength;
    UCHAR Sst;
    } MBB_SNSSAI_INFO;

typedef struct _MBB_SNSSAI_INFO *PMBB_SNSSAI_INFO;

typedef struct _MBB_TAI_LIST_SINGLE_PLMN
    {
    MBB_PLMN Plmn;
    UCHAR ElementCount;
    ULONG TacList[ 1 ];
    } MBB_TAI_LIST_SINGLE_PLMN;

typedef struct _MBB_TAI_LIST_SINGLE_PLMN *PMBB_TAI_LIST_SINGLE_PLMN;

typedef struct _MBB_TAI_LIST_MULTI_PLMNS
    {
    UCHAR ElementCount;
    MBB_TAI TaiList[ 1 ];
    } MBB_TAI_LIST_MULTI_PLMNS;

typedef struct _MBB_TAI_LIST_MULTI_PLMNS *PMBB_TAI_LIST_MULTI_PLMNS;

typedef struct _MBB_TAI_LIST_INFO
    {
    UCHAR ListType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_MbbMessages_0000_0000_0002
        {
        /* [case()] */ MBB_TAI_LIST_SINGLE_PLMN SinglePlmnTaiList;
        /* [case()] */ MBB_TAI_LIST_MULTI_PLMNS MultiPlmnsTaiList;
        } u;
    } MBB_TAI_LIST_INFO;

typedef struct _MBB_TAI_LIST_INFO *PMBB_TAI_LIST_INFO;

typedef struct _MBB_DNN
    {
    UCHAR DnnLength;
    } MBB_DNN;

typedef struct _MBB_DNN *PMBB_DNN;

typedef struct _MBB_LADN
    {
    MBB_DNN Dnn;
    } MBB_LADN;

typedef struct _MBB_LADN *PMBB_LADN;

typedef 
enum _MBB_MODEM_CONFIG_STATUS
    {
        ModemConfigStatusUnknown = 0,
        ModemConfigStatusStarted = 1,
        ModemConfigStatusCompleted = 2,
        ModemConfigStatusMaximum = ( ModemConfigStatusCompleted + 1 ) 
    } MBB_MODEM_CONFIG_STATUS;

typedef struct _MBB_MODEM_CONFIG_INFO
    {
    MBB_MODEM_CONFIG_STATUS ConfigStatus;
    MBB_TLV_IE ConfigName;
    } MBB_MODEM_CONFIG_INFO;

typedef struct _MBB_MODEM_CONFIG_INFO *PMBB_MODEM_CONFIG_INFO;

typedef 
enum _MBB_MICO_MODE
    {
        MicoModeDisabled = 0,
        MicoModeEnabled = 1,
        MicoModeUnsupported = 2,
        MBIMMicoModeDefault = 3,
        MicoModeMaximum = ( MBIMMicoModeDefault + 1 ) 
    } MBB_MICO_MODE;

typedef 
enum _MBB_DRX_PARAMS
    {
        DRXNotSpecified = 0,
        MBIMDRXNotSupported = 1,
        DRXCycle32 = 2,
        DRXCycle64 = 3,
        DRXCycle128 = 4,
        DRXCycle256 = 5,
        DRXCycleMaximum = ( DRXCycle256 + 1 ) 
    } MBB_DRX_PARAMS;

typedef 
enum _MBB_DEFAULT_PDU_HINT
    {
        MBIMDefaultPDUSessionActivationUnlikely = 0,
        MBIMDefaultPDUSessionActivationLikely = 1,
        DefaultPDUMaximum = ( MBIMDefaultPDUSessionActivationLikely + 1 ) 
    } MBB_DEFAULT_PDU_HINT;

typedef 
enum _MBB_MS_LADN_IND
    {
        LADNInfoNotNeeded = 0,
        LADNInfoRequested = 1,
        LADNInfoMaximum = ( LADNInfoRequested + 1 ) 
    } MBB_MS_LADN_IND;

typedef struct _MBB_REGISTRATION_PARAMS_INFO
    {
    MBB_MICO_MODE MicoMode;
    MBB_DRX_PARAMS DRXParams;
    MBB_MS_LADN_IND LADNInfo;
    MBB_DEFAULT_PDU_HINT DefaultPDUHint;
    ULONG ReRegisterIfNeeded;
    } MBB_REGISTRATION_PARAMS_INFO;

typedef struct _MBB_REGISTRATION_PARAMS_INFO *PMBB_REGISTRATION_PARAMS_INFO;

typedef 
enum _MBB_MICO_IND
    {
        RaaiTypeRaNotAllocated = 0,
        RaaiTypeRaAllocated = 1,
        RaaiTypeNotAvailable = 0xffffffff
    } MBB_MICO_IND;

typedef struct _MBB_NW_PARAMS_QUERY_INFO
    {
    USHORT AreConfigurationsNeeded;
    USHORT AreUEPoliciesNeeded;
    } MBB_NW_PARAMS_QUERY_INFO;

typedef struct _MBB_NW_PARAMS_QUERY_INFO *PMBB_NW_PARAMS_QUERY_INFO;

typedef struct _MBB_NW_PARAMS_INFO
    {
    MBB_MICO_IND MicoInd;
    MBB_DRX_PARAMS DRXParams;
    } MBB_NW_PARAMS_INFO;

typedef struct _MBB_NW_PARAMS_INFO *PMBB_NW_PARAMS_INFO;

typedef 
enum _MBB_WAKE_TYPE
    {
        WakeTypeCIDResponse = 0,
        WakeTypeCIDIndication = 1,
        WakeTypePacket = 2
    } MBB_WAKE_TYPE;

typedef struct _MBB_WAKE_REASON
    {
    MBB_WAKE_TYPE WakeType;
    ULONG SessionId;
    } MBB_WAKE_REASON;

typedef struct _MBB_WAKE_REASON *PMBB_WAKE_REASON;

typedef struct _MBB_WAKE_COMMAND
    {
    MBB_COMMAND Command;
    ULONG PayloadOffset;
    ULONG PayloadSize;
    UCHAR DataBuffer[ 1 ];
    } MBB_WAKE_COMMAND;

typedef struct _MBB_WAKE_COMMAND *PMBB_WAKE_COMMAND;

typedef struct _MBB_WAKE_PACKET
    {
    ULONG FilterId;
    ULONG OriginalPacketSize;
    ULONG SavedPacketOffset;
    ULONG SavedPacketSize;
    UCHAR DataBuffer[ 1 ];
    } MBB_WAKE_PACKET;

typedef struct _MBB_WAKE_PACKET *PMBB_WAKE_PACKET;

#include <packoff.h>


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



