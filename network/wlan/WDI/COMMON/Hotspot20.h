#ifndef __INC_HOTSPOT20_H
#define __INC_HOTSPOT20_H


RT_STATUS
GAS_OnInitReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
GAS_OnInitRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
GAS_OnComebackReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
GAS_OnComebackRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

#endif // #ifndef __INC_HOTSPOT20_H

