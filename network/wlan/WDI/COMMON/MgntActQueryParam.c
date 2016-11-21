#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntActQueryParam.tmh"
#endif

u1Byte SS_Rate_Map_G[6][2]= {{40, MGN_54M}, {30, MGN_48M}, {20, MGN_36M}, {12, MGN_24M}, {7, MGN_18M}, {0, MGN_12M}};
u1Byte SS_Rate_Map_B[2][2]= {{7, MGN_11M}, {0, MGN_5_5M}};
//
// 2011/07/08 MH For ALPHA link speedn display, we will add several level for them temporaily. Becasue
// They still not make sure which one is suitable for DWA-133 now. This is a tempoarily revise. We need to rebuild
// a structure for different rate display to prevent wasting memory..
//

u1Byte SS_Rate_Map_AC_1SS_MCS9[10][2]= {{46, MGN_VHT1SS_MCS9},{43, MGN_VHT1SS_MCS8},{39, MGN_VHT1SS_MCS7}, {35, MGN_VHT1SS_MCS5}, {29, MGN_VHT1SS_MCS4}, {25, MGN_VHT1SS_MCS3}, {20, MGN_VHT1SS_MCS2}, {15, MGN_VHT1SS_MCS1}, {10, MGN_VHT1SS_MCS0},{0, MGN_6M}};
//(10以下請顯示6M in 5G)
u1Byte SS_Rate_Map_AC_1SS_MCS7[8][2]= {{41, MGN_VHT1SS_MCS7}, {35, MGN_VHT1SS_MCS5}, {29, MGN_VHT1SS_MCS4}, {25, MGN_VHT1SS_MCS3}, {20, MGN_VHT1SS_MCS2}, {15, MGN_VHT1SS_MCS1}, {10, MGN_VHT1SS_MCS0},{0, MGN_6M}};
//(10以下請顯示6M in 5G)

u1Byte SS_Rate_Map_N_MCS7[7][2]= {{40, MGN_MCS7}, {30, MGN_MCS5}, {25, MGN_MCS4}, {23, MGN_MCS3}, {19, MGN_MCS2}, {8, MGN_MCS1}, {0, MGN_MCS0}};
u1Byte SS_Rate_Map_N_MCS7_Lv1[7][2]= {{40, MGN_MCS7}, {30, MGN_MCS5}, {25, MGN_MCS4}, {19, MGN_MCS3}, {8, MGN_MCS2}, {5, MGN_MCS1}, {2, MGN_MCS0}};
u1Byte SS_Rate_Map_N_MCS7_Lv2[7][2]= {{40, MGN_MCS7}, {30, MGN_MCS6}, {25, MGN_MCS5}, {19, MGN_MCS4}, {8, MGN_MCS3},  {5, MGN_MCS2}, {2, MGN_MCS0}};
u1Byte SS_Rate_Map_N_MCS7_Lv3[5][2]= {{40, MGN_MCS7}, {30, MGN_MCS6}, {20, MGN_MCS4}, {8, MGN_MCS3},  {2, MGN_MCS1}};
u1Byte SS_Rate_Map_N_MCS7_Lv4[5][2]= {{40, MGN_MCS7}, {30, MGN_MCS6}, {10, MGN_MCS4}, {5, MGN_MCS3},  {1, MGN_MCS1}};
u1Byte SS_Rate_Map_N_MCS7_Lv5[5][2]= {{40, MGN_MCS7}, {10, MGN_MCS6}, {8, MGN_MCS5}, {2, MGN_MCS3},  {1, MGN_MCS1}};
u1Byte SS_Rate_Map_N_MCS7_Lv6[3][2]= {{20, MGN_MCS7}, {5, MGN_MCS6}, {2, MGN_MCS5}};

// For rate display for 2T2R.
u1Byte SS_Rate_Map_AC_2SS_MCS9[13][2]= {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS5}, {32, MGN_VHT2SS_MCS4}, {28, MGN_VHT1SS_MCS7}, {24, MGN_VHT1SS_MCS7},  {21, MGN_VHT1SS_MCS6}, {18, MGN_VHT1SS_MCS5}, {14, MGN_VHT1SS_MCS3}, {10, MGN_12M}, {0, MGN_6M}}; 
// Lvl1 replace as original value.
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv1[13][2]=  {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS4}, {32, MGN_VHT1SS_MCS6}, {28, MGN_VHT1SS_MCS4}, {24, MGN_VHT1SS_MCS3},  {21, MGN_VHT1SS_MCS2}, {18, MGN_VHT1SS_MCS1}, {14, MGN_VHT1SS_MCS0}, {10, MGN_12M}, {0, MGN_6M}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv2[11][2]= {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS4}, {32, MGN_VHT1SS_MCS6}, {28, MGN_VHT1SS_MCS4}, {24, MGN_VHT1SS_MCS3},  {20, MGN_VHT1SS_MCS2}, {10, MGN_VHT1SS_MCS1}, {0, MGN_12M}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv3[9][2]= {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS5}, {32, MGN_VHT1SS_MCS7}, {20, MGN_VHT1SS_MCS4}, {10, MGN_VHT1SS_MCS1},  {0, MGN_VHT1SS_MCS0}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv4[7][2]= {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {44, MGN_VHT2SS_MCS7}, {30, MGN_VHT2SS_MCS6}, {20, MGN_VHT2SS_MCS5}, {10, MGN_VHT1SS_MCS4}, {0, MGN_VHT1SS_MCS1}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv5[5][2]= {{50, MGN_VHT2SS_MCS9}, {47, MGN_VHT2SS_MCS8}, {30, MGN_VHT2SS_MCS7}, {15, MGN_VHT2SS_MCS4}, {0, MGN_VHT1SS_MCS2}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS9_Lv6[5][2]= {{45, MGN_VHT2SS_MCS9}, {40, MGN_VHT2SS_MCS8}, {30, MGN_VHT2SS_MCS7}, {10, MGN_VHT2SS_MCS6}, {0, MGN_VHT2SS_MCS4}}; 

//(10以下請顯示6M in 5G)
u1Byte SS_Rate_Map_AC_2SS_MCS7[12][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS5}, {32, MGN_VHT2SS_MCS4}, {28, MGN_VHT1SS_MCS5}, {24, MGN_VHT1SS_MCS4},  {21, MGN_VHT1SS_MCS3}, {18, MGN_VHT1SS_MCS2}, {14, MGN_VHT1SS_MCS1}, {10, MGN_VHT1SS_MCS0}, {0, MGN_6M}}; 
// Lvl1 replace as original value.
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv1[12][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {36, MGN_VHT2SS_MCS4}, {32, MGN_VHT1SS_MCS6}, {28, MGN_VHT1SS_MCS4}, {24, MGN_VHT1SS_MCS3},  {21, MGN_VHT1SS_MCS2}, {18, MGN_VHT1SS_MCS1}, {14, MGN_VHT1SS_MCS0}, {10, MGN_12M}, {0, MGN_6M}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv2[6][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {30, MGN_VHT2SS_MCS4}, {20, MGN_VHT1SS_MCS4}, {10, MGN_VHT1SS_MCS1}, {0, MGN_VHT1SS_MCS1}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv3[5][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {30, MGN_VHT2SS_MCS4}, {15, MGN_VHT1SS_MCS3}, {0, MGN_VHT1SS_MCS1}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv4[5][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {30, MGN_VHT2SS_MCS5}, {15, MGN_VHT1SS_MCS4}, {0, MGN_VHT1SS_MCS1}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv5[5][2]= {{44, MGN_VHT2SS_MCS7}, {40, MGN_VHT2SS_MCS6}, {27, MGN_VHT2SS_MCS6}, {15, MGN_VHT1SS_MCS5}, {0, MGN_VHT1SS_MCS1}}; 
u1Byte SS_Rate_Map_AC_2SS_MCS7_Lv6[3][2]= {{36, MGN_VHT2SS_MCS7}, {20, MGN_VHT2SS_MCS6}, {0, MGN_VHT2SS_MCS4}}; 

u1Byte SS_Rate_Map_AC_3SS_MCS9[12][2]= {{50, MGN_VHT3SS_MCS9}, {47, MGN_VHT3SS_MCS8}, {44, MGN_VHT3SS_MCS7}, {40, MGN_VHT3SS_MCS6}, {36, MGN_VHT3SS_MCS5}, {32, MGN_VHT3SS_MCS4}, {28, MGN_VHT3SS_MCS3}, {24, MGN_VHT3SS_MCS2}, {21, MGN_VHT3SS_MCS1}, {18, MGN_VHT3SS_MCS0}, {10, MGN_12M}, {0, MGN_6M}}; 

//(10以下請顯示6M in 5G)

u1Byte SS_Rate_Map_N_MCS15[7][2]= {{40, MGN_MCS15}, {35, MGN_MCS14}, {31, MGN_MCS12}, {28, MGN_MCS7}, {25, MGN_MCS5}, {23, MGN_MCS3}, {10, MGN_MCS0}};
u1Byte SS_Rate_Map_N_MCS15_Lv1[12][2]= {{40, MGN_MCS15}, {37, MGN_MCS14}, {35, MGN_MCS13}, {31, MGN_MCS12}, {28, MGN_MCS7}, {25, MGN_MCS6}, {22, MGN_MCS5}, {20, MGN_MCS4}, {15, MGN_MCS3}, {12, MGN_MCS2}, {8, MGN_MCS1},  {4, MGN_MCS0}};
u1Byte SS_Rate_Map_N_MCS15_Lv2[7][2]= {{40, MGN_MCS15}, {35, MGN_MCS14}, {28, MGN_MCS12}, {20, MGN_MCS5}, {10, MGN_MCS3}, {5, MGN_MCS2}, {2, MGN_MCS1}};
u1Byte SS_Rate_Map_N_MCS15_Lv3[5][2]= {{40, MGN_MCS15}, {30, MGN_MCS14}, {20, MGN_MCS12}, {10, MGN_MCS7}, {5, MGN_MCS2}};
u1Byte SS_Rate_Map_N_MCS15_Lv4[5][2]= {{40, MGN_MCS15}, {30, MGN_MCS14}, {10, MGN_MCS13}, {2, MGN_MCS7}, {1, MGN_MCS3}};
u1Byte SS_Rate_Map_N_MCS15_Lv5[5][2]= {{40, MGN_MCS15}, {10, MGN_MCS14}, {5, MGN_MCS13}, {2, MGN_MCS12}, {1, MGN_MCS4}};
u1Byte SS_Rate_Map_N_MCS15_Lv6[3][2]= {{20, MGN_MCS15}, {5, MGN_MCS14}, {2, MGN_MCS13}};

u1Byte SS_Rate_Map_N_MCS23[7][2]= {{40, MGN_MCS23}, {35, MGN_MCS22}, {31, MGN_MCS12}, {28, MGN_MCS7}, {25, MGN_MCS5}, {23, MGN_MCS3}, {10, MGN_MCS0}};

u1Byte MSI_SS_Rate_Map_G[6][2]= {{40, MGN_54M}, {30, MGN_54M}, {20, MGN_54M}, {12, MGN_48M}, {7, MGN_36M}, {0, MGN_24M}};

// Temporarily add for Lenovo.
u1Byte LNV_SS_Rate_Map_G[6][2]= {{54, MGN_54M}, {40, MGN_48M}, {30, MGN_36M}, {15, MGN_24M}, {10, MGN_18M}, {5, MGN_12M}};
u1Byte LNV_SS_Rate_Map_B[2][2]= {{10, MGN_11M}, {0, MGN_5_5M}};

u2Byte
CONVERT_RATE	(
	PADAPTER		Adapter,
	u2Byte			MGN_RATE
)	
{
	u2Byte	RetValue = MGN_RATE;
	
	if(MGN_RATE >= MGN_VHT1SS_MCS0 && MGN_RATE <= MGN_VHT4SS_MCS9)
		RetValue = VHTMcsToDataRate(Adapter, MGN_RATE);
	else if(MGN_RATE >= MGN_MCS0 && MGN_RATE <= MGN_MCS31)
		RetValue = HTMcsToDataRate(Adapter, MGN_RATE);
	
	return RetValue;
}		

// Returns the current AP MAC address
BOOLEAN
MgntActQuery_802_11_BSSID(
	PADAPTER		Adapter,
	pu1Byte          	bssidbuf
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	// <1>Set management bssid
	CopyMem( bssidbuf, pMgntInfo->Bssid, 6 );

	//TODO:	
	return TRUE;
}

BOOLEAN
MgntActQuery_802_11_ASSOCIATION_INFORMATION(
	PADAPTER								Adapter,
	PNDIS_802_11_ASSOCIATION_INFORMATION	pAssocInfo
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);

	pu1Byte	pDest = (pu1Byte)pAssocInfo + sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);

	PlatformZeroMemory(pAssocInfo, sizeof(NDIS_802_11_ASSOCIATION_INFORMATION));
	pAssocInfo->Length = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);

	//------------------------------------------------------
	// Association Request related information
	//------------------------------------------------------
	// Req_1. AvailableRequestFixedIEs
	pAssocInfo->AvailableRequestFixedIEs |= NDIS_802_11_AI_REQFI_CAPABILITIES|NDIS_802_11_AI_REQFI_CURRENTAPADDRESS;
	//pAssocInfo->AvailableResponseFixedIEs|= NDIS_802_11_AI_RESFI_CAPABILITIES|NDIS_802_11_AI_RESFI_STATUSCODE|NDIS_802_11_AI_REQFI_CURRENTAPADDRESS;

	// Req_2. RequestFixedIEs: 1.Capabilities, 2.ListenInterval, 3.CurrentAPAddress.
	//2004/07/29, kcwu, mCap should be used
	//pAssocInfo->RequestFixedIEs.Capabilities = pMgntInfo->Asoc_asCap;
	pAssocInfo->RequestFixedIEs.Capabilities = pMgntInfo->mCap;
	pAssocInfo->RequestFixedIEs.ListenInterval = pMgntInfo->ListenInterval;		// Added by Annie, 2006-05-110.
	PlatformMoveMemory(pAssocInfo->RequestFixedIEs.CurrentAPAddress, &pMgntInfo->Bssid[0], 6);

	// Req_3. RequestIELength
	// [WindowsDesignNoteForWPA2]
	// - This field contains the number of octets in the OffetRequestIEs buffer.
	// - This should be set to 0 if a request has not been made.

	// We handle RequestIELength and AsocReq IEs in the following code.


	// Req_4. OffsetRequestIEs
	// [WindowsDesignNoteForWPA2]
	// - This field contains the offset in the buffer containing any variable length information elements from the association or reassociation request message. 
	// - During a query, this contains the information elements that were in the last association or reassociate request attempt. The request may succeed or fail.
	pAssocInfo->OffsetRequestIEs = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);

	//------------------------------------------------------
	//					        Association Service
 	//				      <----------------------------->
	// Status = idle        Status = Wait_Aso, Wait_ReAso  Status = idle
	// mAsso!=1&&mIbss!=1   mAssoc!=1&&mIbss!=1		       mAssoc==1||mIbss==1
	// Return none		    Request return,Response none   Request return, Response return
	//------------------------------------------------------
	if(	(pMgntInfo->State_AsocService == STATE_Asoc_Idle && (pMgntInfo->mAssoc || pMgntInfo->mIbss ))		||
		(pMgntInfo->State_AsocService == STATE_Wait_Asoc_Response ||  pMgntInfo->State_AsocService == STATE_Wait_Reasoc_Response))
	{
		// IE_1. Last SSID in Association Request 				
		pDest[0] = (u1Byte)EID_SsId;
		pDest[1] = (u1Byte)pMgntInfo->Ssid.Length;
		
		PlatformMoveMemory((pu1Byte)pDest + 2,
			&pMgntInfo->Ssid.Octet[0], 
			pMgntInfo->Ssid.Length);
		pDest = (pu1Byte)pDest + (2 + pMgntInfo->Ssid.Length);
		pAssocInfo->RequestIELength += (2 + pMgntInfo->Ssid.Length);
					
		// IE_2. Last Supported Rate in Association Request 
		pDest[0] = EID_SupRates;
		pDest[1] = (u1Byte)pMgntInfo->Regdot11OperationalRateSet.Length;
		PlatformMoveMemory((pu1Byte)pDest + 2,
			pMgntInfo->Regdot11OperationalRateSet.Octet, 
			pMgntInfo->Regdot11OperationalRateSet.Length);
		pDest += (2 + pMgntInfo->Regdot11OperationalRateSet.Length);
		pAssocInfo->RequestIELength += (2 + pMgntInfo->Regdot11OperationalRateSet.Length);

		// IE_3. Last RSNIE in Association Request 	
		if( pSecInfo->SecLvl > RT_SEC_LVL_NONE )
		{
			pDest[0] = ( pSecInfo->SecLvl == RT_SEC_LVL_WPA )? EID_Vendor : EID_WPA2;
			pDest[1] = (u1Byte)pSecInfo->RSNIE.Length;		
			PlatformMoveMemory((pu1Byte)pDest + 2,
				pSecInfo->RSNIE.Octet, 
				pSecInfo->RSNIE.Length);
			pDest += (2 + pSecInfo->RSNIE.Length);
			pAssocInfo->RequestIELength += (2 + pSecInfo->RSNIE.Length);
		}

		CCX_AppendAssocReqCCKMIE(Adapter, pDest, &pAssocInfo->RequestIELength);

		RT_PRINT_DATA( COMP_SEC, DBG_TRACE, ("MgntActQuery_802_11_ASSOCIATION_INFORMATION(): RSNIE"), pSecInfo->RSNIE.Octet, pSecInfo->RSNIE.Length);
	}


	//------------------------------------------------------
	// Association Response related information
	//------------------------------------------------------

	if(	pMgntInfo->State_AsocService == STATE_Asoc_Idle	&&
		(pMgntInfo->mAssoc || pMgntInfo->mIbss )
		)
	{
		// Rsp_1. AvailableResponseFixedIEs
		pAssocInfo->AvailableResponseFixedIEs =
				NDIS_802_11_AI_RESFI_CAPABILITIES |
				NDIS_802_11_AI_RESFI_STATUSCODE  |
				NDIS_802_11_AI_RESFI_ASSOCIATIONID ;

		// Rsp_2. ResponseFixedIEs: 1.Capabilities, 2.StatusCode, 3.AssociationId
		pAssocInfo->ResponseFixedIEs.Capabilities = pMgntInfo->RspCapability;
		pAssocInfo->ResponseFixedIEs.StatusCode = pMgntInfo->RspStatusCode;
		pAssocInfo->ResponseFixedIEs.AssociationId = pMgntInfo->RspAssociationID;

		// Rsp_3. ResponseIELength
		// [WindowsDesignNoteForWPA2]
		// - This field contains the number of octets in the OffsetResponseIEs buffer.
		// - This must be 0 in length for a set.
		// - It must be set to 0 in a query if a response for the last request is not received.

		// It's already initialized 0 by PlatformZeroMemory().
		// We handle ResponseIELength and AsocRsp IEs in the following code.


		// Rsp_4. OffsetResponseIEs
		// [WindowsDesignNoteForWPA2]
		// - This field contains the offset in the buffer containing any variable length information elements from the association or reassociation response message. 
		// - During a query, this contains the information elements that were in the last association or reassociate response.
		pAssocInfo->OffsetResponseIEs = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) + pAssocInfo->RequestIELength;  

		CCX_AppendAssocRspCCKMIE(Adapter, pDest, &(pAssocInfo->ResponseIELength));

	}												  	
	
	return TRUE;
}


// Returns the SSID with which the NIC is associated. 
// The driver returns 0 SSIDLength if the NIC is not associated with any SSID.
BOOLEAN
MgntActQuery_802_11_SSID(
	PADAPTER		Adapter,
	pu1Byte          	ssidbuf,
	pu2Byte	          	ssidlen
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	// check ssidlen <= 32
	if( pMgntInfo->Ssid.Length <= 32 ){
		CopyMem( ssidbuf, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length );
		*ssidlen = pMgntInfo->Ssid.Length;
	}
	else{
		*ssidlen = 0;
		return FALSE;
	}


	return TRUE;
}




//	Description: 
//		Return the current transmit power level in dBm.
//
s4Byte
MgntActQuery_TX_POWER_DBM(
	PADAPTER		Adapter
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;	
	s4Byte			powerlevel;

	Adapter->HalFunc.GetTxPowerLevelHandler( Adapter, &powerlevel );
	
	//
	// Get the min power in dbm. By Bruce, 2009-04-07.
	// Because we may not really update the Tx Power from the CCX Cell Power IE, we shall compare the 
	// client configured power and cell power to get the min power returned to UI.
	//
	// Compare with the Client Configured Power
	if(powerlevel > pMgntInfo->ClientConfigPwrInDbm && pMgntInfo->ClientConfigPwrInDbm != UNSPECIFIED_PWR_DBM)
		powerlevel = pMgntInfo->ClientConfigPwrInDbm;

	CCX_QueryTxPower(Adapter, &powerlevel);

	return powerlevel;
}


// Returns either Infrastructure or IBSS, unknown.
RT_JOIN_NETWORKTYPE
MgntActQuery_802_11_INFRASTRUCTURE_MODE(
	PADAPTER		Adapter
)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	if(pMgntInfo->mAssoc)
	{
		return RT_JOIN_NETWORKTYPE_INFRA;
	}
	else if(pMgntInfo->mIbss)
	{
		return RT_JOIN_NETWORKTYPE_ADHOC;
	}
	else 	
	{
		return (RT_JOIN_NETWORKTYPE)(Adapter->MgntInfo.Regdot11networktype);
	}
}



// Returns the current fragmentation threshold in bytes.
u2Byte
MgntActQuery_802_11_FRAGMENTATION_THRESHOLD(
	PADAPTER		Adapter
)
{
	return Adapter->MgntInfo.FragThreshold;
}


//Returns the current RTS threshold.
u2Byte
MgntActQuery_802_11_RTS_THRESHOLD(
	PADAPTER		Adapter
)
{
	return Adapter->MgntInfo.dot11RtsThreshold;
}



//Returns the set of supported data rates that the radio is capable of running.
BOOLEAN
MgntActQuery_802_11_SUPPORTED_RATES(
	PADAPTER		Adapter,
	pu1Byte	        RateSetbuf,
	pu2Byte			RateSetlen
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if( pMgntInfo->Regdot11OperationalRateSet.Length < 16 ){
		*RateSetlen = pMgntInfo->Regdot11OperationalRateSet.Length;
		CopyMem( RateSetbuf, pMgntInfo->Regdot11OperationalRateSet.Octet, *RateSetlen );
	}
	else{
		return FALSE;
	}	

	return TRUE;
}


BOOLEAN
MgntActQuery_802_11_BSSID_LIST(
	PADAPTER				Adapter,
	PRT_802_11_BSSID_LIST	pBssidList,
	BOOLEAN					bRealCase
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u4Byte		i=0;
	BOOLEAN		matchFound = FALSE;
	PRT_WLAN_BSS		pBSSDesc = NULL;
		
	u1Byte	Rssibuf[MAX_BSS_DESC];
	u2Byte	idxBuf[MAX_BSS_DESC];

	u1Byte	MaxRSSI=0;
	u2Byte	a=0, idx=0, findidx =0, tmpidx=0;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST() ===> \n"));
	//
	// 061214, rcnjko: Report no BSS scanned if RF is off.
	//
	if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS)
	{
		pBssidList->NumberOfItems = 0;
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST() <===, return by RF_CHANGE_BY_PS\n"));
		return TRUE;
	}
	if(ACTING_AS_AP(Adapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST() <===, return by ACTING_AS_AP\n"));
		return TRUE;	// ap mode don't support this oid.
	}

	// This scan list is empty.
	if(pMgntInfo->bFlushScanList)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST() <===, return by bFlushScanList\n"));
		return TRUE;
	}
	//
	// To protect the copying of scan list, using the scan spin lock.
	//
	//
	// This part of copying WLAN BSS from NumBssDesc4Query may conflict with that
	// in ScanComplete(), so that there would be no consistency between  "NumBssDesc4Query" 
	// and "NumBssDesc". After that, some BSS we access with incorrect length infomation will
	// access the invlaid memory or make system crash.
	// By Bruce, 2008-05-29.

	PlatformAcquireSpinLock(Adapter, RT_SCAN_SPINLOCK);
	if(pMgntInfo->NumBssDesc4Query == 0 && pMgntInfo->NumBssDesc > 0)
	{ // Update Bssid list for query as early as possible, 2005.07.22, by rcnjko.
		u2Byte	tmpNumBssDesc;

		tmpNumBssDesc = pMgntInfo->NumBssDesc;
		pMgntInfo->NumBssDesc4Query = tmpNumBssDesc;

		//
		// Roger, 071203: We should redirect the Octet pointer to the new destination. 
		// Rcnjko, 080201: We'd better use the macro CopyWlanBss() to duplicate 
		// RT_WLAN_BSS objects to handle OCTET_STRING copy issues.
		//
		for(i = 0; i < tmpNumBssDesc; i ++)
		{
			CopyWlanBss(pMgntInfo->bssDesc4Query + i, pMgntInfo->bssDesc + i);
		}
	}

	if( bRealCase == TRUE )
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST() indicate Real scan \n"));
		pBssidList->NumberOfItems = (pMgntInfo->NumBssDesc<=MAX_BSS_DESC) ? pMgntInfo->NumBssDesc: MAX_BSS_DESC;
		pBSSDesc = pMgntInfo->bssDesc;
	}
	else
	{
		pBssidList->NumberOfItems = (pMgntInfo->NumBssDesc4Query<=MAX_BSS_DESC) ? pMgntInfo->NumBssDesc4Query: MAX_BSS_DESC;
		pBSSDesc = pMgntInfo->bssDesc4Query;
	}

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActQuery_802_11_BSSID_LIST(), pBssidList->NumberOfItems=%d !!\n", pBssidList->NumberOfItems));
	for(i = 0; i < pBssidList->NumberOfItems; i++)
	{
		Rssibuf[i]= pBSSDesc[i].RSSI;
		idxBuf[i] = (u2Byte)i;
	}
	//Select sort for rssi. 
	for(idx=0;idx<pBssidList->NumberOfItems;idx++)
	{
		MaxRSSI =0;
		findidx =0;
		for(a=idx;a<pBssidList->NumberOfItems;a++)
		{
			if(MaxRSSI<Rssibuf[a])
			{
				MaxRSSI = Rssibuf[a];
				findidx =a;
			}
		}	
		 
		Rssibuf[findidx] =Rssibuf[idx];
		Rssibuf[idx] = MaxRSSI;
		tmpidx = idxBuf[findidx];
		idxBuf[findidx] = idxBuf[idx];
		idxBuf[idx] = tmpidx;
	}

	for(i = 0; i < pBssidList->NumberOfItems; i++)
	{
		CopyWlanBss(&pBssidList->pbssidentry[i], &pBSSDesc[idxBuf[i]]);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);

	// An workaround to prevent that we show empty SSID for the AP we've associated.
	// 2005.02.18, by rcnjko.
	if(pMgntInfo->mAssoc == TRUE)
	{
		BOOLEAN	bCurrApFound = FALSE;
		RT_WLAN_BSS *pBssdesc = NULL;

		// Find the entry of AP associated.
		for(i = 0;i < pBssidList->NumberOfItems; i++)
		{
			if(	(pBssidList->pbssidentry[i].bdCap & cIBSS) == 0 &&
				PlatformCompareMemory(pBssidList->pbssidentry[i].bdBssIdBuf, pMgntInfo->Bssid, 6) == 0 )
			{
				bCurrApFound = TRUE;
				pBssdesc = &(pBssidList->pbssidentry[i]); 
				break;
			}
		}
		
		if(bCurrApFound == TRUE && pBssdesc != NULL)
		{
			if(BeHiddenSsid(pBssdesc->bdSsIdBuf, pBssdesc->bdSsIdLen))
			{
				CopySsid(pBssdesc->bdSsIdBuf, pBssdesc->bdSsIdLen, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
			}
		}
		else
		{
			RT_TRACE(COMP_DBG, DBG_LOUD, ("We do not find the AP currently associated !!!\n"));
		}
	}

	return	TRUE;
}





//Current authentication mode.
BOOLEAN
MgntActQuery_802_11_AUTHENTICATION_MODE(
	PADAPTER		Adapter,
	PRT_AUTH_MODE	pauthmode
)
{
	*pauthmode = Adapter->MgntInfo.SecurityInfo.AuthMode;

	return TRUE;
}




//Returns the current encryption status.
BOOLEAN
MgntActQuery_802_11_ENCRYPTION_STATUS(
	PADAPTER		Adapter,
	PRT_ENC_ALG		pEncAlgorithm
)
{
	*pEncAlgorithm = Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm;
	
	return TRUE;
}



BOOLEAN
MgntActQuery_802_11_ENCRYPTION_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	pu4Byte			KeyLength,
	pu1Byte			KeyMaterial
)
{
	*KeyLength = Adapter->MgntInfo.SecurityInfo.KeyLen[KeyIndex];
	*KeyLength  = (*KeyLength<32)? *KeyLength :32;
	
	PlatformMoveMemory( 
			KeyMaterial,
			Adapter->MgntInfo.SecurityInfo.KeyBuf[KeyIndex], 
			*KeyLength
			);

	return TRUE;
}


u1Byte
MgntActQuery_802_11_CHANNEL_NUMBER(
	PADAPTER		Adapter
)
{
	return Adapter->MgntInfo.dot11CurrentChannelNumber;
}


//
// Description:
//	Return Tx rate.
//	2006.10.20, by shien chang.
//
u2Byte
MgntActQuery_802_11_TX_RATES(
	PADAPTER		Adapter
)
{
	u2Byte		rate;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	// Return current Tx capability.
	if(IS_WIRELESS_MODE_A(Adapter))
		rate = MGN_54M; // 54 M
	else if(IS_WIRELESS_MODE_G(Adapter))
		rate = MGN_54M; // 54 M
	else if(IS_WIRELESS_MODE_B(Adapter))
		rate = MGN_11M; // 11 M
	else if(IS_WIRELESS_MODE_AC(Adapter))
		rate = VHTMcsToDataRate(Adapter, pMgntInfo->VHTHighestOperaRate); 
	else if(IS_WIRELESS_MODE_N(Adapter))
		rate = HTMcsToDataRate(Adapter, pMgntInfo->HTHighestOperaRate); 
	else
		rate = MGN_1M; // 1 M

	return rate;
}


u2Byte
MgntActQuery_802_11_RX_RATES(
	PADAPTER		Adapter
)
{
	u2Byte		rate;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u1Byte 		Rf_Type = RT_GetRFType(Adapter);

	if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_A)
		rate = MGN_54M; // 54 M
	else if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_G)
		rate = MGN_54M; // 54 M
	else if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_B)
		rate = MGN_11M; // 11 M
	else if(IS_WIRELESS_MODE_N_24G(Adapter) || IS_WIRELESS_MODE_N_5G(Adapter))
	{
		// Since we cannot know the Tx capability of peer STA, we use Rx capability as Rx rate.
		if( (Rf_Type == RF_1T1R) ||
		    (Rf_Type == RF_1T2R ) ||
		    (Rf_Type == RF_2T2R && pMgntInfo->HTHighestOperaRate <= MGN_MCS7))
			rate = HTMcsToDataRate(Adapter, MGN_MCS7);
		else if(Rf_Type >= RF_MAX_TYPE)
		{
			if(IS_WIRELESS_MODE_N_24G(Adapter))
				rate = MGN_1M;
			else
				rate = MGN_6M;
		}
		else if(Rf_Type == RF_2T2R)
			rate = HTMcsToDataRate(Adapter, MGN_MCS15);
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_2T3R)
			rate = VHTMcsToDataRate(Adapter, MGN_MCS23);
		else if(Rf_Type == RF_2T4R)
			rate = VHTMcsToDataRate(Adapter, MGN_MCS31);
		else
			rate = HTMcsToDataRate(Adapter, MGN_MCS7);
	}
	else if(IS_WIRELESS_MODE_AC(Adapter))
	{
		if(Rf_Type == RF_2T2R)
			rate = VHTMcsToDataRate(Adapter, MGN_VHT2SS_MCS9);
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_2T3R)
			rate = VHTMcsToDataRate(Adapter, MGN_VHT3SS_MCS9);
 		else if(Rf_Type == RF_2T4R)
			rate = VHTMcsToDataRate(Adapter, MGN_VHT4SS_MCS9);
		else
			rate = VHTMcsToDataRate(Adapter, MGN_VHT1SS_MCS9);
	}
	else
		rate = MGN_1M; // 1 M				

	return rate;
}

WIRELESS_MODE
MgntActQuery_802_11_WIRELESS_MODE(
	PADAPTER		Adapter
)
{
	return Adapter->RegWirelessMode;
}


BOOLEAN
MgntActQuery_802_11_RETRY_LIMIT(
	PADAPTER Adapter, 
	pu2Byte ShortRetryLimit, 
	pu2Byte LongRetryLimit
)
{
	//RTL8185_TODO: Get short/long retry limit
	*ShortRetryLimit = 7;
	*LongRetryLimit = 7;
	RT_TRACE(COMP_INIT, DBG_SERIOUS,("TODO: Get short/long retry limit.\n") );

	return TRUE;
}



BOOLEAN
MgntActQuery_MultiDomainImp(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: Implemented. 
	//
	return FALSE;
}


BOOLEAN
MgntActQuery_CfPollable(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: Consider current BSS type and return TRUE if underlying HW supported.
	//
	return FALSE;
}

BOOLEAN
MgntActQuery_StrictlyOrderedImp(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: Implement it if necessary. 
	//
	return FALSE;
}

u4Byte
MgntActQuery_ShortRetryLimit(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: Implement it. 
	//
	return 7;
}

u4Byte
MgntActQuery_LongRetryLimit(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: Implement it. 
	//
	return 7;
}

u4Byte
MgntActQuery_MaxTxMsduLifeTime(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: use it to timeout MSDU in wait queue. 
	//
	return 512;
}


u4Byte
MgntActQuery_MaxRxMpduLifeTime(
	IN	PADAPTER		Adapter
)
{
	//
	// TODO: use it to timeout fragmentation in queue.
	//
	return 512;
}

u4Byte
MgntActQuery_ExcludedMacAddressList(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pMacAddrList,
	IN	u4Byte			BufLength
)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte		BytesNeeded = (pMgntInfo->ExcludedMacAddrListLength) * 6;

	PlatformZeroMemory(pMacAddrList, BufLength);
	
	if (BytesNeeded > BufLength)
	{
		return 0;	
	}

	PlatformMoveMemory(
		pMacAddrList,
		pMgntInfo->ExcludedMacAddr,
		BytesNeeded);
	return pMgntInfo->ExcludedMacAddrListLength;
}


PRT_CHANNEL_LIST
MgntActQuery_ChannelList(
	IN	PADAPTER		Adapter
	)
{
	PRT_CHANNEL_LIST	pChannelList = NULL;

	// Reconstruct channel list so that channel info modified during customized scan (or other function) could be recovered.
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_CONSTRUCT, NULL, NULL);
	
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, (&pChannelList));
	
	return pChannelList;
}

//
//	Description:
//		Get information about logs supports.
//
BOOLEAN
MgntActQuery_DrvLogTypeList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_TYPE_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
	)
{
#if !DRV_LOG
	*pBytesWritten = *pBytesNeeded = 0;
	return FALSE;

#else
	BOOLEAN bResult;
	u4Byte idx;
	u4Byte DescLen, EntrySize;
	PDRV_LOG_TYPE_ATTRIBUTE_T pEntry;

	//
	// Check minimal buffer size required.
	//
	*pBytesWritten = 0;
	*pBytesNeeded = sizeof(DRV_LOG_TYPE_LIST_T) - sizeof(DRV_LOG_TYPE_ATTRIBUTE_T);
	if(BufferLength < *pBytesNeeded)
		return FALSE;

	//
	// Enumerate into each entry and fill up if we have enough space.
	//
	bResult = TRUE;
	*pBytesWritten = *pBytesNeeded;
	pBuffer->Count = 0;
	pEntry = pBuffer->LogTypeAttributes;
	for(idx = 0; idx < LTYPE_TOTAL_COUNT; idx++)
	{
		//
		// Figure out string length which is not including EOS.
		//
		DescLen = 0;
		while(g_LogTypes[idx].Description[DescLen] != 0)
		{
			if(DescLen < (MAX_LOG_DESC_LEN-1))
			{
				DescLen++;
			}
			else
			{ // This is an unexpted condition and might cause buffer overflow.
				RT_ASSERT(FALSE, ("MgntActQuery_DrvLogTypeList(): check g_LogTypes[%ld].Description!!!\n", idx));

				g_LogTypes[idx].Description[DescLen] = 0; 
				break;
			}
		}

		//
		// Figure out entry size and add it to *pBytesNeeded.
		//
		EntrySize = sizeof(DRV_LOG_TYPE_ATTRIBUTE_T) + DescLen;
		*pBytesNeeded += EntrySize;

		//
		// Fill up this entry if there is enough space.
		//
		if(BufferLength >= *pBytesNeeded)
		{
			pEntry->MaxLogCountPwr = g_LogTypes[idx].MaxLogCountPwr;
			pEntry->DescLen = DescLen + 1; // 1 is for EOS.
			PlatformMoveMemory(
				pEntry->Description, 
				g_LogTypes[idx].Description, 
				pEntry->DescLen);

			*pBytesWritten += EntrySize;
			pEntry = (PDRV_LOG_TYPE_ATTRIBUTE_T)((pu1Byte)(pEntry->Description) + pEntry->DescLen);
			pBuffer->Count++;
		}
		else
		{
			bResult = FALSE;
		}
	}
	
	return bResult;

#endif
}


//
//	Description:
//		Get information about logs supports.
//
BOOLEAN
MgntActQuery_DrvLogAttrList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_ATTR_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
	)
{
#if !DRV_LOG
	*pBytesWritten = *pBytesNeeded = 0;
	return FALSE;

#else
	BOOLEAN bResult;
	u4Byte idx;
	u4Byte DescLen, EntrySize;
	PDRV_LOG_ATTRIBUTE_T pEntry;

	//
	// Check minimal buffer size required.
	//
	*pBytesWritten = 0;
	*pBytesNeeded = sizeof(DRV_LOG_ATTR_LIST_T) - sizeof(DRV_LOG_ATTRIBUTE_T);
	if(BufferLength < *pBytesNeeded)
		return FALSE;

	//
	// Enumerate into each entry and fill up if we have enough space.
	//
	bResult = TRUE;
	*pBytesWritten = *pBytesNeeded;
	pBuffer->Count = 0;
	pEntry = pBuffer->LogAttributes;
	for(idx = 0; idx < LID_TOTAL_COUNT; idx++)
	{
		//
		// Figure out string length which is not including EOS.
		//
		DescLen = 0;
		while(g_LogAttributes[idx].Description[DescLen] != 0)
		{
			if(DescLen < (MAX_LOG_DESC_LEN-1))
			{
				DescLen++;
			}
			else
			{ // This is an unexpted condition and might cause buffer overflow.
				RT_ASSERT(FALSE, ("MgntActQuery_DrvLogAttrList(): check g_LogAttributes[%ld].Description!!!\n", idx));

				g_LogAttributes[idx].Description[DescLen] = 0; 
				break;
			}
		}

		//
		// Figure out entry size and add it to *pBytesNeeded.
		//
		EntrySize = sizeof(DRV_LOG_ATTRIBUTE_T) + DescLen;
		*pBytesNeeded += EntrySize;

		//
		// Fill up this entry if there is enough space.
		//
		if(BufferLength >= *pBytesNeeded)
		{
			pEntry->Type = g_LogAttributes[idx].Type;
			pEntry->DescLen = DescLen + 1; // 1 is for EOS.
			PlatformMoveMemory(
				pEntry->Description, 
				g_LogAttributes[idx].Description, 
				pEntry->DescLen);

			*pBytesWritten += EntrySize;
			pEntry = (PDRV_LOG_ATTRIBUTE_T)((pu1Byte)(pEntry->Description) + pEntry->DescLen);
			pBuffer->Count++;
		}
		else
		{
			bResult = FALSE;
		}
	}
	
	return bResult;

#endif
}


//
//	Description:
//		Retrive log data of specified type.
//
BOOLEAN
MgntActQuery_DrvLogDataList(
	IN	PADAPTER				pAdapter,
	IN	u4Byte					eLogType,
	IN	u4Byte					BufferLength,
	OUT	PDRV_LOG_DATA_LIST_T	pBuffer,
	OUT	pu4Byte					pBytesWritten,
	OUT pu4Byte					pBytesNeeded
	)
{
#if !DRV_LOG
	*pBytesWritten = *pBytesNeeded = 0;
	return FALSE;

#else
	u4Byte idx;
	u4Byte DrvLogCnt;
	u4Byte EntrySize;
	PDRV_LOG_DATA_T pEntry;

	//
	// Figure out max entry size.
	//
	// 070305, rcnjko: I use a max estimation instead of 
	// figure out exact size of the each entry to reduce 
	// computation overhead. Besides, I assume UI can cover 
	// this assumption easily.
	//
	static u4Byte MaxEntrySize = sizeof(DRV_LOG_DATA_IMP_T); 

	//
	// Check minimal buffer size required.
	//
	*pBytesWritten = 0;
	*pBytesNeeded = sizeof(DRV_LOG_DATA_LIST_T) - sizeof(DRV_LOG_DATA_T);
	if(BufferLength < *pBytesNeeded)
		return FALSE;

	//
	// Check if specified log type valid.
	//
	if(eLogType >= LTYPE_TOTAL_COUNT)
	{
		return FALSE;
	}	

	//
	// Enumerate into each entry and fill up if we have enough space.
	//
	*pBytesWritten = *pBytesNeeded;
	DrvLogCnt = GetDrvLogCnt(pAdapter, eLogType);
	pBuffer->Count = 0;
	pEntry = pBuffer->LogDatas;
	for(idx = 0; idx < DrvLogCnt; idx++)
	{
		//
		// Fill up this entry if there is enough space.
		//
		if(BufferLength >= (*pBytesNeeded + MaxEntrySize))
		{
			if( RemoveDrvLog(
					pAdapter,
					(DRV_LOG_TYPE_E)eLogType,
					pEntry) )
			{
				//
				// Now, we can get exact entry size and apply it 
				// to fill up *pBytesNeeded and *pBytesWritten.
				//
				EntrySize = sizeof(DRV_LOG_DATA_T) - sizeof(pEntry->Buffer[0]) + pEntry->BufferLenUsed;

				*pBytesNeeded += EntrySize;
				*pBytesWritten += EntrySize;
				pEntry = (PDRV_LOG_DATA_T)((pu1Byte)(pEntry->Buffer) + pEntry->BufferLenUsed);
				pBuffer->Count++;
			}	
			else
			{ // No more log available.
				break;
			}
		}
		else
		{
			*pBytesNeeded += MaxEntrySize;
		}
	}
	
	return TRUE;
#endif
}


u1Byte    
DecorateTxRateBySingalStrength(
	u4Byte	SignalStrength,
	u1Byte	*SS_Rate_Map,
	u1Byte	MapSize
)
{
	u1Byte index = 0;
	
	for(index=0; index<(MapSize*2); index+=2)
	{
		if(SignalStrength > SS_Rate_Map[index])
		{
			return SS_Rate_Map[index+1];
		}
	}

	return MGN_1M;
}

//
//   Description:
//	Return 11N Tx rate to let Normal User.
//	2008-04-14  by Jacken
//
u2Byte    
MgntActQuery_RT_11N_USER_SHOW_RATES(   
	PADAPTER		Adapter , 
	BOOLEAN			TxorRx,          //FALSE:Tx   TRUE:Rx
	BOOLEAN			bLinkStateRx
)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u1Byte		rate = MGN_1M;
	u1Byte 		rftype = RT_GetRFType(Adapter);
	u4Byte		Sgstrength;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	
	// Forced data rate: Tx rate is forced to a constant value.
	if(pMgntInfo->ForcedDataRate != 0)
	{
		return CONVERT_RATE(Adapter, pMgntInfo->ForcedDataRate);
	}

	if(pMgntInfo->bRegLinkSpeedLevel == 40)	//Real Data rate
	{
		return CONVERT_RATE(Adapter,  pHalData->CurrentRARate);
	}
	
	if(!TxorRx) // Tx rate
	{//This value is also refresh in dm_CheckStatistics() every 2 seconds.
		// 1 == TxRateTypeCurrent
		if(pMgntInfo->OSTxRateDisplayType == 1)
		{
			// Report the successful transmit rate culculate from dm_CheckStatistics().
			return 0;
		}
		// 2 == TxRateTypeStartRate
		else if(pMgntInfo->OSTxRateDisplayType == 2)
		{
			// Report initial rate in HW. 
			return CONVERT_RATE(Adapter, Adapter->TxStats.CurrentInitTxRate);
		}
	}

	// Decorate Tx/Rx ate by Signal Strength
	if(pMgntInfo->bForcedShowRateStill == TRUE)
		Sgstrength = 100;
	else if (ACTING_AS_AP(Adapter))
		Sgstrength = 100;	
	else
		Sgstrength = GET_UNDECORATED_AVERAGE_RSSI(Adapter);

	if(IS_WIRELESS_MODE_A(Adapter) || IS_WIRELESS_MODE_G(Adapter))
	{
		rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_G, sizeof(SS_Rate_Map_G)/2);
	}
	else if(IS_WIRELESS_MODE_B(Adapter))
	{
		rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_B, sizeof(SS_Rate_Map_B)/2);
	}
	else if(IS_WIRELESS_MODE_N_24G(Adapter) || IS_WIRELESS_MODE_N_5G(Adapter))
	{
		BOOLEAN		bMaxRateMcs15;
		
		// Determine the max rate for N mode.
		if(	(rftype==RF_1T1R)  ||
			((!TxorRx) && (rftype==RF_1T2R)) ||
			(TxorRx && (rftype==RF_1T2R)) ||
			(pMgntInfo->HTHighestOperaRate <= MGN_MCS7 && 0 != pMgntInfo->HTHighestOperaRate))
			bMaxRateMcs15 = FALSE;
		else
			bMaxRateMcs15 = TRUE;

		if(rftype == RF_3T3R)
		{
			rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS23, sizeof(SS_Rate_Map_N_MCS23)/2); 
		}
		else if(rftype == RF_2T4R || rftype == RF_2T3R)
		{
			rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS23, sizeof(SS_Rate_Map_N_MCS23)/2); 
		}
		else if(bMaxRateMcs15)
		{
			if (pMgntInfo->bRegLinkSpeedLevel == 0)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15, sizeof(SS_Rate_Map_N_MCS15)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 12)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv1, sizeof(SS_Rate_Map_N_MCS15_Lv1)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 14)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv2, sizeof(SS_Rate_Map_N_MCS15_Lv2)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 16)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv3, sizeof(SS_Rate_Map_N_MCS15_Lv3)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 18)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv4, sizeof(SS_Rate_Map_N_MCS15_Lv4)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 20)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv5, sizeof(SS_Rate_Map_N_MCS15_Lv5)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 24)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15_Lv6, sizeof(SS_Rate_Map_N_MCS15_Lv6)/2); 
			else
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS15, sizeof(SS_Rate_Map_N_MCS15)/2); 
		}		
		else
		{
			if (pMgntInfo->bRegLinkSpeedLevel == 0)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7, sizeof(SS_Rate_Map_N_MCS7)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 12)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv1, sizeof(SS_Rate_Map_N_MCS7_Lv1)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 14)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv2, sizeof(SS_Rate_Map_N_MCS7_Lv2)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 16)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv3, sizeof(SS_Rate_Map_N_MCS7_Lv3)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 18)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv4, sizeof(SS_Rate_Map_N_MCS7_Lv4)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 20)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv5, sizeof(SS_Rate_Map_N_MCS7_Lv5)/2); 
			else if (pMgntInfo->bRegLinkSpeedLevel == 24)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7_Lv6, sizeof(SS_Rate_Map_N_MCS7_Lv6)/2); 
			else
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_N_MCS7, sizeof(SS_Rate_Map_N_MCS7)/2); 
		}
	}						
	else if(IS_WIRELESS_MODE_AC(Adapter))
	{
		if(rftype == RF_1T1R)
		{
			if(pMgntInfo->VHTHighestOperaRate < MGN_VHT1SS_MCS9)
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_1SS_MCS7, sizeof(SS_Rate_Map_AC_1SS_MCS7)/2); 
			else
				rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_1SS_MCS9, sizeof(SS_Rate_Map_AC_1SS_MCS9)/2);
		}
		else if(rftype == RF_2T2R || rftype == RF_1T2R)
		{
			if(pMgntInfo->VHTHighestOperaRate < MGN_VHT2SS_MCS9)
			{
				if (pMgntInfo->bRegLinkSpeedLevel == 0)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7, sizeof(SS_Rate_Map_AC_2SS_MCS7)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 12)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv1, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv1)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 14)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv2, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv2)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 16)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv3, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv3)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 18)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv4, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv4)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 20)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv5, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv5)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 24)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7_Lv6, sizeof(SS_Rate_Map_AC_2SS_MCS7_Lv6)/2); 
				else
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS7, sizeof(SS_Rate_Map_AC_2SS_MCS7)/2); 
			}
			else
			{
				if (pMgntInfo->bRegLinkSpeedLevel == 0)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9, sizeof(SS_Rate_Map_AC_2SS_MCS9)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 12)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv1, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv1)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 14)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv2, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv2)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 16)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv3, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv3)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 18)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv4, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv4)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 20)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv5, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv5)/2); 
				else if (pMgntInfo->bRegLinkSpeedLevel == 24)
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9_Lv6, sizeof(SS_Rate_Map_AC_2SS_MCS9_Lv6)/2); 
				else
					rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_2SS_MCS9, sizeof(SS_Rate_Map_AC_2SS_MCS9)/2); 
			}
		}	
		else if(rftype == RF_3T3R)
		{
			rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_3SS_MCS9, sizeof(SS_Rate_Map_AC_3SS_MCS9)/2); 
		}
		else if(rftype == RF_2T4R || rftype == RF_2T3R)
		{
			rate = DecorateTxRateBySingalStrength(Sgstrength, (pu1Byte)SS_Rate_Map_AC_3SS_MCS9, sizeof(SS_Rate_Map_AC_3SS_MCS9)/2); 
		}
	}
	else
		return MGN_1M;


	if(IS_WIRELESS_MODE_N_24G(Adapter) || IS_WIRELESS_MODE_N_5G(Adapter))
	{
		if(rate > pMgntInfo->HTHighestOperaRate && pMgntInfo->HTHighestOperaRate != 0)
		{			
			rate = pMgntInfo->HTHighestOperaRate;	
		}
	}
	else if(IS_WIRELESS_MODE_AC(Adapter))
	{
		if(rate > pMgntInfo->VHTHighestOperaRate && pMgntInfo->VHTHighestOperaRate != 0)
			rate = pMgntInfo->VHTHighestOperaRate;	
	}
	else
	{
		if(rate > pMgntInfo->HighestOperaRate && pMgntInfo->HighestOperaRate != 0)
			rate = pMgntInfo->HighestOperaRate;
	}

	if(IS_HARDWARE_TYPE_8821U(Adapter) )
	{
		if(rate < pHalData->CurrentRARate)
			rate = pHalData->CurrentRARate;
	}
	
	return CONVERT_RATE(Adapter, rate);
}

//
//	Description:
//		Return the Additional beacon IE field of MgntInfo.
//
BOOLEAN
MgntActQuery_AdditionalBeaconIE(
	IN 		PADAPTER	Adapter,
	OUT 	pu1Byte 		pAdditionalIEBuf,
	IN OUT 	pu4Byte		pAdditionalIEBufLen
	)
{
	PMGNT_INFO 	pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN 	bResult = FALSE;

	do
	{
		if (*pAdditionalIEBufLen < pMgntInfo->AdditionalBeaconIESize)
		{
			*pAdditionalIEBufLen = pMgntInfo->AdditionalBeaconIESize;
			bResult = FALSE;			
			break;
		}

		PlatformMoveMemory(pAdditionalIEBuf, 
			pMgntInfo->AdditionalBeaconIEData, 
			pMgntInfo->AdditionalBeaconIESize);

		bResult = TRUE;
				
	} while(FALSE);

	return bResult;
	
}

//
//	Description:
//		Return the Additional probe rsp IE field of MgntInfo.
//
BOOLEAN
MgntActQuery_AdditionalProbeRspIE(
	IN 		PADAPTER	Adapter,
	OUT 	pu1Byte 		pAdditionalIEBuf,
	IN OUT 	pu4Byte		pAdditionalIEBufLen
	)
{
	PMGNT_INFO 	pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN 	bResult = FALSE;
	
	do
	{
		if (*pAdditionalIEBufLen < pMgntInfo->AdditionalResponseIESize)
		{
			*pAdditionalIEBufLen = pMgntInfo->AdditionalResponseIESize;
			bResult = FALSE;
			break;
		}

		PlatformMoveMemory(pAdditionalIEBuf, 
			pMgntInfo->AdditionalResponseIEData, 
			pMgntInfo->AdditionalResponseIESize);

		bResult = TRUE;
		
	} while(FALSE);

	return bResult;
	
}

RT_AP_TYPE
MgntActQuery_ApType(
	IN PADAPTER pAdapter
	)
{
	if(pAdapter == NULL)
		return RT_AP_TYPE_NONE;
	else 
		return pAdapter->MgntInfo.ApType;
}


#if (P2P_SUPPORT == 1)	
P2P_ROLE
MgntActQuery_P2PMode(
	IN	PADAPTER		Adapter
	)
{
	return GET_P2P_INFO(Adapter)->Role;
}

BOOLEAN
MgntActQuery_P2PScanList(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte 	pBuf,
	IN OUT 	pu4Byte		pBufLen
	)
{
	PP2P_INFO	pP2PInfo = GET_P2P_INFO(Adapter);
	u4Byte		BytesNeeded;

	//RT_TRACE(COMP_P2P, DBG_LOUD, ("MgntActQuery_P2PScanList()\n"));

	if(!P2P_ENABLED(pP2PInfo))
	{
		*pBufLen = 0;
		return TRUE;
	}

	BytesNeeded = pP2PInfo->ScanList4QuerySize* sizeof(P2P_DEVICE_DISCRIPTOR) + sizeof(u4Byte);

	if(*pBufLen < BytesNeeded) 
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("MgntActQuery_P2PScanList(): bytes needed: %u, given: %u\n", BytesNeeded, *pBufLen));
		// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("MgntActQuery_P2PScanList(): sizeof ClientInfoDesc: %u, ScanListSize: %u\n", 
			sizeof(P2P_CLIENT_INFO_DISCRIPTOR), pP2PInfo->ScanList4QuerySize));
		*pBufLen = BytesNeeded;
		return FALSE;
	}

	*((pu4Byte)pBuf) = pP2PInfo->ScanList4QuerySize;
	PlatformMoveMemory(pBuf + sizeof(u4Byte), 
		pP2PInfo->ScanList4Query, 
		pP2PInfo->ScanList4QuerySize * sizeof(P2P_DEVICE_DISCRIPTOR));
	*pBufLen = BytesNeeded;

	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("MgntActQuery_P2PScanList(): %u items indicated\n", 
		pP2PInfo->ScanList4QuerySize));
	
	return TRUE;
}

u4Byte
MgntActQuery_P2PSelfDeviceDescriptor(
	IN PADAPTER Adapter,
	OUT PVOID pDevDesc
	)
{
	return P2PTranslateP2PInfoToDevDesc((GET_P2P_INFO(Adapter)), pDevDesc);
}

//
// Description:
//	Query P2P Channel List
// Arguments:
//	[IN] Adapter -
//		NIC adapter context pointer.
//	[IN] ChannelListBufLen -
//		size in bytes of the buffer pointed by the pChannelList parameter
//	[OUT] pChennelListLen -
//		a pointer to the buffer for returning the number of channels written
//	[OUT] pChannelList -
//		a pointer to the buffer for returning the channel list
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	Enumerates each channel in channel entery list in P2PInfo of the adapter. Duplicated channels 
//	are filtered out. 
//
RT_STATUS
MgntActQuery_P2PChannelList(
	IN PADAPTER Adapter,
	IN u4Byte ChannelListBufLen,
	OUT pu1Byte pChennelListLen,
	OUT pu1Byte pChannelList
	)
{
	u1Byte i = 0, j = 0, k = 0;
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);

	if(ChannelListBufLen < 1) return RT_STATUS_RESOURCE;

	*pChennelListLen = 0;
	
	for(i = 0; i < pP2PInfo->ChannelEntryList.regClasses; i++) // foreach regulatory class
	{
		for(j = 0; j < pP2PInfo->ChannelEntryList.regClass[i].channels; j++) // foreach channels we support in the regulatory class
		{
			u1Byte curChannel = pP2PInfo->ChannelEntryList.regClass[i].channel[j];
			
			if(*pChennelListLen > ChannelListBufLen)
			{
				return RT_STATUS_RESOURCE;
			}

			// Filter duplicated channels from different reg class
			for(k = 0; k < (*pChennelListLen) && pChannelList[k] != curChannel; k++){}
			if(k < (*pChennelListLen)) continue;

			// Add the channel
			pChannelList[(*pChennelListLen)++] = curChannel;

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("MgntActQuery_P2PChannelList: add channel: %u, %4s: %4u, %4s: %4u\n", k,
				"reg", pP2PInfo->ChannelEntryList.regClass[i].regClass, "ch", curChannel));
		} 
	}
	
	return RT_STATUS_SUCCESS;
}

VOID
MgntActQuery_P2PListenChannel(
	IN PADAPTER Adapter,
	OUT pu1Byte pListenChannel
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
	*pListenChannel = pP2PInfo->ListenChannel;
	return;
}
#endif	// #if (P2P_SUPPORT == 1)	

#if (WPS_SUPPORT == 1)
u4Byte
MgntActQuery_WPS_Information(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte		InformationBuffer,
	IN OUT 	u4Byte		InformationBufferLength
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
	u1Byte				BytesNeeded = 3;
	u1Byte				queryInfo = pSimpleConfig->InfoCtrl;
	u4Byte				BytesCopied = 0;
	
	if(pSimpleConfig->WpsIeVersion < SUPPORT_WPS_INFO_VERSION)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActQuery_WPS_Information(): Not support this version!\n"));
		goto End;
	}

	if(InformationBufferLength<BytesNeeded)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActQuery_WPS_Information(): invalid buffer length\n"));
		goto End;
	}

	switch(queryInfo)
	{
		case WPS_INFO_SUPPORT_VERSION:
		{
			*((pu2Byte)InformationBuffer) = 1;
			*(InformationBuffer+2) = pSimpleConfig->WpsIeVersion;
			BytesCopied = BytesNeeded;
		}
		break;

		default:
			RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActQuery_WPS_Information(): unspecified query information!\n"));
			break;
	}
	
	End:
		return BytesCopied;
}
#endif


