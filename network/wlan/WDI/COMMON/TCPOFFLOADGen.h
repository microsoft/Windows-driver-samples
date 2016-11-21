#include "Mp_Precomp.h"

#if TCP_OFFLOAD_SUPPORT	
//
//  Description:
//			Get Register or Hal info to Set up Support TCP-Offload Mode 
VOID
TCP_OFFLOAD_Init(
	IN	PADAPTER				pAdapter 	
);

//
//  Description:
//			Set Current by OID or User mode 
//
//  Note :
//			PRT_SUPPORT_TCPOFFLOAD_CAP "Must" call  TCP_OFFLOAD_CheckSupport  before call TCP_OFFLOAD_SetCap

VOID
TCP_OFFLOAD_SetCap(
	IN	PADAPTER						pAdapter,
	IN	PRT_SUPPORT_TCPOFFLOAD_CAP	pSetSupTCPCap
);

//
// Description:
//			Check support or not !!
//
BOOLEAN
TCP_OFFLOAD_CheckSupport(
	IN	PADAPTER						pAdapter,
	IN	PRT_SUPPORT_TCPOFFLOAD_CAP	pSetSupTCPCap
);

#else // #if !TCP_OFFLOAD_SUPPORT

#define TCP_OFFLOAD_Init(_pAdapter)
#define TCP_OFFLOAD_SetCap(_pAdapter, _pSetSupTCPCap)
#define TCP_OFFLOAD_CheckSupport(_pAdapter, _pSetSupTCPCap)	FALSE

#endif // #if TCP_OFFLOAD_SUPPORT