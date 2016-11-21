/*****************************************************************************
 *	Copyright(c) 2008,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:	__INC_HAL8188EPHYCFG_H
 *
 *
 * Note:	
 *			
 *
 * Export:	Constants, macro, functions(API), global variables(None).
 *
 * Abbrev:	
 *
 * History:
 *		Data		Who		Remark 
 *      08/07/2007  MHC    	1. Porting from 9x series PHYCFG.h.
 *							2. Reorganize code architecture.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef __INC_HAL8723BPHYCFG_H
#define __INC_HAL8723BPHYCFG_H


/*--------------------------Define Parameters-------------------------------*/
#define	IQK_DELAY_TIME_92E		10		//ms
#define	index_mapping_NUM_92E	15
#define 	AVG_THERMAL_NUM_92E	4

#define MAX_TXPWR_IDX_NMODE_92E	63

#define IQK_MAC_REG_NUM_92E	4
#define IQK_ADDA_REG_NUM_92E	16

#define HP_THERMAL_NUM_92E	8

#define IQK_BB_REG_NUM_92E	9

#define RF_PATH_MAX_92E		2


/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 


/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
//1   1. BB register R/W API

extern	u4Byte	
PHY_QueryBBReg8723B(	IN	PADAPTER	Adapter,
								IN	u4Byte		RegAddr,
								IN	u4Byte		BitMask	);


VOID	
PHY_SetBBReg8723B(	IN	PADAPTER	Adapter,
								IN	u4Byte		RegAddr,
								IN	u4Byte		BitMask,
								IN	u4Byte		Data	);


extern	u4Byte	
PHY_QueryRFReg8723B(	IN	PADAPTER			Adapter,
								IN	u1Byte			eRFPath,
								IN	u4Byte			RegAddr,
								IN	u4Byte			BitMask	);


void	
PHY_SetRFReg8723B(	IN	PADAPTER			Adapter,
								IN	u1Byte			eRFPath,
								IN	u4Byte				RegAddr,
								IN	u4Byte				BitMask,
								IN	u4Byte				Data	);

//1 3. Initial BB/RF config by reading MAC/BB/RF txt.
RT_STATUS
phy_BB8723B_Config_ParaFile(
	IN	PADAPTER	Adapter
	);

RT_STATUS
PHY_RFConfig8723B(
	IN	PADAPTER	Adapter	
	);

//
// RF Power setting
//
BOOLEAN	
PHY_SetRFPowerState8723B(			IN	PADAPTER			Adapter, 
											IN	RT_RF_POWER_STATE	eRFPowerState);

//1 5. Tx  Power setting API
VOID
PHY_SetTxPowerIndex_8723B(
	IN	PADAPTER			Adapter,
	IN	u4Byte				PowerIndex,
	IN	u1Byte				RFPath,	
	IN	u1Byte				Rate
	);

u1Byte
PHY_GetTxPowerIndex_8723B(
	IN	PADAPTER			pAdapter,
	IN  u1Byte  			RFPath,
	IN	u1Byte				Rate,	
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u1Byte				Channel
	);

VOID	
PHY_GetTxPowerLevel8723B(			
	IN	PADAPTER		Adapter,
	OUT ps4Byte    		powerlevel	
	);

VOID	
PHY_SetTxPowerLevel8723B(			
	IN	PADAPTER		Adapter,
	IN	u1Byte			channel	
	);

BOOLEAN
PHY_UpdateTxPowerDbm8723B(		IN	PADAPTER	Adapter,
											IN	s4Byte		powerInDbm	);

//1 5. BandWidth setting API

VOID
PHY_SetBWModeWorkItemCallback8723B(
	IN PVOID            pContext
	);


VOID	
PHY_SetBWModeTimerCallback8723B(	IN	PRT_TIMER		pTimer	);

VOID
PHY_HandleSetBWMode8723B(	IN	PADAPTER			pAdapter,
									IN	CHANNEL_WIDTH	ChnlWidth,
									IN	EXTCHNL_OFFSET	Offset	);


//1 6. Channel setting API

void	
PHY_SwChnlTimerCallback8723B(		IN	PRT_TIMER		pTimer	);

void	
PHY_HandleSwChnl8723B(		IN	PADAPTER		pAdapter,
									IN	u1Byte			channel	);

void	
PHY_SwChnlSynchronously8723B(	IN	PADAPTER		pAdapter,
									IN	u1Byte			channel	);
VOID
PHY_SwChnlWorkItemCallback8723B(
	IN PVOID            pContext
	);

#if 0

//1 7.	IQK

void	
PHY_IQCalibrate_8188E(	IN	PADAPTER	pAdapter,	
							IN	BOOLEAN 	bReCovery);


//
// LC calibrate
//
void	
PHY_LCCalibrate_8188E(		IN	PADAPTER	pAdapter);

//
// AP calibrate
//
void	
PHY_APCalibrate_8188E(		IN	PADAPTER	pAdapter,
							IN 	s1Byte		delta);
void	
PHY_DigitalPredistortion_8188E(		IN	PADAPTER	pAdapter);

#endif

extern VOID 
PHY_SetRFPathSwitch_8723B(	IN	PADAPTER	pAdapter,
							IN  BOOLEAN		bMain);

extern BOOLEAN 
PHY_QueryRFPathSwitch_8723B(	IN	PADAPTER	pAdapter);


void	
PHY_SetMonitorMode8723B(IN	PADAPTER	pAdapter,
										IN	BOOLEAN		bEnableMonitorMode	);

#if (USE_WORKITEM || PCI_USE_WATCHDOG_WI)
VOID
RtCheckForHangWorkItemCallback8723B(
                IN PVOID   pContext
);
#endif

VOID 
PHY_ScanOperationBackup8723B(	IN	PADAPTER	Adapter,
										IN	u1Byte		Operation	);



BOOLEAN 
HalSetIO8723B(					IN	PADAPTER			Adapter,
									IN	IO_TYPE				IOType);

VOID
PHY_HandleSwChnl8723B(	// Call after initialization
	IN	PADAPTER	pAdapter,
	IN	u1Byte		channel
	);


#if USE_WORKITEM	
void SetIOWorkItemCallback8723B( IN PVOID            pContext );
#endif
void SetIOTimerCallback8723B( IN PRT_TIMER		pTimer);



BOOLEAN
SetAntennaConfig8723B(
	IN	PADAPTER	Adapter,
	IN	u1Byte		DefaultAnt	
	);

RT_STATUS
PHY_SwitchWirelessBand8723B(
	IN PADAPTER		 Adapter,
	IN u1Byte		Band
	);

u4Byte PHY_GetTxBBSwing_8723B(
	IN	PADAPTER	Adapter,
	IN	BAND_TYPE 	Band,
	IN  u1Byte 		RFPath
	);

VOID
HAL_HandleSwChnl8723B(	// Call after initialization
	IN	PADAPTER	pAdapter,
	IN	u1Byte		channel
	);

VOID
PHY_HandleSwChnlAndSetBW8723B(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bSwitchChannel,
	IN	BOOLEAN				bSetBandWidth,
	IN	u1Byte				ChannelNum,
	IN	CHANNEL_WIDTH	ChnlWidth,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf40MHz,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf80MHz,
	IN	u1Byte				CenterFrequencyIndex1
	);

VOID
PHY_BB8723B_Config_1T(
	IN PADAPTER Adapter
	);

VOID
PHY_SwChnlAndSetBWModeCallback8723B(
	IN PVOID            pContext
	);

VOID
PHY_InitAntennaSelection8723B(
	IN PADAPTER Adapter
	);

VOID
PHY_SetIO_8723B(
    PADAPTER		pAdapter
    );

#endif	// __INC_HAL8188EPHYCFG_H

