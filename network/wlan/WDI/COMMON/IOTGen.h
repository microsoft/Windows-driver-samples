#ifndef __INC_IOTGEN_H
#define __INC_IOTGEN_H


BOOLEAN
IOTActIsDisableEDCATurbo(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
	);

BOOLEAN
IOTActIsEnableBETxOPLimit(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActIsNullDataPowerSaving(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActIsForcedRTSCTS(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc	
	);

BOOLEAN
IOTActWAIOTBroadcom(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActBcmRxFailRelink(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
	);

BOOLEAN
IOTActIsForcedCTS2Self(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActIsDisableTxPowerTraining(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActIsDisableProtectionMode(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

BOOLEAN
IOTActIsForcedDataRate(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

VOID
IOTActForcedDataRate(
	PADAPTER		Adapter
);

#endif
