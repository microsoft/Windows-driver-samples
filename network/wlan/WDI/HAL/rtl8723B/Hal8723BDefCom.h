#ifndef __REALTEK_HAL8723BDEFCOM_H__
#define __REALTEK_HAL8723BDEFCOM_H__


typedef enum _RTL8723B_SRAM_DATA 
{
	LOC_8723B_SRFF,
	LOC_8723B_TXRPT,	
	LOC_8723B_BCN_RPT, 
	LOC_8723B_AMPDU_BU_RST_CTRL,	
	LOC_8723B_RA_MASK 
}RTL8723B_SRAM_DATA;

u1Byte
GetEEPROMSize8723B(
	IN	PADAPTER	Adapter
	);

VERSION_8192C
ReadChipVersion8723B(
	IN	PADAPTER	Adapter
	);

VOID
ReadSdramData_8723B(
	IN	PADAPTER			Adapter,
	IN	u1Byte				macId,
	IN	RTL8723B_SRAM_DATA	Type,
	OUT	pu4Byte				data,
	IN	u1Byte				dataLen
	);


VOID
Hal_ReadPROMContent_BT_8723B(
	IN PADAPTER 	Adapter
	);

VOID
HalSetRegMACID0AID_8723B(
	IN	PADAPTER	Adapter
	);

VOID
SetBcnCtrlReg_8723B(
	IN	PADAPTER	Adapter,
	IN	u1Byte		SetBits,
	IN	u1Byte		ClearBits
	);



//2 WoWLAN

VOID
SetWoWLANCAMEntry8723B(
	PADAPTER			Adapter
);

RT_STATUS
HalSetFWWoWlanMode8723B(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bFuncEn
	);

VOID
GetPortsCommonBasicRate_8723B(
	IN PADAPTER	Adapter,
	OUT pu1Byte		SupportRate,
	OUT pu2Byte		SupportRateLen
);

u1Byte
SelectRTSInitialRate8723B(
	IN	PADAPTER	Adapter
);


VOID
HalSetBrateCfg_8723B(
	IN	PADAPTER			Adapter,
	IN	OCTET_STRING		*mBratesOS,
	OUT	pu2Byte				pBrateCfg
);

VOID
HalUpdateDefaultSetting8723B(
	IN	PADAPTER	pAdapter
	);

VOID
UpdateHalRAMask8723B(
	IN	PADAPTER			Adapter,
	IN	u1Byte				macId,
	IN	PRT_WLAN_STA		pEntry,
	IN	u1Byte				rssi_level
	);

VOID
ActUpdateChannelAccessSetting_8723B(
	PADAPTER					Adapter,
	WIRELESS_MODE				WirelessMode,
	PCHANNEL_ACCESS_SETTING	ChnlAccessSetting
	);


#if RX_AGGREGATION
VOID
RxAggrSettingWorkItemCallback_8723B(
	IN	PVOID pContext
	);
#endif

VOID
Dbg_IQK_TriggerWorkItemCallback_8723B(
	IN PVOID    pContext
);
VOID
Dbg_LCK_TriggerWorkItemCallback_8723B(
	IN PVOID    pContext
);


u4Byte
H2CCmdAction8723B(
	IN	PADAPTER		Adapter,
	IN	PVOID		pBuf,
	IN	u2Byte		bufLen
	);

VOID
SetFwRelatedForWoWLAN8723B(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN			bUsedWoWLANFw
);

u1Byte
GetFwPsStateDef8723B(
	IN	PADAPTER	Adapter,
	IN	u1Byte		FwPsState
);

BOOLEAN
CheckCPWMInterrupt8723B(
	IN	PADAPTER	Adapter
);

u1Byte
GetTxBufferRsvdPageNum8723B(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bWoWLANBoundary
);

VOID
HalSetFwKeepAliveCmd8723B(	
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN	bFuncEn
  );

RT_STATUS
InitLLTTable8723B(
	IN  PADAPTER	Adapter,
	IN	u4Byte		boundary
	);

VOID
SetPowerDownReg_8723B(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bFuncEnable
	);

RT_STATUS
HalDynamicRQPN8723B(
	IN		PADAPTER	Adapter,
	IN		u4Byte		boundary,
	IN		u2Byte		NPQ_RQPNValue,
	IN		u4Byte		RQPNValue
	);

BOOLEAN
IsHwAutoRFOff8723B(
	IN		PADAPTER	Adapter
	);

VOID
ForceLeaveHwClock32K8723B(
	IN	PADAPTER			Adapter
	);

RT_STATUS
RecoverTxBoundarySetting8723B(
	IN PADAPTER			Adapter,
	IN BOOLEAN			bWoWLANSetting
	);

VOID
HalGetAOACReport8723B(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				AOACRptBuf
	);

VOID
HalSetFcsAdjustTsf_8723B(
	IN	PADAPTER			Adapter
	);

VOID
ex_hal8723b_wifi_only_hw_config(
	IN	PADAPTER	Adapter
	);

#endif
