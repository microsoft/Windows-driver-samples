////////////////////////////////////////////////////////////////////////////////
//
//	File name:		N6C_Init.c
//	Description:	NDIS6.x common function for initialization.
//
//	Author:			rcnjko
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"
#include "802_11_OID.h"
#include "RegCommon.h"

#if WPP_SOFTWARE_TRACE
#include "N6C_Init.tmh"
#endif

NDIS_STATUS
CopyFromUnicodeToOS( 
	UCHAR			*pointer,
	UNICODE_STRING	*uniStr,
	USHORT			copyLen)
{
	int		s;
	OCTET_STRING	*os = (OCTET_STRING *)pointer;
	
	for(s=0; s<copyLen; s++)
	{
		os->Octet[s] = (UCHAR)uniStr->Buffer[s];
	}
	
	
	os->Length = copyLen;
	
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CopyFromUnicodeToString( 
	UCHAR			*pointer,
	UNICODE_STRING	*uniStr,
	USHORT			copyLen)
{
	int		s;
	for(s=0; s<copyLen; s++)
	{
		pointer[s] = (UCHAR)uniStr->Buffer[s];
	}
	return NDIS_STATUS_SUCCESS;
}

// for Decoding WEP key
VOID 
RegistryDecode(
	unsigned char *in, 
	unsigned char *out,
	int maxdecode)	
{
	int len;
	int i,j=2;
	unsigned long key;
	unsigned char codechar;

	key=((unsigned long)(in[0]>'9'?in[0]-'A'+10:in[0]-'0'))*16+(in[1]>'9'?in[1]-'A'+10:in[1]-'0');

	codechar=(unsigned char)(((unsigned long)(in[j]>'9'?in[j]-'A'+10:in[j]-'0'))*16+(in[j+1]>'9'?in[j+1]-'A'+10:in[j+1]-'0'));
	len=(unsigned char)(((unsigned long)codechar)^((key*789)&0xff));
	j+=2;

	for(i=0;i<len && i<maxdecode;i++)
	{
		key++;
		if(key>256)
			key=0;
		codechar=(unsigned char)(((unsigned long)(in[j]>'9'?in[j]-'A'+10:in[j]-'0'))*16+(in[j+1]>'9'?in[j+1]-'A'+10:in[j+1]-'0'));
		out[i]=(unsigned char)(((unsigned long)codechar)^((key*789)&0xff));
		j+=2;
	}
	for(j=0;j<(i>>1);j++)
		out[j]=(unsigned char)(((unsigned long)(out[j*2]>'9'?out[j*2]-'A'+10:out[j*2]-'0'))*16+(out[j*2+1]>'9'?out[j*2+1]-'A'+10:out[j*2+1]-'0'));
	//out[i++]=0;
}

UCHAR *
StringToHex(
	char 	*in,
	int 	inlen,
	UCHAR	*out,
	int 	outlen,
	BOOLEAN IsNum)
{
	#define Hex2Num(n)	(   ((n)>='a'&&(n)<='z')   ?   ( (n)-'a'+10 ):( (n)>='A'? (n)-'A'+10:((n>'0')?(n)-'0':0) )   )
	int i,j;
	memset(out,0,outlen);

/*	if(in[0] == 0)
	{
		return out;
	}
*/	
	if(IsNum)
	{
		for(i=inlen-1,j=0;i>=0 && j<outlen;i-=2)
		{
			if(i>0)
			{
				out[j]=Hex2Num(in[i-1]);
				out[j]<<=4;
			}
			out[j++]+=Hex2Num(in[i]);
		}
	}
	else
	{
		for(i=inlen-1,j=outlen-1;i>=0 && j>=0;i-=2)
		{
			if(i>0)
			{
				out[j]=Hex2Num(in[i-1]);
				out[j]<<=4;
			}
			out[j--]+=Hex2Num(in[i]);
		}
	}

	return out;
}

// 
// Translate from NDIS_802_11_POWER_MODE to RT_PS_MODE. 
// 2005.02.15, by rcnjko.
//
int 
TranslateNdisPsToRtPs(
	IN	NDIS_802_11_POWER_MODE	ndisPsMode
	)
{
	RT_PS_MODE rtPsMode;

	switch(ndisPsMode)
	{
	case Ndis802_11PowerModeCAM:
		rtPsMode = eActive; 
		break;	
	case Ndis802_11PowerModeMAX_PSP:
		rtPsMode = eMaxPs; 
		break;	
	case Ndis802_11PowerModeFast_PSP:
		rtPsMode = eFastPs; 
		break;	
	default:
		RT_TRACE(COMP_DBG, DBG_SERIOUS, ("TranslateNdisPsToRtPs(): Unknown ndisPsMode: 0x%X !!!\n", ndisPsMode));
		rtPsMode = eActive; 
		break;
	}

	return rtPsMode;
}

NDIS_STATUS
N6OpenConfigurationHandle(
	IN	PADAPTER		pAdapter,
	IN	PNDIS_HANDLE	pConfigurationHandle
	)
{
	NDIS_HANDLE						ConfigurationHandle;
	NDIS_CONFIGURATION_OBJECT		ConfigObject;
	NDIS_STATUS						Status = NDIS_STATUS_SUCCESS;

	PlatformZeroMemory(&ConfigObject, sizeof(NDIS_CONFIGURATION_OBJECT));
	
	N6_ASSIGN_OBJECT_HEADER(
		ConfigObject.Header,
		NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT,
		NDIS_CONFIGURATION_OBJECT_REVISION_1,
		sizeof(NDIS_CONFIGURATION_OBJECT));

	ConfigObject.NdisHandle = pAdapter->pNdisCommon->hNdisAdapter;
	ConfigObject.Flags = 0;

	Status = NdisOpenConfigurationEx(&ConfigObject, &ConfigurationHandle);
	if (Status != NDIS_STATUS_SUCCESS)
	{
		return Status;
	}

	*pConfigurationHandle = ConfigurationHandle;

	return Status;
}

VOID
N6CloseConfigurationHandle(
	IN	NDIS_HANDLE		ConfigurationHandle
	)
{
	NdisCloseConfiguration(ConfigurationHandle);
}

NDIS_STATUS
N6ReadCommonRegistry(
	IN	PADAPTER		pAdapter
	)
/*++
Routine Description:

	Read the registry in CommonRegTable[].

Arguments:

    pAdapter						Pointer to ADAPTER. 
    WrapperConfigurationContext		For use by NdisOpenConfiguration

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_RESOURCES                                       

--*/
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	NDIS_HANDLE			ConfigurationHandle;
	PMP_REG_ENTRY		pRegEntry;
	PUCHAR				pointer;
	PUCHAR			NetworkAddress;
	UINT				i;
	UINT				Length;
	PNDIS_CONFIGURATION_PARAMETER	ReturnedValue;
	ULONG_PTR			value;
#if READ_BT_REGISTRY	
	u1Byte					result=0;
	PWCHAR					registryName;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
#endif

	RT_TRACE(COMP_INIT, DBG_LOUD, (" ==> N6ReadCommonRegistry()\n"));

	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(pAdapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT,DBG_TRACE, ("<== N6ReadCommonRegistry, Status=%x\n", Status));
		return Status;
	}
	
#if READ_BT_REGISTRY
	registryName = L"BTLoadOption";
	PlatformReadBTFWLoaderDwordRegistry(registryName,&result);
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[BT COEX] BT fwloader BTLoadOption = %d\n", result));
	if(result == 1)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[BT COEX] BT fwloader inform wlan config wifi only\n"));
		pHalData->bt_coexist.bBtExist = 0;
	}
#endif

	//----------------------------------------------------------------------------
	// Initialize related variables if necessary.
	pNdisCommon->RegSSID.Octet = &(pNdisCommon->RegSSIDBuf[0]);
	pNdisCommon->RegLocale.Octet = &(pNdisCommon->RegLocaleBuf[0]);
	pNdisCommon->RegDriverDesc.Octet = &(pNdisCommon->RegDriverDescBuf[0]);

	// For 818x UI and WPA Verify. 2004.11.30, by rcnjko. 
	for(i = 0; i < 4; i++)
	{
		pNdisCommon->RegDefaultKey[i].Octet = pNdisCommon->RegDefaultKeyBuf[i];
		pNdisCommon->RegDefaultKey[i].Length = 0;
		pNdisCommon->RegDefaultKeyW[i].Octet = pNdisCommon->RegDefaultKeyWBuf[i];
		pNdisCommon->RegDefaultKeyW[i].Length = 0;
	}

	pNdisCommon->RegPwrByRateFile.Octet = &(pNdisCommon->RegPwrByRateFileBuf[0]);
	pNdisCommon->RegPwrLimitFile.Octet = &(pNdisCommon->RegPwrLimitFileBuf[0]);
	pNdisCommon->RegSecondaryPwrLimitFile.Octet = &(pNdisCommon->RegSecondaryPwrLimitFileBuf[0]);
	pNdisCommon->RegChannelPlan2G.Octet = pNdisCommon->RegChannelPlan2GBuf;
	pNdisCommon->RegChannelPlan5G.Octet = pNdisCommon->RegChannelPlan5GBuf;	

	//----------------------------------------------------------------------------

	// Read the GUID of the current Adapter from the registry "NetCfgInstanceId".
	{
		NDIS_STRING		RegNetCfginstanceID;

		NdisInitializeString(&RegNetCfginstanceID, NDIS_REG_NET_CFG_INSTANCE_ID_NAME);

		NdisReadConfiguration(
				&Status,
				&ReturnedValue,
				ConfigurationHandle,
				&RegNetCfginstanceID,
				NdisParameterString);

		if (RegNetCfginstanceID.Buffer != NULL)
		{
			NdisFreeString(RegNetCfginstanceID);
		}
		if(NDIS_STATUS_SUCCESS == Status)
		{
			if(NT_SUCCESS(RtlGUIDFromString(&(ReturnedValue->ParameterData.StringData), &pNdisCommon->NetCfgInstanceId)))
			{
				RT_PRINT_UUID(COMP_INIT, DBG_LOUD, "The current adapter GUID is:", pNdisCommon->NetCfgInstanceId);
			}
		}		
	}
	//----------------------------------------------------------------------------
	// Read all the registry values.
	for(i = 0, pRegEntry = CommonRegTable; i < COM_NUM_REG_PARAMS; i++, pRegEntry++)	
	{
		RT_TRACE(COMP_INIT, DBG_TRACE, ("RegName: [%ws]\n", (pRegEntry->RegName).Buffer));
		
		pointer = (PUCHAR) pNdisCommon + pRegEntry->FieldOffset;

		// Get the configuration value for a specific parameter.  Under NT the
		// parameters are all read in as DWORDs.
		NdisReadConfiguration(
			&Status,
			&ReturnedValue,
			ConfigurationHandle,
			&pRegEntry->RegName,
			(enum _NDIS_PARAMETER_TYPE)pRegEntry->Type);

		// If the parameter was present, then check its value for validity.
		if (Status == NDIS_STATUS_SUCCESS)
		{
			if( (pRegEntry->Type == NdisParameterInteger)|| (pRegEntry->Type == NdisParameterHexInteger) 	)
			{
				// Check that param value is not too small or too large
				if(	ReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
					ReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
				{
					value = pRegEntry->Default;
				}
				else
				{
					value = ReturnedValue->ParameterData.IntegerData;
				}
			}
			else if( pRegEntry->Type == NdisParameterString )
			{
				USHORT	copyLen;
				copyLen = ( (ReturnedValue->ParameterData.StringData.Length/2) > (USHORT)pRegEntry->Max) ? pRegEntry->Max : ReturnedValue->ParameterData.StringData.Length/2;

				if( (pRegEntry->FieldOffset == REGISTRY_OFFSET(RegBssidBuf)) )
				{
					CopyFromUnicodeToString( pointer, &ReturnedValue->ParameterData.StringData, 12);
				}
				else if( pRegEntry->FieldOffset == REGISTRY_OFFSET(RegDbgZone))
				{
					int 		i = 0;
					u8Byte	value_u8 = 0;
					for(i = 0;i < copyLen; i++)
						value_u8 |= (((UCHAR)ReturnedValue->ParameterData.StringData.Buffer[i])-48)*(1<<i);

					*((pu8Byte) pointer) = value_u8;
				}
				else
				{
					CopyFromUnicodeToOS( pointer, &ReturnedValue->ParameterData.StringData, copyLen);
					RT_TRACE(COMP_INIT, DBG_TRACE, ("Copied: [%ws]\n", (ReturnedValue->ParameterData.StringData).Buffer));
				}
			}
			else
			{
				value = pRegEntry->Default;	// Prefast warn 6001
			}
		}
		else if(pRegEntry->bRequired)
		{
			RT_TRACE(COMP_DBG, DBG_SERIOUS,(" -- failed\n"));

			RT_ASSERT(FALSE,("Read parameter FAIL!\n"));

			Status = NDIS_STATUS_FAILURE;
			break;
		}
		else
		{
			if( (pRegEntry->Type == NdisParameterInteger) || (pRegEntry->Type == NdisParameterHexInteger) )
			{
				value = pRegEntry->Default;
			}
			else
			{
				if( (pRegEntry->FieldOffset == REGISTRY_OFFSET(RegBssidBuf)))
					;
				else if( pRegEntry->FieldOffset == REGISTRY_OFFSET(RegDbgZone))
					value = 0;
				else
				{
					CopyFromUnicodeToOS( pointer,
								(NDIS_STRING *)(UINT_PTR)pRegEntry->Default,
								((NDIS_STRING *)(UINT_PTR)pRegEntry->Default)->Length/2 );
					((OCTET_STRING*)pointer)->bDefaultStr = TRUE;
				}
				value = pRegEntry->Default;	// Prefast warn 6001
			}
			Status = NDIS_STATUS_SUCCESS;
		}

		// Store the value in the adapter structure.
		switch(pRegEntry->FieldSize)
		{
			case 1:
				*((PUCHAR) pointer) = (UCHAR) value;
				break;
			case 2:
				*((PUSHORT) pointer) = (USHORT) value;
				break;
			case 4:
				*((PULONG) pointer) = (ULONG) value;
				break;
			default:
				RT_TRACE(COMP_DBG, DBG_SERIOUS,("Bogus field size %d\n", pRegEntry->FieldSize));
				break;
		}
	}

	// For 818x UI, 2004.11.30, by rcnjko.
	for(i = 0; i < 4; i++)	// Decode WEP key
	{
		unsigned char temp[30]; 
		
		if(pNdisCommon->RegDefaultKey[i].Length > 0)
		{
			pNdisCommon->RegDefaultKey[i].Octet[pNdisCommon->RegDefaultKey[i].Length] = 0;
			RegistryDecode(pNdisCommon->RegDefaultKey[i].Octet, temp, 10);
			NdisMoveMemory(pNdisCommon->RegDefaultKey[i].Octet, temp, 5);
			pNdisCommon->RegDefaultKey[i].Length = 5;
		}
		if(pNdisCommon->RegDefaultKeyW[i].Length > 0)
		{
			pNdisCommon->RegDefaultKeyW[i].Octet[pNdisCommon->RegDefaultKeyW[i].Length] = 0;
			RegistryDecode(pNdisCommon->RegDefaultKeyW[i].Octet, temp, 26);
			NdisMoveMemory(pNdisCommon->RegDefaultKeyW[i].Octet, temp, 13);
			pNdisCommon->RegDefaultKeyW[i].Length = 13;
		}
	}

	// Convert bssid string to hex.
	StringToHex((char *)pNdisCommon->RegBssidBuf, 12, pNdisCommon->RegBssid, 6,0);

	// Read NetworkAddress registry value 
	// Use it as the current address if any
	if(Status == NDIS_STATUS_SUCCESS)
	{
		NdisReadNetworkAddress(
			&Status,
			(void **)&NetworkAddress,
			&Length,
			ConfigurationHandle);

		RT_TRACE(COMP_INIT, DBG_LOUD, ("NdisReadNetworkAddress, Status=%x\n", Status));
	
		// If there is a NetworkAddress override in registry, use it 
		if ((Status == NDIS_STATUS_SUCCESS) && (Length == ETH_LENGTH_OF_ADDRESS))
		{
			if(ETH_IS_MULTICAST(NetworkAddress) || ETH_IS_BROADCAST(NetworkAddress) || ((NetworkAddress[0]&0x02)==0))
			{
				RT_TRACE(COMP_INIT,DBG_LOUD, 
				("Overriding NetworkAddress is invalid - %02x-%02x-%02x-%02x-%02x-%02x\n", 
				NetworkAddress[0], NetworkAddress[1], NetworkAddress[2],
				NetworkAddress[3], NetworkAddress[4], NetworkAddress[5]));
			}
			else
			{
				if ( (NetworkAddress[0]==0x00) &&
					 (NetworkAddress[1]==0x00) &&
					 (NetworkAddress[2]==0x00) &&
					 (NetworkAddress[3]==0x00) &&
					 (NetworkAddress[4]==0x00) &&
					 (NetworkAddress[5]==0x00) ) 
				{ 
					// Network addr = 00 00 00 00 00 00
					pNdisCommon->bOverrideAddress = FALSE;
				}
				else 
				{
					RT_TRACE(COMP_INIT,DBG_LOUD, 
					("Overriding NetworkAddress - %02x-%02x-%02x-%02x-%02x-%02x\n", 
					NetworkAddress[0], NetworkAddress[1], NetworkAddress[2],
					NetworkAddress[3], NetworkAddress[4], NetworkAddress[5]));
				
					ETH_COPY_NETWORK_ADDRESS(pNdisCommon->CurrentAddress, NetworkAddress);
					pNdisCommon->bOverrideAddress = TRUE;
				}
			}
		}

		Status = NDIS_STATUS_SUCCESS;
	}

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<-- N6ReadCommonRegistry, Status=%x\n", Status));
	return Status;
}


VOID
N6UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
	)
/*++
Routine Description:

	Update the parameter read from registery to coresponding ones in MGNT_INFO 
	or ADAPTER.

Arguments:

    pAdapter						Pointer to ADAPTER. 

Return Value:

	TRUE if succeeded, FALSE otherwise.

Assumption: 
	These modification should be after InitializeMgntVariables() which is called by 
	NicIFAssociateNIC().

--*/
{
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);	
	PRT_DOT11D_INFO			pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO				pDefaultMgntInfo = &(pDefaultAdapter->MgntInfo);
	PRT_FIRMWARE			pFirmware = GET_FIRMWARE(pAdapter);

	HAL_DATA_TYPE			*pHalData	= GET_HAL_DATA(pAdapter);	
	s1Byte 					i = 0, diff = 0;
	u1Byte 					IndicationNumber, tempNumber = 0;
	BOOLEAN					bFcsCapable = FALSE;
	u4Byte					result=0;
	PWCHAR					registryName;
	   
	// Workaround for HCT.
	pAdapter->bInHctTest = pNdisCommon->bRegHctTest ? 1: (pDefaultAdapter->bInHctTest);  //YJ,mod,120626
	pAdapter->bInWFDTest = pNdisCommon->bRegWFDTest ? 1: (pDefaultAdapter->bInWFDTest);  //YJ,mod,120626
	pAdapter->bScanTimeCheck = pNdisCommon->bRegScanTimeCheck ? 1: (pDefaultAdapter->bScanTimeCheck);  //YJ,mod,120626

	// For EQC require to fix the Mac Address.
	pAdapter->bFixedMacAddr = pNdisCommon->bRegFixedMacAddr;

	pAdapter->bDPCISRTest = pNdisCommon->bRegDPCISRTest;		
	pAdapter->bUseThreadHandleInterrupt = pNdisCommon->bRegUseThreadHandleInterrupt;	
	pAdapter->bInChaosTest = pNdisCommon->bRegChaos;
	pAdapter->interfaceIndex = pDefaultAdapter->interfaceIndex;
	pMgntInfo->ChannelPlan = pDefaultMgntInfo->ChannelPlan;

	// regpnpcapabilities set to "FALSE" unload driver when s3s4
	// set to  "TRUE" if support PNP capabilities OID
	if(!pAdapter->bInHctTest)
		pAdapter->bUnloadDriverwhenS3S4 = ((pNdisCommon->bRegPnpCapabilities)? FALSE : TRUE);
	else
		pAdapter->bUnloadDriverwhenS3S4 = FALSE;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("N6UpdateDefaultSetting(): bUnloadDriverwhenS3S4 = %d HctTest %d\n", pAdapter->bUnloadDriverwhenS3S4, pAdapter->bInHctTest));

	MgntActSet_ApType(pAdapter, (BOOLEAN)(pNdisCommon->RegActingAsAp));

	pMgntInfo->ForcedBstDataRate = pNdisCommon->RegForcedBstDataRate;

	// For power save mode. 2005.02.15, by rcnjko. 
	pMgntInfo->dot11PowerSaveMode = (RT_PS_MODE)TranslateNdisPsToRtPs((NDIS_802_11_POWER_MODE)pNdisCommon->RegPowerSaveMode);


	// For RF power state.
	pMgntInfo->RegRfOff = (pNdisCommon->RegRfOff == 1) ? TRUE : FALSE;

	// Security related default setting.
	pMgntInfo->SecurityInfo.RegEnablePreAuth = pNdisCommon->RegEnablePreAuthentication;

	// Update PreAuthentication by Bruce, 2007-11-06.
	pMgntInfo->SecurityInfo.EnablePreAuthentication = pMgntInfo->SecurityInfo.RegEnablePreAuth;

	// HW/SW encryption/decryption. 2006.09.29, by shien chang.
	pMgntInfo->SecurityInfo.RegSWTxEncryptFlag = pNdisCommon->RegSWTxEncryptFlag;
	pMgntInfo->SecurityInfo.RegSWRxDecryptFlag = pNdisCommon->RegSWRxDecryptFlag;

	//
	// WiFi Config, by Bruce, 2007-12-07.
	// Note:
	//	This flag can be set to config special configuration for WiFi, such as EDCA and Wireless Mode.
	// 	It should be just "temporal solution" to pass WiFi.
	//
	pMgntInfo->bWiFiConfg = pNdisCommon->bRegWiFi;

	// 2010/05/18 MH For GPIO detect timer delay setting.	
	pMgntInfo->bRegTimerGPIO=pNdisCommon->bRegTimerGPIO;
	pMgntInfo->bRegGPIODelay=pNdisCommon->bRegGPIODelay;
	pMgntInfo->bRegGPIOBack=pNdisCommon->bRegGPIOBack;

	// 2010/08/25 MH support power down mode switch 
	pMgntInfo->bRegPDNMode = pNdisCommon->bRegPDNMode;

	// 2010/09/01 MH According to PM's request, we support dongle selective suspend mode switch.	
	pMgntInfo->bRegDongleSS = pNdisCommon->bRegDongleSS;

	// 2010/09/13 MH According to PM's request, we support different SS power seave level.	
	pMgntInfo->bRegSSPwrLvl = pNdisCommon->bRegSSPwrLvl;

	// 2011/02/16 MH Add for SS HW radio detect workaround temporarily.
	pMgntInfo->bRegSSWakeCnt = pNdisCommon->bRegSSWakeCnt;
		
	// 2010/12/17 MH Add for RX aggregation mode switch according to TX/RX traffic.	
	pMgntInfo->bRegAggDMEnable = pNdisCommon->bRegAggDMEnable;

	// 2010/12/31 MH Add for UPHY dynamic chaneg.	
	pMgntInfo->bRegUPDMEnable = pNdisCommon->bRegUPDMEnable;

	// 2011/07/08 MH Add for different link speed display.
	pMgntInfo->bRegLinkSpeedLevel = pNdisCommon->bRegLinkSpeedLevel;

	// Update Signal Bar settings
	pMgntInfo->RSSI2GridMode = pNdisCommon->RegRSSI2GridMode;

	// 2011/07/14 MH Add for rx short cut.	
	pMgntInfo->bRegRxSC = pNdisCommon->bRegRxSC;

	// 2011/07/15 Sinda Add for tx short cut.	
	pAdapter->TXSCSupport = (pNdisCommon->bRegTxSC) ? TRUE : FALSE;

	// 2011/12/08 hpfan Add for Tcp Reorder
	pMgntInfo->bTcpReorderEnable = pNdisCommon->bTcpReorderEnable;


	// 2011/09/15 MH Add registry for switching packet compete method.
	pMgntInfo->RegTxMode = pNdisCommon->RegTxMode;

	// 2012/09/14 MH Add for EDCA turbo mode switch threshold.
	// 2012/09/14 MH Add for 88E rate adaptive mode siwtch.
	pMgntInfo->RegEdcaThresh = pNdisCommon->RegEdcaThresh;
	pMgntInfo->RegRALvl = pNdisCommon->RegRALvl;

	// 2012/10/26 MH Add for 8812/8821 AC series dynamic switch.
	pMgntInfo->RegAcUsbDmaTime = pNdisCommon->RegAcUsbDmaTime;
	pMgntInfo->RegAcUsbDmaSize = pNdisCommon->RegAcUsbDmaSize;
	pMgntInfo->RegAcUsbDmaTime2 = pNdisCommon->RegAcUsbDmaTime2;
	pMgntInfo->RegAcUsbDmaSize2 = pNdisCommon->RegAcUsbDmaSize2;

	// 2012/10/31 MH Add for power limit table constraint.
	pMgntInfo->RegTxPwrLimit = pNdisCommon->RegTxPwrLimit;

	// 2012/11/06 Page Add for NByte Access BB reg switch
	pMgntInfo->RegNByteAccess = pNdisCommon->RegNByteAccess;

	pMgntInfo->RegFWOffload = pNdisCommon->RegFWOffload;

	if(pNdisCommon->RegDownloadFW)
		pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
	else
		pFirmware->eFWSource = FW_SOURCE_IMG_FILE;

	pMgntInfo->CustomerID = pNdisCommon->RegCustomerID;

	pMgntInfo->bWaitBeforeGoSkipScan = pNdisCommon->RegWaitBeforeGoSkipScan;

	pMgntInfo->bGoSkipScanForWFD = pNdisCommon->RegGoSkipScanForWFDTest;
	pMgntInfo->bClientSkipScanForWFD = pNdisCommon->RegClientSkipScanForWFDTest;
	pMgntInfo->bForceGoTxData20MBw = pNdisCommon->RegForceGoTxData20MBw;

	if(IS_HARDWARE_TYPE_8821U(pAdapter))	
	{
		pMgntInfo->RegFWOffload = FALSE;
		pNdisCommon->RegFWOffload =FALSE;
	}

	// 2012/11/07 Awk add PowerBase for customers to define their power base.
	pMgntInfo->RegPowerBase = pNdisCommon->RegPowerBase;

	pMgntInfo->RegEnableTxPowerLimit = pNdisCommon->RegEnableTxPowerLimit;
	pMgntInfo->RegEnableTxPowerByRate = pNdisCommon->RegEnableTxPowerByRate;
	pMgntInfo->RegTxPowerLimitTableSel = pNdisCommon->RegTxPowerLimitTableSel;
	pMgntInfo->RegTxPwrLmtDynamicLoading = pNdisCommon->RegTxPwrLmtDynamicLoading;

	pMgntInfo->RegSupportTxPwrTableDump = pNdisCommon->RegSupportTxPwrTableDump;
	
	pMgntInfo->bDisableTXPowerTraining = !pNdisCommon->RegTxPowerTraining;

	pMgntInfo->RegLoadSystemSKUfromUEFI = pNdisCommon->RegLoadSystemSKUfromUEFI;
	pMgntInfo->RegUEFIProfile = pNdisCommon->RegUEFIProfile;

	pMgntInfo->RegLoadSystemSKUfromSMBIOS = pNdisCommon->RegLoadSystemSKUfromSMBIOS;
	pMgntInfo->RegLoadProcessorIDfromSMBIOS= pNdisCommon->RegLoadProcessorIDfromSMBIOS;

	pMgntInfo->RegPwrByRateFile.Octet = pNdisCommon->RegPwrByRateFile.Octet;
	pMgntInfo->RegPwrLimitFile.Octet = pNdisCommon->RegPwrLimitFile.Octet;
	pMgntInfo->RegSecondaryPwrLimitFile.Octet = pNdisCommon->RegSecondaryPwrLimitFile.Octet;

	pMgntInfo->RegChannelPlan2G.Octet = pNdisCommon->RegChannelPlan2GBuf;
	pMgntInfo->RegChannelPlan5G.Octet = pNdisCommon->RegChannelPlan5GBuf; 
	pMgntInfo->RegChannelPlan2G.Length = pNdisCommon->RegChannelPlan2G.Length;
	pMgntInfo->RegChannelPlan5G.Length = pNdisCommon->RegChannelPlan5G.Length;
	pMgntInfo->RegChannelPlan2G.bDefaultStr = pNdisCommon->RegChannelPlan2G.bDefaultStr;
	pMgntInfo->RegChannelPlan5G.bDefaultStr = pNdisCommon->RegChannelPlan5G.bDefaultStr;


	pMgntInfo->RegDecryptCustomFile = pNdisCommon->RegDecryptCustomFile;

	// 2013/04/16 VincentLan Add to switch Spur Calibration Method
	pMgntInfo->RegSpurCalMethod = pNdisCommon->RegSpurCalMethod;
	// 2013/01/23 VincentLan Add to enable IQK firmware offload feature
	pMgntInfo->RegIQKFWOffload = pNdisCommon->RegIQKFWOffload;
	// 2013/11/23 VincentLan add for for KFree Feature Requested by RF David
	pMgntInfo->RegRfKFreeEnable = pNdisCommon->RegRfKFreeEnable;

	pMgntInfo->RegTxDutyEnable = pNdisCommon->RegTxDutyEnable;

	// 2011/11/15 MH Add for user can select different region and map to dedicated power gain offset table.
	pMgntInfo->RegPwrTblSel = pNdisCommon->RegPwrTblSel;

	pMgntInfo->RegTxPwrLevel = pNdisCommon->RegTxPwrLevel;
	
	// 2011/11/15 MH Add for user can select different tx power by rate switch by default value and registry value.
	pMgntInfo->RegPwrByRate = pNdisCommon->RegPwrByRate;
	pMgntInfo->RegPwrRaTbl1 = pNdisCommon->RegPwrRaTbl1;
	pMgntInfo->RegPwrRaTbl2 = pNdisCommon->RegPwrRaTbl2;
	pMgntInfo->RegPwrRaTbl3 = pNdisCommon->RegPwrRaTbl3;
	pMgntInfo->RegPwrRaTbl4 = pNdisCommon->RegPwrRaTbl4;
	pMgntInfo->RegPwrRaTbl5 = pNdisCommon->RegPwrRaTbl5;
	pMgntInfo->RegPwrRaTbl6 = pNdisCommon->RegPwrRaTbl6;
	pMgntInfo->RegPwrRaTbl7 = pNdisCommon->RegPwrRaTbl7;
	pMgntInfo->RegPwrRaTbl8 = pNdisCommon->RegPwrRaTbl8;
	pMgntInfo->RegPwrRaTbl9 = pNdisCommon->RegPwrRaTbl9;
	pMgntInfo->RegPwrRaTbl10 = pNdisCommon->RegPwrRaTbl10;
	pMgntInfo->RegPwrRaTbl11 = pNdisCommon->RegPwrRaTbl11;
	pMgntInfo->RegPwrRaTbl12 = pNdisCommon->RegPwrRaTbl12;
	pMgntInfo->RegPwrRaTbl13 = pNdisCommon->RegPwrRaTbl13;
	pMgntInfo->RegPwrRaTbl14 = pNdisCommon->RegPwrRaTbl14;
	pMgntInfo->RegPwrRaTbl15 = pNdisCommon->RegPwrRaTbl15;
	pMgntInfo->RegPwrRaTbl16 = pNdisCommon->RegPwrRaTbl16;

	pMgntInfo->bUseRxInterruptWorkItem=pNdisCommon->bRegUseRxInterruptWorkItem;

	// Support QoS or not. Added by Annie, 2005-11-09.
	//Isaiah 2006-06-13
	if(pNdisCommon->bRegSupportQoS) //WMM
	{
		pMgntInfo->pStaQos->QosCapability = QOS_WMM;	

		pMgntInfo->pStaQos->b4ac_Uapsd=pNdisCommon->StaUapsd & 0x0F;
		pMgntInfo->pStaQos->MaxSPLength=pNdisCommon->MaxSPLength;

		if(pMgntInfo->pStaQos->b4ac_Uapsd & 0x0F) //UAPSD
		{	
			pMgntInfo->pStaQos->QosCapability |= QOS_WMM_UAPSD;
		}
		else  // 8xB disable WMM Power Save
		{
			pMgntInfo->pStaQos->QosCapability &=~ QOS_WMM_UAPSD;
			pMgntInfo->pStaQos->b4ac_Uapsd=0;
			pMgntInfo->pStaQos->MaxSPLength=0;
		}
		
		pMgntInfo->pStaQos->QosCapabilityBackup = pMgntInfo->pStaQos->QosCapability;
		
		// EDCA Parameters for QAP
		pAdapter->AP_EDCA_PARAM[0] = pNdisCommon->RegApEDCAParamBE;
		pAdapter->AP_EDCA_PARAM[1] = pNdisCommon->RegApEDCAParamBK;
		pAdapter->AP_EDCA_PARAM[2] = pNdisCommon->RegApEDCAParamVI;
		pAdapter->AP_EDCA_PARAM[3] = pNdisCommon->RegApEDCAParamVO;

		// EDCA PArameters for QSTA
		pAdapter->STA_EDCA_PARAM[0] = pNdisCommon->RegStaEDCAParamBE;
		pAdapter->STA_EDCA_PARAM[1] = pNdisCommon->RegStaEDCAParamBK;
		pAdapter->STA_EDCA_PARAM[2] = pNdisCommon->RegStaEDCAParamVI;
		pAdapter->STA_EDCA_PARAM[3] = pNdisCommon->RegStaEDCAParamVO;

		// No Ack Setting
		pMgntInfo->pStaQos->AcNoAck = pNdisCommon->RegAcNoAck;
	}
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("N6UpdateDefaultSetting(): QosCapability=0x%x\n", pMgntInfo->pStaQos->QosCapability) );


	pMgntInfo->bRegHwParaFile=((BOOLEAN)(pNdisCommon->bRegHwParaFile));

	//TX Power Level
	pMgntInfo->TxPowerLevel	= pNdisCommon->RegTxPowerLevel;

	if(pAdapter->pNdis62Common==NULL)
		MgntActSet_TX_POWER_LEVEL(pAdapter, pMgntInfo->TxPowerLevel);	

	// Default LowRfdThreshold.
	pAdapter->LowRfdThreshold = pNdisCommon->LowRfdThreshold;

	// PSPXLinkMode, revised by Roger. 2009.12.03.
	pMgntInfo->bDefaultPSPXlinkMode = (BOOLEAN)(pNdisCommon->PSPXlinkMode);
	pMgntInfo->bPSPXlinkMode = pMgntInfo->bDefaultPSPXlinkMode; // Using default settings first.

	// Channel Plan.
	pMgntInfo->RegChannelPlan = pNdisCommon->RegChannelPlan;

	pMgntInfo->RegDisableAC = pNdisCommon->RegDisableAC;
	pMgntInfo->RegLedInterval = pNdisCommon->RegLedInterval;
	pMgntInfo->Reg8814auEfuse = pNdisCommon->Reg8814auEfuse;
	pMgntInfo->RegValidRFPath = pNdisCommon->RegValidRFPath;
	pMgntInfo->RegPreInitMem = pNdisCommon->RegPreInitMem;
	pMgntInfo->Reg88EIOTAction = pNdisCommon->Reg88EIOTAction;


	// Turbo mode related setting. Added by Roger, 2006.12.07.
	{
		TURBOMODE_TYPE		TurboModeType;
		TurboModeType.charData = (u1Byte)pNdisCommon->RegTurboModeSelect;
		pMgntInfo->bSupportTurboMode = (BOOLEAN)TurboModeType.field.SupportTurboMode;// Default: sholud be FALSE. Roger, 2006.12.07.
		pMgntInfo->bAutoTurboBy8186  = (BOOLEAN)TurboModeType.field.AutoTurboBy8186;	// Default: should be TRUE.  Roger, 2006.12.07.		
		RT_TRACE( COMP_MLME, DBG_LOUD, ("UpdateDefaultSetting(): bSupportTurboMode=%d, bAutoTurboBy8186=%d\n",
											pMgntInfo->bSupportTurboMode,
											pMgntInfo->bAutoTurboBy8186
											));
	}

	// bExcludeUnencrypted, 2006.11.22, by shien chang.
	// NOTE: the default of bExcludeUnencrypted in NDIS5 is TRUE, but in NDIS6
	// The native wifi document required the default value to be FALSE.
	pMgntInfo->bExcludeUnencrypted = FALSE;
	pMgntInfo->SafeModeEnabled = FALSE;	

	// 802.11d.
	pDot11dInfo->bEnabled = pNdisCommon->bRegDot11dEnable;

	// AP mode: station live time. Added by Annie for ASUS, 2006-02-16.
	pMgntInfo->LiveTime = (pNdisCommon->RegLiveTime > 0)? pNdisCommon->RegLiveTime : 600;

	// WDS mode.
	pMgntInfo->WdsMode = pNdisCommon->RegWdsMode;

	// Overwrite pMgntInfo->CustomerID if necessary, otherwise, don't change it. 
	// Note that, pMgntInfo->CustomerID might be initialized in NicIFReadAdapterInfo() 
	// before. 2006.07.03.
	pMgntInfo->CustomerID = (RT_CUSTOMER_ID)(pNdisCommon->RegCustomerID);

	//Fragment threadhold
	pMgntInfo->FragThreshold = pNdisCommon->RegFragThreshold;

	  //change by ylb 20111124 to Fix DTM error: Vertify Beacon after WakeUp	
	{
		PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter);

		while(pTargetAdapter != NULL)
		{
			pTargetAdapter->MgntInfo.bStartApDueToWakeup=FALSE;

			pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
		}
	}

	//========================= BT related======================== 
	pMgntInfo->btHsMode = pNdisCommon->RegbtHsMode;

	pAdapter->bFixBTTdma = (BOOLEAN)pNdisCommon->RegFixBTTdma;
	//========================= BT related end======================== 

	// Set debug monitor components
	EXdbgmon_SetOutComponents((u4Byte)pNdisCommon->RegDbgMonitor);
	//========================= Power Save Mechanism related======================== 

	PSC_UpdateDefaultSetting(pAdapter);

	pMgntInfo->bDisableCck = pNdisCommon->bRegDisableCck;
	pMgntInfo->bHwWpsPbc = pNdisCommon->bRegHwWpsPbc;

	//========================= Power Save Mechanism end======================== 


	//========================= HAL related======================== 
	HAL_UpdateDefaultSetting(pAdapter);
	//========================= HAL related end======================== 

	pAdapter->bBtFwSupport = (BOOLEAN) pNdisCommon->bRegBtFwSupport;
	

	// ROAM Sensitive Level

	pMgntInfo->RegROAMSensitiveLevel = (s1Byte)(0 - pNdisCommon->RegROAMSensitiveLevel);
	// Roam Hysteresis
	pMgntInfo->RegRoamHysteresis = (u1Byte)(pNdisCommon->RegRoamHysteresis);

	// Roaming time limitation.
	pMgntInfo->RegRoamingLimitCount = (pNdisCommon->RegRoamingLimit / RT_CHECK_FOR_HANG_PERIOD);

	// Indicate disconnected status immediately when receiving deauth/disassoc packet.
	pMgntInfo->IndicateByDeauth = pNdisCommon->RegIndicateByDeauth;

	// Protection mode RTS/CTS
	if(pNdisCommon->RegProtectionmode == RTS_CTS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6UpdateDefaultSetting(): RTS/CTS\n"));
		pMgntInfo->bForcedProtectRTSFrame = TRUE;
		pMgntInfo->bForcedProtectCTSFrame = FALSE;
	}
	else if(pNdisCommon->RegProtectionmode == CTS_TO_SELF)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6UpdateDefaultSetting(): CTS-TO-SELF\n"));
		pMgntInfo->bForcedProtectRTSFrame = FALSE;
		pMgntInfo->bForcedProtectCTSFrame = TRUE;
	}
	else
	{

		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6UpdateDefaultSetting(): No Prottection mode\n"));
		pMgntInfo->bForcedProtectRTSFrame = FALSE;
		pMgntInfo->bForcedProtectCTSFrame = FALSE;
	}

	// DM control initial gain
	pMgntInfo->bDMInitialGainEnable = pNdisCommon->bRegDMInitialGain;

	pMgntInfo->ShowRateMode = pNdisCommon->RegShowRate;

	// 20090810 Ask by AzWave To Force TKIP use N mode
	pMgntInfo->bTKIPinNmodeFromReg = pNdisCommon->bRegTKIPinNmode;
	pMgntInfo->bWEPinNmodeFromReg = pNdisCommon->bRegWEPinNmode;
	pMgntInfo->bRegClevoDriver = pNdisCommon->bRegClevoDriver;

	SET_TDLS_ENABLED(pMgntInfo, pNdisCommon->bRegTDLSEnable);

	pMgntInfo->bDefaultAntenna= pNdisCommon->bRegDefaultAntenna;	

	pMgntInfo->bRegVelocity = pNdisCommon->bRegVelocity;

	pMgntInfo->bSendPacketByTimer = pNdisCommon->RegSendPacketByTimer;
	pMgntInfo->bAPTimExtend = (pNdisCommon->RegAPTimExtend && !pNdisCommon->bRegWiFi) ? TRUE : FALSE;

	pMgntInfo->RegSupTCPOffload = TCPOFFLOAD_DISAB;

	TCP_OFFLOAD_Init(pAdapter);


	pAdapter->RegTwoStaConcurrentMode = pNdisCommon->RegTwoStaConcurrentMode;
	pMgntInfo->bSupportTxPacketBufferColease = pNdisCommon->bSupportTxPacketBufferColease;

	pHalData->PAMode = pNdisCommon->RegPAMode;

	pMgntInfo->CurrentWirelessBand = pNdisCommon->RegWirelessBand;
	
	// 2012/07/24 MH Add for win8 usb whql tst & WFD multi channel.
	pMgntInfo->RegUseDefaultCID = pNdisCommon->RegUseDefaultCID;
	pMgntInfo->RegWfdTime = pNdisCommon->RegWfdTime;
	pMgntInfo->RegWfdChnl = pNdisCommon->RegWfdChnl;
	pMgntInfo->RegScanLarge = pNdisCommon->RegScanLarge;
	pMgntInfo->RegScanMiddle = pNdisCommon->RegScanMiddle;
	pMgntInfo->RegScanNormal = pNdisCommon->RegScanNormal;
	pMgntInfo->RegScanActive = pNdisCommon->RegScanActive;
	pMgntInfo->RegForcedScanPeriod = pNdisCommon->RegForcedScanPeriod;

	pMgntInfo->RegPreferBand = pNdisCommon->RegPreferBand;

	// Update antenna detection registry settings for further mechanism, added by Roger, 2012.11.17.
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_ANT_DETECT, (pu1Byte)&(pNdisCommon->RegAntDetection));

	// TxPwr percentage, added by Roger, 2010.03.09.
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_TX_PWR_PERCENTAGE, &(pNdisCommon->RegTxPwrPercentage));	

	// Pre-transition for OID_PNP_SET_POWER OID wake up handling, added by Roger, 2012.11.28.
	pMgntInfo->bPreTransPnP = pNdisCommon->RegPreTransPnP;

	if(pMgntInfo->bWiFiConfg)
		pMgntInfo->bHWRTSEnable = FALSE;
	else
		pMgntInfo->bHWRTSEnable = pNdisCommon->bRegHWRTSEnable;

	// 2013/02/05 MH Add for streamMode switch.
	pMgntInfo->RegStreamMode = pNdisCommon->RegStreamMode;
	// 2013/02/06 MH Add Transmit power control level for all customer in the future.
	// Need to delete? or other customer need the function.
	pMgntInfo->RegTPCLvl = pNdisCommon->RegTPCLvl;
	pMgntInfo->RegTPCLvlD = pNdisCommon->RegTPCLvlD;
	pMgntInfo->RegTPCLvl5g = pNdisCommon->RegTPCLvl5g;
	pMgntInfo->RegTPCLvl5gD = pNdisCommon->RegTPCLvl5gD;

	if(pAdapter->bInHctTest)
		pMgntInfo->RegEnableAdaptivity = 0;
	else
		pMgntInfo->RegEnableAdaptivity = pNdisCommon->RegEnableAdaptivity;	
	pMgntInfo->RegL2HForAdaptivity = pNdisCommon->RegL2HForAdaptivity;	
	pMgntInfo->RegHLDiffForAdaptivity = pNdisCommon->RegHLDiffForAdaptivity;	
	pMgntInfo->RegEnableCarrierSense = pNdisCommon->RegEnableCarrierSense;
	pMgntInfo->RegNHMEnable = pNdisCommon->RegNHMEnable;
	pMgntInfo->RegDmLinkAdaptivity = pNdisCommon->RegDmLinkAdaptivity;
	pMgntInfo->RegDCbackoff = pNdisCommon->RegDCbackoff;
	pMgntInfo->RegAPNumTH = pNdisCommon->RegAPNumTH;
	pMgntInfo->RegPacketDrop = pNdisCommon->RegPacketDrop;
	
	pMgntInfo->EnableResetTxStuck = pNdisCommon->RegEnableResetTxStuck;

	pMgntInfo->RegSifsThresh = pNdisCommon->RegSifsThresh;

	pMgntInfo->RegFwload = pNdisCommon->RegFwload;

	pMgntInfo->RegUsbSafetySwitch = pNdisCommon->RegUsbSafetySwitch;

	pMgntInfo->RegRspPwr = pNdisCommon->RegRspPwr;
	pMgntInfo->RegPktIndicate = pNdisCommon->RegPktIndicate;

	pMgntInfo->bRegAdhocUseHWSec = pNdisCommon->bRegAdhocUseHWSec;
	pMgntInfo->RegClearAMDWakeUpStatus = pNdisCommon->RegClearAMDWakeUpStatus;

	pMgntInfo->RegbCustomizedScanPeriod = pNdisCommon->RegbCustomizedScanPeriod;
	pMgntInfo->RegIntelCustomizedScanPeriod = pNdisCommon->RegIntelCustomizedScanPeriod;
	pMgntInfo->RegAMDCustomizedScanPeriod = pNdisCommon->RegAMDCustomizedScanPeriod;
	
	
	pMgntInfo->RegDisableBTCoexist = pNdisCommon->RegDisableBTCoexist;

	pMgntInfo->EnableMA = pNdisCommon->RegEnableMA;

	pMgntInfo->VhtWeakSecurity = pNdisCommon->RegVhtWeakSecurity;

	//20130606 MH For passive scan control dynamic switch after meeting with RF/FAE team.
	// 0= by channel plan, 1=5g all passive scan / 2= 24g passive scan /3= 2/5g all passive scan
	pMgntInfo->RegPassiveScan = pNdisCommon->RegPassiveScan;

	pMgntInfo->IsAMDIOIC = pNdisCommon->RegIsAMDIOIC;	
	pMgntInfo->RegWmmPage = pNdisCommon->RegWmmPage;
	pMgntInfo->Regbcndelay = pNdisCommon->Regbcndelay;

	// Support FW FCS
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_FW_FCS, &bFcsCapable);
	if(FALSE == bFcsCapable)
		pMgntInfo->RegMultiChannelFcsMode = 0;
	else
	{
		pMgntInfo->RegMultiChannelFcsMode = pNdisCommon->RegMultiChannelFcsMode;
		MultiChannelSetFcsCommonInfo(pAdapter, MULTICHANNEL_FCS_COMMON_NOA_DURATION, (pu1Byte)&pNdisCommon->RegMultiChannelFcsNoA);
		MultiChannelSetFcsCommonInfo(pAdapter, MULTICHANNEL_FCS_COMMON_NOA_STARTTIME, (pu1Byte)&pNdisCommon->RegMCCNoAStartTime);
		MultiChannelSetFcsCommonInfo(pAdapter, MULTICHANNEL_FCS_COMMON_STA_BEACONTIME, (pu1Byte)&pNdisCommon->RegMCCStaBeaconTime);
		MultiChannelSetFcsCommonInfo(pAdapter, MULTICHANNEL_FCS_COMMON_QPKT_LEVEL, (pu1Byte)&pNdisCommon->RegMCCQPktLevel);
	}

	RT_TRACE_F(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->RegMultiChannelFcsMode = %d\n", pMgntInfo->RegMultiChannelFcsMode));

	pMgntInfo->WFDOpChannel = pNdisCommon->RegWFDOpChannel;
	pMgntInfo->WFDPeerOpChannel = pNdisCommon->RegWFDPeerOpChannel;
	pMgntInfo->ConnectionConfigTimeIntv = pNdisCommon->RegConnectionConfigTimeIntv;

	pMgntInfo->RetryTimes = pNdisCommon->RegRetryTimes;

#if(AUTO_CHNL_SEL_NHM == 1)
	pMgntInfo->AutoChnlSel.bAutoChnlSel = pNdisCommon->RegAutoChnlSel;
	if((IS_AUTO_CHNL_SUPPORT(pAdapter)))
		pMgntInfo->AutoChnlSel.AutoChnlSelPeriod = 100; // Default scan period
#endif

	if(0 == pNdisCommon->RegDisableRtkSupportedP2P)
	{// not to disable
		pMgntInfo->bDisableRtkSupportedP2P = FALSE;
	}
	else if(1 == pNdisCommon->RegDisableRtkSupportedP2P)
	{// to disable
		pMgntInfo->bDisableRtkSupportedP2P = TRUE;
	}
	else
	{// auto
		if(OS_SUPPORT_WDI(pAdapter))
		{
		pMgntInfo->bDisableRtkSupportedP2P = TRUE;
	}
		else
		{
			pMgntInfo->bDisableRtkSupportedP2P = FALSE;
		}
	}
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("RegDisableRtkSupportedP2P = %u\n", pNdisCommon->RegDisableRtkSupportedP2P));
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pMgntInfo->bDisableRtkSupportedP2P = %u\n", pMgntInfo->bDisableRtkSupportedP2P));

	pMgntInfo->bSdioPollIntrHandler = pNdisCommon->RegSdioPollIntrHandler;
	pMgntInfo->SdioIntrPollingLimit = pNdisCommon->RegSdioIntrPollingLimit;
	pMgntInfo->RegTxSendAsap = pNdisCommon->RegTxSendAsap;
	if (pMgntInfo->RegTxSendAsap & BIT15)
		pMgntInfo->CurTxSendAsap = 1;
	else
		pMgntInfo->CurTxSendAsap = 0;

	// MAC Address Randomization 
	pMgntInfo->RegSupportMACRandom = pNdisCommon->RegSupportMACRandom;

	pMgntInfo->RegSupportECSA = pNdisCommon->RegSupportECSA;
	
	pMgntInfo->RegSupportFT= pNdisCommon->RegSupportFT;

	// Cancel and suspend all timers in Dx low power state, e.g., SoC Off
	pMgntInfo->RegSuspendTimerInLowPwr = pNdisCommon->RegSuspendTimerInLowPwr;

}



// 
// Initialize variables read from registry for 818x UI.
// 2004.11.30, by rcnjko.
//
VOID 
N6RestoreLastInitSetting(
	IN	PADAPTER		pAdapter	
	)
{
	PMGNT_INFO pMgntInfo = (PMGNT_INFO)(&(pAdapter->MgntInfo));
	PRT_NDIS6_COMMON pNdisCommon = pAdapter->pNdisCommon;

	int nTmp;
	ULONG BytesRead, BytesNeeded;

	// We keep local copy of RegWepEncStatus here for it can be changed via OID_802_11_ENCRYPTION_STATUS. 
	// 2005.03.16, by rcnjko.
	int nTmpRegWepEncStatus = pNdisCommon->RegWepEncStatus; 

	RT_TRACE( COMP_SEC, DBG_LOUD, ("===> N6RestoreLastInitSetting()\n") );

	// OID_802_11_AUTHENTICATION_MODE. 
	nTmp = pNdisCommon->RegAuthentAlg;
	N6CSetInformation(pAdapter, OID_802_11_AUTHENTICATION_MODE, (PVOID)(&nTmp), sizeof(nTmp), &BytesRead, &BytesNeeded);
	
	// OID_802_11_ENCRYPTION_STATUS.
	switch(nTmpRegWepEncStatus) 
	{
	case REG_WEP_STATUS_NO_WEP:
		switch(pNdisCommon->RegEncAlgorithm)
		{
		case REG_NONE_Encryption:
			nTmp = (int)(Ndis802_11EncryptionDisabled);
			break;
		case REG_TKIPv2_Encryption:
			nTmp = (int)Ndis802_11Encryption2Enabled;
			break;
		case REG_AESCCMP_Encryption:
			nTmp = (int)Ndis802_11Encryption3Enabled;
			break;

		case REG_WAPI_PSK:	
			nTmp = (int)Wapi_Encryption;
			break;
		case REG_WAPI_CERT:	
			nTmp = (int)Wapi_Certificate;
			break;

		default:
			nTmp = (int)(Ndis802_11EncryptionDisabled);
			break;
		}
		break;
	
	case REG_WEP_STATUS_WEP64:
	case REG_WEP_STATUS_WEP128:
		nTmp = (int)(Ndis802_11Encryption1Enabled);
		break;

	default:
		nTmp = (int)(Ndis802_11EncryptionDisabled);
		break;
	}
	N6CSetInformation(pAdapter, OID_802_11_ENCRYPTION_STATUS, (PVOID)(&nTmp), sizeof(nTmp), &BytesRead, &BytesNeeded);

	// WEP Key.
	{
		POCTET_STRING		pKey;
		u4Byte				dwKeyLen;
		int					nKeyIdx;
		UCHAR				NdisKeyTmpBuf[128]; 
		PNDIS_802_11_WEP	pNdisKeyTmp;

		for(nKeyIdx = 0; nKeyIdx < 4;nKeyIdx++)
		{
			pNdisCommon->RegDefaultKey[nKeyIdx].Octet = pNdisCommon->RegDefaultKeyBuf[nKeyIdx];
			pNdisCommon->RegDefaultKey[nKeyIdx].Octet = pNdisCommon->RegDefaultKeyBuf[nKeyIdx];
		}
		// Fix stack overflow bug, 2005.01.06, by rcnjko. 
		pNdisKeyTmp = (PNDIS_802_11_WEP)NdisKeyTmpBuf;
		pNdisKeyTmp->Length = 128;

		switch(nTmpRegWepEncStatus)
		{
		case REG_WEP_STATUS_WEP64:
			pKey = pNdisCommon->RegDefaultKey;
			dwKeyLen = 5;
			break;
		case REG_WEP_STATUS_WEP128:
			pKey = pNdisCommon->RegDefaultKeyW;
			dwKeyLen = 13;
			break;
		default:
			pKey = NULL;
			dwKeyLen = 0;
			break;
		}

		for(nKeyIdx = 0; nKeyIdx < 4 && pKey != NULL; nKeyIdx++)
		{
			if(nKeyIdx == pNdisCommon->RegDefaultKeyId)
			{
				pNdisKeyTmp->KeyIndex = nKeyIdx | 0x80000000;
			}
			else
			{
				pNdisKeyTmp->KeyIndex = nKeyIdx;
			}
			if(pKey[nKeyIdx].Length == dwKeyLen)
			{
				pNdisKeyTmp->KeyLength =  dwKeyLen;
				PlatformMoveMemory(pNdisKeyTmp->KeyMaterial, pKey[nKeyIdx].Octet, dwKeyLen);
			}
			else
			{
				pNdisKeyTmp->KeyLength =  0;
			}
			// OID_802_11_ADD_WEP.
			N6CSetInformation(pAdapter, OID_802_11_ADD_WEP, (PVOID)pNdisKeyTmp, 128, &BytesRead, &BytesNeeded);
		}
	}

	// Network type.
	switch(pNdisCommon->RegNetworkType)
	{
	case NI_ADHOC: 
		pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_ADHOC;
		break;
	case NI_Infrastructure: 
		pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;
		break;
	case NI_AUTO: 
		pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_AUTO;
		break;
	default:
		RT_ASSERT(FALSE, ("Unknown RegNetworkType: %d\n", pNdisCommon->RegNetworkType));
		break;
	}

	if(pAdapter->MgntInfo.bRSNAPSKMode )
	{
		pAdapter->MgntInfo.SecurityInfo.RegSWTxEncryptFlag = EncryptionDecryptionMechanism_Auto;
		pAdapter->MgntInfo.SecurityInfo.RegSWRxDecryptFlag = EncryptionDecryptionMechanism_Auto;
		SecSetSwEncryptionDecryption(pAdapter, TRUE, TRUE);
		// WIN7, Don't set Sec Here
		if(pMgntInfo->NdisVersion < RT_NDIS_VERSION_6_20)
		pAdapter->HalFunc.DisableHWSecCfgHandler(pAdapter);
	}

	pMgntInfo->bEDCCASupport = pNdisCommon->bRegEDCCASupport;
	
	RT_TRACE( COMP_SEC, DBG_LOUD, ("<=== N6RestoreLastInitSetting()\n") );
	
}



//   Description:
//			Restore all config for wake up in IBSS mode 
//
VOID
N6RestoreLastInitSettingAterWakeUP(
	IN	PADAPTER		pAdapter	
	)
{
	PMGNT_INFO pMgntInfo = (PMGNT_INFO)(&(pAdapter->MgntInfo));
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PRT_NDIS6_COMMON pNdisCommon = pAdapter->pNdisCommon;
	u4Byte				nKeyIdx = 0;
	POCTET_STRING		pKey;
	BOOLEAN				IsDefaultKeyId;
	u1Byte	MacAddress[ETHERNET_ADDRESS_LENGTH] = {0x00,0x00,0x00,0x00,0x00,0x00};
	
//add by ylb for wapi S3 BSOD 20110906	
      if(WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
              return;
	
	//if( pNdisCommon->RegNetworkType == RT_JOIN_NETWORKTYPE_INFRA )
	//	return; // Autoconfig will do all thing to connent to AP.

	// 1. Set Network Type
	//BssType = dot11_BSS_type_independent;
	// 20080430, Mark by Mars
	// We should not set BssType in  N6CSet_DOT11_DESIRED_BSS_TYPE
	// It will make BssType to be auto and cause Ad Hoc WPA2 in Vista Fial
	//N6CSet_DOT11_DESIRED_BSS_TYPE(pAdapter , &BssType );

	// 2.set Auth Mod	
	N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(pAdapter , pNdisCommon->RegAuthalg);
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("N6RestoreLastInitSettingAterWakeUP pSecInfo->AuthMode = 0x%08X\n",pSecInfo->AuthMode));
	// 3. Set Uncast Chiper
	N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(pAdapter, pNdisCommon->RegPairwiseAlg);
	pSecInfo->DefaultTransmitKeyIdx = (u1Byte)pNdisCommon->RegRSNAKeyID;
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("N6RestoreLastInitSettingAterWakeUP pSecInfo->PairwiseEncAlgorithm = 0x%08X\n",pSecInfo->PairwiseEncAlgorithm));
	// 4. Set Muticase chiper , autconfig not set this OID in AH-mode
	//N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM( pAdapter, pNdisCommon->RegGroupALg);

	// 5. Set Key if NEED!!
	if( (pNdisCommon->RegPairwiseAlg == DOT11_CIPHER_ALGO_WEP40 
		||pNdisCommon->RegPairwiseAlg == DOT11_CIPHER_ALGO_WEP104
		||pNdisCommon->RegPairwiseAlg == DOT11_CIPHER_ALGO_WEP )   // Need to check if wep-802.1x used  wep to Pairwise key.we need to remove it.
		// &&  pNdisCommon->RegAuthalg == DOT11_AUTH_ALGO_80211_OPEN  // remove it for Infra mode.
		)
	{

		for(nKeyIdx = 0; nKeyIdx < 4;nKeyIdx++)
		{
			pNdisCommon->RegDefaultKey[nKeyIdx].Octet = pNdisCommon->RegDefaultKeyBuf[nKeyIdx];
			pNdisCommon->RegDefaultKeyW[nKeyIdx].Octet = pNdisCommon->RegDefaultKeyWBuf[nKeyIdx];
		}
		
		if( pSecInfo->PairwiseEncAlgorithm ==  RT_ENC_ALG_WEP104 ){
			pKey = pNdisCommon->RegDefaultKeyW;
		}else{
			pKey = pNdisCommon->RegDefaultKey;
		}

		
		for( nKeyIdx = 0 ; nKeyIdx < 4 ; nKeyIdx++ )
		{
			if(nKeyIdx == pNdisCommon->RegDefaultKeyId)
				IsDefaultKeyId = TRUE;
			else
				IsDefaultKeyId = FALSE;
			if( pKey[nKeyIdx].Length != 0 ){
			MgntActSet_802_11_ADD_WEP(
				pAdapter,
				pSecInfo->PairwiseEncAlgorithm,
				nKeyIdx,
				pKey[nKeyIdx].Length,
				pKey[nKeyIdx].Octet,
				IsDefaultKeyId
				);
		}
		}
	}else if(pMgntInfo->bRSNAPSKMode) 
	{
		MgntActSet_RSNA_ADD_DEAULT_KEY(
			pAdapter,
			pNdisCommon->RegPairwiseAlg,
			pNdisCommon->RegRSNAKeyID,
			pNdisCommon->RegRSNADefaultkey.Length,
			pNdisCommon->RegRSNADefaultkey.Octet,
			MacAddress
			);
	}
}

//
//	Description:
//		Initialize common NDIS resource in PRT_NDIS6_COMMON.
//	2006.05.07, by rcnjko.
//
VOID
InitNdis6CommonResources(
	IN	PADAPTER		pAdapter	
	)
{
	PRT_NDIS6_COMMON 	pNdisCommon = pAdapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo 	= &(pAdapter->MgntInfo);	
	u2Byte				index = 0;

	RTInitializeSListHead( &(pNdisCommon->TxNBLWaitQueue) );

	pNdisCommon->bReleaseNblWaitQueueInProgress = FALSE;
	WDI_InitRxQueue(pAdapter);
	NdisInitializeEvent( &(pNdisCommon->AllPacketReturnedEvent) );

	PlatformInitializeWorkItem(
		pAdapter,
		&(pNdisCommon->SetRFPowerStateWorkItem), 
		(RT_WORKITEM_CALL_BACK)SetRFPowerStateWorkItemCallback,
		(PVOID)pAdapter,
		"SetRFPowerStateWorkItem");

	PlatformInitializeWorkItem(
		pAdapter,
		&(pNdisCommon->SetAdhocLinkStateWorkItem), 
		(RT_WORKITEM_CALL_BACK)SetAdhocLinkStateWorkItemCallback,
		(PVOID)pAdapter,
		"SetAdhocLinkStateWorkItem");

	PlatformInitializeTimer(pAdapter, &(pNdisCommon->InitializeAdapterTimer), 
		(RT_TIMER_CALL_BACK)InitializeAdapterTimerCallback, NULL, "InitializeTimer");

	PlatformInitializeTimer(pAdapter, &pNdisCommon->N6CSendSingleNetBufferListTimer, 
		(RT_TIMER_CALL_BACK)N6CSendSingleNetBufferListTimerCallback, 
		NULL, "N6CSendSingleNetBufferListTimer");	

	PlatformInitializeTimer(pAdapter, &(pNdisCommon->PNPReConnentTimer), 
		(RT_TIMER_CALL_BACK)PNPReConnentTimerCallback, NULL, "PNPReConnentTimer");

	if(pMgntInfo->bSupportPacketCoalescing)
	{
		// Do colese
		PlatformInitializeTimer(pAdapter, &(pNdisCommon->D0RxIndicatTimer), 
			(RT_TIMER_CALL_BACK)D0RxIndicatTimerCallback, NULL, "D0RxIndicatTimer");
		
		RTInitializeListHead( &(pNdisCommon->D0FilterPktQueue) );
			
		NdisAllocateSpinLock(&(pNdisCommon->D0FilterPktLock));
			
		//pNdisCommon->D0FilterState = RT_D0_FILTER_NONE; // Init D0FilterState !!
		pNdisCommon->D0FilterState = RT_D0_FILTER_INIT;  // For Test !!!
	}

        PlatformInitializeWorkItem(
		pAdapter,
		&(pNdisCommon->InitializeAdapterWorkItem), 
		(RT_WORKITEM_CALL_BACK)InitializeAdapterWorkItemCallback,
		(PVOID)pAdapter,
		"InitializeAdapterWorkItem");

	PlatformInitializeWorkItem(
		pAdapter,
		&(pNdisCommon->ReleaseDataFrameQueuedWorkItem), 
		(RT_WORKITEM_CALL_BACK)N6CReleaseDataFrameQueuedWorkItemCallback,
		(PVOID)pAdapter,
		"N6CReleaseDataFrameQueuedWorkItemCallback");

}


//
//	Description:
//		Release common NDIS resource in PRT_NDIS6_COMMON.
//	2006.05.07, by rcnjko.
//
VOID
ReleaseNdis6CommonResources(
	IN	PADAPTER		pAdapter	
	)
{
	PRT_NDIS6_COMMON 	pNdisCommon = pAdapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo 	= &(pAdapter->MgntInfo);
    PRT_SDIO_DEVICE     device = GET_RT_SDIO_DEVICE(pAdapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("ReleaseNdis6CommonResources()\n"));

	PlatformFreeWorkItem( &(pNdisCommon->SetRFPowerStateWorkItem) );
	PlatformFreeWorkItem( &(pNdisCommon->SetAdhocLinkStateWorkItem) );

	PlatformFreeWorkItem( &(pNdisCommon->InitializeAdapterWorkItem) );
	PlatformCancelTimer(pAdapter, &(pNdisCommon->InitializeAdapterTimer));	
	PlatformReleaseTimer(pAdapter, &(pNdisCommon->InitializeAdapterTimer));	
	PlatformCancelTimer(pAdapter, &(pNdisCommon->N6CSendSingleNetBufferListTimer));
	PlatformReleaseTimer(pAdapter, &(pNdisCommon->N6CSendSingleNetBufferListTimer));
	
	PlatformCancelTimer(pAdapter, &(pNdisCommon->PNPReConnentTimer));
	PlatformReleaseTimer(pAdapter, &(pNdisCommon->PNPReConnentTimer));

	if(pMgntInfo->bSupportPacketCoalescing)
	{
		PlatformCancelTimer(pAdapter, &(pNdisCommon->D0RxIndicatTimer));
		PlatformReleaseTimer(pAdapter, &(pNdisCommon->D0RxIndicatTimer));

		NdisFreeSpinLock(&(pNdisCommon->D0FilterPktLock));
	}
			
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Release TxSemaphore IOSemaphore before N6CDeInitThread\n"));
		// Release Tx semaphore before cancel corresponding Tx thread.
		PlatformReleaseSemaphore(&device->TxSemaphore);
		// Free Tx semaphore.
		PlatformFreeSemaphore(&device->TxSemaphore);
#if RTL8723_SDIO_IO_THREAD_ENABLE 
		// Release IO semaphore before cancel corresponding IO thread.
		PlatformReleaseSemaphore(&device->IOSemaphore);
		// Free IO semaphore.
		PlatformFreeSemaphore(&device->IOSemaphore);
#endif
		//
		// <Roger_Notes> In the WDI model, the MS Component handles the original MiniportHaltEx call 
		// and splits it into multiple WDI interface calls. So we only need to handle DeinitThread here instead.
		// 2015.03.09.
		//
		if(IsDefaultAdapter(pAdapter))
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("ReleaseNdis6CommonResources N6CDeInitThread for SDIO+-\n"));
			N6CDeInitThread((PVOID)pAdapter); // Added for SDIO WDI
		}

	PlatformFreeWorkItem( &(pNdisCommon->ReleaseDataFrameQueuedWorkItem) );

}


//
//	Description:
//		Wrapper function for status indication on NDIS 6.
//	2006.10.03, by shien chang.
//
VOID
N6IndicateStatus(
	IN	PADAPTER		pAdapter,
	IN	NDIS_STATUS		GeneralStatus,
	IN	PVOID			StatusBuffer,
	IN	UINT			StatusBufferSize
	)
{
	NDIS_HANDLE		MiniportAdapterHandle = pAdapter->pNdisCommon->hNdisAdapter;
	NDIS_STATUS_INDICATION		StatusIndication;

	PlatformZeroMemory(&StatusIndication, sizeof(NDIS_STATUS_INDICATION));

	N6_ASSIGN_OBJECT_HEADER(
		StatusIndication.Header,
		NDIS_OBJECT_TYPE_STATUS_INDICATION,
		NDIS_STATUS_INDICATION_REVISION_1,
		sizeof(NDIS_STATUS_INDICATION));

	StatusIndication.SourceHandle = MiniportAdapterHandle;
	StatusIndication.StatusCode = GeneralStatus;
	StatusIndication.StatusBuffer = StatusBuffer;
	StatusIndication.StatusBufferSize = StatusBufferSize;

	if(pAdapter->pNdis62Common)
	{
		StatusIndication.PortNumber = GET_PORT_NUMBER(pAdapter);

		RT_TRACE(COMP_INDIC, DBG_LOUD, ("Indicate Status: 0x%X, Indicate to PortNumber: %d\n", GeneralStatus, StatusIndication.PortNumber));
	}
	StatusIndication.RequestId=NULL;
	StatusIndication.DestinationHandle=NULL;

	if(!OS_SUPPORT_WDI(pAdapter))
	{
		NdisMIndicateStatusEx(MiniportAdapterHandle, &StatusIndication);
	}
}

VOID
N6CInitializeSpinLocks(
	IN	PADAPTER		pAdapter	
	)
{
	TX_InitializeSpinlock(pAdapter);
	PlatformInitializeSpinLock(pAdapter, RT_TX_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RX_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RM_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_CAM_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_SCAN_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_LOG_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_BW_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_CHNLOP_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RF_OPERATE_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_INITIAL_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RF_STATE_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_H2C_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);

#if VISTA_USB_RX_REVISE
	PlatformInitializeSpinLock(pAdapter, RT_USBRX_CONTEXT_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_USBRX_POSTPROC_SPINLOCK);
#endif

	PlatformInitializeSpinLock(pAdapter, RT_PORT_SPINLOCK);	

	PlatformInitializeSpinLock(pAdapter, RT_GEN_TEMP_BUF_SPINLOCK);

	PlatformInitializeSpinLock(pAdapter, RT_AWB_SPINLOCK);

	PlatformInitializeSpinLock(pAdapter, RT_BTData_SPINLOCK);

	PlatformInitializeSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

	PlatformInitializeSpinLock(pAdapter, RT_HW_TIMER_SPIN_LOCK);

	PlatformInitializeSpinLock(pAdapter, RT_P2P_SPIN_LOCK);
	PlatformInitializeSpinLock(pAdapter, RT_IQK_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_DYN_TXPWRTBL_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);	

	PlatformInitializeSpinLock(pAdapter, RT_INDIC_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_RFD_SPINLOCK);
	
#if DRV_LOG_REGISTRY
	PlatformInitializeSpinLock(pAdapter, RT_DRV_STATE_SPINLOCK);	
#endif

#if (AUTO_CHNL_SEL_NHM == 1)
	PlatformInitializeSpinLock(pAdapter, RT_ACS_SPINLOCK);	
#endif

	PlatformInitializeSpinLock(pAdapter, RT_RX_REF_CNT_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_FW_PS_SPINLOCK);
	PlatformInitializeSpinLock(pAdapter, RT_SYNC_IO_CNT_SPINLOCK);

}


VOID
N6CFreeSpinLocks(
	IN	PADAPTER		pAdapter	
	)
{
	TX_FreeSpinlock(pAdapter);
	PlatformFreeSpinLock(pAdapter, RT_TX_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RX_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_CAM_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RM_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_SCAN_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_LOG_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_BW_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_CHNLOP_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RF_OPERATE_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_INITIAL_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RF_STATE_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_H2C_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);

#if VISTA_USB_RX_REVISE
	PlatformFreeSpinLock(pAdapter, RT_USBRX_CONTEXT_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_USBRX_POSTPROC_SPINLOCK);
#endif


	PlatformFreeSpinLock(pAdapter, RT_PORT_SPINLOCK);	

	PlatformFreeSpinLock(pAdapter, RT_GEN_TEMP_BUF_SPINLOCK);

	PlatformFreeSpinLock(pAdapter, RT_AWB_SPINLOCK);
	
	PlatformFreeSpinLock(pAdapter, RT_BTData_SPINLOCK);

	PlatformFreeSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

	PlatformFreeSpinLock(pAdapter, RT_HW_TIMER_SPIN_LOCK);

	PlatformFreeSpinLock(pAdapter, RT_P2P_SPIN_LOCK);
	PlatformFreeSpinLock(pAdapter, RT_IQK_SPINLOCK);	
	PlatformFreeSpinLock(pAdapter, RT_DYN_TXPWRTBL_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);	
	
	PlatformFreeSpinLock(pAdapter, RT_INDIC_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_RFD_SPINLOCK);

#if DRV_LOG_REGISTRY
	PlatformFreeSpinLock(pAdapter, RT_DRV_STATE_SPINLOCK);
#endif

#if (AUTO_CHNL_SEL_NHM == 1)
	PlatformFreeSpinLock(pAdapter, RT_ACS_SPINLOCK);	
#endif

	PlatformFreeSpinLock(pAdapter, RT_RX_REF_CNT_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_FW_PS_SPINLOCK);
	PlatformFreeSpinLock(pAdapter, RT_SYNC_IO_CNT_SPINLOCK);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<===N6CFreeSpinLocks()\n"));
}


