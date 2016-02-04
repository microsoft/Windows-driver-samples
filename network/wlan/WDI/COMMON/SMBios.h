#ifndef __INC_SMBIOS_H
#define __INC_SMBIOS_H

#if (SMBIOS_SUPPORT == 1)

#define SMBIOSBUFSize	0x1000

typedef enum _SMBIOS_TYPE{
	SMBIOS_BIOS_INFORMATION					= 0,
	SMBIOS_SYSTEM_INFORMATION				= 1,
	SMBIOS_BASE_BOARD_INFORMATION			= 2,
	SMBIOS_SYSTEM_ENCLOSURE_OR_CHASSIS		= 3,
	SMBIOS_PROCESSOR_INFORMATION			= 4,
	SMBIOS_MEMORY_CONTROLLER_INFORMATION	= 5,
	SMBIOS_MEMORY_MODULE_INFORMATION		= 6,
	SMBIOS_CACHE_INFORMATION				= 7,
	SMBIOS_PORT_CONNECTOR_INFORMATION		= 8,
	SMBIOS_SYSTEM_SLOTS						= 9,
	SMBIOS_ON_BOARD_DEVICES_INFORMATION		= 10,
	SMBIOS_OEM_STRINGS						= 11,
	SMBIOS_MUTUAL_AUTHENTICATION			= 133,
	SMBIOS_MAX								= 127,
	SMBIOS_SKU								= 248
}SMBIOS_TYPE;

typedef enum _SMBIOS_STATUS{
	SMBIOS_STATUS_SUCCESS			= 0,
	SMBIOS_STATUS_WMIOpenBlockFail	= 1,
	SMBIOS_STATUS_AllocateMemFail	= 2,
	SMBIOS_STATUS_WMIQueryDataFail	= 3,
	SMBIOS_STATUS_MutualAuthSuccess	= 4,
	SMBIOS_STATUS_MutualAuthFail	= 5,
	SMBIOS_STATUS_FAILURE			= 6
}SMBIOS_STATUS;

typedef enum _UEFI_STATUS{
	UEFI_STATUS_SUCCESS				= 0,
	UEFI_STATUS_AllocateMemFail		= 1,
	UEFI_STATUS_GetUEFIFail			= 2,
	UEFI_STATUS_ProfileNotDefined	= 3,
	UEFI_STATUS_ProfileUnknown		= 4,
	UEFI_STATUS_UnsupportedOS		= 5,
	UEFI_STATUS_FAILURE				= 6
}UEFI_STATUS;

typedef enum _UEFI_PROFILE{
	UEFI_PROFILE_UNDEFINED			= 0,
	UEFI_PROFILE_LENOVO_CHINA		= 1,
	UEFI_PROFILE_MAX				= 100
}UEFI_PROFILE;

typedef struct _SMBIOS_DATA{
	u4Byte			country;
	u1Byte 			SAR_Power;

	u8Byte			ProcessorID;

	BOOLEAN			bAsusTX201;
	BOOLEAN 		bWABTSeries;
	BOOLEAN 		bB550;
	
#if(MUTUAL_AUTHENTICATION == 1)
	SMBIOS_STATUS	MutualAuth;
	BOOLEAN 		MutualAuthEnable;
	BOOLEAN 		MutualAuthFail;
#endif
}SMBIOS_DATA, *PSMBIOS_DATA;

#if(MUTUAL_AUTHENTICATION == 1)
#define SMBIOS_SET_MA_ENABLE(_Adapter, _Enable)		(GET_HAL_DATA(_Adapter)->SmbiosData.MutualAuthEnable = _Enable)
#define SMBIOS_GET_MA_STATUS(_Adapter)				(GET_HAL_DATA(_Adapter)->SmbiosData.MutualAuthFail)
#endif
#define SMBIOS_GET_COUNTRY_CODE(_Adapter)			(GET_HAL_DATA(_Adapter)->SmbiosData.country)
#define SMBIOS_GET_SAR_TABLE(_Adapter)				(GET_HAL_DATA(_Adapter)->SmbiosData.SAR_Power)

#define SMBIOS_GET_PROCESSOR_ID(_Adapter)			(GET_HAL_DATA(_Adapter)->SmbiosData.ProcessorID)

#define SMBIOS_GET_IS_B550(_Adapter)				(GET_HAL_DATA(_Adapter)->SmbiosData.bB550)

VOID
SmbiosCheckSystemInfo(
	IN	PADAPTER	Adapter
	);

VOID
UEFICheckSystemInfo(
	IN	PADAPTER	Adapter
	);

#if(MUTUAL_AUTHENTICATION == 1)
VOID
SmbiosGetMutualAuth(
	IN	PADAPTER	Adapter,
	IN	pu1Byte 	pSMBIOSData,
	IN	ULONG		RemainLen
	);
VOID
MutualAuthWorkItemCallback( 
    IN PVOID      pContext
	);
VOID
SmbiosCheckMutualAuth(
	IN	PADAPTER	Adapter
	);
#else
#define SmbiosGetMutualAuth(_Adapter, _pSMBIOSData, _RemainLen)
#define SmbiosCheckMutualAuth(_Adapter)
#endif

#else
#define SMBIOS_DATA		u1Byte

#define SMBIOS_GET_MA_STATUS(_Adapter)		FALSE
#define SMBIOS_GET_COUNTRY_CODE(_Adapter)	0
#define SMBIOS_GET_SAR_TABLE(_Adapter)		0

#define SMBIOS_GET_PROCESSOR_ID(_Adapter)	0
#define SMBIOS_GET_IS_B550(_Adapter)		0

#define UEFICheckSystemInfo(_Adapter)
#define SmbiosCheckSystemInfo(_Adapter)
#define SmbiosGetMutualAuth(_Adapter, _pSMBIOSData, _RemainLen)
#define MutualAuthWorkItemCallback(_pContext)	NULL
#define SmbiosCheckMutualAuth(_Adapter)

#endif

#endif
