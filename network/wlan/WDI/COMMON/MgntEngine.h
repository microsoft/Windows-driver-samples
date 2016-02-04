#ifndef __INC_MGNTENGINE_H
#define __INC_MGNTENGINE_H

#define CopyMem(dest,src,len)                           \
		{ \
			if( src != 0 ){ \
				PlatformMoveMemory( (PVOID)(dest), (PVOID)(src), len ); \
			} \
		}

#define CopyMemOS(dest,src,len)                                                 \
		{ \
			if( (src).Octet != 0 ){ \
				PlatformMoveMemory( (PVOID)((dest)->Octet), (PVOID)((src).Octet), len ); \
				(dest)->Length = len; \
			} \
        }

enum StateErr		// requests disassoc or deauth (MmIndicate signal)
{
	noerr  = 1,
	class2 = 2,
	class3 = 3,
};





enum Synchronization_Sta_State{
	STATE_Min               = 0,
	STATE_No_Bss            = 1,
	STATE_Bss               = 2,
	STATE_Ibss_Active       = 3,
	STATE_Ibss_Idle         = 4,
	STATE_Act_Receive       = 5,
	STATE_Pas_Listen        = 6,
	STATE_Act_Listen        = 7,
	STATE_Join_Wait_Beacon  = 8,
	STATE_Max               = 9,
};




VOID
MlmeDeauthenticateRequest(
	PADAPTER		Adapter,
	pu1Byte			auSta,
	u1Byte			asRsn
	);


VOID
MlmeDisassociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asSta,
	u1Byte			asRsn
	);


// ----------------------------------
VOID
Mgnt_Indicate(
	PADAPTER			Adapter,
	PRT_RFD				pRfd,
	u1Byte				error_code
	);

RT_STATUS
EncapDataFrame_ParsePkt(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD		pRfd
	);

VOID
CopyWlanBss(
	IN	PRT_WLAN_BSS	dest,
	IN	PRT_WLAN_BSS	src
	);

#endif // #ifndef __INC_MGNTENGINE_H
