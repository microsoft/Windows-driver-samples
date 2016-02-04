
s1Byte
PHY_GetTxPowerLimit(
	IN	PADAPTER			Adapter,
	IN	u4Byte				RegPwrTblSel,
	IN	BAND_TYPE			Band,
	IN	CHANNEL_WIDTH	Bandwidth,
	IN	u1Byte				RfPath,
	IN	u1Byte				DataRate,
	IN	u1Byte				Channel
	);

VOID
PHY_SetTxPowerLimit(
	IN	PDM_ODM_T			pDM_Odm,
	IN	pu1Byte				Regulation,
	IN	pu1Byte				Band,
	IN	pu1Byte				Bandwidth,
	IN	pu1Byte				RateSection,
	IN	pu1Byte				RfPath,
	IN	pu1Byte 			Channel,
	IN	pu1Byte				PowerLimit
	);

VOID 
PHY_ConvertTxPowerLimitToPowerIndex(
	IN	PADAPTER			Adapter
	);

VOID
PHY_InitTxPowerLimit(
	IN	PDM_ODM_T			pDM_Odm
	);