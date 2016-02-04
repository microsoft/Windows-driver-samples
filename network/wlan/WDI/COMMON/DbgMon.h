#ifndef	__DBG_MON_H__
#define __DBG_MON_H__

#define	RT_SPRINTF	rsprintf
#define	RT_PRINT(_MSG_) 		EXdbgmon_Print(_MSG_)

#define	DBGM_CLI_BUF_SIZE	100

// OUTPUT DIRECTION
#define	DBGM_OUT_WPP		BIT0
#define	DBGM_OUT_CLI		BIT1


#define	DBGM_COMP_WIFI_BASIC			BIT0
#define	DBGM_COMP_NDIS					BIT1
#define	DBGM_COMP_TX_INFO				BIT2
#define	DBGM_COMP_RX_INFO				BIT3
#define	DBGM_COMP_RX_RATE				BIT4
#define	DBGM_COMP_BT_COEX				BIT5
#define	DBGM_COMP_BT_HS					BIT6
#define	DBGM_COMP_BT_HCI				BIT7
#define	DBGM_COMP_WOL					BIT8
#define	DBGM_COMP_PWR_SVG				BIT9
#define	DBGM_COMP_ANTDET_INFO			BIT10
#define	DBGM_COMP_SKU					BIT11
#define	DBGM_COMP_DYN_TXPWR_LOAD		BIT12
#define	DBGM_COMP_CHANNELPLAN			BIT13
#define DBGM_COMP_TX_RATE				BIT14
#define	DBGM_COMP_TXPWR_DUMP			BIT15



#define	MAX_WOL_DBG_REG_NUM			20

typedef enum _DBGM_CALLER_{
	DBGM_CALLER_DBG_CMD					= 0x0,
	DBGM_CALLER_AUTO					= 0x1,
	DBGM_CALLER_MAX	
}DBGM_CALLER,*PDBGM_CALLER;


typedef enum _DBGM_WIFI_BW_MODE{
	DBGM_WIFI_BW_LEGACY					= 0x0,
	DBGM_WIFI_BW_HT20					= 0x1,
	DBGM_WIFI_BW_HT40					= 0x2,
	DBGM_WIFI_BW_HT80					= 0x3,
	DBGM_WIFI_BW_HT160					= 0x4,
	DBGM_WIFI_BW_MAX	
}DBGM_WIFI_BW_MODE,*PDBGM_WIFI_BW_MODE;

typedef enum _DBGM_WIFI_TRAFFIC_DIR{
	DBGM_WIFI_TRAFFIC_TX					= 0x0,
	DBGM_WIFI_TRAFFIC_RX					= 0x1,
	DBGM_WIFI_TRAFFIC_MAX	
}DBGM_WIFI_TRAFFIC_DIR,*PDBGM_WIFI_TRAFFIC_DIR;

typedef enum _DBGM_PWR_MODE_{
	DBGM_PWR_DC_MODE						= 0x0,
	DBGM_PWR_AC_MODE						= 0x1,
	DBGM_PWR_MODE_MAX	
}DBGM_PWR_MODE,*PDBGM_PWR_MODE;

typedef enum _DBGM_WIFI_FREQ_{
	DBGM_WIFI_FREQ_24G					= 0x0,
	DBGM_WIFI_FREQ_5G					= 0x1,
	DBGM_WIFI_FREQ_MAX	
}DBGM_WIFI_FREQ,*PDBGM_WIFI_FREQ;


typedef struct _DBGM_BASIC_WIFI_INFO_{	
	u2Byte				vid;
	u2Byte				did;
	u2Byte				svid;
	u2Byte				smid;
	u1Byte				customerId;
	u1Byte				macAddr[6];
	
	u2Byte				apNum;
	u1Byte				chnl;
	u4Byte				bandwidth;
	u1Byte				freq;
	s4Byte				rssi;
	u1Byte				channelPlan;
	u1Byte				regulation_24G;
	u1Byte				regulation_5G;
	u4Byte				odmAbility;
	u1Byte				regAdaptivity;

	BOOLEAN				bScanProcess;
	BOOLEAN				bLinkProcess;
	BOOLEAN				bRoamProcess;
	BOOLEAN				bStaConnected;
	BOOLEAN				bApConnected;
	BOOLEAN				bBtHsConnected;
	BOOLEAN				bP2pConnected;

	u1Byte				portActivated[5];
	BOOLEAN				bWifiFwLoaded;
	u4Byte				wifiFwVer;
	u1Byte				wifiFwLoadType;

	// traffic
	BOOLEAN				bWifiBusy;
	u1Byte				wifiTrafficDir;

	BOOLEAN				bBtExist;
	BOOLEAN				bBtStackNotified;
	u2Byte				hciVer;

	// power save related
	BOOLEAN				powerMode;		//AC/DC mode
	u1Byte				regIpsMode;
	u1Byte				regLpsMode;
	BOOLEAN				bRegLowPowerEnable;
	BOOLEAN				bInactivePs;
	BOOLEAN				bLeisurePs;
	BOOLEAN				bLowPowerEnable;
	BOOLEAN				bUnderIps;
	BOOLEAN				bUnderLps;
	BOOLEAN				bLowPower;		// 32k low power
	BOOLEAN				bWowlanLPS;
	BOOLEAN				bWowlan32k;

	// ap type info
	u1Byte				apType;		
	u4Byte				numTxBcnOk;
	u4Byte				numTxBcnErr;
	u4Byte				numTxBcnUpdate;

	// connection debug control
	BOOLEAN				bFixBssid;
	u1Byte				fixedBssid[6];
} DBGM_BASIC_WIFI_INFO, *PDBGM_BASIC_WIFI_INFO;

typedef struct _DBGM_TX_INFO_{
	u4Byte			numTxInterrupt;
	u4Byte			wrongrwTxpointerCnt;
	u4Byte			numIdleTcb;
} DBGM_TX_INFO, *PDBGM_TX_INFO;

typedef struct _DBGM_RX_INFO_{
	u4Byte			numRxInterrupt;
	u1Byte			rxReorderPendTime;
	u2Byte			regRxReorderPendTime;
	u4Byte			rxBeaconNum;
	u4Byte			cntOfdmCca;
	u4Byte			cntCckCca;
	u4Byte			cntOfdmFail;
	u4Byte			cntCckFail;
	u4Byte			rxReorderIndEnterCnt;
	u4Byte			rxReorderIndAllowCnt;
	u4Byte			rxReorderIndRejectCnt[3];
} DBGM_RX_INFO, *PDBGM_RX_INFO;

typedef struct _DBGM_WOL_INFO_{
	BOOLEAN			bPowerOn;
	BOOLEAN			bWolFWReady;
	u2Byte			WolFwVer;
	u2Byte			WolFwSubVer;
	BOOLEAN			bARPOffload;
	BOOLEAN			bNSOffload;
	BOOLEAN			bGTKOffload;
	BOOLEAN			bRegARPOffload;
	BOOLEAN			bRegNSOffload;
	BOOLEAN			bRegGTKOffload;
	BOOLEAN			bPtlOffload;
	u1Byte			WoLPatternNum;
	u1Byte			WoLPktNoPtnNum;
	BOOLEAN			bDownloadWoWLANFWOK;
	BOOLEAN			bEnterWoWLANLPS;
	BOOLEAN			bEnterDxIPS;
	BOOLEAN			bRegPnpKeepLink;
	BOOLEAN			bPerformPnpReconnect;

	// Security info
	u4Byte			GroupEnAlgo;
	u8Byte			PreTxIV;
	u8Byte			PreKeyRelayCounter;

	// NLO related info
	BOOLEAN			bDxNLOEnable;
	u1Byte			NLONumberSetToFw;
	u4Byte			NLOFastScanPeriod;
	u4Byte			NLOFastScanIterations;
	u4Byte			NLOSlowScanPeriod;
	RT_OFFLOAD_NETWORK 	OffloadNetworkList[NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE];

	// Wake up reason and counter
	u1Byte			cntRxDescPtn;
	u1Byte			cntRxDescMgc;
	u1Byte			cntRxDescUct;
	u1Byte			wakeUpReason[25];
	u4Byte			WakeByWoLEventCnt;
	u4Byte			WakeByUnknownReasonCnt;

	// PNP sleep/wake related info.
	u4Byte			PnpSleepEnterD2Cnt;
	u4Byte			PnpSleepEnterD3Cnt;
	u4Byte			PnpSleepEnterUnknownDxCnt;
	u4Byte			PnpSleepDxTotalCnt;
	u4Byte			PnpWakeD0Cnt;
	u8Byte			LastPnpSleepTime;
	u8Byte			LastPnpWakeTime;

	// AOAC report
	RT_AOAC_REPORT	AOACReport;

	// HW debug registers
	HW_REG_VALUE	WoLMacReg[MAX_WOL_DBG_REG_NUM];
} DBGM_WOL_INFO, *PDBGM_WOL_INFO;

// RT DEBUG CONTROL
typedef struct _RT_DBG_MON_{
	u1Byte					outDirection;		// control output to WPP or CLI.
	u4Byte					outComponents;		// what components we need to output

	DBGM_BASIC_WIFI_INFO	basicWifiInfo;		// basic wifi information
	DBGM_TX_INFO			txInfo;				// tx info
	DBGM_RX_INFO			rxInfo;				// rx info
	DBGM_WOL_INFO			wolInfo;			// WoWLAN info
} RT_DBG_MON, *PRT_DBG_MON;

extern const char *const rateStr[];

VOID
EXdbgmon_ClearWolInfo(
	);
VOID
EXdbgmon_Print(
	const char 		*pMsg
	);
VOID
EXdbgmon_SetOutComponents(
	u4Byte	outCompBitsDefine
	);

VOID
EXdbgmon_SetWolCntRxDesc(
	IN	u1Byte		WakeType
	);

VOID
EXdbgmon_GetWolBeforeSleepInfo(
	IN	PADAPTER	Adapter
	);

VOID
EXdbgmon_GetWolPowerInfo(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bPowerOn
	);

VOID
EXdbgmon_GetWolWakeReasonInfo(
	IN	PADAPTER	Adapter
	);

VOID
EXdbgmon_ShowInfo(
	IN	PADAPTER	Adapter,
	IN	u1Byte		caller,
	IN	u4Byte		showType
	);
VOID
EXdbgmon_AutoCollectStatisticsInfo(
	IN	PADAPTER 	Adapter
	);



#endif	/* __DBG_MON_H__ */
