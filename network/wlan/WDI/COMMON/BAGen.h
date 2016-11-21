#ifndef __INC_BAGEN_H
#define __INC_BAGEN_H

RT_STATUS
OnADDBAReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
	);

RT_STATUS
OnADDBARsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
    );

RT_STATUS
OnDELBA(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
    );

VOID
ResetBaEntry(
	PBA_RECORD		pBA
	);

VOID
ExtendBAEntry(
	PADAPTER		Adapter,
	PBA_RECORD		pBA
	);

VOID
TsInitAddBA(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTS,
	u1Byte			Policy,
	BOOLEAN			bOverwritePending
	);

VOID
TsInitDelBA(
	PADAPTER			Adapter,
	PTS_COMMON_INFO	pTsCommonInfo,
	TR_SELECT			TxRxSelect
	);

VOID
BaSetupTimeOut(
	PRT_TIMER		pTimer
	);

VOID
TxBaInactTimeout(
	PRT_TIMER		pTimer
	);

VOID
RxBaInactTimeout(
	PRT_TIMER		pTimer
	);

RT_STATUS
OnBAReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	OCTET_STRING	ospdu
	);

#endif
