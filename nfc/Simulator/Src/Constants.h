/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Constants.h

Abstract:

    This header contains definitions common for NFC drivers

--*/

#pragma once

//
// Message Types
//
#define WINDOWS_PROTOCOL            L"Windows"
#define WINDOWS_PROTOCOL_CHARS      (ARRAYSIZE(WINDOWS_PROTOCOL) - 1)

#define WINDOWSURI_TYPE             L"Uri"
#define WINDOWSURI_TYPE_CHARS       (ARRAYSIZE(WINDOWSURI_TYPE) - 1)

#define WINDOWSURI_PROTOCOL         WINDOWS_PROTOCOL WINDOWSURI_TYPE
#define WINDOWSURI_PROTOCOL_CHARS   (ARRAYSIZE(WINDOWSURI_PROTOCOL) - 1)

#define WINDOWSMIME_TYPE            L"Mime"
#define WINDOWSMIME_TYPE_CHARS      (ARRAYSIZE(WINDOWSMIME_TYPE) - 1)

#define WINDOWSMIME_PROTOCOL        WINDOWS_PROTOCOL WINDOWSMIME_TYPE
#define WINDOWSMIME_PROTOCOL_CHARS  (ARRAYSIZE(WINDOWSMIME_PROTOCOL) - 1)

#define NDEF_PROTOCOL               L"NDEF"  
#define NDEF_PROTOCOL_CHARS         (ARRAYSIZE(NDEF_PROTOCOL) - 1)  
  
#define NDEF_UNKNOWN_TYPE           L"Unknown"  
#define NDEF_UNKNOWN_TYPE_CHARS     (ARRAYSIZE(NDEF_UNKNOWN_TYPE) - 1)  

#define NDEF_EMPTY_TYPE             L"Empty"
#define NDEF_EMPTY_TYPE_CHARS       (ARRAYSIZE(NDEF_EMPTY_TYPE) - 1)
  
#define NDEF_EXT_TYPE               L"ext."  
#define NDEF_EXT_TYPE_CHARS         (ARRAYSIZE(NDEF_EXT_TYPE) - 1)  
  
#define NDEF_MIME_TYPE              L"MIME."  
#define NDEF_MIME_TYPE_CHARS        (ARRAYSIZE(NDEF_MIME_TYPE) - 1)  
  
#define NDEF_URI_TYPE               L"URI."  
#define NDEF_URI_TYPE_CHARS         (ARRAYSIZE(NDEF_URI_TYPE) - 1)  
  
#define NDEF_WKT_TYPE               L"wkt."  
#define NDEF_WKT_TYPE_CHARS         (ARRAYSIZE(NDEF_WKT_TYPE) - 1)

#define WRITETAG_TYPE               L"WriteTag"  
#define WRITETAG_TYPE_CHARS         (ARRAYSIZE(WRITETAG_TYPE) - 1)
  
#define DEVICE_ARRIVED              L"DeviceArrived"
#define DEVICE_DEPARTED             L"DeviceDeparted"
#define WRITEABLETAG_PROTOCOL       L"WriteableTag"
#define PAIRING_BLUETOOTH_PROTOCOL  L"Pairing:Bluetooth"
#define PAIRING_UPNP_PROTOCOL       L"Pairing:UPnP"
#define LAUNCHAPP_WRITETAG_PROTOCOL L"LaunchApp:WriteTag"

//
// Namespaces
//
#define NFP_NAMESPACE               L"Nfp"
#define NFP_NAMESPACE_CHARS         (ARRAYSIZE(NFP_NAMESPACE) - 1)

#define PUBS_NAMESPACE              L"Pubs\\"
#define PUBS_NAMESPACE_CHARS        (ARRAYSIZE(PUBS_NAMESPACE) - 1)

#define SUBS_NAMESPACE              L"Subs\\"
#define SUBS_NAMESPACE_CHARS        (ARRAYSIZE(SUBS_NAMESPACE) - 1)

#define SE_NAMESPACE                L"SE"
#define SE_NAMESPACE_CHARS          (ARRAYSIZE(SE_NAMESPACE) - 1)

#define SEMANAGE_NAMESPACE          L"SEManage"
#define SEMANAGE_NAMESPACE_CHARS    (ARRAYSIZE(SEMANAGE_NAMESPACE) - 1)

#define SEEVENTS_NAMESPACE          L"SEEvents"
#define SEEVENTS_NAMESPACE_CHARS    (ARRAYSIZE(SEEVENTS_NAMESPACE) - 1)

#define SMARTCARD_READER_NAMESPACE  L"SCReader"
#define SMARTCARD_READER_NAMESPACE_CHARS (ARRAYSIZE(SMARTCARD_READER_NAMESPACE) - 1)

#define RM_NAMESPACE                L"RadioManage"
#define RM_NAMESPACE_CHARS          (ARRAYSIZE(RM_NAMESPACE) - 1)

#define MAX_MESSAGE_QUEUE_SIZE      50

//
// Macros
//
#define IsStringPrefixed(_STRING_, _PREFIX_) \
    (CompareStringOrdinal(_STRING_, (_PREFIX_ ## _CHARS), _PREFIX_, (_PREFIX_ ## _CHARS), TRUE) == CSTR_EQUAL)
  
#define IsStringEqual(_STRING1_, _STRING2_) \
    (CompareStringOrdinal(_STRING1_, -1, _STRING2_, -1, TRUE) == CSTR_EQUAL)