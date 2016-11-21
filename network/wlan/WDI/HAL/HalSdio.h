#ifndef __INC_HALSDIO_H
#define __INC_HALSDIO_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalSdio.h
	
Abstract:
	Prototype of HalsdioXXX() and related data structure.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2010-12-12 Roger            Create.	
	
--*/

#define		SUPPORT_HW_RADIO_DETECT(_Adapter)	TRUE

#define DRIVER_EARLY_INT_TIME			0x10
#define BCN_DMA_ATIME_INT_TIME		0x02


#define MAX_NUM_OUT_PIPE					16
#define MAX_NUM_SDIO_TX_QUEUE			3

//
// <Roger_Notes> The number of OutPipe request must larger than or equal to the number of 
// pending request for each pipe. 2009. 01. 12.
//
#define MAX_NUM_REQUEST_PER_OUT_PIPE	8
#define MAX_NUM_REQUEST_PER_IN_PIPE		128	

#define MAX_NUM_REQUEST_PER_TX_QUEUE	3
#define MAX_NUM_REQUEST_PER_RX_QUEUE	1	

#define HAL_SDIO_COMPLETED_OK				BIT1	
#define HAL_SDIO_COMPLETED_ERROR			BIT2
#define HAL_SDIO_COMPLETED_CANCELED		BIT3

#define SDIO_TRANSFERS_MASK (DF_TX_BIT | DF_RX_BIT | DF_IO_BIT)
#define SDIO_TX_TRANSFERS_MASK		DF_TX_BIT
#define SDIO_RX_TRANSFERS_MASK		DF_RX_BIT

#define SDIO_TX_TRANSFER_SIZE	64

#define RT_DISABLE_SDIO_TX_TRANSFERS(__pAdapter) RT_DISABLE_FUNC(__pAdapter, SDIO_TX_TRANSFERS_MASK)
#define RT_ENABLE_SDIO_TX_TRANSFERS(__pAdapter)  RT_ENABLE_FUNC(__pAdapter, SDIO_TX_TRANSFERS_MASK)
#define RT_DISABLE_SDIO_RX_TRANSFERS(__pAdapter) RT_DISABLE_FUNC(__pAdapter, SDIO_RX_TRANSFERS_MASK)
#define RT_ENABLE_SDIO_RX_TRANSFERS(__pAdapter)  RT_ENABLE_FUNC(__pAdapter, SDIO_RX_TRANSFERS_MASK)
#define RT_DISABLE_SDIO_TRANSFERS(__pAdapter) RT_DISABLE_FUNC(__pAdapter, SDIO_TRANSFERS_MASK)
#define RT_ENABLE_SDIO_TRANSFERS(__pAdapter)  RT_ENABLE_FUNC(__pAdapter, SDIO_TRANSFERS_MASK)

#define RT_SDIO_CANNOT_IO(__pAdapter) \
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved || \
			 RT_IS_FUNC_DISABLED((__pAdapter), DF_IO_BIT))

#define RT_SDIO_CANNOT_TX(__pAdapter) \
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved || \
			 RT_IS_FUNC_DISABLED((__pAdapter), DF_TX_BIT))

#define RT_SDIO_CANNOT_RX(__pAdapter) \
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved || \
			 RT_IS_FUNC_DISABLED((__pAdapter), DF_RX_BIT))
			 
#define RT_GetInterfaceSelection(_Adapter) 	((u1Byte)GET_HAL_DATA(_Adapter)->InterfaceSel)

#define IS_NEED_OFFSET_ON_AMPDU(_Adapter)	FALSE
#define IS_CHK_AMSDU_BUF_CNT(_Adapter)		FALSE
#define IS_RW_PTR(_Adapter)					FALSE
#define IS_SET_RX_AGGR(_Adapter)			FALSE
#define IS_RSTO_WMM_AFTER_SCAN(_Adapter)	FALSE

typedef struct _SDIO_OUT_RESOURCE
{
	s4Byte				PipeCnt;			// # of OUT pipes.
	s4Byte				QueueCnt;		// # of Tx Queues

	//
	// Memory block of SDIO_OUT_CONTEXT objects.
	//
	pu1Byte				SendContextsBuffer;
	u4Byte				SendContextsBufferSz;

	//
	// Queue to manage free SDIO_OUT_CONTEXT objects.
	//
	RT_LIST_ENTRY		TxFreeContextList;	// An link list of SDIO_OUT_CONTEXT. Note that, every out pipe shall get and return SDIO_OUT_CONTEXT from and to here.
	u4Byte				TxFreeContextCount;	// Number of conext available to use.
	s4Byte				TxContextCount;		// Total number of SDIO_OUT_CONTEXT allocated. 
} SDIO_OUT_RESOURCE, *PSDIO_OUT_RESOURCE;

typedef struct _SDIO_OUT_CONTEXT 
{
	RT_LIST_ENTRY		List;			

	//
	// TODO: Replace pTcb as HalReserved[0].
	//
	pu1Byte				Buffer;			// Pointer to the buffer to Transfer data to SDIO Host.
	u4Byte				BufLen;			// Length of Buffer.
	PRT_TCB				pTcb;			// Pointer to corresponding
	BOOLEAN				bData0Byte;		// TRUE if this context is used as 0-byte padding for 512N-byte buffer on USB2.0 or 64N-byte on USB1.1.
	BOOLEAN				bTxPending;		// TRUE if this context is pending in USBD.
	u1Byte				PendingCount;	// Used in CheckForHang check sending stuck or not.
	u1Byte				TxQueueIndex;	// An index to the Tx Queue, i.e., 0: HIQ, 1: MIQ, 2:LOQ
	u1Byte				AggrNum;		// SDIO Tx aggregation number.

	//
	// RT_PLATFORM reserved.  
	// For example, 
	// - on Windows NDIS5/WDM: [0]: PADAPTER, [1]: NULL, [2]: NULL.
	// - On Windows NDIS6/WDF: [0]: PADAPTER, [1]: NULL, [2]: WDFREQUEST.
	//
	PVOID				PlatformReserved[4];	
} SDIO_OUT_CONTEXT, *PSDIO_OUT_CONTEXT;


typedef struct _SDIO_IN_RESOURCE
{
	s4Byte				PipeCnt;			// # of IN pipes.
	s4Byte				QueueCnt;		// # of Rx Queues

	//
	// Memory block of SDIO_IN_CONTEXT objects.
	//
	pu1Byte				ContextsBuffer;
	u4Byte				ContextsBufferSz;

	//
	// Queue to manage free USB_IN_CONTEXT objects.
	//
	RT_LIST_ENTRY		FreeContextList;	// An link list of USB_IN_CONTEXT. Note that, every out pipe shall get and return USB_IN_CONTEXT from and to here.
	u4Byte				FreeContextCount;	// Current number of conext available to use.
	s4Byte				ContextCounAlloc;	// Total number of USB_IN_CONTEXT allocated. 
} SDIO_IN_RESOURCE, *PSDIO_IN_RESOURCE;

typedef struct _SDIO_IN_CONTEXT 
{
	RT_LIST_ENTRY		List;			

	pu1Byte				Buffer;			// Pointer to the buffer to IN from USBD.
	u4Byte				BufferLen;		// Length of Buffer allowed USBD to use.
	u4Byte				BufLenUsed;		// Length of Buffer used for the bulk IN transfer. 
	BOOLEAN				bRxPending;		// TRUE if this context is pending in USBD.
	u1Byte				RxPipeIndex;	// An index to the IN pipe.

	//
	// RT_PLATFORM reserved.  
	// For example, 
	// - on Windows NDIS5/WDM: [0]:PADAPTER, [1]: NULL, [2]: NULL.
	// - On Windows NDIS6/WDF:
	//
	PVOID				PlatformReserved[4];

	//
	// HAL reserved: Data: pRFD, CMD
	//
	PVOID				HalReserved[1];

#if VISTA_USB_RX_REVISE
	u4Byte				RetStatus;				
#endif
} SDIO_IN_CONTEXT, *PSDIO_IN_CONTEXT;


typedef struct _HAL_SDIO_INFO
{
	SDIO_OUT_RESOURCE	OutResource;
	SDIO_IN_RESOURCE		InResource;
} HAL_SDIO_INFO, *PHAL_SDIO_INFO;

#define HAL_BUS_INFO_TYPE HAL_SDIO_INFO

#define GET_HAL_BUS_INFO(__pAdapter) ((HAL_BUS_INFO_TYPE*)((__pAdapter)->pHalBusInfo))

#define GET_USB_DEVICE_SPEED(_pAdapter)  USB_DEVICE_SPEED_FULL


#define GET_OUT_RES(__pBusInfo) (&((__pBusInfo)->OutResource))
#define GET_IN_RES(__pBusInfo) (&((__pBusInfo)->InResource))
#define FREE_OUT_CONTEXT_CNT(__pAdapter) ( GET_OUT_RES(GET_HAL_BUS_INFO(__pAdapter))->TxFreeContextCount )

//================================================================================

typedef struct _HAL_DATA_SDIO
{
	//
	// Read-only hardware information, it should not be changed after set.
	// Make a copy and change its copy if need.
	//
	u2Byte				HardwareType;
	VERSION_8192C			VersionID;
	VERSION_CVID			CVID_Version;	// For Version of Structure ChipVersion: 0:v1(before 8703B), 1:v2(since 8703B)

	RT_MULTI_FUNC			MultiFunc; // For multi-function consideration.
	RT_POLARITY_CTL			PolarityCtl; // For Wifi PDn Polarity control.
	RT_REGULATOR_MODE		RegulatorMode; // switching regulator or LDO
	//u1Byte					HwROFEnable; // Enable GPIO[9] as WL RF HW PDn source


	//
	// For SDIO Interface HAL related
	//
	BOOLEAN				bMacPwrCtrlOn; //Auto FSM to Turn On, include clock, isolation, power control for MAC only

	RT_BUS_TYPE			BusType;
	RT_EEPROM_TYPE			EEType;
	BOOLEAN 				bMPMode;
	u1Byte				MWIEnable;
	u1Byte				AutoloadFailFlag;
	BOOLEAN				bNOPG;

	// TODO: Let it be platform independent.
	PlatformMutex			mxRFOperate;	// RF access lock
	PlatformMutex			mxCCKControl;
	PlatformMutex			mxChnlBwControl;

	// add for 92D Phy mode/mac/Band mode 
	//MACPHY_MODE_8192D		MacPhyMode92D;
	//u1Byte			BandType92D;	//0: 2.4G,        1:  5G
	BAND_TYPE			CurrentBandType;	//0:2.4G, 1:5G
	BAND_TYPE			LastBandType;	//0:2.4G, 1:5G		
	BAND_TYPE			BandSet;
	BOOLEAN				bIsVS;
	
	//
	// Add For EEPROM Efuse switch and  Efuse Shadow map Setting
	//
	BOOLEAN				EepromOrEfuse;
	u1Byte				EfuseMap[2][HWSET_MAX_SIZE_512]; //92C:256bytes, 88E:512bytes, we use union set (512bytes)
	u2Byte				EfuseUsedBytes;
	u1Byte				EfuseUsedPercentage;
	EFUSE_HAL			EfuseHal;

	//
	// For 92C USB endpoint setting
	//
	u1Byte				OutEpQueueSel;
	u1Byte				OutEpNumber;

	//
	//SDIO Tx FIFO related.
	//
	u1Byte				SdioTxFIFOFreePage[SDIO_TX_FREE_PG_QUEUE];
	u2Byte				SdioTxFIFOFreePage8814A[SDIO_TX_FREE_PG_QUEUE_8814A];		//3 20130422 KaiYuan modify for 8814; for other IC, it is u1Byte
	u4Byte				SdioTxRsvdPageSize; // for download beacon queue

	//
	// Default Settings
	//
	u4Byte					ChipID;
	u4Byte					ChipVer;
	BOOLEAN					bNicWirelessMode;
	BOOLEAN					bIs88eSeries;
	BOOLEAN					bIsNicExistBt;
	BOOLEAN					bEnableSingleAmpdu;
	BOOLEAN					bNeedExtIQK;
	BOOLEAN					bCheckHiddenSsid;
	BOOLEAN					bDisableRtsOnAmpdu;
	BOOLEAN					bSupClkOnExcepResn;
	u2Byte					MaxRxDmaBufferSize;
	u2Byte					MaxTxBufSize;
	u2Byte					TcbNumOfMemSize0;
	u2Byte					TcbNumOfMemSize1;
	u2Byte					TxDescNum;
	u2Byte					TxDescNumBeQ;
	u2Byte					TxDescNumMp;
	u2Byte					TxDescNumBeQMp;
	u2Byte					MaxRxBufSize;
	u2Byte					MaxSubframeCntMmSize0;
	u2Byte					MaxSubframeCntMmSize1;
	u2Byte					RfdNumMmSize0;
	u2Byte					RxDescNumMmSize0;
	u2Byte					RfdNumMmSize1;
	u2Byte					RxDescNumMmSize1;
	BOOLEAN					bNicSupportWapi;
	BOOLEAN					bRemoveZeroLenBuf;
	BOOLEAN					bLeavePsBeforeEnNic;

	//
	// EEPROM setting.
	//
	u1Byte				EEPROMVersion;
	u2Byte				EEPROMVID;
	u2Byte				EEPROMPID;
	u2Byte				EEPROMSVID;
	u2Byte				EEPROMSDID;
	u1Byte				EEPROMCustomerID;
	u1Byte				EEPROMSubCustomerID;	
	u1Byte				EEPROMRegulatory;
	u1Byte				EEPROMThermalMeter;			// ThermalMeter value.	
	u1Byte				EEPROMBluetoothCoexist;	
	u1Byte				EEPROMBluetoothType;
	u1Byte				EEPROMBluetoothAntNum;
	u1Byte				EEPROMBluetoothSingleAntPath;
	u2Byte				EEPROMChannelPlan;
	// 2011/03/10 Vivi Add for 92C/D common binary.
	u1Byte				USBDummyOffset;
	u1Byte				USBALLDummyLength;
	u1Byte				EFUSECloudKey_EX[EEPROM_CLOUD_KEY_LENGTH_EX];
	
	//
	// SDIO ISR Related
	//
	// Interrupt relatd register information.
	RT_INT_REG			IntArray[1];
	RT_INT_REG			IntrMask[1];
	RT_INT_REG			IntrMaskToClear[1];
	RT_INT_REG			SysIntArray[1];
	RT_INT_REG			SysIntrMask[1];
	RT_INT_REG			SysIntrMaskToClear[1];
	u4Byte				RxIntLength; // The length of rx packet from the intrrupt information in SDIO only.
	LOG_INTERRUPT			InterruptLog;
	LOG_SYS_INTERRUPT		SysInterruptLog;

	//
	// The same as 92CE. May merge 92CU and 92CE into the other struct
	//

	u1Byte				ThermalMeter[2];				// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u1Byte				ThermalValue;
	u1Byte				ThermalValue_LCK;
	u1Byte				ThermalValue_IQK;
	u1Byte				ThermalValue_DPK;		
	u1Byte				ThermalValue_AVG[AVG_THERMAL_NUM];
	u1Byte				ThermalValue_AVG_index;		
	u1Byte				ThermalValue_RxGain;
	u1Byte				ThermalValue_Crystal;
	u1Byte				ThermalValue_DPKstore;
	u1Byte				ThermalValue_DPKtrack;
	BOOLEAN				TxPowerTrackingInProgress;

	//s2Byte				index_mapping_DPK_current[4][index_mapping_DPK_NUM];
	
	BOOLEAN				bRfPiEnable;
	BOOLEAN				bReloadtxpowerindex;	
	BOOLEAN				bDoneTxpower;		
	u1Byte				CrystalCap;						
	u1Byte				CrystalFreq;
	u1Byte				CrystalSrc;
	
	s1Byte				OFDM_index_HP[2];
	s1Byte				CCK_index_HP;		
	u1Byte				ThermalValue_HP[HP_THERMAL_NUM];
	u1Byte				ThermalValue_HP_index;

	BOOLEAN				bAPKdone;	
	BOOLEAN				bDPKdone[2];

	//for TxPwrTracking
	s4Byte				RegE94;
	s4Byte 				RegE9C;
	s4Byte				RegEB4;
	s4Byte				RegEBC;
	//for IQK	
	BOOLEAN				bIQKInitialized;
	BOOLEAN				bAntennaDetected;
	u4Byte				ADDA_backup[IQK_ADDA_REG_NUM];
	u4Byte				IQK_MAC_backup[IQK_MAC_REG_NUM];
	u4Byte				IQK_BB_backup_recover[9];
	u4Byte				IQK_BB_backup[IQK_BB_REG_NUM];	

	//---------------------------------------------------------------------------------//
	//3 [2.4G]
	u1Byte				Index24G_CCK_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER_2G];
	u1Byte				Index24G_BW40_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER_2G];
	//If only one tx, only BW20 and OFDM are used.
	s1Byte				CCK_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];	
	s1Byte				OFDM_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s1Byte				BW20_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s1Byte				BW40_24G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	//3 [5G]
	u1Byte				Index5G_BW40_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER_5G];
	u1Byte				Index5G_BW80_Base[MAX_RF_PATH][CHANNEL_MAX_NUMBER_5G_80M];		
	s1Byte				OFDM_5G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s1Byte				BW20_5G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s1Byte				BW40_5G_Diff[MAX_RF_PATH][MAX_TX_COUNT];
	s1Byte				BW80_5G_Diff[MAX_RF_PATH][MAX_TX_COUNT];

	//
	// TX power by rate table at most 4RF path.
	// The register is 
	//
	// VHT TX power by rate off setArray = 
	// Band:-2G&5G = 0 / 1
	// RF: at most 4*4 = ABCD=0/1/2/3
	// CCK=0 OFDM=1/2 HT-MCS 0-15=3/4/56 VHT=7/8/9/10/11			
	//
	u1Byte				TxPwrByRateTable;
	u1Byte				TxPwrByRateBand;
	s1Byte				TxPwrByRateOffset[TX_PWR_BY_RATE_NUM_BAND]
										 [TX_PWR_BY_RATE_NUM_RF]
										 [TX_PWR_BY_RATE_NUM_RF]
										 [TX_PWR_BY_RATE_NUM_RATE];	
    //---------------------------------------------------------------------------------//

	
	u1Byte				PowerIndex_backup[6];
	u1Byte				TxPwrLevelCck[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];
	u1Byte				TxPwrLevelHT40_1S[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];		// For HT 40MHZ pwr
	u1Byte				TxPwrLevelHT40_2S[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];		// For HT 40MHZ pwr	
	s1Byte				TxPwrHt20Diff[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];			// HT 20<->40 Pwr diff
	u1Byte				TxPwrLegacyHtDiff[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];		// For HT<->legacy pwr diff

	// Power Limit Table for 2.4G
	s1Byte				TxPwrLimit_2_4G[MAX_REGULATION_NUM]
									   [MAX_2_4G_BANDWITH_NUM]
	                                   [MAX_RATE_SECTION_NUM]
	                                   [CHANNEL_MAX_NUMBER_2G]
	                                   [MAX_RF_PATH];

	// Power Limit Table for 5G
	s1Byte				TxPwrLimit_5G[MAX_REGULATION_NUM]
									 [MAX_5G_BANDWITH_NUM]
									 [MAX_RATE_SECTION_NUM]
								 	 [CHANNEL_MAX_NUMBER_5G]
								 	 [MAX_RF_PATH];

	u1Byte				Regulation2_4G;
	u1Byte				Regulation5G;

	// Store the original power by rate value of the base of each rate section of rf path A & B
	u1Byte				TxPwrByRateBase2_4G[TX_PWR_BY_RATE_NUM_RF]
										   [TX_PWR_BY_RATE_NUM_RF]
									  	   [MAX_BASE_NUM_IN_PHY_REG_PG_2_4G];
	u1Byte				TxPwrByRateBase5G[TX_PWR_BY_RATE_NUM_RF]
										 [TX_PWR_BY_RATE_NUM_RF]
									  	 [MAX_BASE_NUM_IN_PHY_REG_PG_5G];

	// Used for TX power setting shadow map.
	u1Byte				TxPwrShadow[MAX_RF_PATH][DESC_RATEMAX];
	
	// For power group
	u1Byte				PwrGroupHT20[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];
	u1Byte				PwrGroupHT40[RF_PATH_MAX_92C_88E][CHANNEL_MAX_NUMBER];
	
	u1Byte				LegacyHTTxPowerDiff;			// Legacy to HT rate power diff

	// Read/write are allow for following hardware information variables
	u1Byte				DefaultInitialGain[4];
	u1Byte				pwrGroupCnt;
	u4Byte				MCSTxPowerLevelOriginalOffset[MAX_PG_GROUP][16];
	u4Byte				CCKTxPowerLevelOriginalOffset;
	u1Byte				TxPowerLevelCCK[14];			// CCK channel 1~14
	u1Byte				TxPowerLevelOFDM24G[14];		// OFDM 2.4G channel 1~14
	u1Byte				TxPowerLevelOFDM5G[14];			// OFDM 5G
	u1Byte				AntennaTxPwDiff[3];				// Antenna gain offset, index 0 for B, 1 for C, and 2 for D

	u4Byte					AntennaTxPath;					// Antenna path Tx (Used in MP driver)
	u4Byte					AntennaRxPath;					// Antenna path Rx (Used in MP driver)
	u4Byte					ValidTxPath;					// Valid Antenna path Tx (Used in Normal driver)
	u4Byte					ValidRxPath;					// Valid Antenna path Tx (Used in Normal driver)
	u1Byte 					CCKRxPath;  //Back CCX Rx Path
	u1Byte 					CCKTxPath;  //Back CCX Tx Path
	
	u4Byte				LedControlNum;
	u4Byte				LedControlMode;

	// The current Tx Power Level
	u1Byte				CurrentCckTxPwrIdx;
	u1Byte				CurrentOfdm24GTxPwrIdx;
	u1Byte				CurrentBW2024GTxPwrIdx;
	u1Byte				CurrentBW4024GTxPwrIdx;	
	u1Byte				OriginalCckTxPwrIdx;
	u1Byte				OriginalOfdm24GTxPwrIdx;

	u1Byte				EEPROMC9;
	u1Byte				EEPROMCC;
	u1Byte				PAMode;

	u1Byte				InternalPA5G[2];	//pathA / pathB
	u1Byte				Delta_IQK;
	u1Byte				Delta_LCK;

	// For compatiable compile for 92D.
	u4Byte				RegA24;
	u1Byte				RegC04;
	u4Byte				RegD04;
	u4Byte				RegRF3C[2];	//pathA / pathB	
	u4Byte				RegRF36;
	
	HAL92C_P2P_PS_OFFLOAD		p2pFwPsOffload;

	//
	// Read/write are allow for following hardware information variables
	//
	RF_TYPE_E			RFChipID;


	// Customer-defined power by rate / power limit table loading status
	u1Byte				CustomPwrByRateFileStatus;
	u1Byte				CustomPwrLimitFileStatus;

	u1Byte				CurrentPwrLimitTableSel;	// Primary = 0/Secondary = 1

	u1Byte				DynamicTxPwrLoadingInfo;	// Dynamic Tx Power Table Loading info
	u1Byte				TxPwrTableDumpInfo;			// Tx Power Table Dump info

	//2 Management related
	WIRELESS_MODE			CurrentWirelessMode;	//For 8185
	u1Byte				CurrentChannel;
	RT_TIMER			SwChnlTimer;
#if USE_WORKITEM
	RT_WORK_ITEM			SwChnlWorkItem;
#endif
	BOOLEAN				SwChnlInProgress;
	u1Byte				SwChnlStage;
	u1Byte				SwChnlStep;

	// bandwidth mode
	CHANNEL_WIDTH		CurrentChannelBW;
	RT_TIMER			SetBWModeTimer;
	u1Byte				SetBWModeInProgress;
#if USE_WORKITEM
	RT_WORK_ITEM		SetBWModeWorkItem;
	RT_WORK_ITEM		RtCheckForHangWorkItem;
	RT_WORK_ITEM		RtCheckResetWorkItem;
#endif

	BOOLEAN				RFOff;
	BOOLEAN				bIntolerantAC;
	BOOLEAN				AdhocLinkState;
	BOOLEAN				SetRFPowerStateInProgress;

	//2 Tx Related variables
	u2Byte				ShortRetryLimit;
	u2Byte				LongRetryLimit;

	//2 Rx Related variables
	u2Byte				EarlyRxThreshold;
	u4Byte				ReceiveConfig;
	u2Byte				RxTag;

        //2 QoS Related variable
	u1Byte				AcmControl;
		
	u4Byte				FWChannelSwitchComplete;
	BOOLEAN				bSwChnlAndSetBWInProgress;
	u1Byte				CurrentCenterFrequencyIndex1;
	BOOLEAN				bSwChnl;
	BOOLEAN					bSetChnlBW;
	RT_WORK_ITEM			SwChnlAndSetBWModeWorkItem;

	//2 Led Related variable
	LED_STRATEGY_SDIO	LedStrategy; 
	LED_SDIO			SwLed0;
	LED_SDIO			SwLed1;
	LED_SDIO			SwLed2;
	LED_SDIO			SwLed3;
	LED_SDIO			SwLedAll;
	BOOLEAN				bLedOpenDrain; // Support Open-drain arrangement for controlling the LED. Added by Roger, 2009.10.16.


	// RF power state: on, sleep, off.
	RT_RF_POWER_STATE		eRFPowerState;

	// Channel Access related register.
	u1Byte				SlotTime;
	u4Byte				SifsTime;

	// Command
	u1Byte				Command;

	// Data Rate Config. Added by Annie, 2006-04-13.
	u2Byte				BasicRateSet;
	BOOLEAN				bShortPreamble;
	//
	// CCX Radio Measurement;
	//
	u1Byte				LastClmDur;
	u1Byte				LastNhmDur;
	u4Byte				ClmAcc;
	u4Byte				ClmDurAcc;
	u4Byte				NhmAcc[8];
	u4Byte				NhmDurAcc;
	

	//
	// Default setting (initialized from registry).
	//
	BOOLEAN				bRegUseLed; // 0: disable LED, 1: eanble LED.

	
	//
	// High Power Mechanism.
	//
	// For Default UI Tx power modification
	s4Byte				DefaultTxPwrDbm;
	s4Byte				MinCCKDbm;
	s4Byte				MinLOFDMDbm;
	s4Byte				MinHOFDMDbm;
	s4Byte				MinPwrDbm;
	s4Byte				MaxCCKDbm;
	s4Byte				MaxLOFDMDbm;
	s4Byte				MaxHOFDMDbm;
	s4Byte				MaxPwrDbm;

	//
	// Turbo Mode Mechanism. Added by Roger, 2006.12.15.
	//
	BOOLEAN				bInTurboMode;

	// Interface selection. Added by Annie, 2005-11-18.
	u1Byte				InterfaceSel;
	u1Byte				PackageType;	
	u1Byte				RFEType;
	BOOLEAN				bIsMPChip;

	s1Byte				BufOfLines[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];


	BOOLEAN				bSendingBeacon; // TRUE if we are sending beacon to host control and not yet completed.

        // Sw Antenna Diversity
	//20070103 porting by David 
	BOOLEAN				bRegSwAntennaDiversityMechanism; // TRUE if S/W antenna diversity mechanism is allowed, see also EEPROMSwAntennaDiversity and bSwAntennaDiverity.
	BOOLEAN				bAntennaDiversityTimerIssued;
	RT_WORK_ITEM	        	SwAntennaWorkItem;
	u1Byte				CurrAntennaIndex;			// Index to current Antenna (both Tx and Rx).
	u1Byte				AdTickCount;				// Times of SwAntennaDiversityTimer happened.
	u1Byte				AdCheckPeriod;				// # of period SwAntennaDiversityTimer to check Rx signal strength for SW Antenna Diversity. 
	u1Byte				AdMinCheckPeriod;			// Min value of AdCheckPeriod. 
	u1Byte				AdMaxCheckPeriod;			// Max value of AdCheckPeriod.  
	BOOLEAN				bSwAntennaDiverity;			// TRUE if we want to enable SW Antenna Diversity mechanism.
	RT_TIMER			SwAntennaDiversityTimer;	// Timer object for SW Antenna Diversity mechanism.
	u4Byte				AdRxOkCnt;					// ROK packet count from current BSS in ANTENNA_DIVERSITY_TIMER_PERIOD.
	s4Byte				AdRxSignalStrength;			// Rx signal strength for Antenna Diversity, which had been smoothing, its valid range is [0,100].	
	s4Byte				AdRxSsThreshold;			// Signal strength threshold to switch antenna.
	s4Byte				AdMaxRxSsThreshold;			// Max value of AdRxSsThreshold.
	BOOLEAN				bAdSwitchedChecking;		// TRUE if we shall shall check Rx signal strength for last time switching antenna.
	s4Byte				AdRxSsBeforeSwitched;		// Rx signal strength before we swithed antenna.
	//
	//1 The following was added for 8192sUsb
	
	/* Firmware related */	
	PRT_FIRMWARE			pFirmware;
	u1Byte				RegFWOffload;
	u4Byte					RsvdBitMap;
	
	// Beamforming RF path number
	u1Byte					RegBeamformerCapRfNum;
	u1Byte					RegBeamformeeCapRfNum;

	/*PHY related*/
	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D

	u1Byte				NumTotalRFPath;

	/*Last RxDesc TSF value*/
	u4Byte				LastRxDescTSFHigh;
	u4Byte				LastRxDescTSFLow;
	

	//
	// 8190 40MHz mode
	//
	u1Byte				nCur40MhzPrimeSC;	// Control channel sub-carrier
	u1Byte				nCur80MhzPrimeSC;   //used for primary 40MHz of 80MHz mode
	
	/* Rx Descriptor Debug message for aggregation related histogram */
	u1Byte				nRxAMPDU_AggrNum;
	u2Byte				nRxAMPDU_Size;
		

	//
	// Joseph test for Silent Reset
	// Driver cannot open file when it is not in initialization state.
	// So we need a buffer to save the content of configuration file for reset use.
	//
	s1Byte				BufOfLines1[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLines2[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLines3[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLines4[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLines5[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLines6[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];

	s4Byte				nLinesRead1;
	s4Byte				nLinesRead2;
	s4Byte				nLinesRead3;
	s4Byte				nLinesRead4;
	s4Byte				nLinesRead5;
	s4Byte				nLinesRead6;

	s1Byte				BufOfLinesPwrLmt[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s4Byte				nLinesReadPwrLmt;
	s1Byte				BufOfLinesPwrByRate[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s4Byte				nLinesReadPwrByRate;

	s1Byte				BufRadioA[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufRadioB[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufRadioC[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufRadioD[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s4Byte				nLinesBufRadioA;
	s4Byte				nLinesBufRadioB;
	s4Byte				nLinesBufRadioC;
	s4Byte				nLinesBufRadioD;
	
	BOOLEAN				bCCKinCH14;
	s1Byte				CCK_index;
	s1Byte				OFDM_index[2];
	u4Byte				TXPowerTrackingCallbackCnt;		//debug only

	
	//cosa add for 2T4R/1T2R software control 10/24/2007
	u1Byte				RF_Type;// 1 means 2T4R, 0 means 1T2R

		// For EDCA Turbo mode
	BOOLEAN				bIsAnyNonBEPkts;
	
        u8Byte				SystemStartTime;
        u8Byte				SystemCurrTime;

	// Use to calculate PWBD.
	s4Byte				UndecoratedSmoothedPWDB;
	s4Byte				UndecoratedSmoothedCCK;
	s4Byte				UndecoratedSmoothedOFDM;
	s4Byte				UndecoratedSmoothedBeacon;
	u4Byte				RxRate;
	u8Byte				PacketMap;
	u4Byte				OFDM_pkt;
	u1Byte				ValidBit;
	s4Byte				EntryMinUndecoratedSmoothedPWDB;
	s4Byte				EntryMaxUndecoratedSmoothedPWDB;
	s4Byte				BT_EntryMinUndecoratedSmoothedPWDB;
	s4Byte				BT_EntryMaxUndecoratedSmoothedPWDB;

	//SW Antenna Switch
	s4Byte				RSSI_sum_A;
	s4Byte				RSSI_cnt_A;
	s4Byte				RSSI_sum_B;
	s4Byte				RSSI_cnt_B;
	BOOLEAN				RSSI_test;
	PRT_WLAN_STA			RSSI_target;

	u1Byte				AntDivCfg;
	u1Byte				AntDetection;
	u1Byte				PathDivCfg;
        u1Byte                          TRxAntDivType;       // RF type, Read from 88E EFUSE 0xC9 
	u1Byte					ReverseDPDT;		
	u4Byte				OFDM_Pkt_Cnt;
	u4Byte				CCK_Pkt_Cnt;

	RT_WORK_ITEM			PSDMonitorWorkitem;

       //WLAN PSD for BT AFH
	BOOLEAN				bPSDinProcess;
	RT_TIMER			PSDTimer;
 	u1Byte				RSSI_BT;
	RT_TIMER			PSDTriggerTimer;
    //---------------2011.08.24 ------------------
	
	// 2008/01/10 MH HW Trun on/off RF according to GPIO1 
	RT_WORK_ITEM			GPIOChangeRFWorkItem;
	// TRUE if RF is turned to OFF by HW (e.g. GPIO1), FALSE otherwise.
	BOOLEAN				bHwRadioOff; 

	// 2010/11/25 Check PBC GPIO
	RT_WORK_ITEM			GPIOCheckPBCWorkItem;
	
	// Ratr table used bitmap
	u1Byte				RATRTableBitmap;

	BOOLEAN				bInMonitorMode; // Indicate if underlying h/w is configured in monitor mode. 
	
	//Add by Jacken Tx Power Control for Near/Far Range 2008/03/06
	u1Byte				DynamicTxHighPowerLvl;  //Tx High power level
	u1Byte				LastDTPLvl;
	
	SCAN_OPERATION_BACKUP_OPT	ScanOperationBackupOtpType;
	RT_WORK_ITEM			ScanOperationBackupWorkItem;


	// RF and BB simple mechanism for synchronizing access. Added by Roger, 2009.06.29.
	u4Byte				bChangeBBInProgress; // BaseBand RW is still in progress.
	u4Byte				bChangeRFInProgress; // RF RW is still in progress.


	RT_WORK_ITEM			IOWorkItem;
	IO_TYPE				CurrentIOType;
	BOOLEAN				SetIOInProgress;
	u1Byte				NumOfPipes;
	QUEUE_INDEX_LIST 		OutPipeToTxQueueMap[MAX_NUM_SDIO_TX_QUEUE];
	u1Byte				TxQueueToOutPipeMap[MAX_TX_QUEUE];

	u1Byte				DMFlag; // Indicate if each dynamic mechanism's status.
 
	u1Byte				PreRpwmVal; // by tynli. For recording the previous RPWM value.
	u1Byte				LastHCPWM1Val;

	u4Byte				RfRegChnlVal[MAX_RF_PATH];


	u1Byte				PbcGPIOCnt;

	BOOLEAN				bRegUsbInTokenRev; //For Intel ATOM CPU utilization, 2009.06.04.


	//----------------------------------------------------------------------------
	//  RTL8192C USB Tx/Rx Aggregation
	//----------------------------------------------------------------------------
	u4Byte				TxTransferSize;

#if TX_AGGREGATION
	BOOLEAN				UsbTxAggMode;
	u1Byte				UsbTxAggTotalNum;
	u1Byte				UsbTxAggDescNum;
	//u1Byte				UsbTxAggPerBulkNum;
#endif
#if RX_AGGREGATION

	u2Byte				HwRxPageSize;				// Hardware setting
	u4Byte				MaxRxAggBlock;
	
	RX_AGG_MODE		RxAggMode;
	u1Byte				RegRxAggBlockCount;
	u1Byte				RxAggBlockCount;			// USB Block count. Block size is 512-byte in hight speed and 64-byte in full speed
	u1Byte				RegRxAggBlockTimeout;
	u1Byte				RxAggBlockTimeout;
	u1Byte				RegRxAggPageCount;
	u1Byte				RxAggPageCount;			// 8192C DMA page count
	u1Byte				RegRxAggPageTimeout;
	u1Byte				RxAggPageTimeout;
	RT_WORK_ITEM		RxAggrSettingWorkItem;
#endif
	// 2010/12/10 MH Add for USB aggreation mode dynamic shceme.
	BOOLEAN				UsbRxHighSpeedMode;

	u1Byte				RegTxPause;

	//Add for maintain H2C box number. by tynli. 2009.10.06.
	u1Byte 				LastHMEBoxNum;
	u4Byte				lastFwCmdElementId;

	RT_WORK_ITEM		FillH2CCmdWorkItem;
	BOOLEAN				bH2CSetInProgress;	
	RT_TIMER				FillH2CCmdTimer;
	BOOLEAN				H2CStopInsertQueue; //by tynli. 2010.10.07.
#if(FW_QUEME_MECHANISM_NEW != 1)
	H2C_CMD_8192C    	H2CCmd[MAX_H2C_QUEUE_NUM];	//for queuing the h2c cmds which are sent in dispatch level.
	u1Byte				H2CQueueHead;
	u1Byte				H2CQueueTail;
#endif
	u4Byte				h2cStatistics[H2C_STATUS_MAX];
	u1Byte				lastSuccessH2cEid;
	u1Byte				firstFailedH2cEid;

	RT_TIMER			DualMacSetOperationForAnotherMacTimer;


	// Record Fw PS mode status. Added by tynli. 2010.04.19.
	u1Byte				FwPSState;
	BOOLEAN				bFwClkChangeInProgress;
	BOOLEAN				bFwDwRsvdPageInProgress;
	// For 32k power save. 2011.03.22. by tynli.
	RT_TIMER			FwClockOffTimer;
	RT_WORK_ITEM			HandleH2CJoinBssRptWorkItem;
	BOOLEAN				bAllowSwToChangeHwClock;	//by tynli. for 32k.
	BOOLEAN				bKeepInPSPeriod;	 // by tynli. for 32k.
	RT_WORK_ITEM		FwClockOffWorkItem;
	u1Byte				HwClkOnExceptionReason;
	u1Byte				ForceHwClkOnExceptionReason;

#if RT_PLATFORM == PLATFORM_MACOSX
	u1Byte				RegBcnCtrlVal;
#else
	u4Byte				RegBcnCtrlVal;
#endif

	RT_WORK_ITEM			ResumeTxBeaconWorkItem;

	// Beacon function related global variable.
	u1Byte				RegFwHwTxQCtrl;
	u1Byte				RegReg542;
	u1Byte				RegCR_1;
	BOOLEAN				bRegUsbSS;
	//Record if Card Disable by HW auto SM.
	BOOLEAN				bCardDisableHWSM;

	//For check TxDMA error status.
	BOOLEAN				bClearTxDMAErrorFlag;
	u1Byte				USBResetTxHangEanble;

	// Record Fw PS mode status. Added by tynli. 2010.04.19.
	BOOLEAN				bFwCurrentInPSMode;
	u2Byte				FwRsvdPageStartOffset; //2010.06.23. Added by tynli. Reserve page start offset except beacon in TxQ.

	// 2010/08/09 MH Add CU power down mode.
	BOOLEAN				pwrdown;
	BOOLEAN				bRfOnOffInt;// HW RF on/off interrupt 

	// Add Tx Feedback for SDIO, by Hana, 2015.02.10
	// TxFeedback Context Place Holder for Private Data: ---------------------------
	//	+ Only access this private data inside the module by TxFeedbackGetContext()	
	PRIVATE_DATA_ZONE 	TxFeedback[TX_FEEDBACK_SIZE_OF_CONTEXT];
	// ----------------------------------------------------------------------

	BOOLEAN				bHostSuspend;	// To check Host suspend status on FPGA platform, added by Roger, 2010.09.28.
	u2Byte				HostSuspendCnt; // Suspend counter for FPGA host, added by Roger, 2010.09.28.

	// 2010/11/22 MH Add for slim combo debug mode selective.
	// This is used for fix the drawback of CU TSMC-A/UMC-A cut. HW auto suspend ability. Close BT clock.
	BOOLEAN				SlimComboDbg;

 	u1Byte 				RTSInitRate;	 // 2010.11.24.by tynli.
	RT_WORK_ITEM			SetRTSRateWorkItem;

	RT_AMPDU_BRUST		AMPDUBurstMode; //92C maybe not use, but for compile successfully
	RT_AMPDU_BRUST		AMPDUBurstModebackup;
	u1Byte				AMPDUBurstNum;
	BOOLEAN				bAutoAMPDUBurstMode;
	u2Byte				AutoAMPDUBurstModeThreshold;
	
	u2Byte				TxHignTPThreshold;
	u2Byte				RxHignTPThreshold;
	
	u1Byte				SwBeaconType;	// The Beacon type defined by BACON_SEND_XX.

	BOOLEAN				bMACFuncEnable;

	IQK_MATRIX_REGS_SETTING		IQKMatrixRegSetting[IQK_Matrix_Settings_NUM];

	RT_WORK_ITEM			IQKTriggerWorkItem;
	RT_WORK_ITEM			LCKTriggerWorkItem;
	RT_WORK_ITEM			DPKTriggerWorkItem;	
	
	s4Byte				MinUndecoratedPWDBForDM;
	s4Byte				LastMinUndecoratedPWDBForDM;
	
	s1Byte				BufOfLinesAGC2_4G[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];
	s1Byte				BufOfLinesAGC5G[MAX_LINES_HWCONFIG_TXT][MAX_BYTES_LINE_HWCONFIG_TXT];

	s4Byte				nLinesReadAGC2_4G;
	s4Byte				nLinesReadAGC5G;	

//To reduce stack in phy_SwChnlStepByStep, 20110719 by sherry
	SwChnlCmd			PreCommonCmd[MAX_PRECMD_CNT];
	SwChnlCmd			PostCommonCmd[MAX_POSTCMD_CNT];
	SwChnlCmd			RfDependCmd[MAX_RFDEPENDCMD_CNT];

	BOOLEAN				bPathDiv_Enable;	//For 92D Non-interrupt Antenna Diversity by Neil ,add by wl.2011.07.19

	BOOLEAN 			bPreEdccaEnable; // 2011.11.25. by tynli.

	// FW common structure
	FW_COM_STR			fwComStr;

	//
	// 2011/09/22 MH Add for common PHy intergration interface.
	//
	DM_ODM_T				DM_OutSrc;
	BOOLEAN					bLinked;

	u1Byte			CurrentRARate;
	
	u1Byte			u1RsvdPageLoc[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1RsvdPageLoc2[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1RsvdPageLoc3[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1RsvdPageLocREALWOW[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1FwPatternRsvdPageLoc[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1RsvdPageLocScanOffload[MAX_H2C_CMD_DATA_SIZE];
	u1Byte			u1RsvdPageFcsLoc[MAX_H2C_CMD_DATA_SIZE];

	BOOLEAN 		bReInitLLTTable;

	u1Byte			SdioTxSequence;
	BOOLEAN				bSpurCalComplete;	// <20130515, VincentLan> To make sure spur calibration be execute only once

	BOOLEAN				bPhyValueInitReady; 

	BOOLEAN				bNeedIQK;
	BOOLEAN				bNeedQueuePacketInIQKProgress; 
          
	u1Byte				ExternalPA_2G;
	u1Byte				ExternalLNA_2G;
	u1Byte				ExternalPA_5G;
	u1Byte				ExternalLNA_5G;
	u2Byte				TypeGLNA;
	u2Byte				TypeGPA;
	u2Byte				TypeALNA;
	u2Byte				TypeAPA;
	
	s1Byte 				TxBBSwing_2G;
	s1Byte 				TxBBSwing_5G;	
	
	u1Byte				RTSEN;
	
	u1Byte				CurrScanOffloadType; //Current scan offload type

	// <VincentL, 130102> add for RfBbGain
	u1Byte 				RfBbGain;
	// <VincentL, 131119> Add for KFree Feature Requested by RF David.
	u1Byte				RfKFreeEnable;

	// <VincentL, 131231> Add to determine IQK ON/OFF in certain case, Suggested by Cheng.
	u1Byte				IQK_MP_Switch;
	
	// <VincentL> for FW IQK time measurement 
	u8Byte				IQK_StartTimer;
	u8Byte				IQK_EndTimer;
	
	// Rate Adaptive Mask control
	BOOLEAN				bUseRAMask;

	u1Byte				u1ForcedIgiLb;			// forced IGI lower bound
	u4Byte				MacIdPktSleep;

	RT_ADCSMP			ADCSmp;
	RT_WORK_ITEM		ADCSmpWorkItem;

	u1Byte				RomDLFwEnable;
	u1Byte				bP2PInProcess;
	u1Byte				RxDetectorBackUp;

	BOOLEAN				bDropRxPacket;

	BB_INIT_REGISTER	RegForRecover[5];

	RT_WORK_ITEM		MacIdSleepWorkItem;

	BOOLEAN				bDynTxPwrTblLoadingInProgress;

	SMBIOS_DATA			SmbiosData;
	u4Byte				BackUp_BB_REG_4_2nd_CCA[3];
	BOOLEAN				bSWToBW40M;

	//8723D LPS 32K Close Power of IO
	LPS_DEEP_SLEEP_CONTEXT	LPSDeepSleepContext;	
}HAL_DATA_SDIO, *PHAL_DATA_SDIO;


#define HAL_DATA_TYPE		HAL_DATA_SDIO
#define PHAL_DATA_TYPE		PHAL_DATA_SDIO
#define GET_HAL_DATA(__pAdapter)	((HAL_DATA_TYPE *)((__pAdapter)->HalData))
#define IS_BOOT_FROM_EFUSE(_Adapter)	(!((PHAL_DATA_TYPE)(_Adapter->HalData))->EepromOrEfuse)

//================================================================================
//	Prototype of functions to export.
//================================================================================
//
//	SDIO Initialization and DeInitialization interfaces.
//
BOOLEAN
HalSdioAllocResource(
	IN	PADAPTER		pAdapter,
	IN	s4Byte			TxQueueCnt,
	IN	s4Byte			RxQueueCnt
	);

VOID
HalSdioFreeResource(
	IN	PADAPTER		pAdapter
	);

u1Byte
MapTxQueueToOutPipe(
	IN	PADAPTER	pAdapter,
	IN	u1Byte	SpecifiedQueueID
);


//
//	SDIO Rx Transfer
//
PSDIO_IN_CONTEXT 
HalSdioGetInContext( 
	IN	PADAPTER	pAdapter
	);

VOID 
HalSdioReturnInContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_IN_CONTEXT			pContext
	);


//
//	SDIO Tx Transfer
//
PSDIO_OUT_CONTEXT 
HalSdioGetTxContext( 
	IN	PADAPTER	pAdapter
	);

VOID 
HalSdioReturnTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	);

VOID
HalSdioTxComplete(
	IN	PADAPTER			pAdapter,
	IN	PSDIO_OUT_CONTEXT	pContext,
	IN	u4Byte				status
	);

VOID
HalSdioUdtDefSet(
	IN	PADAPTER 	pAdapter
	);

#endif//__INC_HALUSB_H
