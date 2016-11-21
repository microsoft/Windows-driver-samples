#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Hotspot20.tmh"
#endif

#include "Hotspot20.h"

RT_STATUS
GAS_OnInitReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS	RtStatus = RT_STATUS_SUCCESS;
	pu1Byte		pOUI = NULL;
	
	FunctionIn(COMP_MLME);

	PlatformIndicateActionFrame(pAdapter, (PVOID)posMpdu);
	
	pOUI = Frame_GAS_QueryReq_OUI(*posMpdu);
	RT_PRINT_DATA(COMP_MLME, DBG_LOUD, ("GAS Initial request: "), pOUI, 3);
	if( PlatformCompareMemory(pOUI, WFA_OUI, SIZE_OUI) == 0 )
	{
		pOUI = Frame_GAS_QueryReq_Type(*posMpdu);

		if(0x09 == *pOUI)
			P2P_OnSDReq(pAdapter, pRfd, posMpdu);
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("No matched OUI: %2x:%2x:%2x\n", pOUI[0], pOUI[1], pOUI[2]));
	}

	FunctionOut(COMP_MLME);
	return RtStatus;
}

RT_STATUS
GAS_OnInitRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS	RtStatus = RT_STATUS_SUCCESS;
	pu1Byte		pOUI = NULL;
	
	FunctionIn(COMP_MLME);

	PlatformIndicateActionFrame(pAdapter, (PVOID)posMpdu);

	pOUI = Frame_GAS_QueryRsp_OUI(*posMpdu);
	RT_PRINT_DATA(COMP_MLME, DBG_LOUD, ("GAS Initial response: "), pOUI, 3);
	if( PlatformCompareMemory(pOUI, WFA_OUI, SIZE_OUI) == 0 )
	{
		pOUI = Frame_GAS_QueryRsp_Type(*posMpdu);
		
		if(0x09 == *pOUI)
			P2P_OnSDRsp(pAdapter, pRfd, posMpdu);
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("No matched OUI: %2x:%2x:%2x\n", pOUI[0], pOUI[1], pOUI[2]));
	}
	
	FunctionOut(COMP_MLME);
	return RtStatus;
}

RT_STATUS
GAS_OnComebackReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS	RtStatus = RT_STATUS_SUCCESS;
	pu1Byte		pOUI = NULL;
	
	FunctionIn(COMP_MLME);

	PlatformIndicateActionFrame(pAdapter, (PVOID)posMpdu);
	
	pOUI = Frame_GAS_ComebackRsp_OUI(*posMpdu);
	if( PlatformCompareMemory(pOUI, WFA_OUI, SIZE_OUI) == 0 )
	{
		pOUI = Frame_GAS_ComebackRsp_Type(*posMpdu);
		
		if(0x09 == *pOUI)
			P2P_OnSDComebackReq(pAdapter, pRfd, posMpdu);
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("No matched OUI: %2x:%2x:%2x\n", pOUI[0], pOUI[1], pOUI[2]));
	}
	
	FunctionOut(COMP_MLME);
	return RtStatus;
}

RT_STATUS
GAS_OnComebackRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS	RtStatus = RT_STATUS_SUCCESS;
	pu1Byte		pOUI = NULL;
	
	FunctionIn(COMP_MLME);

	PlatformIndicateActionFrame(pAdapter, (PVOID)posMpdu);
	
	pOUI = Frame_GAS_ComebackRsp_OUI(*posMpdu);
	if( PlatformCompareMemory(pOUI, WFA_OUI, SIZE_OUI) == 0 )
	{
		pOUI = Frame_GAS_ComebackRsp_Type(*posMpdu);
		
		if(0x09 == *pOUI)
			P2P_OnSDComebackRsp(pAdapter, pRfd, posMpdu);
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("No matched OUI: %2x:%2x:%2x\n", pOUI[0], pOUI[1], pOUI[2]));
	}
	
	FunctionOut(COMP_MLME);
	return RtStatus;
}

