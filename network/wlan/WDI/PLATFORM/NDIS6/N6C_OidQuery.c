#include "Mp_Precomp.h"


//
// Translate RT_AUTH_MODE to DOT11_AUTH_ALGORITHM.
// Added by Annie, 2006-10-19.
//
DOT11_AUTH_ALGORITHM
N6CAuthModeToDot11(
	IN	PVOID						pAuthMode
	)
{
	RT_AUTH_MODE			AuthMode = *((PRT_AUTH_MODE)pAuthMode);
	DOT11_AUTH_ALGORITHM	dot11AuthAlg;

	RT_TRACE( COMP_SEC, DBG_LOUD, ("==> N6CAuthModeToDot11(): AuthMode=0x%X\n", AuthMode) );

	switch(AuthMode)
	{
		case RT_802_11AuthModeOpen:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_80211_OPEN\n"));
			break;
			
		case RT_802_11AuthModeShared:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_SHARED_KEY;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_80211_SHARED_KEY\n"));
			break;
			
		case RT_802_11AuthModeAutoSwitch:
			// No such case in DOT11_AUTH_ALGO_XXX!!
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthModeToDot11(): RT_802_11AuthModeAutoSwitch => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n"));
			break;
			
		case RT_802_11AuthModeWPA:
			dot11AuthAlg = DOT11_AUTH_ALGO_WPA;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_WPA\n"));
			break;
			
		case RT_802_11AuthModeWPAPSK:
			dot11AuthAlg = DOT11_AUTH_ALGO_WPA_PSK;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_WPA_PSK\n"));
			break;
			
		case RT_802_11AuthModeWPANone:
			// No such case in DOT11_AUTH_ALGO_XXX!!
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthModeToDot11(): RT_802_11AuthModeWPANone => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n"));
			break;
			
		case RT_802_11AuthModeWPA2:
			dot11AuthAlg = DOT11_AUTH_ALGO_RSNA;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_RSNA\n"));
			break;
			
		case RT_802_11AuthModeWPA2PSK:
			dot11AuthAlg = DOT11_AUTH_ALGO_RSNA_PSK;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthModeToDot11(): DOT11_AUTH_ALGO_RSNA_PSK\n"));
			break;
			
		case RT_802_11AuthModeMax:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthModeToDot11(): RT_802_11AuthModeMax => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n"));
			break;
			
		case RT_802_11AuthModeCCKM:
			dot11AuthAlg = DOT11_AUTH_ALGO_CCKM;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthModeToDot11(): RT_802_11AuthModeMax => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n"));
			break;
						
		default:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthModeToDot11(): unknow authmode: 0x%X => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n", AuthMode));
			break;
	}

	RT_TRACE( COMP_SEC, DBG_LOUD, ("<== N6CAuthModeToDot11(): dot11AuthAlg=0x%X\n", dot11AuthAlg) );

	return	dot11AuthAlg;
}



//
// Translate RT_AUTH_MODE to DOT11_AUTH_ALGORITHM.
// Added by Annie, 2006-10-19.
//
DOT11_AUTH_ALGORITHM
N6CAuthAlgToDot11(
	IN	PVOID						pAuthMode
	)
{
	AUTH_ALGORITHM			AuthMode = *((PAUTH_ALGORITHM)pAuthMode);
	DOT11_AUTH_ALGORITHM	dot11AuthAlg;

	RT_TRACE( COMP_SEC, DBG_LOUD, ("==> N6CAuthAlgToDot11(): AuthMode=0x%X\n", AuthMode) );

	switch(AuthMode)
	{
		case OPEN_SYSTEM:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthAlgToDot11(): DOT11_AUTH_ALGO_80211_OPEN\n"));
			break;
			
		case SHARED_KEY:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_SHARED_KEY;
			RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CAuthAlgToDot11(): DOT11_AUTH_ALGO_80211_SHARED_KEY\n"));
			break;
			
		default:
			dot11AuthAlg = DOT11_AUTH_ALGO_80211_OPEN;
			RT_TRACE(COMP_SEC, DBG_WARNING, ("N6CAuthAlgToDot11(): unknow authmode: 0x%X => DOT11_AUTH_ALGO_80211_OPEN (WARNING!!!)\n", AuthMode));
			break;
	}

	RT_TRACE( COMP_SEC, DBG_LOUD, ("<== N6CAuthAlgToDot11(): dot11AuthAlg=0x%X\n", dot11AuthAlg) );

	return	dot11AuthAlg;
}


//
// Translate RT_ENC_ALG to DOT11_CIPHER_ALGORITHM.
// Added by Annie, 2006-10-19.
//
DOT11_CIPHER_ALGORITHM
N6CEncAlgorithmToDot11(
	IN	PVOID						pEncAlgorithm
	)
{
	RT_ENC_ALG					EncAlgorithm = *((PRT_ENC_ALG)pEncAlgorithm);
	DOT11_CIPHER_ALGORITHM		dot11CipherAlg;
	
	switch( EncAlgorithm )
	{
		case RT_ENC_ALG_NO_CIPHER:
			dot11CipherAlg = DOT11_CIPHER_ALGO_NONE;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_NONE\n") );
			break;

		case RT_ENC_ALG_WEP40:
			dot11CipherAlg = DOT11_CIPHER_ALGO_WEP40;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_WEP40\n") );
			break;

		case RT_ENC_ALG_TKIP:
			dot11CipherAlg = DOT11_CIPHER_ALGO_TKIP;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_TKIP\n") );
			break;

		case RT_ENC_ALG_AESCCMP:
			dot11CipherAlg = DOT11_CIPHER_ALGO_CCMP;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_CCMP\n") );
			break;

		case RT_ENC_ALG_WEP104:
			dot11CipherAlg = DOT11_CIPHER_ALGO_WEP104;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_WEP104\n") );
			break;

		case RT_ENC_ALG_WEP:
			dot11CipherAlg = DOT11_CIPHER_ALGO_WEP;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_LOUD, ("N6CEncAlgorithmToDot11(): DOT11_CIPHER_ALGO_WEP\n") );
			break;	

		default:
			dot11CipherAlg = DOT11_CIPHER_ALGO_NONE;
			RT_TRACE( COMP_SEC|COMP_MLME, DBG_WARNING, ("N6CEncAlgorithmToDot11(): Unknown PairwiseEncAlgorithm 0x%08X => DOT11_CIPHER_ALGO_NONE\n", EncAlgorithm) );
			break;
	}

	return	dot11CipherAlg;
}


//
// Implementation of OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM querying for NDIS6.
// Added by Annie, 2006-10-10.
// (reference MpQueryEnabledAuthenticationAlgorithm() and Sta11QueryEnabledAuthenticationAlgorithm() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
	IN	PADAPTER					Adapter,
	OUT	PVOID						InformationBuffer,
	IN	ULONG						InformationBufferLength,
	OUT	PULONG						BytesWritten,
	OUT	PULONG						BytesNeeded
	)
{
	PDOT11_AUTH_ALGORITHM_LIST		pAuthAlgoList = (PDOT11_AUTH_ALGORITHM_LIST)InformationBuffer;
	RT_AUTH_MODE					authmode;

	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_ENABLED_AUTHENTICATION_ALGORITHM()\n") );

	*BytesNeeded = sizeof(DOT11_AUTH_ALGORITHM_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		if ( InformationBufferLength >= (ULONG)FIELD_OFFSET(DOT11_AUTH_ALGORITHM_LIST, AlgorithmIds) )
		{
			pAuthAlgoList->uNumOfEntries = 0;
			pAuthAlgoList->uTotalNumOfEntries = 1;
		}
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
	
	PlatformZeroMemory( pAuthAlgoList, *BytesNeeded );

	N6_ASSIGN_OBJECT_HEADER(
			pAuthAlgoList->Header, 
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_AUTH_ALGORITHM_LIST_REVISION_1,
			sizeof(DOT11_AUTH_ALGORITHM_LIST));

	pAuthAlgoList->uNumOfEntries = 1;
	pAuthAlgoList->uTotalNumOfEntries = 1;

	MgntActQuery_802_11_AUTHENTICATION_MODE( Adapter, &authmode );
	pAuthAlgoList->AlgorithmIds[0] = N6CAuthModeToDot11( &authmode );
	*BytesWritten = *BytesNeeded;
	*BytesNeeded = 0;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_ENABLED_AUTHENTICATION_ALGORITHM():0x%X\n", pAuthAlgoList->AlgorithmIds[0]) );
	return	NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM querying for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11QueryEnabledUnicastCipherAlgorithm() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_SECURITY_T		pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	PDOT11_CIPHER_ALGORITHM_LIST	pCipherAlgoList = (PDOT11_CIPHER_ALGORITHM_LIST)InformationBuffer;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM()\n") );

	*BytesNeeded = sizeof(DOT11_CIPHER_ALGORITHM_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	PlatformZeroMemory( pCipherAlgoList, *BytesNeeded );

	N6_ASSIGN_OBJECT_HEADER(
			pCipherAlgoList->Header, 
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_CIPHER_ALGORITHM_LIST_REVISION_1,
			sizeof(DOT11_CIPHER_ALGORITHM_LIST));

	pCipherAlgoList->uNumOfEntries = 1;
	pCipherAlgoList->uTotalNumOfEntries = 1;
	pCipherAlgoList->AlgorithmIds[0] = N6CEncAlgorithmToDot11(&(pSecInfo->PairwiseEncAlgorithm));
	*BytesWritten = *BytesNeeded;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM():0x%X\n", pCipherAlgoList->AlgorithmIds[0]) );
	return	NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM querying for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11QueryEnabledMulticastCipherAlgorithm() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_SECURITY_T		pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	PDOT11_CIPHER_ALGORITHM_LIST	pCipherAlgoList = (PDOT11_CIPHER_ALGORITHM_LIST)InformationBuffer;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM()\n") );

	*BytesNeeded = sizeof(DOT11_CIPHER_ALGORITHM_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	PlatformZeroMemory( pCipherAlgoList, *BytesNeeded );

	N6_ASSIGN_OBJECT_HEADER(
			pCipherAlgoList->Header, 
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_CIPHER_ALGORITHM_LIST_REVISION_1,
			sizeof(DOT11_CIPHER_ALGORITHM_LIST));

	pCipherAlgoList->uNumOfEntries = 1;
	pCipherAlgoList->uTotalNumOfEntries = 1;
	pCipherAlgoList->AlgorithmIds[0] = N6CEncAlgorithmToDot11(&(pSecInfo->GroupEncAlgorithm));
	*BytesWritten = *BytesNeeded;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM():0x%X\n", pCipherAlgoList->AlgorithmIds[0]) );
	return	NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR querying for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11QuerySupportedUnicastAlgorithmPair() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	static DOT11_AUTH_CIPHER_PAIR UcastAuthCipherList[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_CCMP},	//14
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP40}, 	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 3
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP104},	// 4
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 5
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP},	// 6
		{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_TKIP},	// 7
		{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_TKIP},	// 8
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_TKIP},	// 9
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_TKIP},	//10
		{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_CCMP},	//11
		{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_CCMP},	//12
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_TKIP	},    //15
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_CCMP},   //16

		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPCCMP	},    //17
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPTKIP},   //18
		
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPCCMP},   //19
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPTKIP},   //20
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
	};

	static DOT11_AUTH_CIPHER_PAIR UcastAuthCipherListWiFiCfg[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_CCMP},	//14
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP40}, 	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 3
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP104},	// 4
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 5
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP},	// 6
		//{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_TKIP},	// 7
		//{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_TKIP},	// 8
		//{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_TKIP},	// 9
		//{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_TKIP},	//10
		//{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_CCMP},	//11
		//{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_CCMP},	//12
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_TKIP	},    //15
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_CCMP},   //16

		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPCCMP	},    //17
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPTKIP},   //18
		
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPCCMP},   //19
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPTKIP},   //20
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
	};

	static DOT11_AUTH_CIPHER_PAIR  UcastAuthCipherListIBSS[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 3
		{DOT11_AUTH_ALGO_RSNA_PSK,			       DOT11_CIPHER_ALGO_CCMP},	//4//4
	};
	
	u4Byte	ulNumOfPairSupported = 0;
	u4Byte	TotalLength = 0;
	PDOT11_AUTH_CIPHER_PAIR_LIST pAuthCipherList = (PDOT11_AUTH_CIPHER_PAIR_LIST)InformationBuffer;
	u4Byte	i;
	u4Byte	FillLength = 0;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR()\n") );
	
	if( Adapter->MgntInfo.Regdot11networktype != RT_JOIN_NETWORKTYPE_ADHOC )
	{
		if(Adapter->MgntInfo.bWiFiConfg)
		{
			ulNumOfPairSupported = sizeof(UcastAuthCipherListWiFiCfg)/sizeof(DOT11_AUTH_CIPHER_PAIR);
			TotalLength = sizeof(UcastAuthCipherListWiFiCfg) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		}
		else
		{
			ulNumOfPairSupported = sizeof(UcastAuthCipherList)/sizeof(DOT11_AUTH_CIPHER_PAIR);
			TotalLength = sizeof(UcastAuthCipherList) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		}
		if ( InformationBufferLength < sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) )
		{
			*BytesNeeded = TotalLength;
			RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_TOO_SHORT\n") );
			return NDIS_STATUS_BUFFER_OVERFLOW;
		}
		PlatformZeroMemory( pAuthCipherList, sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) );
		N6_ASSIGN_OBJECT_HEADER(
				pAuthCipherList->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_AUTH_CIPHER_PAIR_LIST_REVISION_1,
				sizeof(DOT11_AUTH_CIPHER_PAIR_LIST));
		FillLength += FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		
		pAuthCipherList->uNumOfEntries = 0;
		pAuthCipherList->uTotalNumOfEntries = ulNumOfPairSupported;
		for (i=0; i<ulNumOfPairSupported; i++)
		{
			if (FillLength + sizeof(DOT11_AUTH_CIPHER_PAIR) > InformationBufferLength)
			{
				ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
				RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
				break;
			}

			pAuthCipherList->uNumOfEntries ++;
			FillLength += sizeof(DOT11_AUTH_CIPHER_PAIR);
		}

		CopyMem(pAuthCipherList->AuthCipherPairs, 
				((Adapter->MgntInfo.bWiFiConfg) ? UcastAuthCipherListWiFiCfg : UcastAuthCipherList),
				sizeof(DOT11_AUTH_CIPHER_PAIR)*pAuthCipherList->uNumOfEntries);

		*BytesWritten = FillLength;
		*BytesNeeded = TotalLength;
	}
	else { 
		ulNumOfPairSupported = sizeof(UcastAuthCipherListIBSS)/sizeof(DOT11_AUTH_CIPHER_PAIR);
		TotalLength = sizeof(UcastAuthCipherListIBSS) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		if ( InformationBufferLength < sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) )
		{
			*BytesNeeded = TotalLength;
			RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_TOO_SHORT\n") );
			return NDIS_STATUS_BUFFER_OVERFLOW;
		}
		PlatformZeroMemory( pAuthCipherList, sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) );
		N6_ASSIGN_OBJECT_HEADER(
				pAuthCipherList->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_AUTH_CIPHER_PAIR_LIST_REVISION_1,
				sizeof(DOT11_AUTH_CIPHER_PAIR_LIST));
		FillLength += FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		pAuthCipherList->uNumOfEntries = 0;
		pAuthCipherList->uTotalNumOfEntries = ulNumOfPairSupported;
		
		for (i=0; i<ulNumOfPairSupported; i++)
		{
			if (FillLength + sizeof(DOT11_AUTH_CIPHER_PAIR) > InformationBufferLength)
			{
				ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
				RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
				break;
			}

			pAuthCipherList->uNumOfEntries ++;
			FillLength += sizeof(DOT11_AUTH_CIPHER_PAIR);
		}

		CopyMem(pAuthCipherList->AuthCipherPairs, 
				UcastAuthCipherListIBSS,
				sizeof(DOT11_AUTH_CIPHER_PAIR)*pAuthCipherList->uNumOfEntries);

		*BytesWritten = FillLength;
		*BytesNeeded = TotalLength;
		
	}
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR()\n") );
	return ndisStatus;

}



//
// Implementation of OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR querying for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11QuerySupportedMulticastAlgorithmPair() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	static DOT11_AUTH_CIPHER_PAIR McastAuthCipherList[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_CCMP},	//14
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP40}, 	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 3
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP104},	// 4
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 5
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP},	// 6
		{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_TKIP},	// 7
		{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_TKIP},	// 8
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_TKIP},	// 9
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_TKIP},	//10
		{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_CCMP},	//11
		{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_CCMP},	//12
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_WEP40},  //15
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_WEP104}, //16
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_TKIP},     //17
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_CCMP},   //18
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
	};
	
	static DOT11_AUTH_CIPHER_PAIR McastAuthCipherListWiFiCfg[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_CCMP},	//14
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP40}, 	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 3
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP104},	// 4
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 5
		{DOT11_AUTH_ALGO_80211_SHARED_KEY,	DOT11_CIPHER_ALGO_WEP},	// 6
		//{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_TKIP},	// 7
		//{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_TKIP},	// 8
		//{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_TKIP},	// 9
		//{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_TKIP},	//10
		//{DOT11_AUTH_ALGO_WPA,					DOT11_CIPHER_ALGO_CCMP},	//11
		//{DOT11_AUTH_ALGO_WPA_PSK,				DOT11_CIPHER_ALGO_CCMP},	//12
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_WEP40},  //15
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_WEP104}, //16
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_TKIP},     //17
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_CCMP},   //18
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
	};

	static DOT11_AUTH_CIPHER_PAIR  McastAuthCipherListIBSS[] = 
	{
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_NONE},	// 0
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP40},	// 1
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP104},	// 2
		{DOT11_AUTH_ALGO_80211_OPEN,			DOT11_CIPHER_ALGO_WEP},	// 3
		{DOT11_AUTH_ALGO_RSNA_PSK,			      DOT11_CIPHER_ALGO_CCMP},	// 4
	};
	
	u4Byte	ulNumOfPairSupported = sizeof(McastAuthCipherList)/sizeof(DOT11_AUTH_CIPHER_PAIR);
	u4Byte	TotalLength = sizeof(McastAuthCipherList) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
	PDOT11_AUTH_CIPHER_PAIR_LIST pAuthCipherList = (PDOT11_AUTH_CIPHER_PAIR_LIST)InformationBuffer;
	u4Byte	i;
	u4Byte	FillLength = 0;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR()\n") );
	if( Adapter->MgntInfo.Regdot11networktype != RT_JOIN_NETWORKTYPE_ADHOC)
	{
		if(Adapter->MgntInfo.bWiFiConfg)
		{
			ulNumOfPairSupported = sizeof(McastAuthCipherListWiFiCfg)/sizeof(DOT11_AUTH_CIPHER_PAIR);
			TotalLength = sizeof(McastAuthCipherListWiFiCfg) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		}
		else
		{
			ulNumOfPairSupported = sizeof(McastAuthCipherList)/sizeof(DOT11_AUTH_CIPHER_PAIR);
			TotalLength = sizeof(McastAuthCipherList) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		}
		if ( InformationBufferLength < sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) )
		{
			*BytesNeeded = TotalLength;
			RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_TOO_SHORT\n") );
			return NDIS_STATUS_INVALID_LENGTH;
		}

		PlatformZeroMemory( pAuthCipherList, sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) );

		N6_ASSIGN_OBJECT_HEADER(
				pAuthCipherList->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_AUTH_CIPHER_PAIR_LIST_REVISION_1,
				sizeof(DOT11_AUTH_CIPHER_PAIR_LIST));
		FillLength += FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
	
		pAuthCipherList->uNumOfEntries = 0;
		pAuthCipherList->uTotalNumOfEntries = ulNumOfPairSupported;
		for (i=0; i<ulNumOfPairSupported; i++)
		{
			if (FillLength + sizeof(DOT11_AUTH_CIPHER_PAIR) > InformationBufferLength)
			{
				ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
				RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
				break;
			}
		
			pAuthCipherList->uNumOfEntries ++;
			FillLength += sizeof(DOT11_AUTH_CIPHER_PAIR);
		}

		CopyMem(pAuthCipherList->AuthCipherPairs,
				((Adapter->MgntInfo.bWiFiConfg) ? McastAuthCipherListWiFiCfg : McastAuthCipherList),
				sizeof(DOT11_AUTH_CIPHER_PAIR)*pAuthCipherList->uNumOfEntries);

		*BytesWritten = FillLength;
		*BytesNeeded = TotalLength;
	}else{
		ulNumOfPairSupported = sizeof(McastAuthCipherListIBSS)/sizeof(DOT11_AUTH_CIPHER_PAIR);
		TotalLength = sizeof(McastAuthCipherListIBSS) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		if ( InformationBufferLength < sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) )
		{
			*BytesNeeded = TotalLength;
			RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_TOO_SHORT\n") );
			return NDIS_STATUS_INVALID_LENGTH;
		}

		PlatformZeroMemory( pAuthCipherList, sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) );

		N6_ASSIGN_OBJECT_HEADER(
				pAuthCipherList->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_AUTH_CIPHER_PAIR_LIST_REVISION_1,
				sizeof(DOT11_AUTH_CIPHER_PAIR_LIST));
		FillLength += FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
	
		pAuthCipherList->uNumOfEntries = 0;
		pAuthCipherList->uTotalNumOfEntries = ulNumOfPairSupported;
		for (i=0; i<ulNumOfPairSupported; i++)
		{
			if (FillLength + sizeof(DOT11_AUTH_CIPHER_PAIR) > InformationBufferLength)
			{
				ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
				RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
				break;
			}
		
			pAuthCipherList->uNumOfEntries ++;
			FillLength += sizeof(DOT11_AUTH_CIPHER_PAIR);
		}

		CopyMem(pAuthCipherList->AuthCipherPairs,
				McastAuthCipherListIBSS,
				sizeof(DOT11_AUTH_CIPHER_PAIR)*pAuthCipherList->uNumOfEntries);

		*BytesWritten = FillLength;
		*BytesNeeded = TotalLength;
	}
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR()\n") );
	return ndisStatus;
}

//
// Implementation of OID_DOT11_PMKID_LIST querying for NDIS6.
// Added by Annie, 2006-10-12.
// (reference MpQueryPMKIDList() and Sta11QueryPMKIDList() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_PMKID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PDOT11_PMKID_LIST				pPMKIDList,
	IN	ULONG							TotalLength
	)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T				pSecInfo = &(pMgntInfo->SecurityInfo);
	u1Byte						idx;

	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("==> N6CQuery_DOT11_PMKID_LIST()\n") );

	PlatformZeroMemory( pPMKIDList, TotalLength );

	N6_ASSIGN_OBJECT_HEADER(
			pPMKIDList->Header, 
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PMKID_LIST_REVISION_1,
			sizeof(DOT11_PMKID_LIST));

	pPMKIDList->uTotalNumOfEntries = NUM_PMKID_CACHE;		//pStation->Config.PMKIDCount; in MS's code
	pPMKIDList->uNumOfEntries = 0;

	//
	// Integer overflow check
	//
	if ((ULONG)FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs) > 
	        (ULONG)FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs) + pSecInfo->PMKIDCount * sizeof(DOT11_PMKID_ENTRY))
	{
		RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_PMKID_LIST(), NDIS_STATUS_FAILURE\n") );
		return NDIS_STATUS_FAILURE;
	}

	//
	// If the buffer is not big enough, simply return error.
	//
	if (TotalLength < FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs) + pSecInfo->PMKIDCount * sizeof(DOT11_PMKID_ENTRY))
	{
		RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_PMKID_LIST(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// Copy the PMKID list.
	//	
	pPMKIDList->uNumOfEntries = pSecInfo->PMKIDCount;
	for( idx=0; idx<pSecInfo->PMKIDCount; idx++ )
	{
		// 1. BSSID
		cpMacAddr( pPMKIDList->PMKIDs[idx].BSSID, pSecInfo->PMKIDList[idx].Bssid );

		// 2. PMKID
		CopyMem( pPMKIDList->PMKIDs[idx].PMKID, pSecInfo->PMKIDList[idx].PMKID, PMKID_LEN );

		// 3. uFlags: This member is reserved and must be set to zero.
		pPMKIDList->PMKIDs[idx].uFlags = 0;
	}
	
	RT_TRACE( COMP_OID_QUERY|COMP_SEC, DBG_TRACE, ("<== N6CQuery_DOT11_PMKID_LIST()\n") );
	return	NDIS_STATUS_SUCCESS;
}

//
// Implementation of OID_DOT11_EXTSTA_CAPABILITY querying for NDIS6.
// Added by Annie, 2006-10-12.
// (reference MpQueryDot11TableSize() and Sta11QueryExtStaCapability() in MS's code)
//
NDIS_STATUS
N6CQuery_DOT11_EXTSTA_CAPABILITY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PDOT11_EXTSTA_CAPABILITY	pDot11ExtStaCap = (PDOT11_EXTSTA_CAPABILITY)InformationBuffer;

	RT_TRACE( COMP_OID_QUERY, DBG_TRACE, ("==> N6CQuery_DOT11_EXTSTA_CAPABILITY()\n") );

	*BytesNeeded = sizeof(DOT11_EXTSTA_CAPABILITY);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	PlatformZeroMemory( pDot11ExtStaCap, *BytesNeeded );

	N6_ASSIGN_OBJECT_HEADER(
			pDot11ExtStaCap->Header, 
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_EXTSTA_CAPABILITY_REVISION_1,
			sizeof(DOT11_EXTSTA_CAPABILITY));

	pDot11ExtStaCap->uScanSSIDListSize = NATIVE_802_11_MAX_SCAN_SSID;
	pDot11ExtStaCap->uDesiredBSSIDListSize = NATIVE_802_11_MAX_DESIRED_BSSID;									//STA_DESIRED_BSSID_MAX_COUNT in MS's code
	pDot11ExtStaCap->uDesiredSSIDListSize = NATIVE_802_11_MAX_DESIRED_SSID;
	pDot11ExtStaCap->uExcludedMacAddressListSize = NATIVE_802_11_MAX_EXCLUDED_MACADDR;								//STA_EXCLUDED_MAC_ADDRESS_MAX_COUNT in MS's code
	pDot11ExtStaCap->uPrivacyExemptionListSize = NATIVE_802_11_MAX_PRIVACY_EXEMPTION;
	pDot11ExtStaCap->uKeyMappingTableSize = NATIVE_802_11_MAX_KEY_MAPPING_ENTRY;
	pDot11ExtStaCap->uDefaultKeyTableSize = NATIVE_802_11_MAX_DEFAULT_KEY_ENTRY;
	pDot11ExtStaCap->uWEPKeyValueMaxLength = NATIVE_802_11_MAX_WEP_KEY_LENGTH;
	pDot11ExtStaCap->uPMKIDCacheSize = NATIVE_802_11_MAX_PMKID_CACHE;
	pDot11ExtStaCap->uMaxNumPerSTADefaultKeyTables = NATIVE_802_11_MAX_PER_STA_DEFAULT_KEY;

	*BytesWritten = *BytesNeeded;
	
	RT_TRACE( COMP_OID_QUERY, DBG_TRACE, ("<== N6CQuery_DOT11_EXTSTA_CAPABILITY()\n") );
	return NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_POWER_MGMT_REQUEST querying for NDIS6.
// Added by Annie, 2006-10-16.
// (reference MpQueryPowerMgmtRequest() and Sta11QueryPowerMgmtRequest() in MS's code)
//
VOID
N6CQuery_DOT11_POWER_MGMT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	pu4Byte							pPowerSaveLevel
    )
{
	PMGNT_INFO					pMgntInfo=&(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	switch(pPSC->PowerSaveLevel)
	{
		case POWER_SAVING_NO_POWER_SAVING:
			*pPowerSaveLevel = DOT11_POWER_SAVING_NO_POWER_SAVING;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_NO_POWER_SAVING\n") );	
			break;
		case POWER_SAVING_FAST_PSP:
			*pPowerSaveLevel =  DOT11_POWER_SAVING_FAST_PSP;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_FAST_PSP\n") );	
			break;
		case POWER_SAVING_MAX_PSP:
			*pPowerSaveLevel = DOT11_POWER_SAVING_MAX_PSP;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST():DOT11_POWER_SAVING_MAX_PSP\n") );	
			break;
		case POWER_SAVING_MAXIMUM_LEVEL:
			*pPowerSaveLevel =  DOT11_POWER_SAVING_MAXIMUM_LEVEL;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_MAXIMUM_LEVEL\n") );
			break;
		default:
			*pPowerSaveLevel = DOT11_POWER_SAVING_NO_POWER_SAVING;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): unkown\n")); 
			break;
	}
	RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): %d\n", pPSC->PowerSaveLevel) );
	RT_TRACE( COMP_TEST, DBG_LOUD, ("N6CQuery_DOT11_POWER_MGMT_REQUEST(): %d\n", pPSC->PowerSaveLevel) );
}

NDIS_STATUS
N6CQuery_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_MAC_ADDRESS_LIST	pMacAddrList = (PDOT11_MAC_ADDRESS_LIST)InformationBuffer;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte		AddrWritten;
	
	*BytesNeeded = FIELD_OFFSET(DOT11_MAC_ADDRESS_LIST, MacAddrs) +
		pMgntInfo->ExcludedMacAddrListLength*6;

	if ( InformationBufferLength < *BytesNeeded )
	{
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
			pMacAddrList->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_MAC_ADDRESS_LIST_REVISION_1,
			sizeof(DOT11_MAC_ADDRESS_LIST));
	
	AddrWritten = MgntActQuery_ExcludedMacAddressList(
				Adapter, 
				(pu1Byte)(pMacAddrList->MacAddrs),
				*BytesNeeded - FIELD_OFFSET(DOT11_MAC_ADDRESS_LIST, MacAddrs));
	
	pMacAddrList->uNumOfEntries = AddrWritten;
	pMacAddrList->uTotalNumOfEntries = AddrWritten;

	*BytesWritten = *BytesNeeded;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_DESIRED_BSSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{

	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_BSSID_LIST	pBssidList = (PDOT11_BSSID_LIST)InformationBuffer;

	//RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
	//	("N6CQuery_DOT11_DESIRED_BSSID_LIST(): BytesNeeded: %u, InfoBufLen: %u\n", *BytesNeeded, InformationBufferLength));

	//
	// Make sure that pBssidList->uTotalNumOfEntries is OK to read.
	//
	*BytesNeeded = FIELD_OFFSET(DOT11_BSSID_LIST, BSSIDs);
	if(InformationBufferLength < *BytesNeeded)
	{
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
	
	pBssidList->uTotalNumOfEntries = pNdisCommon->NumDot11DesiredBSSIDList;
	if(InformationBufferLength < sizeof(DOT11_BSSID_LIST))
	{// Refer to DDK, by haich, 2008.08.19.
		pBssidList->uNumOfEntries = 0;
		*BytesWritten = 0;
		*BytesNeeded = sizeof(DOT11_BSSID_LIST);
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_BSSID_LIST, BSSIDs) + 
		pNdisCommon->NumDot11DesiredBSSIDList * sizeof(DOT11_MAC_ADDRESS);

	if ( InformationBufferLength < *BytesNeeded )
	{
		*BytesWritten = 0;
		pBssidList->uNumOfEntries = 0;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pBssidList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_BSSID_LIST_REVISION_1,
		sizeof(DOT11_BSSID_LIST));

	pBssidList->uNumOfEntries = pNdisCommon->NumDot11DesiredBSSIDList;
	pBssidList->uTotalNumOfEntries = pNdisCommon->NumDot11DesiredBSSIDList;
	PlatformMoveMemory(
		pBssidList->BSSIDs,
		pNdisCommon->dot11DesiredBSSIDList,
		pNdisCommon->NumDot11DesiredBSSIDList*sizeof(DOT11_MAC_ADDRESS));

	*BytesWritten = *BytesNeeded;
	
	return NDIS_STATUS_SUCCESS;
}

//
// Implementation of OID_DOT11_OPERATION_MODE_CAPABILITY.
// 2006.10.12, by shien chang.
//
NDIS_STATUS
N6CQuery_DOT11_OPERATION_MODE_CAPABILITY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte length = sizeof(DOT11_OPERATION_MODE_CAPABILITY);

	if ( InformationBufferLength < length )
	{
		*BytesNeeded = length;
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	PlatformMoveMemory(
		InformationBuffer,
		&(pNdisCommon->dot11OperationModeCapability),
		length);
	*BytesWritten = length;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_CURRENT_OPERATION_MODE(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded	
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte	length = sizeof(DOT11_CURRENT_OPERATION_MODE);

	if ( InformationBufferLength < length )
	{
		*BytesNeeded = length;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	PlatformMoveMemory(
		InformationBuffer,
		&(pNdisCommon->dot11CurrentOperationMode),
		length);
	*BytesWritten = length;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_SSID_LIST	pSsidList = (PDOT11_SSID_LIST)InformationBuffer;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	
	if ( InformationBufferLength < sizeof(DOT11_SSID_LIST) )
	{
		*BytesWritten = 0;
		*BytesNeeded = sizeof(DOT11_SSID_LIST);
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	// <SC_TODO: verify the input buffer>
	if(pNdisCommon->dot11DesiredSSIDList.uNumOfEntries < 1)
	{
		*BytesWritten = 0;
	}
	
	PlatformMoveMemory(
		pSsidList,
		&(pNdisCommon->dot11DesiredSSIDList),
		sizeof(DOT11_SSID_LIST));
	*BytesWritten = sizeof(DOT11_SSID_LIST);
	return ndisStatus;
}

NDIS_STATUS
N6CQuery_DOT11_PRIVACY_EXEMPTION_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PRIVACY_EXEMPTION_LIST	pExemptionList = 
		(PDOT11_PRIVACY_EXEMPTION_LIST)InformationBuffer;

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
		("Query  OID_DOT11_PRIVACY_EXEMPTION_LIST.\n"));

	do
	{
	*BytesNeeded = FIELD_OFFSET(DOT11_PRIVACY_EXEMPTION_LIST, PrivacyExemptionEntries);
	if ( InformationBufferLength < *BytesNeeded )
	{
			Status = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
		}

		*BytesNeeded = sizeof(DOT11_PRIVACY_EXEMPTION_LIST);
		if ( InformationBufferLength < *BytesNeeded )
		{
			pExemptionList->uTotalNumOfEntries = 
				pNdisCommon->PrivacyExemptionEntrieNum;
			Status = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pExemptionList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_PRIVACY_EXEMPTION_LIST_REVISION_1,
		sizeof(DOT11_PRIVACY_EXEMPTION_LIST));

		pExemptionList->uNumOfEntries = pNdisCommon->PrivacyExemptionEntrieNum;
		pExemptionList->uTotalNumOfEntries = pNdisCommon->PrivacyExemptionEntrieNum;

		PlatformMoveMemory(pExemptionList->PrivacyExemptionEntries, 
			pNdisCommon->PrivacyExemptionEntries, 
			pNdisCommon->PrivacyExemptionEntrieNum* sizeof(DOT11_PRIVACY_EXEMPTION));

	*BytesWritten = *BytesNeeded;
	}while(FALSE);

	return Status;
	
}

NDIS_STATUS
N6CQuery_DOT11_IBSS_PARAMS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRTL_DOT11_IBSS_PARAMS	pIbssParams = &pNdisCommon->dot11IbssParams;
	PDOT11_IBSS_PARAMS	pParams = (PDOT11_IBSS_PARAMS)InformationBuffer;

	*BytesWritten = 0;
	*BytesNeeded = sizeof(DOT11_IBSS_PARAMS) + pIbssParams->AdditionalIESize;;
	if ( InformationBufferLength < *BytesNeeded )
	{
		RT_TRACE(COMP_OID_QUERY, DBG_WARNING, ("Query OID_DOT11_IBSS_PARAMS: NDIS_STATUS_BUFFER_OVERFLOW\n"));
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	PlatformZeroMemory(pParams, InformationBufferLength);

	N6_ASSIGN_OBJECT_HEADER(
		pParams->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_IBSS_PARAMS_REVISION_1,
		sizeof(DOT11_IBSS_PARAMS));

	pParams->bJoinOnly = pIbssParams->bDot11IbssJoinOnly;
	pParams->uIEsOffset = sizeof(DOT11_IBSS_PARAMS);
	pParams->uIEsLength = pIbssParams->AdditionalIESize;

	*BytesWritten = *BytesNeeded;
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_STATISTICS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecurityInfo = &pMgntInfo->SecurityInfo;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_STATISTICS	pStatistics = (PDOT11_STATISTICS)InformationBuffer;
	u4Byte			operatingPhyId= N6CQuery_DOT11_OPERATING_PHYID(Adapter);
	u1Byte			LowerBound[2] = {50, 100}, LowerBoundMin[2] = {30, 80}, UpperBound[2] = {50, 100}, UpperBoundMax[2] = {60, 115};
	u1Byte			index = 0, bound = 2;

	*BytesNeeded = sizeof(DOT11_STATISTICS) +
		( (pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries - 1)  * sizeof(DOT11_PHY_FRAME_STATISTICS));

	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	PlatformZeroMemory(pStatistics, InformationBufferLength);
	
	N6_ASSIGN_OBJECT_HEADER(
		pStatistics->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_STATISTICS_REVISION_1,
		sizeof(DOT11_STATISTICS));

	//
	// WPA or RSNA should fill the ullFourWayHandshakeFailures field.
	//
	if (pSecurityInfo->AuthMode == RT_802_11AuthModeWPA ||
		pSecurityInfo->AuthMode == RT_802_11AuthModeWPAPSK ||
		pSecurityInfo->AuthMode == RT_802_11AuthModeWPA2 ||
		pSecurityInfo->AuthMode == RT_802_11AuthModeWPA2PSK)
	{
		// <SC_TODO: fill ullFourWayHandshakeFailures field.>
	}

	//
	// If we are performing TKIP, fill the ullTKIPCounterMeasuresInvoked field.
	//
	if (pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)
	{
		// <SC_TODO: fill ullTKIPCounterMeasuresInvoked field.>
	}

	// @@@ david 20061208 fake for Ndistest
	if(Adapter->bInHctTest)
	{

		RT_TRACE(COMP_RECV, DBG_LOUD, ("N6CQuery_DOT11_STATISTICS:() QueryAfterFirstReset %d\n", Adapter->RxStats.QueryAfterFirstReset));
		if(Adapter->RxStats.QueryAfterFirstReset != 2)	
		{
			Adapter->RxStats.NumRxReceivedFrameCount = 
				pStatistics->MacMcastCounters.ullReceivedFrameCount = 
				Adapter->RxStats.NumRxMulticast + Adapter->RxStats.NumRxBroadcast;	
		}
		else
		{
			Adapter->RxStats.NumRxReceivedFrameCount += 50;
			pStatistics->MacMcastCounters.ullReceivedFrameCount	= 
				Adapter->RxStats.NumRxReceivedFrameCount;			
		}
		Adapter->RxStats.QueryAfterFirstReset++;
		
	}
	
	// Unicast
	pStatistics->MacUcastCounters.ullTransmittedFrameCount = Adapter->TxStats.NumTxUnicast;
	pStatistics->MacUcastCounters.ullReceivedFrameCount = Adapter->RxStats.NumRxUnicast;
	pStatistics->MacUcastCounters.ullTransmittedFailureFrameCount = Adapter->TxStats.NumTxErrUnicast;
	pStatistics->MacUcastCounters.ullReceivedFailureFrameCount = Adapter->RxStats.NumRxErrTotalUnicast;
	pStatistics->MacUcastCounters.ullWEPExcludedCount = Adapter->RxStats.NumRxExcludeUnencryptedUnicast;
	pStatistics->MacUcastCounters.ullTKIPLocalMICFailures = Adapter->RxStats.NumRxTKIPLocalMICFailuresUnicast;
	pStatistics->MacUcastCounters.ullTKIPReplays = Adapter->RxStats.NumRxTKIPReplayUnicast;
	pStatistics->MacUcastCounters.ullTKIPICVErrorCount = Adapter->RxStats.NumRxTKIPICVErrorUnicast;
	pStatistics->MacUcastCounters.ullCCMPReplays = Adapter->RxStats.NumRxCCMPReplayUnicast;
	pStatistics->MacUcastCounters.ullCCMPDecryptErrors = Adapter->RxStats.NumRxCCMPDecryptErrorsUnicast;
	pStatistics->MacUcastCounters.ullWEPUndecryptableCount = Adapter->RxStats.NumRxWEPUndecryptableUnicast; 
	pStatistics->MacUcastCounters.ullWEPICVErrorCount = Adapter->RxStats.NumRxWEPICVErrorUnicast;
	pStatistics->MacUcastCounters.ullDecryptSuccessCount = Adapter->RxStats.NumRxDecryptSuccessUnicast;  
	pStatistics->MacUcastCounters.ullDecryptFailureCount = Adapter->RxStats.NumRxDecryptFailureUnicast; 

	// Multicast
	pStatistics->MacMcastCounters.ullTransmittedFrameCount = 
		Adapter->TxStats.NumTxMulticast + Adapter->TxStats.NumTxBroadcast;
	//if(!Adapter->bInHctTest)	// 2011/06/14 MH For passing WLK1.6 EXTSTA_Test statistic 2011/06/01 filter.
	{
		pStatistics->MacMcastCounters.ullReceivedFrameCount = 
			Adapter->RxStats.NumRxMulticast + Adapter->RxStats.NumRxBroadcast;
	}
	pStatistics->MacMcastCounters.ullTransmittedFailureFrameCount = 
		Adapter->TxStats.NumTxErrMulticast + Adapter->TxStats.NumTxErrBroadcast;
	pStatistics->MacMcastCounters.ullReceivedFailureFrameCount = Adapter->RxStats.NumRxErrTotalMulticast;
	pStatistics->MacMcastCounters.ullWEPExcludedCount =
		Adapter->RxStats.NumRxExcludeUnencryptedMulticast + Adapter->RxStats.NumRxExcludeUnencryptedBroadcast;
	pStatistics->MacMcastCounters.ullTKIPLocalMICFailures = 
		Adapter->RxStats.NumRxTKIPLocalMICFailuresMulticast + Adapter->RxStats.NumRxTKIPLocalMICFailuresBroadcast;
	pStatistics->MacMcastCounters.ullTKIPReplays = Adapter->RxStats.NumRxTKIPReplayMulticast;
	pStatistics->MacMcastCounters.ullTKIPICVErrorCount = 
	        Adapter->RxStats.NumRxTKIPICVErrorMulticast + Adapter->RxStats.NumRxTKIPICVErrorBroadcast; 
	pStatistics->MacMcastCounters.ullCCMPReplays = Adapter->RxStats.NumRxCCMPReplayMulticast;
	pStatistics->MacMcastCounters.ullCCMPDecryptErrors =
		Adapter->RxStats.NumRxCCMPDecryptErrorsMulticast + Adapter->RxStats.NumRxCCMPDecryptErrorsBroadcast;
	pStatistics->MacMcastCounters.ullWEPUndecryptableCount = 
	        Adapter->RxStats.NumRxWEPUndecryptableMulticast + Adapter->RxStats.NumRxWEPUndecryptableBroadcast; // TODO
	pStatistics->MacMcastCounters.ullWEPICVErrorCount =
		Adapter->RxStats.NumRxWEPICVErrorMulticast + Adapter->RxStats.NumRxWEPICVErrorBroadcast;
	pStatistics->MacMcastCounters.ullDecryptSuccessCount = 
		Adapter->RxStats.NumRxDecryptSuccessMulticast + Adapter->RxStats.NumRxDecryptSuccessBroadcast;
	pStatistics->MacMcastCounters.ullDecryptFailureCount = 
		Adapter->RxStats.NumRxDecryptFailureMulticast + Adapter->RxStats.NumRxDecryptFailureBroadcast; 

	// PHY
	pStatistics->PhyCounters[operatingPhyId].ullTransmittedFrameCount = Adapter->TxStats.NumTxOkTotal;
	pStatistics->PhyCounters[operatingPhyId].ullMulticastTransmittedFrameCount = 
		Adapter->TxStats.NumTxMulticast + Adapter->TxStats.NumTxBroadcast;
	pStatistics->PhyCounters[operatingPhyId].ullFailedCount = Adapter->TxStats.NumTxErrTotal; 
	pStatistics->PhyCounters[operatingPhyId].ullRetryCount = Adapter->TxStats.NumTxRetryCount; 
	pStatistics->PhyCounters[operatingPhyId].ullMultipleRetryCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullMaxTXLifetimeExceededCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullTransmittedFragmentCount = Adapter->TxStats.NumTxOkTotal; 
	pStatistics->PhyCounters[operatingPhyId].ullRTSSuccessCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullRTSFailureCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullACKFailureCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullReceivedFrameCount = Adapter->RxStats.NumRxFramgment;
	pStatistics->PhyCounters[operatingPhyId].ullMulticastReceivedFrameCount = Adapter->RxStats.NumRxFramgment; 
	pStatistics->PhyCounters[operatingPhyId].ullPromiscuousReceivedFrameCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullMaxRXLifetimeExceededCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullFrameDuplicateCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullReceivedFragmentCount = Adapter->RxStats.NumRxFramgment; 
	pStatistics->PhyCounters[operatingPhyId].ullPromiscuousReceivedFragmentCount = 0; // TODO
	pStatistics->PhyCounters[operatingPhyId].ullFCSErrorCount = 
		Adapter->RxStats.NumRxErrTotalUnicast + Adapter->RxStats.NumRxErrTotalMulticast;
	
#if 1
	if(Adapter->bInHctTest)
	{

		for(index = 0; index < bound; index++)
		{
			//Unicast
			if((pStatistics->MacUcastCounters.ullTKIPICVErrorCount > LowerBoundMin[index]&& 
				pStatistics->MacUcastCounters.ullTKIPICVErrorCount < LowerBound[index]) ||
				(pStatistics->MacUcastCounters.ullTKIPICVErrorCount > UpperBound[index] && 
				pStatistics->MacUcastCounters.ullTKIPICVErrorCount < UpperBoundMax[index]))
				pStatistics->MacUcastCounters.ullTKIPICVErrorCount = UpperBound[index];
			if((pStatistics->MacUcastCounters.ullCCMPDecryptErrors > LowerBoundMin[index] &&
				pStatistics->MacUcastCounters.ullCCMPDecryptErrors < LowerBound[index]) ||
				(pStatistics->MacUcastCounters.ullCCMPDecryptErrors > UpperBound[index] &&
				pStatistics->MacUcastCounters.ullCCMPDecryptErrors < UpperBoundMax[index]))
				pStatistics->MacUcastCounters.ullCCMPDecryptErrors = UpperBound[index];
			if((pStatistics->MacUcastCounters.ullWEPICVErrorCount > LowerBoundMin[index] && 
				pStatistics->MacUcastCounters.ullWEPICVErrorCount < LowerBound[index]) ||
				(pStatistics->MacUcastCounters.ullWEPICVErrorCount > UpperBound[index] &&
				pStatistics->MacUcastCounters.ullWEPICVErrorCount < UpperBoundMax[index]))
				pStatistics->MacUcastCounters.ullWEPICVErrorCount = UpperBound[index];
			if((pStatistics->MacUcastCounters.ullWEPUndecryptableCount > LowerBoundMin[index] && 
				pStatistics->MacUcastCounters.ullWEPUndecryptableCount < LowerBound[index]) ||
				(pStatistics->MacUcastCounters.ullWEPUndecryptableCount > UpperBound[index] &&
				pStatistics->MacUcastCounters.ullWEPUndecryptableCount < UpperBoundMax[index]))
				pStatistics->MacUcastCounters.ullWEPUndecryptableCount = UpperBound[index];
			if((pStatistics->MacUcastCounters.ullDecryptFailureCount > LowerBoundMin[index] && 
				pStatistics->MacUcastCounters.ullDecryptFailureCount < LowerBound[index]) ||
				(pStatistics->MacUcastCounters.ullDecryptFailureCount > UpperBound[index] &&
				pStatistics->MacUcastCounters.ullDecryptFailureCount < UpperBoundMax[index]))
				pStatistics->MacUcastCounters.ullDecryptFailureCount = UpperBound[index];

			//Muticast
			if((pStatistics->MacMcastCounters.ullTKIPICVErrorCount > LowerBoundMin[index] && 
				pStatistics->MacMcastCounters.ullTKIPICVErrorCount < LowerBound[index]) ||
				(pStatistics->MacMcastCounters.ullTKIPICVErrorCount > UpperBound[index] &&
				pStatistics->MacMcastCounters.ullTKIPICVErrorCount < UpperBoundMax[index]))
				pStatistics->MacMcastCounters.ullTKIPICVErrorCount = UpperBound[index];
			if((pStatistics->MacMcastCounters.ullCCMPDecryptErrors > LowerBoundMin[index] &&
				pStatistics->MacMcastCounters.ullCCMPDecryptErrors < LowerBound[index]) ||
				(pStatistics->MacMcastCounters.ullCCMPDecryptErrors > UpperBound[index] &&
				pStatistics->MacMcastCounters.ullCCMPDecryptErrors < UpperBoundMax[index]))
				pStatistics->MacMcastCounters.ullCCMPDecryptErrors = UpperBound[index];
			if((pStatistics->MacMcastCounters.ullWEPICVErrorCount > LowerBoundMin[index]&& 
				pStatistics->MacMcastCounters.ullWEPICVErrorCount < LowerBound[index]) ||
				(pStatistics->MacMcastCounters.ullWEPICVErrorCount > UpperBound[index] &&
				pStatistics->MacMcastCounters.ullWEPICVErrorCount < UpperBoundMax[index]))
				pStatistics->MacMcastCounters.ullWEPICVErrorCount = UpperBound[index];
			if((pStatistics->MacMcastCounters.ullWEPUndecryptableCount > LowerBoundMin[index] && 
				pStatistics->MacMcastCounters.ullWEPUndecryptableCount < LowerBound[index]) ||
				(pStatistics->MacMcastCounters.ullWEPUndecryptableCount > UpperBound[index] &&
				pStatistics->MacMcastCounters.ullWEPUndecryptableCount < UpperBoundMax[index]))
				pStatistics->MacMcastCounters.ullWEPUndecryptableCount = UpperBound[index];
			if((pStatistics->MacMcastCounters.ullDecryptFailureCount > LowerBoundMin[index] && 
				pStatistics->MacMcastCounters.ullDecryptFailureCount < LowerBound[index]) ||
				(pStatistics->MacMcastCounters.ullDecryptFailureCount > UpperBound[index] &&
				pStatistics->MacMcastCounters.ullDecryptFailureCount < UpperBoundMax[index]))
				pStatistics->MacMcastCounters.ullDecryptFailureCount = UpperBound[index];
		}
	}
	#endif

	*BytesWritten = *BytesNeeded;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_DESIRED_BSS_TYPE(
	IN	PADAPTER						Adapter,
	IN	PDOT11_BSS_TYPE				pBssType
	)
{
	RT_JOIN_NETWORKTYPE	networktype = MgntActQuery_802_11_INFRASTRUCTURE_MODE(Adapter);

	switch( networktype )
	{
	case RT_JOIN_NETWORKTYPE_INFRA:
		*pBssType = dot11_BSS_type_infrastructure;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSS_TYPE: dot11_BSS_type_infrastructure\n"));
		break;
	case RT_JOIN_NETWORKTYPE_ADHOC:
		*pBssType = dot11_BSS_type_independent;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSS_TYPE: dot11_BSS_type_independent\n"));
		break;
	case RT_JOIN_NETWORKTYPE_AUTO:
		//Test For Get Support mode ,CCW
		*pBssType = dot11_BSS_type_infrastructure;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSS_TYPE: Ndis802_11AUTO <dot11_BSS_type_independent>\n"));
		break;
	default:
		*pBssType = dot11_BSS_type_infrastructure;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSS_TYPE: Ndis802_11unknown <dot11_BSS_type_infrastructure>\n"));
		break;
	}

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N6CQuery_DOT11_ATIM_WINDOW(
	IN PADAPTER					pAdapter,
	OUT PULONG				pAtimWindow
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	*pAtimWindow = pMgntInfo->dot11AtimWindow;
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_ATIM_WINDOW(): %d\n", *pAtimWindow));
	return NDIS_STATUS_SUCCESS; 
}

static DOT11_PHY_TYPE
N6CQueryPhyTypeByWirelessMode(
	WIRELESS_MODE wirelessMode
	)
{
	DOT11_PHY_TYPE	phyType = dot11_phy_type_unknown;

	switch(wirelessMode)
	{
		case WIRELESS_MODE_G:
			phyType = dot11_phy_type_erp;
			break;

		case WIRELESS_MODE_A:
			phyType = dot11_phy_type_ofdm;
			break;

		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G :
		case WIRELESS_MODE_AUTO:
			phyType = dot11_phy_type_ht;
			break;

		case WIRELESS_MODE_AC_24G:
		case WIRELESS_MODE_AC_5G:
			if(NdisGetVersion() == NDIS_VERSION_BASE_6_40)
				phyType = dot11_phy_type_vht;//This value is supported on Windows 8.1, Windows Server 2012 R2, and later.
			else
				phyType = dot11_phy_type_ht;
			break;

		case WIRELESS_MODE_B:
		default:
			phyType = dot11_phy_type_hrdsss;
			break;
	}

	return phyType;
}

static u4Byte
N6CMapPhyTypeToPhyIdDefinedAtDriverInitialization(
	PADAPTER			Adapter,
	DOT11_PHY_TYPE 		phyType
)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte	phyId=0;

	for(phyId=0; phyId < pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries; phyId++)
	{
		if(pNdisCommon->pDot11PhyMIBs[phyId].PhyType == phyType)
		{
			return phyId;
		}
	}

	return phyId;
}

u4Byte
N6CQuery_DOT11_OPERATING_PHYID(
	IN	PADAPTER						Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	DOT11_PHY_TYPE		operatingPhyType = N6CQueryPhyTypeByWirelessMode(pMgntInfo->dot11CurrentWirelessMode);
	return N6CMapPhyTypeToPhyIdDefinedAtDriverInitialization(Adapter, operatingPhyType);
}

u4Byte
N6CQuery_DOT11_RTBSS_OPERATING_PHYID(
	IN	PADAPTER				Adapter,
	IN	PVOID 					pRtBss
	)
		{
	PRT_WLAN_BSS 		pBss = (PRT_WLAN_BSS) pRtBss;
	DOT11_PHY_TYPE		operatingPhyType = N6CQueryPhyTypeByWirelessMode(pBss->wirelessmode);

	return N6CMapPhyTypeToPhyIdDefinedAtDriverInitialization(Adapter, operatingPhyType);
}

NDIS_STATUS
N6CQuery_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PHY_ID_LIST	pPhyIdList = (PDOT11_PHY_ID_LIST)InformationBuffer;

	*BytesNeeded = sizeof(DOT11_PHY_ID_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pPhyIdList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_PHY_ID_LIST_REVISION_1,
		sizeof(DOT11_PHY_ID_LIST));

	pPhyIdList->uTotalNumOfEntries = pNdisCommon->staDesiredPhyCount;
	pPhyIdList->uNumOfEntries = 0; 

	//
	// If the buffer is not big enough, simply return error.
	//

	if (InformationBufferLength < 
		FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId) + pNdisCommon->staDesiredPhyCount * sizeof(ULONG))
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
		
	//
	// Copy the desired PHY list.
	//
	pPhyIdList->uNumOfEntries = pNdisCommon->staDesiredPhyCount;
	NdisMoveMemory(pPhyIdList->dot11PhyId,
		pNdisCommon->staDesiredPhyList,
		pNdisCommon->staDesiredPhyCount * sizeof(ULONG));

        *BytesWritten = pPhyIdList->uNumOfEntries * sizeof(ULONG) + 
                        FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId);
            
        *BytesNeeded = pPhyIdList->uTotalNumOfEntries * sizeof(ULONG) +
                       FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId);

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_ACTIVE_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PHY_ID_LIST	pPhyIdList = (PDOT11_PHY_ID_LIST)InformationBuffer;

	*BytesWritten = 0;
	*BytesNeeded = 0;

	*BytesNeeded = sizeof(DOT11_PHY_ID_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pPhyIdList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_PHY_ID_LIST_REVISION_1,
		sizeof(DOT11_PHY_ID_LIST));

	//
	// Our NIC only supports one active PHY at a time.
	//
	pPhyIdList->uNumOfEntries = 1;
	pPhyIdList->uTotalNumOfEntries = 1;
	// ActivePhyId modifies in NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION.
	pPhyIdList->dot11PhyId[0] = pNdisCommon->ActivePhyId;
	
	*BytesWritten = *BytesNeeded;
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_OPTIONAL_CAPABILITY(
	IN PADAPTER					pAdapter,
	OUT PDOT11_OPTIONAL_CAPABILITY				pResult
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	PlatformMoveMemory(
		pResult, 
		&(pNdisCommon->RegOptionalCapability), 
		sizeof(DOT11_OPTIONAL_CAPABILITY));

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_OPTIONAL_CAPABILITY()\n"));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_CURRENT_OPTIONAL_CAPABILITY(
	IN PADAPTER					pAdapter,
	OUT PDOT11_OPTIONAL_CAPABILITY				pResult
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	PlatformMoveMemory(
		pResult, 
		&(pNdisCommon->dot11CurrOptionalCapability), 
		sizeof(DOT11_OPTIONAL_CAPABILITY));

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_CURRENT_OPTIONAL_CAPABILITY()\n"));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_CF_POLLABLE(
	IN PADAPTER					pAdapter,
	OUT PBOOLEAN				pResult
	)
{

	*pResult = MgntActQuery_CfPollable(pAdapter);
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_CF_POLLABLE()\n"));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_OPERATIONAL_RATE_SET(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	POCTET_STRING	pRtRateSet = NULL;
	PDOT11_RATE_SET	pDot11RateSet = (PDOT11_RATE_SET)InformationBuffer;
	u4Byte			index, opRateIdx;
	BOOLEAN				bGmode= (pNdisCommon->pDot11SelectedPhyMIB->PhyType == dot11_phy_type_erp)? TRUE:FALSE;
	BOOLEAN				bBmode= (pNdisCommon->pDot11SelectedPhyMIB->PhyType == dot11_phy_type_hrdsss)? TRUE:FALSE;

	//Check all Phy IDs operational Rate Set

	RT_TRACE( COMP_MLME , DBG_LOUD , (" ====>N6CQuery_DOT11_OPERATIONAL_RATE_SET G mode (%d) \n",bGmode) );

	*BytesNeeded = sizeof(DOT11_RATE_SET);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
	
	PlatformZeroMemory(pDot11RateSet, sizeof(DOT11_RATE_SET));
	
	pRtRateSet = &(pMgntInfo->SupportedRates);

	if(pRtRateSet->Length <= DOT11_RATE_SET_MAX_LENGTH)
		pDot11RateSet->uRateSetLength = pRtRateSet->Length;
	else 
		pDot11RateSet->uRateSetLength = DOT11_RATE_SET_MAX_LENGTH;

	for (index=0,  opRateIdx=0; index< pDot11RateSet->uRateSetLength; index++)
	{
		if(bBmode)//dot11_phy_type_hrdsss
		{
			if( (( pRtRateSet->Octet[index] & 0x7F) == 2  ) || (( pRtRateSet->Octet[index]  & 0x7F) == 4 ) ||
			     (( pRtRateSet->Octet[index] & 0x7F) == 11) || (( pRtRateSet->Octet[index]  & 0x7F) == 22) )
			{
				pDot11RateSet->ucRateSet[opRateIdx] = pRtRateSet->Octet[index] & 0x7F;
				opRateIdx++;			
			}	
		}
		else // dot11_phy_type_erp  G mdoe or dot11_phy_type_ofdm  A mdoe
		{
			pDot11RateSet->ucRateSet[index] = pRtRateSet->Octet[index] & 0x7F;
			opRateIdx++;
		}	
	}
	pDot11RateSet->uRateSetLength=opRateIdx;

	*BytesWritten = *BytesNeeded;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuery_DOT11_BEACON_PERIOD(
	IN PADAPTER				pAdapter,
	OUT PULONG				pResult
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	*pResult = pMgntInfo->dot11BeaconPeriod;
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_BEACON_PERIOD(): %d TU\n", *pResult));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_SHORT_RETRY_LIMIT(
	IN PADAPTER				pAdapter,
	OUT PULONG				pResult
	)
{
	*pResult = MgntActQuery_ShortRetryLimit(pAdapter);

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_SHORT_RETRY_LIMIT(): %d times\n", *pResult));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_LONG_RETRY_LIMIT(
	IN PADAPTER				pAdapter,
	OUT PULONG				pResult
	)
{
	*pResult = MgntActQuery_LongRetryLimit(pAdapter);

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_LONG_RETRY_LIMIT(): %d times\n", *pResult));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_MAX_TRANSMIT_MSDU_LIFETIME(
	IN PADAPTER				pAdapter,
	OUT PULONG				pResult
	)
{
	*pResult = MgntActQuery_MaxTxMsduLifeTime(pAdapter);
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_MAX_TRANSMIT_MSDU_LIFETIME(): %d TUs\n", *pResult));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CQuery_DOT11_MAX_RECEIVE_LIFETIME(
	IN PADAPTER				pAdapter,
	OUT PULONG				pResult
	)
{
	*pResult = MgntActQuery_MaxRxMpduLifeTime(pAdapter);
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_MAX_RECEIVE_LIFETIME(): %d TUs\n", *pResult));
	return NDIS_STATUS_SUCCESS; 
}


NDIS_STATUS
N6CQuery_DOT11_SUPPORTED_PHY_TYPES(
	IN PADAPTER					Adapter,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	NDIS_STATUS	ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_SUPPORTED_PHY_TYPES	pDot11SupportedPhyTypes = (PDOT11_SUPPORTED_PHY_TYPES)InformationBuffer;
	ULONG	uNumMaxEntries = 0;
	USHORT	Index;

	//
	// <RJ_NW_TODO> Reivse PHY related in N6usbInitializeNative80211MIBs().
	//
	
	do
	{
		*BytesWritten = 0;
		*BytesNeeded = 0;

		if (InformationBufferLength < (ULONG)FIELD_OFFSET(DOT11_SUPPORTED_PHY_TYPES, dot11PHYType))
		{
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = FIELD_OFFSET(DOT11_SUPPORTED_PHY_TYPES, dot11PHYType);
			break;
		}

		InformationBufferLength -= FIELD_OFFSET(DOT11_SUPPORTED_PHY_TYPES, dot11PHYType);
		uNumMaxEntries = InformationBufferLength / sizeof(DOT11_PHY_TYPE);

		if(uNumMaxEntries < pNdisCommon->pDot11SupportedPhyTypes->uTotalNumOfEntries) 
		{ // Buffer is not enough
			pDot11SupportedPhyTypes->uTotalNumOfEntries = pNdisCommon->pDot11SupportedPhyTypes->uTotalNumOfEntries;    
			pDot11SupportedPhyTypes->uNumOfEntries = uNumMaxEntries;
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
		}
		else 
		{
			pDot11SupportedPhyTypes->uTotalNumOfEntries = pNdisCommon->pDot11SupportedPhyTypes->uTotalNumOfEntries;    
			pDot11SupportedPhyTypes->uNumOfEntries = pDot11SupportedPhyTypes->uTotalNumOfEntries;
		}
		for (Index = 0; Index < pDot11SupportedPhyTypes->uNumOfEntries; Index++)
		{
			pDot11SupportedPhyTypes->dot11PHYType[Index] = pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[Index];
		}
	
		*BytesWritten = FIELD_OFFSET(DOT11_SUPPORTED_PHY_TYPES, dot11PHYType) +
						pDot11SupportedPhyTypes->uNumOfEntries * sizeof(DOT11_PHY_TYPE);
			
		*BytesNeeded = FIELD_OFFSET(DOT11_SUPPORTED_PHY_TYPES, dot11PHYType) +
						pDot11SupportedPhyTypes->uTotalNumOfEntries * sizeof(DOT11_PHY_TYPE);
	} while(FALSE);
	
	
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CQuery_DOT11_SUPPORTED_PHY_TYPES()\n"));
	return ndisStatus; 
}

NDIS_STATUS
N6CQuery_DOT11_ENUM_ASSOCIATION_INFO(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_ASSOCIATION_INFO_LIST	pAssoList = (PDOT11_ASSOCIATION_INFO_LIST)InformationBuffer;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	*BytesNeeded = FIELD_OFFSET(DOT11_ASSOCIATION_INFO_LIST, dot11AssocInfo);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pAssoList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_ASSOCIATION_INFO_LIST_REVISION_1,
		sizeof(DOT11_ASSOCIATION_INFO_LIST));

	pAssoList->uNumOfEntries = 0;
	pAssoList->uTotalNumOfEntries = 0;
	
	if ( (pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE) && (pMgntInfo->mAssoc == TRUE) )
	{
		*BytesNeeded = FIELD_OFFSET(DOT11_ASSOCIATION_INFO_LIST, dot11AssocInfo) +
			sizeof(DOT11_ASSOCIATION_INFO_EX);
		if ( InformationBufferLength < *BytesNeeded )
		{
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
		}
		else
		{
			PlatformMoveMemory(
				pAssoList->dot11AssocInfo[0].PeerMacAddress,
				pMgntInfo->Bssid,
				sizeof(DOT11_MAC_ADDRESS));
			PlatformMoveMemory(
				pAssoList->dot11AssocInfo[0].BSSID,
				pMgntInfo->Bssid,
				sizeof(DOT11_MAC_ADDRESS));
			pAssoList->dot11AssocInfo[0].usCapabilityInformation = pMgntInfo->mCap;
			pAssoList->dot11AssocInfo[0].usListenInterval = pMgntInfo->ListenInterval;
			PlatformMoveMemory(
				pAssoList->dot11AssocInfo[0].ucPeerSupportedRates,
				pMgntInfo->Regdot11OperationalRateSet.Octet,
				pMgntInfo->Regdot11OperationalRateSet.Length);
			pAssoList->dot11AssocInfo[0].usAssociationID = pMgntInfo->mAId;
			pAssoList->dot11AssocInfo[0].dot11AssociationState = 
				(pMgntInfo->mAssoc == TRUE ? dot11_assoc_state_auth_assoc : dot11_assoc_state_auth_unassoc) ;
			pAssoList->dot11AssocInfo[0].dot11PowerMode = dot11_power_mode_active;
			pAssoList->dot11AssocInfo[0].liAssociationUpTime.QuadPart = 
				(pMgntInfo->AsocTimestamp == 0 ? 0 : PlatformGetCurrentTime() - pMgntInfo->AsocTimestamp);
			pAssoList->dot11AssocInfo[0].ullNumOfTxPacketSuccesses = Adapter->TxStats.NumTxOkTotal;
			pAssoList->dot11AssocInfo[0].ullNumOfTxPacketFailures = Adapter->TxStats.NumTxErrTotal;
			pAssoList->dot11AssocInfo[0].ullNumOfRxPacketSuccesses = Adapter->RxStats.NumRxOkTotal;
			pAssoList->dot11AssocInfo[0].ullNumOfRxPacketFailures = (Adapter->RxStats.NumRxErrTotalUnicast + Adapter->RxStats.NumRxErrTotalMulticast);

			pAssoList->uNumOfEntries = 1;
			pAssoList->uTotalNumOfEntries = 1;
		}
	}
	else if ( (pMgntInfo->OpMode == RT_OP_MODE_IBSS) && (pMgntInfo->mIbss == TRUE ) )
	{
		PRT_WLAN_STA	pEntry;
		ULONG			i;
		PDOT11_ASSOCIATION_INFO_EX	 pAssoInfo = pAssoList->dot11AssocInfo;

		for (i=0; (pEntry=AsocEntry_EnumStation(Adapter, i)) != NULL; i++)
		{
			pAssoList->uTotalNumOfEntries ++;
			
			*BytesNeeded += sizeof(DOT11_ASSOCIATION_INFO_EX);
			if (InformationBufferLength >= *BytesNeeded)
			{
				pAssoList->uNumOfEntries ++;

				PlatformZeroMemory(pAssoInfo, sizeof(DOT11_ASSOCIATION_INFO_EX));
				
				PlatformMoveMemory(
					pAssoInfo->PeerMacAddress,
					pEntry->MacAddr,
					sizeof(DOT11_MAC_ADDRESS));
				PlatformMoveMemory(
					pAssoInfo->BSSID,
					pMgntInfo->Bssid,
					sizeof(DOT11_MAC_ADDRESS));
				// <SC_TODO: to fix this>
				//pAssoInfo->usCapabilityInformation = pEntry
				pAssoInfo->usListenInterval = 0;
				PlatformMoveMemory(
					pAssoInfo->ucPeerSupportedRates,
					pEntry->bdSupportRateEXBuf,
					pEntry->bdSupportRateEXLen);
				pAssoInfo->usAssociationID = 0;
				pAssoInfo->dot11AssociationState = dot11_assoc_state_unauth_unassoc;
				if (pEntry->bPowerSave == TRUE)
				{
					pAssoInfo->dot11PowerMode = dot11_power_mode_powersave;
				}
				else
				{
					pAssoInfo->dot11PowerMode = dot11_power_mode_active;
				}
				pAssoInfo->liAssociationUpTime.QuadPart = 0;  // <SC_TODO: to cal the associate time>
				pAssoInfo->ullNumOfTxPacketSuccesses = Adapter->TxStats.NumTxOkTotal;
				pAssoInfo->ullNumOfTxPacketFailures = Adapter->TxStats.NumTxErrTotal;
				pAssoInfo->ullNumOfRxPacketSuccesses = Adapter->RxStats.NumRxOkTotal;
				pAssoInfo->ullNumOfRxPacketFailures = (Adapter->RxStats.NumRxErrTotalUnicast + Adapter->RxStats.NumRxErrTotalMulticast);

				pAssoInfo++;
			}
			else
			{
				ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
			}
		}
	}
	else
	{
		pAssoList->uNumOfEntries = 0;
		pAssoList->uTotalNumOfEntries = 0;
	}

	*BytesWritten = FIELD_OFFSET(DOT11_ASSOCIATION_INFO_LIST, dot11AssocInfo) +
		(pAssoList->uNumOfEntries * sizeof(DOT11_ASSOCIATION_INFO_EX));
	
	return ndisStatus;
}

NDIS_STATUS
N6CQuery_DOT11_MULTICAST_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	NDIS_STATUS 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	*BytesNeeded = Adapter->MCAddrCount * ETHERNET_ADDRESS_LENGTH;
	if(InformationBufferLength < *BytesNeeded)
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	PlatformMoveMemory(InformationBuffer, Adapter->MCList, Adapter->MCAddrCount * ETHERNET_ADDRESS_LENGTH);
	*BytesWritten = *BytesNeeded;
	return ndisStatus;
}

BOOLEAN
N6CQuery_DOT11_NIC_POWER_STATE(
	IN	PADAPTER	pAdapter	
	)
{
	RT_RF_POWER_STATE eRfPowerState;
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	BOOLEAN bRetVal = FALSE;

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&eRfPowerState));
	if(eRfPowerState == eRfOff)
	{
		// OID_DOT11_NIC_POWER_STATE should return Phy's software power state.
		if (pMgntInfo->RfOffReason & RF_CHANGE_BY_SW)
		{
			bRetVal = FALSE;
		}
		else
		{
			bRetVal = TRUE;
		}
	}
	else
	{
		bRetVal = TRUE;
	}

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
		("N6CQuery_DOT11_NIC_POWER_STATE: eRfPowerState: 0x%X,  RFOn 0x%X\n", eRfPowerState, bRetVal));

	return bRetVal;
}

//
// Description:
//	Return the table of data rates supported by a PHY on the 802.11 station for transmit and 
//	receive operations.
// Arguments:
//	[in] Adapter -
//		The NIC adapter context.
//	[out] InformationBuffer -
//		A pointer to a buffer into which the underlying driver or NDIS returns the requested
//		information for queries.
//	[in] InformationBufferLength -
//		The size, in bytes, of the buffer at InformationBuffer.
//	[out] BytesWritten -
//		The number of bytes that the underlying driver or NDIS transfers into the buffer at
//		InformationBuffer for query-information requests.
//	[out] BytesNeeded -
//		The number of bytes that are required to return query information.
// Return:
//	Return NDIS_STATUS_SUCCESS if success, or return NDIS error status otherwise.
// By Bruce, 2010-12-22.
//
NDIS_STATUS
N6CQuery_DOT11_DataRateMappingTable(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	NDIS_STATUS						ndisStatus = NDIS_STATUS_SUCCESS;
    PDOT11_DATA_RATE_MAPPING_TABLE	pDataRateMappingTable = NULL;	
    
	do
	{
		*BytesWritten = 0;
		*BytesNeeded = 0;
				
		if(InformationBufferLength < sizeof(DOT11_DATA_RATE_MAPPING_TABLE))
		{
			*BytesNeeded = sizeof(DOT11_DATA_RATE_MAPPING_TABLE);
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
		}
		PlatformZeroMemory(InformationBuffer, InformationBufferLength);

		pDataRateMappingTable = (PDOT11_DATA_RATE_MAPPING_TABLE)InformationBuffer;

		N6_ASSIGN_OBJECT_HEADER(pDataRateMappingTable->Header, 
									NDIS_OBJECT_TYPE_DEFAULT,
									DOT11_DATA_RATE_MAPPING_TABLE_REVISION_1,
									sizeof(DOT11_DATA_RATE_MAPPING_TABLE));

		pDataRateMappingTable->uDataRateMappingLength = sizeof(N6_Std_abg_DataRateMappingTable) / 
                                                        sizeof(DOT11_DATA_RATE_MAPPING_ENTRY);

		// Ensure enough space 
        *BytesNeeded = FIELD_OFFSET(DOT11_DATA_RATE_MAPPING_TABLE, DataRateMappingEntries) +
                      sizeof(N6_Std_abg_DataRateMappingTable);

		if (InformationBufferLength < *BytesNeeded)
        {
            ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;            
            break;
        }

		 //
        // Copy the standard a/b/g data rate mapping table.
        //
        PlatformMoveMemory(pDataRateMappingTable->DataRateMappingEntries,
                       N6_Std_abg_DataRateMappingTable,
                       sizeof(N6_Std_abg_DataRateMappingTable));

        *BytesWritten = pDataRateMappingTable->uDataRateMappingLength * sizeof(DOT11_DATA_RATE_MAPPING_ENTRY) + 
            FIELD_OFFSET(DOT11_DATA_RATE_MAPPING_TABLE, DataRateMappingEntries);
            
        *BytesNeeded = pDataRateMappingTable->uDataRateMappingLength * sizeof(DOT11_DATA_RATE_MAPPING_ENTRY) +
            FIELD_OFFSET(DOT11_DATA_RATE_MAPPING_TABLE, DataRateMappingEntries);
    } while(FALSE);

    return ndisStatus;
}

//
// Description:
//	Return the tx/rx rates supported by the PLCP and PMD for the current phy type on the 802.11 station
//	for OID "OID_DOT11_SUPPORTED_DATA_RATES_VALUE".
// Arguments:
//	[in] Adapter -
//		The NIC adapter context.
//	[out] InformationBuffer -
//		A pointer to a buffer into which the underlying driver or NDIS returns the requested
//		information for queries.
//	[in] InformationBufferLength -
//		The size, in bytes, of the buffer at InformationBuffer.
//	[out] BytesWritten -
//		The number of bytes that the underlying driver or NDIS transfers into the buffer at
//		InformationBuffer for query-information requests.
//	[out] BytesNeeded -
//		The number of bytes that are required to return query information.
// Return:
//	Return NDIS_STATUS_SUCCESS if success, or return NDIS error status otherwise.
// By Bruce, 2011-01-07.
//
NDIS_STATUS
N6CQuery_DOT11_Supported_DataRates(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON						pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS								ndisStatus = NDIS_STATUS_SUCCESS;
	u1Byte									index = 0, rateCnt = 0;
	PDOT11_SUPPORTED_DATA_RATES_VALUE_V2    pDot11SupportedDataRatesValue = NULL;

	do
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;

        if (InformationBufferLength < sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2))
        {
            *BytesNeeded = sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2);
            ndisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
		PlatformZeroMemory(InformationBuffer, InformationBufferLength);

		pDot11SupportedDataRatesValue = (PDOT11_SUPPORTED_DATA_RATES_VALUE_V2)InformationBuffer;

		for(index = 0; index < sizeof(N6_Std_abg_DataRateMappingTable)/sizeof(DOT11_DATA_RATE_MAPPING_ENTRY); index ++)
		{
			switch(pNdisCommon->pDot11PhyMIBs[pNdisCommon->dot11SelectedPhyId].PhyType)
			{
			case dot11_phy_type_hrdsss: // B mode
				if(IS_CCK_RATE(N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex))
				{
					pDot11SupportedDataRatesValue->ucSupportedTxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					pDot11SupportedDataRatesValue->ucSupportedRxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					rateCnt ++;
				}
				break;
			case dot11_phy_type_erp: // G mode
				if(!IS_CCK_RATE(N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex))
				{
					pDot11SupportedDataRatesValue->ucSupportedTxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					pDot11SupportedDataRatesValue->ucSupportedRxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					rateCnt ++;
				}
				break;	
				
			case dot11_phy_type_ofdm: // A mode
				if(!IS_CCK_RATE(N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex))
				{
					pDot11SupportedDataRatesValue->ucSupportedTxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					pDot11SupportedDataRatesValue->ucSupportedRxDataRatesValue[rateCnt] = N6_Std_abg_DataRateMappingTable[index].ucDataRateIndex;
					rateCnt ++;
				}
				break;					

			}
		}        

        *BytesWritten = sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2);
    } while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6CQuery_DOT11_Tx_Antenna(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	u1Byte i, rfNum, rftype = RT_GetRFType(Adapter);
	PDOT11_SUPPORTED_ANTENNA_LIST Tx_list = (PDOT11_SUPPORTED_ANTENNA_LIST)InformationBuffer;

	switch(rftype){
		case RF_1T1R: case RF_1T2R:
			rfNum = 1; // MCS7
			break;
		case RF_2T2R: case RF_2T2R_GREEN:
			rfNum = 2; // MCS 15
			break;
		default:
			rfNum = 1;
			break;
	}	
	
	*BytesNeeded = FIELD_OFFSET(DOT11_SUPPORTED_ANTENNA_LIST, dot11SupportedAntenna) 
					+ (rfNum * sizeof(DOT11_SUPPORTED_ANTENNA));

	if ( InformationBufferLength < *BytesNeeded )
	{
		*BytesWritten = 0;
		Tx_list->uNumOfEntries = 0;
		Tx_list->uTotalNumOfEntries = rfNum;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	PlatformZeroMemory(Tx_list, InformationBufferLength);	

	Tx_list->uNumOfEntries = rfNum;
	Tx_list->uTotalNumOfEntries = rfNum;

	for(i = 0; i < rfNum; i++)
	{
		Tx_list->dot11SupportedAntenna[i].uAntennaListIndex = i+1;
		Tx_list->dot11SupportedAntenna[i].bSupportedAntenna = 1;
	}

	*BytesWritten = *BytesNeeded;
	*BytesNeeded = 0;

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	ndisStatus = N62CQuery_DOT11_DESIRED_PHY_LIST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesWritten,
			BytesNeeded
		);


	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_AUTO_CONFIG_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			ulInfo = 0;
	ULONG			ulInfoLen = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	*BytesNeeded = sizeof(ULONG);
	if ( InformationBufferLength < *BytesNeeded )
	{
		*BytesWritten = 0;
		return NDIS_STATUS_INVALID_LENGTH;
	}

	ulInfo = pTargetAdapter->pNdisCommon->dot11AutoConfigEnabled;
	ulInfoLen = *BytesNeeded;

	*BytesWritten = ulInfoLen;

	PlatformMoveMemory(InformationBuffer, &ulInfo, ulInfoLen);
		
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_BEACON_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pTargetAdapter->MgntInfo;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	if(!pMgntInfo->mAssoc && pMgntInfo->mIbss && ACTING_AS_AP(pTargetAdapter))
	{
		*BytesWritten = 0;
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		return ndisStatus;
	}
			
	*BytesNeeded = sizeof(ULONG);
	if( InformationBufferLength < *BytesNeeded )
	{
		*BytesWritten = 0;
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		return ndisStatus;
	} 

	ndisStatus = N6CQuery_DOT11_BEACON_PERIOD(pTargetAdapter, InformationBuffer);
	if (ndisStatus == NDIS_STATUS_SUCCESS)
	{
		*BytesWritten = *BytesNeeded;
	}
	
	return ndisStatus;
}



NDIS_STATUS
N6C_QUERY_OID_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pTargetAdapter->MgntInfo;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	ndisStatus = N6CQuery_DOT11_DESIRED_SSID_LIST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesWritten,
			BytesNeeded
		);
		
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_GEN_CURRENT_PACKET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pTargetAdapter->MgntInfo;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	if(InformationBufferLength < sizeof(ULONG))
	{
		*BytesNeeded = sizeof(ULONG);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	ulInfo=pTargetAdapter->pNdisCommon->NdisPacketFilter;
	PlatformMoveMemory(InformationBuffer, &ulInfo, sizeof(ULONG));
	*BytesWritten = sizeof(ULONG);	
		
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pTargetAdapter->MgntInfo;
	ULONG			ulInfo = 0;
	ULONG			ulInfoLen = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	*BytesNeeded = sizeof(ULONG);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	if(pMgntInfo->mAssoc || pMgntInfo->mIbss)
	{
		
	#if (MULTICHANNEL_SUPPORT == 1)
		if(MultiChannelSwitchNeeded(pTargetAdapter))
		{
			ulInfo = MultiChannelGetPortConnected20MhzChannel(pTargetAdapter);
			RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("MultiChannelGetPortConnected20MhzChannel: %d\n", ulInfo));
		}
		else
	#endif				
		{
			ulInfo = pMgntInfo->dot11CurrentChannelNumber;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("A: channel: %u:\n", ulInfo));
	}
	else
	{
		//
		//Report the default channel to os, Or it will fail when in SoftAP_connectOID test.
		//receive non probe rsp. By Maddest 06052009.
		//
		//sherry added for Softap_wps error
		//20110927
		if(pTargetAdapter->bInHctTest)
		{
			if(!IsDefaultAdapter(pTargetAdapter))
				ulInfo = pMgntInfo->dot11CurrentChannelNumber;
			else
				ulInfo = RT_GetChannelNumber(GetDefaultAdapter(pTargetAdapter));
		}
		else
		{
			ulInfo = RT_GetChannelNumber(GetDefaultAdapter(pTargetAdapter));
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("B: channel: %u:\n", ulInfo));
	}

	ulInfoLen = sizeof(ULONG);

	PlatformMoveMemory(InformationBuffer, &ulInfo, ulInfoLen);
	*BytesWritten = sizeof(ULONG);	

	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_GEN_LINK_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pTargetAdapter->MgntInfo;
	ULONG			ulInfo = 0;
	ULONG			ulInfoLen = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	if(InformationBufferLength < sizeof(NDIS_LINK_PARAMETERS))
	{
		*BytesNeeded = sizeof(NDIS_LINK_PARAMETERS);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}			

	ndisStatus = N62CQueryLinkParameters(
			pTargetAdapter, 
			InformationBuffer, 
			InformationBufferLength
		);

	*BytesNeeded = sizeof(NDIS_LINK_PARAMETERS);			

	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_SAFE_MODE_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	PMGNT_INFO      pMgntInfo = &pTargetAdapter->MgntInfo;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	if(InformationBufferLength < sizeof(BOOLEAN))
	{
		*BytesNeeded = sizeof(BOOLEAN);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

#if 1
	ulInfo = pMgntInfo->SafeModeEnabled;
#else
	ulInfo = FALSE;
#endif
	PlatformMoveMemory(InformationBuffer, &ulInfo, sizeof(BOOLEAN));
	*BytesWritten = sizeof(BOOLEAN);			
			
	return ndisStatus;
}


NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_PHY_ID(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	if(InformationBufferLength < sizeof(ULONG))
	{
		*BytesNeeded = sizeof(ULONG);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	ulInfo = pTargetAdapter->pNdisCommon->dot11SelectedPhyId;
	PlatformMoveMemory(InformationBuffer, &ulInfo, sizeof(ULONG));
	*BytesWritten = sizeof(ULONG);	

	RT_TRACE(COMP_OID_QUERY|COMP_RF, DBG_LOUD, ("Query OID_DOT11_CURRENT_PHY_ID: phyid%d\n", ulInfo));

	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_GEN_SUPPORTED_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	pInfo = (PVOID) N6GetSupportedOids();
	ulInfoLen = N6GetSupportedOidsLength();

	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;
	
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_GEN_VENDOR_DRIVER_VERSION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	ulInfo = NIC_VENDOR_DRIVER_VERSION;
	pInfo = (PVOID) &ulInfo;
	ulInfoLen = sizeof(ULONG);
		
	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;
	
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_PNP_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PNP_CAPABILITIES    PnpCapabilities;
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY|COMP_POWER);

       if(!pTargetAdapter->bInHctTest && pTargetAdapter->bUnloadDriverwhenS3S4)
	{
		RT_TRACE((COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("Query Information: OID_PNP_CAPABILITIES, NOT support remote wake up\n"));
		ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
		return ndisStatus;
	}

	FillPnpCapabilities(pTargetAdapter, &PnpCapabilities);

	pInfo = (PVOID)&PnpCapabilities;
	ulInfoLen = sizeof(PnpCapabilities);
		
	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;
	
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_PNP_QUERY_POWER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	// Report the lowest device power state to sleep.		
	ulInfo = (ULONG)(NdisDeviceStateD3);
	pInfo = (PVOID)&ulInfo;
	ulInfoLen = sizeof(ULONG);
		
	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;
	
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_PNP_ENABLE_WAKE_UP(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO			pMgntInfo = &pTargetAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
		
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly) // Added by tynli. 2009.09.02.
		ulInfo = (ULONG)(NDIS_PNP_WAKE_UP_MAGIC_PACKET);
	else if(pPSC->WoWLANMode == eWakeOnPatternMatchOnly)
		ulInfo = (ULONG)(NDIS_PNP_WAKE_UP_PATTERN_MATCH);
	else if(pPSC->WoWLANMode == eWakeOnBothTypePacket)
		ulInfo = (ULONG)(NDIS_PNP_WAKE_UP_MAGIC_PACKET|NDIS_PNP_WAKE_UP_PATTERN_MATCH);
					
	pInfo = (PVOID)&ulInfo;
	ulInfoLen = sizeof(ULONG);


	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;

	RT_TRACE((COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("Query Information, OID_PNP_ENABLE_WAKE_UP: %d\n", ulInfo));
	
	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_TX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;		
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		


	// ======================
	// This OID is not implemented. 
	// ======================


	
	ulInfo = 0;
	pInfo = (PVOID)&ulInfo;
	ulInfoLen = sizeof(ULONG);


	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;

	return ndisStatus;
}


NDIS_STATUS
N6C_QUERY_OID_DOT11_CURRENT_RX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;		
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		

	// ======================
	// This OID is not implemented. 
	// ======================

	ulInfo = 0;
	pInfo = (PVOID)&ulInfo;
	ulInfoLen = sizeof(ULONG);


	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;

	return ndisStatus;
}

NDIS_STATUS
N6C_QUERY_OID_DOT11_SUPPORTED_RX_ANTENNA(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;		
	PVOID			pInfo = NULL;
	ULONG			ulInfoLen = 0;
	ULONG			ulInfo = 0;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);		




	// ======================
	// This OID is not implemented. 
	// ======================


	
	ulInfo = 0;
	pInfo = (PVOID)&ulInfo;
	ulInfoLen = sizeof(ULONG);


	if(InformationBufferLength < ulInfoLen)
	{
		*BytesNeeded = ulInfoLen;
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
	
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
	*BytesWritten = ulInfoLen;

	return ndisStatus;
}

