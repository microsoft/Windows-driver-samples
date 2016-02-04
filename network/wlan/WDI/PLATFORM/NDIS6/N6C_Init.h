////////////////////////////////////////////////////////////////////////////////
//
//	File name:		N6C_Init.h
//	Description:	NDIS6.x common function for initialization.
//
//	Author:			shien chang
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_N6C_REGISTRY_H
#define __INC_N6C_REGISTRY_H

#define  NUM_SUPPORTED_VWIFI_COMBINATIONS   1
#define ADDR2DATA ULONG_PTR

#define	NDIS_REG_NET_CFG_INSTANCE_ID_NAME	"NetCfgInstanceId"

typedef struct _MP_REG_ENTRY
{
	NDIS_STRING		RegName;	// variable name text
	BOOLEAN			bRequired;	// 1 -> required, 0 -> optional
	UCHAR			Type;		// NdisParameterInteger/NdisParameterHexInteger/NdisParameterStringle/NdisParameterMultiString
	UINT			FieldOffset;	// offset to MP_ADAPTER field
	UINT			FieldSize;	// size (in bytes) of the field
	ULONG_PTR			Default;
	UINT			Min;			// minimum value allowed
	UINT			Max;		// maximum value allowed
} MP_REG_ENTRY, *PMP_REG_ENTRY;

// For 818x UI, 2004.11.30, by rcnjko.
typedef enum	_REG_WEP_STATUS
{
	REG_WEP_STATUS_MIN,		// 0
	REG_WEP_STATUS_NO_WEP,	// 1
	REG_WEP_STATUS_WEP64,	// 2
	REG_WEP_STATUS_WEP128	// 3
}REG_WEP_STATUS;

// For 818x UI, 2004.11.30, by rcnjko.
typedef enum REG_EncryptionAlgorithm
{
	REG_WEP_Encryption,		// 0	
	REG_TKIP_Encryption,	// 1
	REG_TKIPv2_Encryption,	// 2
	REG_AESWRAP_Encryption,	// 3
	REG_AESCCMP_Encryption,	// 4
	REG_CKIP_Encryption,	// 5
	REG_NONE_Encryption,	// 6
	REG_WAPI_PSK,
	REG_WAPI_CERT

}REG_EncryptionAlgorithm;

// For 818x UI, 2004.11.30, by rcnjko.
typedef	enum	_REG_NETWORK_INFRA
{
	NI_ADHOC,
	NI_Infrastructure,
	NI_AUTO
}REG_NETWORK_INFRA;

// Forward declaration.
typedef	struct _ADAPTER	ADAPTER, *PADAPTER;

NDIS_STATUS
CopyFromUnicodeToOS( 
	UCHAR			*pointer,
	UNICODE_STRING	*uniStr,
	USHORT			copyLen);

NDIS_STATUS
CopyFromUnicodeToString( 
	UCHAR			*pointer,
	UNICODE_STRING	*uniStr,
	USHORT			copyLen);

int 
TranslateNdisPsToRtPs(
	IN	NDIS_802_11_POWER_MODE	ndisPsMode);

NDIS_STATUS
N6OpenConfigurationHandle(
	IN	PADAPTER		pAdapter,
	IN	PNDIS_HANDLE	pConfigurationHandle
	);

VOID
N6CloseConfigurationHandle(
	IN	NDIS_HANDLE		ConfigurationHandle
	);

NDIS_STATUS
N6ReadCommonRegistry(
	IN	PADAPTER		pAdapter
	);

VOID
N6UpdateDefaultSetting(
	IN	PADAPTER		pAdapter);


VOID 
N6RestoreLastInitSetting(
	IN	PADAPTER		pAdapter);

//
//Name : N6RestoreLastInitSettingAterWakeUP
//Function : Reconnent to AH when wake up
//
VOID
N6RestoreLastInitSettingAterWakeUP(
	IN	PADAPTER		pAdapter	);

VOID
InitNdis6CommonResources(
	IN	PADAPTER		pAdapter);

VOID
ReleaseNdis6CommonResources(
	IN	PADAPTER		pAdapter);

VOID
N6IndicateStatus(
	IN	PADAPTER		pAdapter,
	IN	NDIS_STATUS		GeneralStatus,
	IN	PVOID			StatusBuffer,
	IN	UINT			StatusBufferSize);

VOID
N6CInitializeSpinLocks(
	IN	PADAPTER		pAdapter);

VOID
N6CFreeSpinLocks(
	IN	PADAPTER		pAdapter);

#endif
