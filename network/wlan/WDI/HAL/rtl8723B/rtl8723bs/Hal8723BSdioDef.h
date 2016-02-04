#ifndef __INC_HAL8723BSDIODEF_H
#define __INC_HAL8723BSDIODEF_H


/*--------------------------Define MACRO--------------------------------------*/

extern u1Byte	DescString8723BSdio[];
extern EEPROM_OFFSET EEPROMOffset8723BSdio;

extern u1Byte	DescString8192ESdio[];
extern EEPROM_OFFSET EEPROMOffset8192ESdio;

#define DRIVER_EARLY_INT_TIME_8723B			0x05

#define	HAL92CSDIO_DEFAULT_BEACON_TYPE	BEACON_SEND_AUTO_HW

#define SDIO_8723B_DUMMY_UNIT				8
#define SDIO_8723B_DUMMY_OFFSET			1
#define SDIO_8723B_ALL_DUMMY_LENGTH		SDIO_8723B_DUMMY_OFFSET * SDIO_8723B_DUMMY_UNIT
#define HWDESC_HEADER_LEN_SDIO_8812		40
#define SDIO_8723B_HWDESC_HEADER_LEN		(HWDESC_HEADER_LEN_SDIO_8812 + SDIO_8723B_ALL_DUMMY_LENGTH)	// 8byte dummy

#define MAX_RX_DMA_BUFFER_SIZE_8812		0x3E80
#define MAX_RX_DMA_BUFFER_SIZE_8723B		0x2800	// RX 10K

#define TX_TOTAL_PAGE_NUMBER_8723B		0xF3 // TX 32K

#define TX_PAGE_BOUNDARY_8723B			(TX_TOTAL_PAGE_NUMBER_8723B + 1)
#define TX_PAGE_BOUNDARY_WOWLAN_8723B		0xE0

// For Normal Chip Setting
// (HPQ + LPQ + NPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER
#define NORMAL_PAGE_NUM_PUBQ_8723B			0xC2 //0xD9
#define NORMAL_PAGE_NUM_LPQ_8723B			0x08 //0x0d
#define NORMAL_PAGE_NUM_HPQ_8723B			0x08 //0x0d
#define NORMAL_PAGE_NUM_NPQ_8723B			0x20 //0x00


//Note: For WMM Normal Chip Setting ,modify later
#define WMM_NORMAL_TX_TOTAL_PAGE_NUMBER_8723B	TX_PAGE_BOUNDARY_8723B
#define WMM_NORMAL_TX_PAGE_BOUNDARY_8723B		(WMM_NORMAL_TX_TOTAL_PAGE_NUMBER_8723B + 1)

#define WMM_NORMAL_PAGE_NUM_PUBQ_8723B		NORMAL_PAGE_NUM_PUBQ_8723B
#define WMM_NORMAL_PAGE_NUM_HPQ_8723B		NORMAL_PAGE_NUM_HPQ_8723B
#define WMM_NORMAL_PAGE_NUM_LPQ_8723B		NORMAL_PAGE_NUM_LPQ_8723B
#define WMM_NORMAL_PAGE_NUM_NPQ_8723B		NORMAL_PAGE_NUM_NPQ_8723B

// RQPN setting for WoWLAN
#define WOWLAN_NORMAL_PAGE_NUM_NPQ_8723B	0x03
#define WOWLAN_RSVD_QUEUE_PAGE_NUM_8723B	0x80c20d0d

#define TX_SELE_HQ					BIT(0)		// High Queue
#define TX_SELE_LQ					BIT(1)		// Low Queue
#define TX_SELE_NQ					BIT(2)		// Normal Queue

#define RX_PAGE_SIZE_REG_VALUE		PBP_128
#define BCN_DMA_ATIME_INT_TIME		0x02


#endif // #ifndef __INC_HAL8723BSDIODEF_H

VOID
DumpHardwareProfile8723BSdio(
	IN	PADAPTER		Adapter
	);

VOID
DisableInterrupt8723BSdio (
	IN PADAPTER			Adapter
	);

VOID
ClearInterrupt8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
ClearSysInterrupt8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
InitInterrupt8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
InitSysInterrupt8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
EnableInterrupt8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
DumpLoggedInterruptHistory8723BSdio(
	PADAPTER		Adapter
	);

BOOLEAN
InterruptRecognized8723BSdio(
	IN	PADAPTER			Adapter,
	IN	PVOID				pContent,
	IN	u4Byte				ContentLen
	);

//1======================================================
// Tx Related
// Function prototypes for Hal8190PciGen.c
//1======================================================

VOID
InitializeVariables8723BSdio(
	IN	PADAPTER			Adapter
	);

VOID
DeInitializeVariables8723BSdio(
	IN PADAPTER			Adapter
	);

VOID
CancelAllTimer8723BSdio(
	IN	PADAPTER			Adapter
	);

VOID
ReleaseAllTimer8723BSdio(
	PADAPTER			Adapter
	);

VOID
HalSdioUpdateDefSet8723BS(
	IN	PADAPTER		pAdapter
	);

BOOLEAN
HalSdioSetQueueMapping8723BSdio(
	IN	PADAPTER	pAdapter,
	IN	u1Byte		NumIn,
	IN	u1Byte		NumOut
	);

VOID
GetInitialRQPValue8723BS(
	IN  PADAPTER	Adapter,
	OUT pu2Byte	pNPQRQPNVaule,
	OUT pu4Byte	pRQPNValue
	);

u1Byte
GetInitialTxBufferBoundary8723BS(
	IN  PADAPTER Adapter
	);

RT_STATUS	
InitializeAdapter8723BSdio(
	IN PADAPTER			Adapter,
	IN u1Byte			Channel
	);

RT_STATUS
ReadAdapterInfo8723BS(
	IN PADAPTER			Adapter
	);

VOID
FillH2CCmd8723BSdioWorkItemCallback(
	IN	PVOID            pContext
	);

VOID
FillH2CCmd8723BSdioTimerCallback(
	IN PRT_TIMER		pTimer
	);

VOID
HaltAdapter8723BSdio(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bReset
	);

VOID
SleepAdapter8723BSdio(
	IN	PADAPTER			Adapter
	);

VOID
ShutdownAdapter8723BSdio(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bReset
	);

VOID
SetHwReg8723BSdio(
	IN PADAPTER				pAdapter,
	IN u1Byte				variable,
	IN pu1Byte				val
	);

VOID
GetHwReg8723BSdio(
	IN PADAPTER				Adapter,
	IN u1Byte				variable,
	OUT pu1Byte				val
	);

BOOLEAN
GetHalDefVar8723BSdio(
	IN	PADAPTER				Adapter,
	IN	HAL_DEF_VARIABLE		eVariable,
	IN	PVOID					pValue
	);

BOOLEAN
GetInterrupt8723BSdio(
	IN	PADAPTER		pAdapter,
	IN	HAL_INT_TYPE	intType
	);

BOOLEAN
SetHalDefVar8723BSdio(
	IN	PADAPTER				Adapter,
	IN	HAL_DEF_VARIABLE		eVariable,
	IN	PVOID					pValue
	);

VOID
ActSetWirelessMode8723BSdio(
	PADAPTER			pAdapter,
	u2Byte				btWirelessMode
	);

VOID
SetBeaconRelatedRegisters8723BSdio(
	IN	PADAPTER		Adapter,
	IN	PVOID			pOpMode,
	IN	u2Byte			BcnInterval,
	IN 	u2Byte			AtimWindow
	);

VOID
ResetAllHWTimer8723BSdio(
	IN	PADAPTER		Adapter
	);

VOID
UpdateLPSStatus8723BSdio(
	IN 	PADAPTER			Adapter,
	IN	u1Byte				RegLeisurePsMode,
	IN	u1Byte				RegPowerSaveMode
	);

VOID
UpdateIPSStatus8723BSdio(
	IN 	PADAPTER			Adapter,
	IN	u1Byte				RegInactivePsMode
	);

VOID
EnableHWSecurityConfig8723BSdio(
	IN	PADAPTER		Adapter
	);

VOID
DisableHWSecurityConfig8723BSdio(
	IN	PADAPTER		Adapter
	);

VOID
SetKey8723BSdio(
	IN	PADAPTER		Adapter,
	IN	u4Byte			KeyIndex,
	IN	pu1Byte			pMacAddr,
	IN	BOOLEAN			IsGroup,
	IN	u4Byte			EncAlgo,
	IN	BOOLEAN			IsWEPKey, //if OID = OID_802_11_WEP
	IN	BOOLEAN			ClearAll
	);

VOID
TxFillCmdDesc8723BSdio(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb,
	IN	u1Byte			QueueIndex,
	IN	u2Byte			index,
	IN	BOOLEAN			bFirstSeg,
	IN	BOOLEAN			bLastSeg,
	IN	pu1Byte			VirtualAddress,
	IN	u4Byte			PhyAddressLow,
	IN	u4Byte			BufferLen,
	IN    u4Byte			DescPacketType,
	IN	u4Byte			PktLen
	);

VOID
TxFillDescriptor8723BSdio(
	IN	PADAPTER	Adapter,
	IN	PRT_TCB		pTcb,
	IN	u2Byte		nBufIndex,
	IN	u2Byte		nFragIndex,
	IN	u2Byte		nFragBufferIndex,
	IN	u2Byte		nCurDesc
	);

VOID
AllowAllDestAddr8723BSdio(
	IN	PADAPTER				Adapter,
	IN	BOOLEAN					bAllowAllDA,
	IN	BOOLEAN					WriteIntoReg
	);

VOID
AllowErrorPacket8723BSdio(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bAllowErrPkt,
	IN	BOOLEAN				WriteIntoReg
	);

VOID
QueryRxDescStatus8723BSdio(
	IN		PADAPTER				Adapter,
	IN		PVOID					pDescIn,
	IN OUT	PRT_RFD					pRfd
	);

u4Byte
GetRxPacketShiftBytes8723BSdio(
	IN	PRT_RFD		pRfd
	);

BOOLEAN
HalQueryRxDMARequest8723BSdio(
	IN 	PADAPTER 		Adapter,
	OUT pu2Byte		pRxReqLength	
	);

BOOLEAN
GetNmodeSupportBySecCfg8723BSdio(
	IN	PADAPTER		Adapter
	);

VOID
GPIOCheckPBC8723BSdio(
	IN	PADAPTER	pAdapter
	);

VOID
GpioDetectTimerStart8723BSdio(
	IN	PADAPTER	Adapter
);

VOID
SetRTSRateWorkItemCallback_8723BS(
	IN PVOID			pContext
	);

VOID
HalSdioGetCmdAddr8723BSdio(
	IN	PADAPTER			pAdapter,
	IN 	u1Byte				DeviceID,
	IN	u4Byte				Addr,
	OUT	pu4Byte				pCmdAddr
	);

VOID
HalRxAggr8723BSdio(
	IN PADAPTER 	Adapter,
	IN BOOLEAN	Value
	);

BOOLEAN
HalQueryTxBufferStatus8723BSdio(
	IN	PADAPTER	Adapter	
	);

BOOLEAN
HalQueryTxBufferAvailable8723BSdio(
	IN	PADAPTER	Adapter,
	IN	PSDIO_OUT_CONTEXT pContext	
	);

BOOLEAN
HalSdioRxHandleInterrupt8723BSdio(
	IN 	PADAPTER 		pAdapter
	);

VOID
HalDropRxFIFO8723BSdio(
	IN 	PADAPTER 		Adapter
	);

VOID
HalDownloadRSVDPage8723BS(
	IN	PADAPTER			Adapter
	);

VOID
ResetHalRATRTable8723BSdio(
	PADAPTER		Adapter
	);

VOID
HandleH2CJoinBssRptWorkItemCallback_8723BS(
	IN	PVOID            pContext
);

BOOLEAN
CheckCPWMInterrupt8723BS(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HalSdioIoRegCmd52Available8723BS(
	IN	PADAPTER	Adapter,
	IN	u4Byte		offset
	);

RT_STATUS
HalSetRsvdPageBndy8723BS(
	IN		PADAPTER	Adapter,
	IN		u1Byte		RQPNType
	);

