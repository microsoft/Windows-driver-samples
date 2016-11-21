
u1Byte
PHY_GetTxPowerByRateBase(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Band,
	IN	u1Byte			RfPath,
	IN  u1Byte			TxNum,
	IN	RATE_SECTION	RateSection
	);

u1Byte
PHY_GetRateSectionIndexOfTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	);


VOID
PHY_GetRateValuesOfTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Value,
	OUT	pu1Byte		RateIndex,
	OUT	ps1Byte		PwrByRateVal,
	OUT	pu1Byte		RateNum
	);

u1Byte
PHY_GetRateIndexOfTxPowerByRate(
	IN	u1Byte	Rate
	);

VOID 
PHY_SetTxPowerIndexByRateSection(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				RFPath,	
	IN	u1Byte				Channel,
	IN	u1Byte				RateSection
	);


s1Byte
PHY_GetTxPowerByRate( 
	IN	PADAPTER		pAdapter, 
	IN	u1Byte			Band, 
	IN	u1Byte			RFPath, 
	IN	u1Byte			TxNum, 
	IN  u1Byte			RateIndex
	);

VOID
PHY_SetTxPowerByRate( 
	IN	PADAPTER		pAdapter, 
	IN	u1Byte			Band, 
	IN	u1Byte			RFPath, 
	IN	u1Byte			TxNum, 
	IN  u1Byte			Rate,
	IN	s1Byte			Value
	);

VOID
PHY_SetTxPowerLevelByPath(
	IN	PADAPTER		Adapter,
	IN	u1Byte			channel,
	IN	u1Byte			path
	);

VOID 
PHY_SetTxPowerIndexByRateArray(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				RFPath,
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u1Byte				Channel,
	IN	pu1Byte				Rates,
	IN	u1Byte				RateArraySize
	);

VOID
PHY_InitTxPowerByRate(
	IN	PADAPTER	pAdapter
	);

VOID
PHY_StoreTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		Band,
	IN	u4Byte		RfPath,
	IN	u4Byte		TxNum,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN  u4Byte		Data
	);


VOID
PHY_TxPowerByRateConfiguration(
	IN  PDM_ODM_T	 pDM_Odm
	);

u1Byte
PHY_GetTxPowerIndexBase(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				RFPath,
	IN	u1Byte				Rate,	
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u1Byte				Channel,
	OUT PBOOLEAN			bIn24G
	);

s1Byte
PHY_GetTxPowerTrackingOffset( 
	PADAPTER pAdapter,
	u1Byte	 RFPath,
	u1Byte	 Rate
	);


