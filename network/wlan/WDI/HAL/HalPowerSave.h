#ifndef __INC_HALPOWERSAVE_H
#define __INC_HALPOWERSAVE_H

//===========================================================
// Defines
//===========================================================

//===========================================================
// Enums
//===========================================================

//===========================================================
// Structures
// Driver context for LPS 32K Close IO Power
typedef struct _LPS_DEEP_SLEEP_CONTEXT
{
	BOOLEAN		bEnable;
	pu1Byte		pStaticConfigureFile;			
	pu1Byte	  	pDynamicConfigureFile;	
	u4Byte		ConfigurationFileOffset;
	u2Byte		ConfigureFileLengthForPTK;
	u2Byte		ConfigureFileLengthForGTK;
	BOOLEAN		bChangeTxBoundaryInProgress;
	BOOLEAN		bH2CSetPartialOffParam;
	BOOLEAN		bEnterLPSFail;
}LPS_DEEP_SLEEP_CONTEXT,*PLPS_DEEP_SLEEP_CONTEXT;
//===========================================================

//===========================================================
// Function Prototype
//===========================================================

VOID
SetFwClockOn(
	IN	PADAPTER		Adapter,
	IN	u1Byte		RpwmVal,
	IN	BOOLEAN		bNeedTurnoffClk
	);

VOID
SetFwPSAllOn(
	IN	PADAPTER		Adapter
	);

VOID
SetFwPSRfOn(
	IN	PADAPTER		Adapter
	);

VOID
SetFwPSRFOffLowPower(
	IN	PADAPTER		Adapter
	);

VOID
FwClockOffTimerCallback(
	IN	PRT_TIMER		pTimer
   	);


VOID
FwClockOffWorkItemCallback(
	IN PVOID			pContext
	);


u4Byte
FwLeisurePSLeave(
	IN	PADAPTER		Adapter,
	IN	PH2C_CMD_8192C	pPwrModeCmd
	);

u4Byte
FwLeisurePSEnter(
	IN	PADAPTER		Adapter,
	IN	PH2C_CMD_8192C	pPwrModeCmd
	);


BOOLEAN
FwLPS_IsInPSAndSupportLowPower(
	IN	PADAPTER		Adapter
	);

VOID
ForceInitFwClockOn(
	IN	PADAPTER		Adapter
	);

BOOLEAN
ForceInitFwClockOnWithACK(
	IN	PADAPTER		Adapter
	);

u4Byte
FwInactivePSEnterHandler(
	IN	PADAPTER		Adapter,
	IN	PH2C_CMD_8192C	pH2cCmd
	);

u4Byte
FwInactivePSLeaveHandler(
	IN	PADAPTER		Adapter,
	IN	PH2C_CMD_8192C	pH2cCmd
	);

VOID
FwInactivePSEnter(
	IN	PADAPTER	Adapter,
	IN	u1Byte		Mode
	);

VOID
FwInactivePSLeave(
	IN	PADAPTER	Adapter
	);

VOID 
FwLPSDeepSleepInit(
	IN PADAPTER Adapter
);

VOID 
FwLPSDeepSleepDeInit(
	IN PADAPTER Adapter
);

BOOLEAN
FwLPSCheckChangeTxBoundary(
	IN PADAPTER pAdapter
);

BOOLEAN
FwLPSCheckEnterLPSFail(
	IN PADAPTER pAdapter
);

#endif // end of __INC_HALPOWERSAVE_H
