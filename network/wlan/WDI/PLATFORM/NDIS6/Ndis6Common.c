////////////////////////////////////////////////////////////////////////////////
//
//	File name:		Ndis5Common.c
//	Description:	NDIS5.x common helper functions.
//
//	Author:			rcnjko
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"


#if WPP_SOFTWARE_TRACE
#include "Ndis6Common.tmh"
#endif

#include "CustomOid.h"

WDI_TXRX_STATISTICS		GLWdiTxRxStatistics;
u4Byte					GLWdiTestType=0;
WDI_CMD_STATISTICS		GLWdiCmdStatistics;

typedef struct _OID
{ 
	NDIS_OID				Oid;
	NDIS_REQUEST_TYPE	Type;
} OID;

static OID NdisInitPendingOid[] = {
	{OID_DOT11_SCAN_REQUEST, NdisRequestMethod},
	{OID_DOT11_SCAN_REQUEST, NdisRequestSetInformation},
	{OID_DOT11_NIC_POWER_STATE, NdisRequestMethod},
	{OID_DOT11_NIC_POWER_STATE, NdisRequestSetInformation},
	{OID_DOT11_NIC_POWER_STATE, NdisRequestQueryInformation},
	{OID_DOT11_START_AP_REQUEST, NdisRequestMethod},
	{OID_DOT11_START_AP_REQUEST, NdisRequestSetInformation}};

//
// define local function prototype.
//
static	VOID
InitializeAdapterCommon(
	IN 	PADAPTER 	pAdapter
	);

VOID
N6CopyPacket(
	IN	PNET_BUFFER				pNetBuffer,
	OUT	PRT_TX_LOCAL_BUFFER		pLocalBuffer,
	OUT pu2Byte					pPktLength
	)
{                             
	PMDL			pCurrMdl, pNextMdl;                     
	PUCHAR			ptr;
	UINT			BufLen;
	UINT			RemainMdlLength;

	RemainMdlLength = NET_BUFFER_DATA_LENGTH(pNetBuffer);

	// Initialize packet lenth coalesced.
	*pPktLength = 0;

	// Enumerate the list of MDL.
	for (pCurrMdl = NET_BUFFER_CURRENT_MDL(pNetBuffer);
		pCurrMdl != NULL;
		pCurrMdl = pNextMdl)
	{
		pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
		
		// Prefast warning C30030: Warning: Allocating executable memory via specifying a MM_PAGE_PRIORITY type without a bitwise OR with MdlMappingNoExecute
		// false positive and currently it is safe here, so disable the warning
#pragma warning ( disable:30030 ) 
		ptr = (pCurrMdl == NET_BUFFER_CURRENT_MDL(pNetBuffer)) ?
			((PUCHAR)MmGetSystemAddressForMdlSafe(pCurrMdl, NormalPagePriority))+NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer) :
			MmGetSystemAddressForMdlSafe(pCurrMdl, NormalPagePriority);
		BufLen = (pCurrMdl == NET_BUFFER_CURRENT_MDL(pNetBuffer)) ?
				MmGetMdlByteCount(pCurrMdl)-NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer) :
				MmGetMdlByteCount(pCurrMdl);

		if(RemainMdlLength < BufLen)
		{
			BufLen = RemainMdlLength;
			RemainMdlLength = 0;
		}
		else
		{
			RemainMdlLength -= BufLen;
		}

		// Copy the data.
		if(BufLen > 0)
		{
			PlatformMoveMemory( 
				(PVOID)( pLocalBuffer->Buffer.VirtualAddress + *pPktLength ), 
				ptr, 
				BufLen);
			*pPktLength += (u2Byte)BufLen;
		}
	}
}



//
//	Description:
//		Parse the file content into the lines of buffer, pBufOfLines,
//		which is a 2-dimention array, for example,
//		u1Byte pBufOfLines[nMaxNumLine][nMaxByteCntLine].
//
VOID
ParseFileBufToLines(
	IN 		PVOID		pAdapter,
	IN		pu1Byte		pucFilePtr,
	IN		u4Byte		nFileLen,
	IN OUT	pu1Byte		pBufOfLines,
	IN		s4Byte		nMaxNumLine,
	IN		s4Byte		nMaxByteCntLine,
	OUT		ps4Byte		pnNumLinesRead
	)
{
	int		nNumLinesRead;
	int		nIndex;
	pu1Byte	pucBuffer;

	BOOLEAN	bExceedBytesOfLine;
	u1Byte	szEnd[4] = {'0', 'x', 'f', 'f'};
	u1Byte	szEnd_88e[5] = {'0', 'x', 'f', 'f', 'f'};

	pu1Byte	pFileEnd = pucFilePtr + nFileLen;
	BOOLEAN bReachFileEnd = FALSE;

	RT_TRACE(COMP_INIT, DBG_TRACE, ("----> ParseFileBufToLines()\n"));

	*pnNumLinesRead = nNumLinesRead = 0;

	while((*pucFilePtr != '\0') && (pucFilePtr != pFileEnd))
	{
		// Read a line from file and copy into buffer.
		pucBuffer = (pBufOfLines + (nMaxByteCntLine * nNumLinesRead));
		nIndex = 0;
		bExceedBytesOfLine = FALSE;
		while((*pucFilePtr != '\0') && (*pucFilePtr != '\r') && (*pucFilePtr != '\n')) 
		{
			if(nIndex < (nMaxByteCntLine-1)) // 1 is reserved for EOS.
			{
				pucBuffer[nIndex] = *pucFilePtr;
				pucFilePtr++;
				nIndex++;
			}
			else
			{
				// Skip the remainding bytes if this line exceeds max byte count of a line, nMaxByteCntLine.
				pucFilePtr++;
				bExceedBytesOfLine = TRUE;
			}

			if(pucFilePtr == pFileEnd)
			{
				bReachFileEnd = TRUE;
				break;
			}
		}

		if(nIndex > 0)
		{
			// Append EOS to the line read and enumerate to next line.
			pucBuffer[nIndex] = '\0';
			nNumLinesRead++;
			RT_TRACE(COMP_INIT, DBG_TRACE, ("%04d: \"%s\"\n", nNumLinesRead, pucBuffer));
		
			// Message indication: Exceed Max Byte count of line.
			if(bExceedBytesOfLine)
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("ParseFileBufToLines(): Exceed nMaxByteCntLine! nNumLinesRead:%d pucBuffer: \"%s\"\n", nNumLinesRead, pucBuffer));
			}

			// Reach file end. End of reading/parsing process.
			if(bReachFileEnd)
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("ParseFileBufToLines(): Reach File End!!\n"));
				break;
			}

			{
				// According to the format of configuration file, Offset 0xff is EndOfLine
				if(!PlatformCompareMemory(pucBuffer, szEnd_88e, sizeof(szEnd_88e)))
				{
					RT_TRACE(COMP_INIT, DBG_TRACE, ("ParseFileBufToLines():End Of configuration file, break\n"));
					break;
				}			
			}
		}
	
		// Goto next line.
		pucFilePtr++;
	}

	*pnNumLinesRead = nNumLinesRead;

	RT_TRACE(COMP_INIT, DBG_TRACE, ("<---- ParseFileBufToLines()\n"));
}



//
//	Description:
//		Open the file specifed and read it into pBufOfLines, 
//		which is a 2-dimention array, for example,
//		u1Byte pBufOfLines[nMaxNumLine][nMaxByteCntLine].
//
//	Note:
//		In NDIS5, the file to open shall be placed at the directory the same as 
//		driver binary.
//
RT_STATUS
PlatformReadFile(
	IN		PVOID		Adapter,
	IN		ps1Byte		szFileName,
	IN OUT	pu1Byte		pBufOfLines,
	IN		s4Byte		nMaxNumLine,
	IN		s4Byte		nMaxByteCntLine,
	OUT		ps4Byte		pnNumLinesRead
	)
{
	RT_STATUS				rtStatus = RT_STATUS_FAILURE;
	NDIS_STRING				NdisStrFileName;
	NDIS_PHYSICAL_ADDRESS	ndisPhyAddr;
	NDIS_STATUS				ndisStatus;
	NDIS_HANDLE				hFileHandle;
	UINT					ulFileLength;
	pu1Byte					pMappedFile = NULL;

	// Check input parameters.
	if(szFileName == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformReadFile(): szFileName should not be NULL!\n"));
		return rtStatus;
	}
	if(pBufOfLines == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformReadFile(): pBufOfLines should not be NULL!\n"));
		return rtStatus;
	}

	// Convert szFileName to NDIS_STRING.
	NdisInitializeString(&NdisStrFileName, (PUCHAR)szFileName);
	if(NdisStrFileName.Buffer != NULL && NdisStrFileName.Length > 0)
	{
		// Open the file specified.
		ndisPhyAddr.LowPart = ndisPhyAddr.HighPart = -1;
		NdisOpenFile(&ndisStatus,
					&hFileHandle,
					&ulFileLength,
					&NdisStrFileName,
					ndisPhyAddr);
		if(ndisStatus == NDIS_STATUS_SUCCESS)
		{
			// Map the file into memory.
			NdisMapFile(&ndisStatus, (PVOID *)(&pMappedFile), hFileHandle);
			if(ndisStatus == NDIS_STATUS_SUCCESS)
			{
				// Read the file into pBufOfLines.
				ParseFileBufToLines(Adapter, pMappedFile, ulFileLength, pBufOfLines, nMaxNumLine, nMaxByteCntLine, pnNumLinesRead);
	
				// Return Success only when Config Success
				rtStatus = RT_STATUS_SUCCESS; 
	
				// Relase the memory for mapping the file.
				NdisUnmapFile(hFileHandle);
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadFile(): failed to map the file, %s!, ndisStatus: %#X\n", szFileName, ndisStatus));
			}
	
			// Close the file.
			NdisCloseFile(hFileHandle);
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadFile(): failed to open the file, %s!, ndisStatus: %#X\n", szFileName, ndisStatus));
		}
	
		// Release the NDIS_STRING allocated via NdisInitializeString().
		NdisFreeString(NdisStrFileName);
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadFile(): NdisInitializeString() failed! szFileName: %s\n", szFileName));
	}

	return rtStatus;
}

RT_STATUS
PlatformOpenFile(
	IN		PVOID				Adapter,
	IN		ps1Byte				szFileName,
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	)
{
	RT_STATUS				rtStatus = RT_STATUS_FAILURE;
	NDIS_STRING				NdisStrFileName;
	NDIS_PHYSICAL_ADDRESS	ndisPhyAddr;
	NDIS_STATUS				ndisStatus;
	pu1Byte					pMappedFile = NULL;

	
	// Check input parameters.
	if(szFileName == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PlatformOpenFile(): szFileName should not be NULL!\n"));
		return rtStatus;
	}

	// Convert szFileName to NDIS_STRING.
	NdisInitializeString(&NdisStrFileName, (PUCHAR)szFileName);
	if(NdisStrFileName.Buffer != NULL && NdisStrFileName.Length > 0)
	{
		// Open the file specified.
		ndisPhyAddr.LowPart = ndisPhyAddr.HighPart = -1;
		NdisOpenFile(&ndisStatus,
					&(pFileHandler->FileHandler),
					&(pFileHandler->FileLength),
					&NdisStrFileName,
					ndisPhyAddr);
		
		NdisFreeString(NdisStrFileName);
		if(ndisStatus == NDIS_STATUS_SUCCESS)
		{
			rtStatus = RT_STATUS_SUCCESS;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("PlatformOpenFile(): failed to open the file, %s!, ndisStatus: %#X\n", szFileName, ndisStatus));
		}
		
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PlatformOpenFile(): NdisInitializeString() failed! szFileName: %s\n", szFileName));
	}

	return rtStatus;
}

RT_STATUS
PlatformMapFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	)
{
	RT_STATUS				rtStatus = RT_STATUS_FAILURE;
	NDIS_STATUS				ndisStatus;

	
	// Map the file into memory.
	NdisMapFile(&ndisStatus, (PVOID *)(&(pFileHandler->MappedFile)), pFileHandler->FileHandler);
	if(ndisStatus == NDIS_STATUS_SUCCESS) {
		rtStatus = RT_STATUS_SUCCESS;
	}
	else {
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PlatformMapFile(): failed to map the file!, ndisStatus: %#X\n", ndisStatus));
	}

	return rtStatus;
}

VOID
PlatformUnMapFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	)
{
	// Relase the memory for mapping the file.
	NdisUnmapFile(pFileHandler->FileHandler);
}

VOID
PlatformCloseFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	)
{
	// Relase the memory for mapping the file.
	NdisCloseFile(pFileHandler->FileHandler);
}

//
//	Description:
//		Indication for PHY power state changed.
//	061013, by rcnjko.
//
VOID
N6IndicateCurrentPhyPowerState(
	IN	PADAPTER		pAdapter,
	IN	ULONG			phyId
    )
{
	RT_RF_POWER_STATE rfState;
	DOT11_PHY_STATE_PARAMETERS phyStateParams;
	
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO			pDefaultMgnt = &pDefaultAdapter->MgntInfo;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> N6IndicateCurrentPhyPowerState()\n"));	

	//Add for win 7, to avoid get the wrong Hw reg. By Maddest, 05112009
	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));

	if(pDefaultMgnt->RegRfOff)
	{
		RT_TRACE(COMP_MLME,DBG_LOUD,("N6IndicateCurrentPhyPowerState: RegRFOFF CASE \n"));
		rfState = eRfOff;
		pAdapter->MgntInfo.RfOffReason |= RF_CHANGE_BY_SW;
	}
	
	if(rfState  == eRfOff)
	{
		if (pAdapter->MgntInfo.RfOffReason >= RF_CHANGE_BY_HW)
		{
			rfState  = eRfOff;
		}
		else
		{
			rfState  = eRfOn;
		}
	}
	PlatformZeroMemory(&phyStateParams, sizeof(DOT11_PHY_STATE_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		phyStateParams.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_PHY_STATE_PARAMETERS_REVISION_1,
		sizeof(DOT11_PHY_STATE_PARAMETERS));
	
	phyStateParams.uPhyId = DOT11_PHY_ID_ANY;
	//
	// <Roger_Notes> We set following parameters to deal with mobility center 
	// which RF on/off option NEVER disabled issue.
	// 2008.03.06.
	//
	if( rfState == eRfOff )
	{	
		phyStateParams.bHardwarePhyState =(pAdapter->MgntInfo.RfOffReason & RF_CHANGE_BY_HW) ? FALSE: TRUE; 
		phyStateParams.bSoftwarePhyState = (pAdapter->MgntInfo.RfOffReason & RF_CHANGE_BY_SW) ? FALSE: TRUE;
	}	
	else
	{// We set HW/SW options all available.
		phyStateParams.bHardwarePhyState = TRUE;
		phyStateParams.bSoftwarePhyState = TRUE;
	}

	RT_TRACE(COMP_RF, DBG_LOUD, ("Indicate radio sts HW/SW=%d/%d\n", phyStateParams.bHardwarePhyState, phyStateParams.bSoftwarePhyState));

	if( !OS_SUPPORT_WDI(pAdapter) )
	{
		N6IndicateStatus(
			pAdapter, 
			NDIS_STATUS_DOT11_PHY_STATE_CHANGED,
			&phyStateParams,
			sizeof(phyStateParams));
	}
}

//
//	Description:
//		Write "RFOff" registry value
//	2006.11.13, by shien chang.
//
NDIS_STATUS
N6WriteRegRfPowerState(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bRfOn)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("RFOff");
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT,DBG_TRACE, ("N6WriteRegRfPowerState(): Status=%x\n", Status));
		return Status;
	}
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = (bRfOn ? 0 : 1);
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "Adhoc link state" registry value
//	2009.02.24, by MH.
//
NDIS_STATUS
N6WriteAdhocLinkState(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			AdhocLinkState)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("AdhocLinkState");
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT,DBG_LOUD, ("N6WriteAdhocLinkState(): Status=%x\n", Status));
		return Status;
	}
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = ((AdhocLinkState==TRUE) ? TRUE : FALSE);
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "U2/U3 mode state" registry value
//	2012.12.12, by MH.
//
NDIS_STATUS
N6WriteUsbCurrentMode(
	IN	PADAPTER		Adapter,
	IN	u1Byte			UsbMode)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbCurMode");
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s: UsbMode=%x\n", __FUNCTION__, UsbMode));
		return Status;
	}
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = UsbMode;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "U2/U3 mode switch counter" registry value
//	2012.12.13, by MH.
//
NDIS_STATUS
N6WriteUsbMode3To2Counter(
	IN	PADAPTER		Adapter)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbMode3To2Cnt");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s:\n", __FUNCTION__));
		return Status;
	}

	pNdisCommon->RegUsbMode3To2Cnt++;
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = pNdisCommon->RegUsbMode3To2Cnt;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "U2/U3 mode switch counter" registry value
//	2012.12.14, by MH.
//
NDIS_STATUS
N6WriteUsbMode2To3Counter(
	IN	PADAPTER		Adapter)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbMode2To3Cnt");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s():\n", __FUNCTION__));
		return Status;
	}

	pNdisCommon->RegUsbMode2To3Cnt++;
	pNdisCommon->RegUsbMode2To3CntPrev = pNdisCommon->RegUsbMode2To3Cnt;	

	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = pNdisCommon->RegUsbMode2To3Cnt;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "USB mode switch counter" registry value
//	2013.05.23, by MH.
//
NDIS_STATUS
N6WriteUsbModeSwitchFlag(
	IN	PADAPTER		Adapter,
	IN	u1Byte			UsbSwitch)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbSp");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s:\n", __FUNCTION__));
		return Status;
	}

	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = UsbSwitch;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "USB mode switch Channel" registry value
//	2013.05.23, by MH.
//
NDIS_STATUS
N6WriteUsbModeSwitchChnl(
	IN	PADAPTER		Adapter,
	IN	u2Byte			Channel)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbChnl");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s:\n", __FUNCTION__));
		return Status;
	}

	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = Channel;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Write "USB mode switch Channel" registry value
//	2013.06.06, by MH.
// Switch by 0 = from service 1= by driver itself.
//
NDIS_STATUS
N6WriteUsbModeSwitchBy(
	IN	PADAPTER		Adapter,
	IN	u1Byte			SwitchBy)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbSwBy");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s:\n", __FUNCTION__));
		return Status;
	}

	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = SwitchBy;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}




//
//	Description:
//		Write "U2/U3 mode switch counter" registry value
//	2012.12.14, by MH.
//
NDIS_STATUS
N6WriteUsbMode2To3CounterFail(
	IN	PADAPTER		Adapter)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("UsbMode2To3CntFail");
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_DISP(FUSWITCH, FUSWITCH_ERROR, ("%s:\n", __FUNCTION__));
		return Status;
	}

	pNdisCommon->RegUsbMode2To3CntFail++;
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = pNdisCommon->RegUsbMode2To3CntFail;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	return Status;
}


//
//	Description:
//		Indicate scan complete.
//	2006.11.14, by shien chang.
//
VOID
N6IndicateScanComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	)
{
	NDIS_STATUS		ndisStatus;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	
	if (status == RT_STATUS_SUCCESS)
	{
		ndisStatus = NDIS_STATUS_SUCCESS;
	}
	else
	{
		ndisStatus = NDIS_STATUS_FAILURE;
	}
	
	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_SCAN_CONFIRM,
		&ndisStatus,
		sizeof(NDIS_STATUS));	

}

//
//	Description:
//		Indicate connection started.
//	2006.10.17, by shien chang.
//
VOID
N6IndicateConnectionStart(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	DOT11_CONNECTION_START_PARAMETERS ConnectStartParam;
	
	Adapter->LastConnectStartIndicationTime = PlatformGetCurrentTime();
	
	PlatformZeroMemory(&ConnectStartParam, sizeof(DOT11_CONNECTION_START_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		ConnectStartParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_CONNECTION_START_PARAMETERS_REVISION_1,
		sizeof(DOT11_CONNECTION_START_PARAMETERS));

	if (pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
	{
		ConnectStartParam.BSSType = dot11_BSS_type_infrastructure;
	}
	else if (pMgntInfo->OpMode == RT_OP_MODE_IBSS)
	{
		ConnectStartParam.BSSType = dot11_BSS_type_independent ;

		//
		// 061227, rcnjko: 
		// For fake AP mode, we tell upper layer take our MAC address as the BSSID 
		// to make the frames sent from and indicate to upper layer having correct 
		// BSSID when we are configured to be AP mode later.
		//
		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
		{
			PlatformMoveMemory(
				ConnectStartParam.AdhocBSSID,
				Adapter->CurrentAddress, 
				sizeof(DOT11_MAC_ADDRESS));
		}
		else
		{
			PlatformMoveMemory(
				ConnectStartParam.AdhocBSSID,
				 pMgntInfo->Bssid, 
				sizeof(DOT11_MAC_ADDRESS));
		}

		ConnectStartParam.AdhocSSID.uSSIDLength = pMgntInfo->Ssid.Length;
		PlatformMoveMemory(
			ConnectStartParam.AdhocSSID.ucSSID,
			pMgntInfo->Ssid.Octet,
			pMgntInfo->Ssid.Length);
	}

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_CONNECTION_START,
		&ConnectStartParam,
		sizeof(DOT11_CONNECTION_START_PARAMETERS));

}

VOID
N6IndicateConnectionComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	DOT11_CONNECTION_COMPLETION_PARAMETERS	ConnectCompleteParam;
	
	N6_ASSIGN_OBJECT_HEADER(
		ConnectCompleteParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_CONNECTION_COMPLETION_PARAMETERS_REVISION_1,
		sizeof(DOT11_CONNECTION_COMPLETION_PARAMETERS));

	if (status == RT_STATUS_SUCCESS)
	{
		ConnectCompleteParam.uStatus = DOT11_ASSOC_STATUS_SUCCESS;
	}
	else 
	{
		ConnectCompleteParam.uStatus = DOT11_ASSOC_STATUS_FAILURE;
	}

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_CONNECTION_COMPLETION,
		&ConnectCompleteParam,
		sizeof(DOT11_CONNECTION_COMPLETION_PARAMETERS));

}

VOID
N6IndicateAssociationStart(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PASOC_INFO	pAsocInfo = &(pMgntInfo->AsocInfo);
	DOT11_ASSOCIATION_START_PARAMETERS	AssoStartParam;

	PlatformZeroMemory(&AssoStartParam, sizeof(DOT11_ASSOCIATION_START_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
		AssoStartParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_ASSOCIATION_START_PARAMETERS_REVISION_1,
		sizeof(DOT11_ASSOCIATION_START_PARAMETERS));
	
	PlatformMoveMemory(
		AssoStartParam.MacAddr,
		pAsocInfo->PeerAddr,
		sizeof(DOT11_MAC_ADDRESS));
	AssoStartParam.SSID.uSSIDLength = pMgntInfo->Ssid.Length;
	PlatformMoveMemory(
		AssoStartParam.SSID.ucSSID,
		pMgntInfo->Ssid.Octet,
		pMgntInfo->Ssid.Length);

	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Peer:", pAsocInfo->PeerAddr);
	RT_PRINT_STR(COMP_MLME, DBG_LOUD, "SSID", pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	AssoStartParam.uIHVDataOffset = 0;
	AssoStartParam.uIHVDataSize = 0;

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_ASSOCIATION_START,
		&AssoStartParam,
		sizeof(DOT11_ASSOCIATION_START_PARAMETERS));

}

VOID
N6IndicateAssociationComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	RT_AUTH_MODE	authmode;
	PDOT11_ASSOCIATION_COMPLETION_PARAMETERS	pAssoCompleteParam;
	RT_STATUS		rtStatus;
	ULONG			AllocSize;

	pu1Byte pInfoStart = NULL;
	int nInfoOffset = 0;

	// <SC_TODO: what's the matter when auth failed>

	AllocSize = sizeof(DOT11_ASSOCIATION_COMPLETION_PARAMETERS) +
				sizeof(u4Byte); // PHYID
	if(status == RT_STATUS_SUCCESS)
	{
		if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
		{
			AllocSize += MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocReqLength) +
						MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocRespLength) +
						MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
		}
		else if (pMgntInfo->OpMode == RT_OP_MODE_IBSS)
		{
			AllocSize += MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
		}
		else if (pMgntInfo->OpMode == RT_OP_MODE_AP)
		{
			ConstructBeaconFrame(Adapter);
			MgntUpdateAsocInfo(Adapter, UpdateAsocBeacon, pMgntInfo->beaconframe.Octet, pMgntInfo->beaconframe.Length);
			AllocSize += MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
		}
		else
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("DrvIFIndicateAssociationComplete(): OpMode: %d\n", pMgntInfo->OpMode));
		}
	}
	rtStatus = PlatformAllocateMemory(
				Adapter,
				&pAssoCompleteParam,
				AllocSize);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, 
			("DrvIFIndicateAssociationComplete(): failed to allocate memory for indication.\n"));
			return;
	}
	PlatformZeroMemory(pAssoCompleteParam, AllocSize);
	
	N6_ASSIGN_OBJECT_HEADER(
		pAssoCompleteParam->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_ASSOCIATION_COMPLETION_PARAMETERS_REVISION_2,
		sizeof(DOT11_ASSOCIATION_COMPLETION_PARAMETERS));

	PlatformMoveMemory(
		pAssoCompleteParam->MacAddr,
		pMgntInfo->AsocInfo.PeerAddr,
		sizeof(DOT11_MAC_ADDRESS));
	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Peer:", pAssoCompleteParam->MacAddr);

	
	if (status == RT_STATUS_SUCCESS)
	{
		pAssoCompleteParam->uStatus = DOT11_ASSOC_STATUS_SUCCESS;
	}
	else
	{
		pAssoCompleteParam->uStatus = DOT11_ASSOC_STATUS_FAILURE;
	}

	if (pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
	{
		pAssoCompleteParam->bReAssocReq = pMgntInfo->AsocInfo.FlagReAsocReq;
		pAssoCompleteParam->bReAssocResp = pMgntInfo->AsocInfo.FlagReAsocResp;
	}
	else
	{
		pAssoCompleteParam->bReAssocReq = FALSE;
		pAssoCompleteParam->bReAssocResp = FALSE;
	}

	pAssoCompleteParam->uAssocReqOffset = 0;
	pAssoCompleteParam->uAssocReqSize = 0;
	pAssoCompleteParam->uAssocRespOffset = 0;
	pAssoCompleteParam->uAssocRespSize = 0;

	pAssoCompleteParam->uIHVDataOffset = 0;
	pAssoCompleteParam->uIHVDataSize = 0;

	// DOT11_AUTH_ALGORITHM  AuthAlgo;
	MgntActQuery_802_11_AUTHENTICATION_MODE( Adapter, &authmode );
	pAssoCompleteParam->AuthAlgo = N6CAuthModeToDot11( &authmode );

	// DOT11_CIPHER_ALGORITHM  UnicastCipher;
	pAssoCompleteParam->UnicastCipher = N6CEncAlgorithmToDot11( &(pSecInfo->PairwiseEncAlgorithm) );

	// DOT11_CIPHER_ALGORITHM  MulticastCipher;
	pAssoCompleteParam->MulticastCipher = N6CEncAlgorithmToDot11( &(pSecInfo->GroupEncAlgorithm) );

	pAssoCompleteParam->bFourAddressSupported = FALSE;
	pAssoCompleteParam->bPortAuthorized = FALSE;
	pAssoCompleteParam->DSInfo = DOT11_DS_UNKNOWN;
	pAssoCompleteParam->uEncapTableOffset = 0;
	pAssoCompleteParam->uEncapTableSize = 0;

	//
	// Set Roaming Info in CCKM Fast Roaming
	//
	if(pSecInfo->AuthMode == DOT11_AUTH_ALGO_CCKM){
		if(pAssoCompleteParam->bReAssocReq){
			pAssoCompleteParam->bPortAuthorized = TRUE;
		}
	}

	//
	// Set up Extra information.
	//
	pInfoStart = (pu1Byte)pAssoCompleteParam;
	nInfoOffset = sizeof(DOT11_ASSOCIATION_COMPLETION_PARAMETERS);

	// PHY ID.
	if (status == RT_STATUS_SUCCESS)
	{
		pNdisCommon->ActivePhyId = N6CQuery_DOT11_OPERATING_PHYID(Adapter);
		
	pAssoCompleteParam->uActivePhyListSize = sizeof(u4Byte);
	pAssoCompleteParam->uActivePhyListOffset = nInfoOffset;
	{
		PULONG	pLong = (PULONG)((PUCHAR)pAssoCompleteParam + sizeof(DOT11_ASSOCIATION_COMPLETION_PARAMETERS));
			*((ULONG UNALIGNED *)pLong) = pNdisCommon->ActivePhyId;
		}				
	}
	else
	{
		//set the uActivePhyListOffset and uActivePhyListSize members to zero 
		//if uStatus is not set to NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION.
		pAssoCompleteParam->uActivePhyListSize = 0;
		pAssoCompleteParam->uActivePhyListOffset = 0;
	}				
	
	nInfoOffset += pAssoCompleteParam->uActivePhyListSize;

	if(status == RT_STATUS_SUCCESS)
	{
		if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
		{
			// Beacon
			pAssoCompleteParam->uBeaconOffset = nInfoOffset;
			pAssoCompleteParam->uBeaconSize = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
								MMPDU_BODY(pMgntInfo->AsocInfo.Beacon), 
								pAssoCompleteParam->uBeaconSize);
			nInfoOffset += pAssoCompleteParam->uBeaconSize;
	
			// Association Request.
			pAssoCompleteParam->uAssocReqOffset = nInfoOffset;
			pAssoCompleteParam->uAssocReqSize = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocReqLength); 
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
							MMPDU_BODY(pMgntInfo->AsocInfo.AsocReq), 
							pAssoCompleteParam->uAssocReqSize);
			nInfoOffset += pAssoCompleteParam->uAssocReqSize;

			// Association Response.
			pAssoCompleteParam->uAssocRespOffset = nInfoOffset;
			pAssoCompleteParam->uAssocRespSize = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocRespLength);
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
							MMPDU_BODY(pMgntInfo->AsocInfo.AsocResp), 
							pAssoCompleteParam->uAssocRespSize);
			nInfoOffset += pAssoCompleteParam->uAssocRespSize;
		
		}
		else if(pMgntInfo->OpMode == RT_OP_MODE_IBSS)
		{
			// Beacon
			pAssoCompleteParam->uBeaconOffset = nInfoOffset;
			pAssoCompleteParam->uBeaconSize = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
								MMPDU_BODY(pMgntInfo->AsocInfo.Beacon), 
								pAssoCompleteParam->uBeaconSize);
			nInfoOffset += pAssoCompleteParam->uBeaconSize;
		}
		else if(pMgntInfo->OpMode == RT_OP_MODE_AP)
		{

			RT_PRINT_DATA(COMP_AP, DBG_LOUD, "INDIC ASSOC COMP BEACON2", pMgntInfo->AsocInfo.Beacon, pMgntInfo->AsocInfo.BeaconLength);
			// Beacon
			pAssoCompleteParam->uBeaconOffset = nInfoOffset;
			pAssoCompleteParam->uBeaconSize = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
								MMPDU_BODY(pMgntInfo->AsocInfo.Beacon), 
								pAssoCompleteParam->uBeaconSize);
			nInfoOffset += pAssoCompleteParam->uBeaconSize;
		}
	}

	RT_TRACE(COMP_SEC, DBG_LOUD, ("===> pMgntInfo->bInBIPMFPMode = %d\n", pMgntInfo->bInBIPMFPMode));

	
	if( pMgntInfo->bInBIPMFPMode && TEST_FLAG(pMgntInfo->targetAKMSuite, AKM_RSNA_1X_SHA256 | AKM_RSNA_PSK_SHA256))
		pAssoCompleteParam->MulticastMgmtCipher  = DOT11_CIPHER_ALGO_BIP;
	else
		pAssoCompleteParam->MulticastMgmtCipher  = DOT11_CIPHER_ALGO_NONE;

	// If reasoncode == 30
	// TIE 
	// ID | LEN | Timeout Interval Type | Timeout Interval Value
	// 56 | 5    | 3 = Association ComeBack time
	// Set up uAssocComebackTime
	if( pMgntInfo->RspStatusCode == 30 && pMgntInfo->TIE.Length != 0)
	{
		if(pMgntInfo->TIE.Octet[0] == 3 )
		{
			pAssoCompleteParam->uAssocComebackTime = (u4Byte)(pMgntInfo->TIE.Octet[1] & 0xff);
			pAssoCompleteParam->uAssocComebackTime += (u4Byte)(pMgntInfo->TIE.Octet[2] << 8) & 0xff;
			pAssoCompleteParam->uAssocComebackTime += (u4Byte)(pMgntInfo->TIE.Octet[3] << 16) & 0xff;
			pAssoCompleteParam->uAssocComebackTime += (u4Byte)(pMgntInfo->TIE.Octet[4] << 24) & 0xff;
		}
		else
			pAssoCompleteParam->uAssocComebackTime = 0;
	}
	else 
		pAssoCompleteParam->uAssocComebackTime = 0;

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION,
		pAssoCompleteParam,
		AllocSize);

	
	PlatformFreeMemory(pAssoCompleteParam, AllocSize);

}

//
// Note:
//	After indicating Disassociation event to Ndis, the OS waits 10 secs and the miniport driver can then
//	indicate Roaming Start event if need. After 10 secs, Ndis reset the miniport driver by setting the OID
//	OID_DOT11_RESET_REUEST. By Bruce, 2009-02-10.
//
VOID
N6IndicateDisassociation(
	IN	PADAPTER		Adapter,
	IN	u2Byte			reason)
{
	DOT11_DISASSOCIATION_PARAMETERS DisAssoParam;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PASOC_INFO	pAsocInfo = &(pMgntInfo->AsocInfo);

	PlatformZeroMemory(&DisAssoParam, sizeof(DOT11_DISASSOCIATION_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
		DisAssoParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_DISASSOCIATION_PARAMETERS_REVISION_1,
		sizeof(DOT11_DISASSOCIATION_PARAMETERS));

	if (pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
	{
		pu1Byte		pBSSID;

		//
		// If roaming is failed and disassociation event should be indicated, 
		// we should indicate the previous BSSID before roaming. 
		// By Bruce, 2008-05-08.
		//
		if(pMgntInfo->RoamingType == RT_ROAMING_NORMAL)
		{
			pBSSID = pMgntInfo->APaddrbeforeRoaming;
		}
		else
		{
			pBSSID = pMgntInfo->Bssid;
		}
		PlatformMoveMemory(
			DisAssoParam.MacAddr,
			pBSSID,
			sizeof(DOT11_MAC_ADDRESS));
	}
	else
	{
		if(pAsocInfo->bDeauthAddrValid)
		{
			PlatformMoveMemory(
				DisAssoParam.MacAddr,
				pAsocInfo->DeauthAddr,
				sizeof(DOT11_MAC_ADDRESS));
			pAsocInfo->bDeauthAddrValid = FALSE;
		}
		else
		{
			N6_MAKE_WILDCARD_MAC_ADDRESS(DisAssoParam.MacAddr);
		}
	}

	if(reason == disas_lv_ss ||reason == inactivity)
		DisAssoParam.uReason = DOT11_ASSOC_STATUS_PEER_DISASSOCIATED | reason;
	else if (reason == deauth_lv_ss)
	{
		DisAssoParam.uReason = DOT11_ASSOC_STATUS_PEER_DEAUTHENTICATED| reason;
	}
	else if (reason == dest_unreachable){
		DisAssoParam.uReason = DOT11_ASSOC_STATUS_UNREACHABLE;//| reason;
	}
	else
		DisAssoParam.uReason = reason;

	DisAssoParam.uIHVDataOffset = 0;
	DisAssoParam.uIHVDataSize = 0;

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_DISASSOCIATION,
		&DisAssoParam,
		sizeof(DOT11_DISASSOCIATION_PARAMETERS));

}

VOID
N6IndicateRoamingStart(
	IN	PADAPTER		Adapter
	)
{
	DOT11_ROAMING_START_PARAMETERS RoamingStartParam;
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	Adapter->LastRoamingStartIndicationTime = PlatformGetCurrentTime();

	PlatformZeroMemory(&RoamingStartParam, sizeof(DOT11_ROAMING_START_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		RoamingStartParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_ROAMING_START_PARAMETERS_REVISION_1,
		sizeof(DOT11_ROAMING_START_PARAMETERS));

	if (pMgntInfo->OpMode == RT_OP_MODE_IBSS)
	{
		PlatformMoveMemory(
			RoamingStartParam.AdhocBSSID,
			pMgntInfo->Bssid,
			sizeof(DOT11_MAC_ADDRESS));

		PlatformMoveMemory(
			RoamingStartParam.AdhocSSID.ucSSID,
			pMgntInfo->Ssid.Octet,
			pMgntInfo->Ssid.Length);
		RoamingStartParam.AdhocSSID.uSSIDLength = pMgntInfo->Ssid.Length;

		RoamingStartParam.uRoamingReason = DOT11_ASSOC_STATUS_ROAMING_ADHOC;
	}
	else
	{
		RoamingStartParam.uRoamingReason = DOT11_ASSOC_STATUS_ROAMING_ASSOCIATION_LOST;
	}
		
	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_ROAMING_START,
		&RoamingStartParam,
		sizeof(DOT11_ROAMING_START_PARAMETERS));

}

VOID
N6IndicateRoamingComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	)
{
	DOT11_ROAMING_COMPLETION_PARAMETERS RoamingCompleteParam;
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	PlatformZeroMemory(&RoamingCompleteParam, sizeof(DOT11_ROAMING_COMPLETION_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		RoamingCompleteParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_ROAMING_COMPLETION_PARAMETERS_REVISION_1,
		sizeof(DOT11_ROAMING_COMPLETION_PARAMETERS));

	if (status == RT_STATUS_SUCCESS)
	{
		RoamingCompleteParam.uStatus = DOT11_ASSOC_STATUS_SUCCESS;
	}
	else
	{
		RoamingCompleteParam.uStatus = DOT11_ASSOC_STATUS_FAILURE;
	}

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_ROAMING_COMPLETION,
		&RoamingCompleteParam,
		sizeof(DOT11_ROAMING_COMPLETION_PARAMETERS));

}


//
//	Description:
//		Percedure for periodically reporting link quality.
//	2006.11.29, by shien chang.
//
VOID
LinkQualityReportCallback(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_LINK_QUALITY_PARAMETERS pLinkQualityParam;
	PDOT11_LINK_QUALITY_ENTRY		pLinkQuality;
	u4Byte							AllocSize;
	RT_STATUS						rtStatus;
	/*static*/ u1Byte				LastLinkQuality /*= 0xFF*/; // init to an impossible value.

	FunctionIn(COMP_MLME);

	// Because Ndistest will recognize link quality status indication as nic power status
	// indication in the NicPowerState item, we don't indicate link quality status without
	// associated. 2006.12.04, by shien chang.
	if ( (!pMgntInfo->mAssoc && !pMgntInfo->mIbss) ||
		(pMgntInfo->bMediaConnect == FALSE) ||
		MgntScanInProgress(pMgntInfo))
	{
		RT_TRACE_F(COMP_INDIC, DBG_TRACE, 
			("LinkQualityReportCallback(): Do not indicate because pMgntInfo->mAssoc(%d), pMgntInfo->mIbss (%d), pMgntInfo->bMediaConnect (%d), MgntScanInProgress(pMgntInfo) = %d\n",
			pMgntInfo->mAssoc, pMgntInfo->mIbss, pMgntInfo->bMediaConnect, MgntScanInProgress(pMgntInfo)));
		LastLinkQuality = 0xFF; // reset the value to an impossible value.
		return;
	}

	N6IndicateLinkSpeed(Adapter);
	
	AllocSize = sizeof(DOT11_LINK_QUALITY_PARAMETERS) + 
			sizeof(DOT11_LINK_QUALITY_ENTRY);

	rtStatus = PlatformAllocateMemory(Adapter, &pLinkQualityParam, AllocSize);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, 
			("LinkQualityReportCallback(): failed to allocate resources for link quality reporting.\n"));
		return;
	}

	{// refer to driver spec 1.2.9, 2009.01.07, haich.
		static u1Byte	LinkQualityDiffThreshold = 1;
		u1Byte		CurLinkQuality = GetSignalQuality(Adapter);
		u1Byte		LinkQualityDiff /*= (CurLinkQuality > LastLinkQuality) ? (CurLinkQuality - LastLinkQuality) : (LastLinkQuality - CurLinkQuality)*/;
		BOOLEAN 	bProceed = TRUE;

		LastLinkQuality = Adapter->RxStats.LastLinkQuality;
		LinkQualityDiff = (CurLinkQuality > LastLinkQuality) ? (CurLinkQuality - LastLinkQuality) : (LastLinkQuality - CurLinkQuality);

		// 
		// Note that we have to indicate the first notification after associated.
		//

		RT_TRACE(COMP_INDIC, DBG_LOUD, 
				("LinkQualityReportCallback(): CurLinkQuality (%u), LastLinkQuality (%u)\n",
				CurLinkQuality, LastLinkQuality));

		if(0xFF == LastLinkQuality)
		{
			bProceed = TRUE;
		}
		if(LinkQualityDiff < LinkQualityDiffThreshold)
		{// diff lower than the threshold
			bProceed = FALSE;
		}
		else if(CurLinkQuality/20 == LastLinkQuality/20) // The 5 groups: [0, 19], [20, 39], [40, 59], [60, 79], [80, 100].
		{// same group
			bProceed = FALSE;
		}

		if(pMgntInfo->RegFakeRoamSignal[0])
			bProceed = TRUE;
		
		if(!bProceed)
		{
			RT_TRACE(COMP_INDIC, DBG_LOUD, ("LinkQualityReportCallback(): not to indicate\n"));

			// Free corrsponding memory to handle exceptions.
			PlatformFreeMemory(pLinkQualityParam, AllocSize);
			return;
		}
	//	LastLinkQuality = CurLinkQuality;
		Adapter->RxStats.LastLinkQuality = CurLinkQuality;
	}

	PlatformZeroMemory(pLinkQualityParam, AllocSize);

	N6_ASSIGN_OBJECT_HEADER(
		pLinkQualityParam->Header, 
		NDIS_OBJECT_TYPE_DEFAULT, 
		DOT11_LINK_QUALITY_PARAMETERS_REVISION_1, 
		sizeof(DOT11_LINK_QUALITY_PARAMETERS));

	pLinkQualityParam->uLinkQualityListSize = 1;
	pLinkQualityParam->uLinkQualityListOffset = sizeof(DOT11_LINK_QUALITY_PARAMETERS);

	pLinkQuality = (PDOT11_LINK_QUALITY_ENTRY)( 
		((pu1Byte)pLinkQualityParam) + sizeof(DOT11_LINK_QUALITY_PARAMETERS) );
	PlatformMoveMemory(
		pLinkQuality->PeerMacAddr,
		pMgntInfo->Bssid,
		6);

	//
	// SQ indication presented on Vista and later OS:
	// 0: 0 bar in WiFi status
	// 1-19: 1 bar in WiFi status
	// 20-39: 2 bar in WiFi status
	// 40-59: 3 bar in WiFi status
	// 60-79: 4 bar in WiFi status
	// Above 80: 5 bar in WiFi status
	//
	if (pMgntInfo->mAssoc || pMgntInfo->mIbss)
	{
		pLinkQuality->ucLinkQuality = GetSignalQuality(Adapter);
	}
	else
	{
		pLinkQuality->ucLinkQuality = 0;
	}

	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateLinkStateChanged(Adapter, FALSE, 0);
	}
	else
	{
		N6IndicateStatus(
			Adapter, 
			NDIS_STATUS_DOT11_LINK_QUALITY, 
			pLinkQualityParam,
			AllocSize);
	}

	PlatformFreeMemory(pLinkQualityParam, AllocSize);
}


//
//	Description:
//		An new state machine for ndis6 mlme status indication.
//	2006.11.29, by shien chang.
//
VOID
N6InitializeIndicateStateMachine(
	IN	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PN6_INDICATE_STATE_MACHINE	pEngine = &(pNdisCommon->IndicationEngine);
	
	pEngine->CurrOpMode = SM_mNone;
	pEngine->Flag = SM_fNone;
	pEngine->CurrentState = N6_STATE_INITIAL;
	pEngine->NextState = N6_STATE_CONNECT_START;
}


VOID
N6PushIndicateStateMachineAp(
	IN	PADAPTER						Adapter,
	IN	RT_STA_INDICATE_STATE			StateToEnter,
	IN	u2Byte							StatusOrReasonCode
	)
{

	PRT_STA_INDICATE_STATE_MACHINE pIndicateEngine;

	//RT_ASSERT((Adapter->MgntInfo.pCurrentSta && (StateToEnter != N6_STATE_DISASOC)), 
	//		("N6PushIndicateStateMachineAp(): pCurrentSta == NULL and StateToEnter != N6_STATE_DISASOC.\n"));

	if(MgntActQuery_ApType(Adapter) != RT_AP_TYPE_VWIFI_AP)
	{
		return;
	}

	if(Adapter->MgntInfo.pCurrentSta)
	{
		pIndicateEngine = &(Adapter->MgntInfo.pCurrentSta->IndicationEngine);
	}
	else
	{
		RT_TRACE(COMP_INDIC, DBG_LOUD, ("N6PushIndicateStateMachineAp(): pCurrentSta is NULL.\n"));
		return;
	}
	
	if (pIndicateEngine->NextState & StateToEnter)
	{
		RT_STA_INDICATE_STATE OriginalState = pIndicateEngine->CurrentState;
		RT_TRACE(COMP_INDIC, DBG_LOUD,("Transit from 0x%X to 0x%X.\n", OriginalState, StateToEnter));
		RT_PRINT_ADDR(COMP_INDIC, DBG_LOUD, "STA:", Adapter->MgntInfo.pCurrentSta->MacAddr);

		pIndicateEngine->CurrentState = StateToEnter;
		switch (StateToEnter)
		{
		case RT_STA_STATE_INITIAL:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_INITIAL\n"));
			//pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = RT_STA_STATE_AP_INCOMING_ASOC_STARTED;
			break;
			
		case RT_STA_STATE_AP_INCOMING_ASOC_STARTED:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_AP_INCOMING_ASOC_STARTED\n"));
			
			//pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = 0;

			pIndicateEngine->NextState |= RT_STA_STATE_AP_INCOMING_ASOC_COMPLETE;
			pIndicateEngine->NextState |= RT_STA_STATE_AP_INCOMING_ASOC_REQ_RECVD;

			N62CAPIndicateIncomAssocStart(Adapter);
			break;

		case RT_STA_STATE_AP_INCOMING_ASOC_REQ_RECVD:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_AP_INCOMING_ASOC_REQ_RECVD\n"));
			pIndicateEngine->NextState = RT_STA_STATE_AP_INCOMING_ASOC_COMPLETE;
			N62CAPIndicateIncomAssocReqRecv(Adapter);
			break;

		case RT_STA_STATE_AP_INCOMING_ASOC_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_AP_INCOMING_ASOC_COMPLETE\n"));
			pIndicateEngine->NextState = 0;
			
			//if(!IS_STATUS_CODE_SUCCESS(StatusOrReasonCode))
			//{
			//	pIndicateEngine->Flag = SM_fConnected;
			//}

			pIndicateEngine->NextState |= N6_STATE_DISASOC;
			//pIndicateEngine->NextState |= N6_STATE_AP_INCOMING_ASOC_STARTED;
			
			N62CAPIndicateIncomAssocComplete(Adapter, StatusOrReasonCode);
			break;

		case RT_STA_STATE_DISASOC:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_DISASOC\n"));
			//pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = RT_STA_STATE_AP_INCOMING_ASOC_STARTED;
			N62CAPIndicateDisassociation(Adapter, StatusOrReasonCode);
			break;

		default:
			break;
		}
	}
}


VOID
N6PushIndicateStateMachineInfra(
	IN	PADAPTER	Adapter,
	IN	N6_INDICATE_STATE	StateToEnter,
	IN	u2Byte		StatusOrReasonCode
	)
{
	PN6_INDICATE_STATE_MACHINE	pIndicateEngine = 
				&(Adapter->pNdisCommon->IndicationEngine);

	if (pIndicateEngine->NextState & StateToEnter)
	{
		RT_TRACE(COMP_INDIC, DBG_LOUD, ("NextState 0x%x, StateToEnter 0x%x", pIndicateEngine->NextState, StateToEnter));	
		pIndicateEngine->CurrentState = StateToEnter;
		switch (StateToEnter)
		{
		case N6_STATE_INITIAL:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_INITIAL\n"));

			pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = N6_STATE_CONNECT_START;
			break;

		case N6_STATE_CONNECT_START:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_CONNECT_START\n"));

			pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_CONNECT_COMPLETE;
			N6IndicateConnectionStart(Adapter);
			break;

		case N6_STATE_ASOC_START:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ASOC_START\n"));
			
			pIndicateEngine->NextState = N6_STATE_ASOC_COMPLETE;
			N6IndicateAssociationStart(Adapter);
			break;

		case N6_STATE_ASOC_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ASOC_COMPLETE\n"));

			pIndicateEngine->NextState = 0;
			if ( !IS_STATUS_CODE_SUCCESS(StatusOrReasonCode) )
			{
				pIndicateEngine->NextState |= N6_STATE_ASOC_START;
			}
			
			if ( pIndicateEngine->Flag == SM_fRoaming )
			{
				pIndicateEngine->NextState |= N6_STATE_ROAM_COMPLETE;
			}
			else
			{
				pIndicateEngine->NextState |= N6_STATE_CONNECT_COMPLETE;
			}
			N6IndicateAssociationComplete(Adapter, StatusOrReasonCode);
			break;

		case N6_STATE_CONNECT_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_CONNECT_COMPLETE\n"));

			pIndicateEngine->Flag = SM_fConnected;
			pIndicateEngine->NextState = N6_STATE_DISASOC | N6_STATE_ROAM_START;
			N6IndicateConnectionComplete(Adapter, StatusOrReasonCode);
			break;

		case N6_STATE_DISASOC:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_DISASOC\n"));

			pIndicateEngine->NextState = N6_STATE_ROAM_START;
			{
				N6IndicateDisassociation(Adapter, StatusOrReasonCode);
				RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_DISASOC DO IT  !!\n"));
			}
			break;

		case N6_STATE_ROAM_START:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ROAM_START\n"));

			pIndicateEngine->Flag = SM_fRoaming;
			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_ROAM_COMPLETE;
			N6IndicateRoamingStart(Adapter);
			break;

		case N6_STATE_ROAM_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ROAM_COMPLETE\n"));

			pIndicateEngine->Flag = SM_fConnected;
			pIndicateEngine->NextState = N6_STATE_DISASOC | N6_STATE_ROAM_START;
			N6IndicateRoamingComplete(Adapter, StatusOrReasonCode);
			break;

		default:
			break;
		}
	}
}

VOID
N6PushIndicateStateMachineIBSS(
	IN	PADAPTER	Adapter,
	IN	N6_INDICATE_STATE	StateToEnter,
	IN	u2Byte		StatusOrReasonCode
	)
{
	PN6_INDICATE_STATE_MACHINE	pIndicateEngine = 
				&(Adapter->pNdisCommon->IndicationEngine);

	RT_TRACE(COMP_INDIC, DBG_LOUD,("==>N6PushIndicateStateMachineIBSS() pIndicateEngine->Flag  %d\n",pIndicateEngine->Flag));
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("NextState 0x%x, StateToEnter 0x%x", pIndicateEngine->NextState, StateToEnter));	
	
	if (pIndicateEngine->NextState & StateToEnter)
	{
		RT_TRACE(COMP_INDIC, DBG_TRACE, ("NextState 0x%x, StateToEnter 0x%x", pIndicateEngine->NextState, StateToEnter));	
	
		pIndicateEngine->CurrentState = StateToEnter;
		switch (StateToEnter)
		{
		case N6_STATE_INITIAL:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_INITIAL\n"));

			pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = N6_STATE_CONNECT_START;
			break;

		case N6_STATE_CONNECT_START:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_CONNECT_START\n"));

			pIndicateEngine->Flag = SM_fNone;
			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_CONNECT_COMPLETE;
			N6IndicateConnectionStart(Adapter);
			break;

		case N6_STATE_ASOC_START:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ASOC_START\n"));

			pIndicateEngine->NextState = N6_STATE_ASOC_COMPLETE;
			N6IndicateAssociationStart(Adapter);
			break;

		case N6_STATE_ASOC_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ASOC_COMPLETE\n"));

			if ( (pIndicateEngine->Flag == SM_fConnected) ||(pIndicateEngine->Flag == SM_fRoaming) )
			{
				RT_TRACE(COMP_INDIC, DBG_LOUD,("==>N6PushIndicateStateMachineIBSS() N6_STATE_ASOC_COMPLETE  SM_fConnected\n"));
				pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_DISASOC|N6_STATE_ROAM_COMPLETE;
				if(pIndicateEngine->Flag == SM_fConnected)	//Association Operation Guidelines for Independent BSS Networks diagram 1	
					pIndicateEngine->NextState |= N6_STATE_ROAM_START;					
			}
			else
			{
				RT_TRACE(COMP_INDIC, DBG_LOUD,("==>N6PushIndicateStateMachineIBSS() N6_STATE_ASOC_COMPLETE not SM_fConnected\n"));
				pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_CONNECT_COMPLETE;
			}
			N6IndicateAssociationComplete(Adapter, StatusOrReasonCode);
			break;

		case N6_STATE_CONNECT_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_CONNECT_COMPLETE\n"));

			pIndicateEngine->Flag = SM_fConnected;
			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_DISASOC| N6_STATE_ROAM_START;
			N6IndicateConnectionComplete(Adapter, StatusOrReasonCode);
			break;

		case N6_STATE_DISASOC:
			// David says: We should resume from disassociate state by indicate an
			// NDIS_STATUS_DOT11_ASSOCIATION_START or the starting of IBSS
			// will have problem.
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_DISASOC\n"));

			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_ROAM_START | N6_STATE_DISASOC;
			N6IndicateDisassociation(Adapter, StatusOrReasonCode);
			break;

		//
		//Porting from 8xb to avoid IBSS can not relink after s3/s4 on Vista, by Maddest
		//
		case N6_STATE_ROAM_START :
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ROAM_START\n"));

			pIndicateEngine->Flag = SM_fRoaming;
			pIndicateEngine->NextState = N6_STATE_ASOC_START | N6_STATE_ROAM_COMPLETE;
			N6IndicateRoamingStart(Adapter);
			break;
		//
		//Porting from 8xb to avoid IBSS can not relink after s3/s4 on Vista, by Maddest
		//			
		case N6_STATE_ROAM_COMPLETE:
			RT_TRACE(COMP_INDIC, DBG_LOUD,("N6_STATE_ROAM_COMPLETE\n"));

			pIndicateEngine->Flag = SM_fConnected;
			pIndicateEngine->NextState = N6_STATE_DISASOC | N6_STATE_ROAM_START|N6_STATE_ASOC_START;
			N6IndicateRoamingComplete(Adapter, StatusOrReasonCode);
			break;
			
		default:
			break;
		}

	}
	RT_TRACE(COMP_INDIC, DBG_LOUD,("<==N6PushIndicateStateMachineIBSS() \n"));
}

//
//	Description:
//		Push forward the state machine of ndis6 status indication.
//	2006.11.29, by shien chang.
//
VOID
N6PushIndicateStateMachine(
	IN	PADAPTER		Adapter,
	IN	N6_INDICATE_STATE	StateToEnter,
	IN	u2Byte			StatusOrReasonCode
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PN6_INDICATE_STATE_MACHINE	pIndicateEngine = 
				&(Adapter->pNdisCommon->IndicationEngine);
	
	PlatformAcquireSpinLock(Adapter, RT_INDIC_SPINLOCK);
	
	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
	{
		N6PushIndicateStateMachineAp(Adapter, StateToEnter, StatusOrReasonCode);
		PlatformReleaseSpinLock(Adapter, RT_INDIC_SPINLOCK);
		return;
	}

	if (pIndicateEngine->CurrentState == N6_STATE_INITIAL)
	{
		switch (pMgntInfo->OpMode)
		{
		case RT_OP_MODE_INFRASTRUCTURE:
			pIndicateEngine->CurrOpMode = SM_mInfra;
			break;

		case RT_OP_MODE_IBSS:
			pIndicateEngine->CurrOpMode = SM_mAdhoc;
			break;

		case RT_OP_MODE_AP:
			if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
			{
				//
				// 061227, rcnjko:
				// Because our UI will set a fake IBSS profile before setting 
				// AP profile CurrOpMode will be set to SM_mAdhoc for AP mode case.
				//
				RT_TRACE(COMP_MLME, DBG_WARNING, 
					("N6PushIndicateStateMachine(): N6_STATE_INITIAL with RT_OP_MODE_AP is unexpected!!!\n"));
				pIndicateEngine->CurrOpMode = SM_mAdhoc;

			}
			RT_TRACE(COMP_MLME, DBG_WARNING, 
				("N6PushIndicateStateMachine(): TODO!\n"));
			PlatformReleaseSpinLock(Adapter, RT_INDIC_SPINLOCK);
			return;
			break;

		case RT_OP_MODE_NO_LINK:
			RT_TRACE(COMP_INDIC,DBG_LOUD, ("RT_OP_MODE_NO_LINK\n"));
			PlatformReleaseSpinLock(Adapter, RT_INDIC_SPINLOCK);
			return;
			break;
		}
	}
	
	if (pIndicateEngine->CurrOpMode == SM_mInfra)
	{
		N6PushIndicateStateMachineInfra(Adapter, StateToEnter, StatusOrReasonCode);
	}
	else if (pIndicateEngine->CurrOpMode == SM_mAdhoc)
	{
		N6PushIndicateStateMachineIBSS(Adapter, StateToEnter, StatusOrReasonCode);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_INDIC_SPINLOCK);
}

//
//	Description:
//		Map status or reason code to DOT11 status code.
//	2006.12.01, by shien chang.
//
DOT11_ASSOC_STATUS
N6TranslateToDot11AssocStatus(
	IN	N6_INDICATE_STATE	CurrentState,
	IN	u2Byte			StatusOrReasonCode
	)
{
	switch (CurrentState)
	{
	case N6_STATE_ASOC_COMPLETE:
	case N6_STATE_CONNECT_COMPLETE:
	case N6_STATE_ROAM_COMPLETE:
		// Status code.
		
		break;
		
	case N6_STATE_DISASOC:
		// Reason code.
		
		break;
		
	default:
		break;
	}

	// not implement.
	return DOT11_ASSOC_STATUS_SUCCESS;
}

int
GetSignedInteger(
	u4Byte	usDW,
	u1Byte	BitNumber
	)
{
	u4Byte	u4bValue, SignBit;
	int		iValue, idx;
	u4Byte	Mask = 0x00000001;
	u4Byte	u4bTmp = 0x00000001;

	if( BitNumber >= 32 )
	{
		return	0;
	}
	
	SignBit = 0x00000001 << (BitNumber-1);
	for( idx=1; idx<(BitNumber-1); idx++ )
	{
		Mask |= ( u4bTmp << idx );
	}
	
	u4bValue = (usDW & Mask);

	if( usDW & SignBit )
	{ // negative
		u4bValue = ~(u4bValue) + 1;
		u4bValue &= Mask;
		iValue = 0 - (int)u4bValue;
	}
	else
	{ // positive
		iValue = (int)u4bValue;
	}

	return	iValue;
}

BOOLEAN
N6Dot11AddrIsBcast(
	IN	DOT11_MAC_ADDRESS	dot11Addr)
{
	pu1Byte	pAddr = (pu1Byte)dot11Addr;

	return (
		( (pAddr[0] == 0xFF) && (pAddr[1] == 0xFF) && (pAddr[2] == 0xFF) &&
		  (pAddr[3] == 0xFF) && (pAddr[4] == 0xFF) && (pAddr[5] == 0xFF) ) ? 
		TRUE : FALSE);
}


//DTM will set this wrong addr, by Maddest 070823
BOOLEAN
N6Dot11AddrIsfe(
	IN	DOT11_MAC_ADDRESS	dot11Addr)
{
	pu1Byte	pAddr = (pu1Byte)dot11Addr;

	return (
		( (pAddr[0] == 0xFe) && (pAddr[1] == 0xFF) && (pAddr[2] == 0xFF) &&
		  (pAddr[3] == 0xFF) && (pAddr[4] == 0xFF) && (pAddr[5] == 0xFF) ) ? 
		TRUE : FALSE);
}

//
//	Description:
//		Callback function of SetRFPowerStateWorkItem, 
//		which is used by OID to change RF power state.
//
VOID
SetRFPowerStateWorkItemCallback(
	IN PVOID			pContext
	)
{
	PADAPTER 			Adapter = (PADAPTER)pContext;
	PADAPTER			pAdapter = GetDefaultAdapter(Adapter);	
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(Adapter);
	PRT_NDIS6_COMMON 	pNdisCommon = pAdapter->pNdisCommon;
	BOOLEAN				bValue;
	NDIS_STATUS			ndisStatus;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	pMgntInfo->eSwRfPowerState = pNdisCommon->eRfPowerStateToSet;

	// Write registry for persistent.
	bValue = (pMgntInfo->eSwRfPowerState == eRfOn) ? TRUE : FALSE;

	//
	//It is a temp soluation, when DTM 1.0c Test with Nicpowerstate, after reinitialize the regoff value will
	//wrong, by Maddest 070823
	//
	pNdisCommon->RegRfOff=(bValue ==TRUE) ? FALSE : TRUE;
	pMgntInfo->RegRfOff = (bValue ==TRUE) ? FALSE : TRUE;

	ndisStatus = N6WriteRegRfPowerState(pAdapter, bValue);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_NIC_POWER_STATE(): failed to write registry.\n"));
	}

	if(pNdisCommon->eRfPowerStateToSet == eRfOn)
	{
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
		{
			RT_DISABLE_ASPM(pAdapter);
			RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_ASPM);
		}
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
		{
			RT_LEAVE_D3(pAdapter, FALSE);
			RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3);
		}

	}
	// Change RF power state.
	MgntActSet_RF_State(pAdapter, pNdisCommon->eRfPowerStateToSet, RF_CHANGE_BY_SW,FALSE);
	// Report the new power state to the OS.

	DrvIFIndicateCurrentPhyStatus(pAdapter);

	if(pNdisCommon->eRfPowerStateToSet == eRfOff)
	{
//		PlatformEFIOWrite1Byte(pAdapter, 0x03, 0x31);
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
		{
			RT_ENABLE_ASPM(pAdapter);
			RT_SET_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_ASPM);
		}
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
		{
			RT_ENTER_D3(pAdapter, FALSE);
			RT_SET_PS_LEVEL(pAdapter, RT_RF_OFF_LEVL_PCI_D3);
		}

		// Turn off LED if RF is not ON.
		pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_POWER_OFF); 
	}

	//
	// SW radio off ext port must report the same pwr status as default port
	//
//	N62ExtPortIndicatePwrStatus(pAdapter);

	if( !OS_SUPPORT_WDI(pAdapter) )
	{
		// Complete pending OID.
		N6CompletePendedOID(
				pAdapter,
				RT_PENDED_OID_RF_STATE,
				NDIS_STATUS_SUCCESS);
	}
}


//
//	Description:
//		Callback function of SetAdhocLinkStateWorkItem, 
//		which is used by OID to change adhoc link state.
//		2009/02/25 MH Write Adhoc Link State
//
VOID
SetAdhocLinkStateWorkItemCallback(
	IN PVOID			pContext
	)
{
	PADAPTER pAdapter = (PADAPTER)pContext;
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON pNdisCommon = pAdapter->pNdisCommon;
	BOOLEAN				bValue;
	NDIS_STATUS			ndisStatus;
	HAL_DATA_TYPE 		*pHalData = GET_HAL_DATA(pAdapter);	

	// Write registry for persistent.
	bValue = (pMgntInfo->eSwRfPowerState == eRfOn) ? TRUE : FALSE;	

	ndisStatus = N6WriteAdhocLinkState(pAdapter, pHalData->AdhocLinkState);
		
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("SetAdhocLinkStateWorkItemCallback(): failed to write registry.\n"));
	}

}


//
//	Description:
//		Initialize Rx return packet waiting mechanism.
//
//	Assumption:
//		1. AllPacketReturnedEvent is initialized.
//		2. RT_RX_SPINLOCK is NOT acquired.
//
VOID
N6CStartWaitReturnPacketMechanism(
	IN	PADAPTER		Adapter
	)
{
	//PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);

	if( !OS_SUPPORT_WDI(Adapter) )
	{
		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_ASSERT((RT_GET_RCV_REF(Adapter) == 0), 
			("N6CStartWaitReturnPacketMechanism(): RefCnt(%d) != 0 !!!\n", 
			 RT_GET_RCV_REF(Adapter)));
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
	}
	//RT_INC_RCV_REF(Adapter);

	//PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
}

//
//	Description:
//		Wait event for returned packet. 2007.01.19, by shien chang.
//
//	Assumption:
//		1. AllPacketReturnedEvent is initialized.
//		2. RT_RX_SPINLOCK is NOT acquired.
//		3. N6XXX_CANNOT_RX() is TRUE.
//
VOID
N6CWaitForReturnPacket(
	IN	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
	if (RT_GET_RCV_REF(Adapter) > 0)
	{
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6CWaitForReturnPacket(): wait for un-returned packet (%d)\n", RT_GET_RCV_REF(Adapter)));

		//PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

		while (TRUE)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("N6CWaitForReturnPacket(): wait for AllPacketReturnedEvent\n"));
			// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
			// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
			// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
			if ( NdisWaitEvent(&pNdisCommon->AllPacketReturnedEvent, 2000) )
			{
				NdisResetEvent(&pNdisCommon->AllPacketReturnedEvent);
				break;
			}
		}

		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6CWaitForReturnPacket(): end of waiting returned packet\n"));
	}
	else	{
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);		
	}
}


NDIS_STATUS
N6AllocateNative80211MIBs(
	IN	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS		rtStatus;
	ULONG			AllocSize;

	do {
		// pDot11SupportedPhyTypes.
		AllocSize = sizeof(DOT11_SUPPORTED_PHY_TYPES) + (NATIVE_802_11_MAX_NUM_PHY_TYPES-1) * sizeof(DOT11_PHY_TYPE);
		rtStatus = PlatformAllocateMemory(
			Adapter,
			&(pNdisCommon->pDot11SupportedPhyTypes),
			AllocSize
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6AllocateNative80211MIBs(): failed to allocate pDot11SupportedPhyTypes\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		PlatformZeroMemory(pNdisCommon->pDot11SupportedPhyTypes, AllocSize);

		// pDot11PhyMIBs.
		AllocSize = sizeof(NIC_PHY_MIB) * NATIVE_802_11_MAX_NUM_PHY_TYPES;
		rtStatus = PlatformAllocateMemory(
			Adapter,
			&(pNdisCommon->pDot11PhyMIBs),
			AllocSize
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6AllocateNative80211MIBs(): failed to allocate pDot11PhyMIBs\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}

	}while (FALSE);

	//
	// If allocation failed, reclaim memory resources.
	//
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		if (pNdisCommon->pDot11SupportedPhyTypes)
		{
			PlatformFreeMemory(
				pNdisCommon->pDot11SupportedPhyTypes,
				sizeof(DOT11_SUPPORTED_PHY_TYPES) + (NATIVE_802_11_MAX_NUM_PHY_TYPES-1) * sizeof(DOT11_PHY_TYPE)
				);
			pNdisCommon->pDot11SupportedPhyTypes = NULL;
		}

		if (pNdisCommon->pDot11PhyMIBs)
		{
			PlatformFreeMemory(
				pNdisCommon->pDot11PhyMIBs,
				sizeof(NIC_PHY_MIB) * NATIVE_802_11_MAX_NUM_PHY_TYPES
				);
			pNdisCommon->pDot11PhyMIBs = NULL;
		}
	}
	
	return ndisStatus;
}


VOID
N6InitializeNative80211MIBs(
	IN 	PADAPTER 	Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u1Byte	RateSetBuf[16]={0};
	u2Byte	RateSetLen=0;
	u4Byte	index=0, opRateIdx=0, BasicRateIdx=0;
	u4Byte 	curPhyId=0;
	u4Byte	WirelessModeCapa=0;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	PRT_NLO_INFO	pNLOInfo = &(pMgntInfo->NLOInfo);

	// Set operation mode.
	pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_STATION;
	pNdisCommon->dot11CurrentOperationMode.uReserved = 0;

	// dot11OperationModeCapability.
	pNdisCommon->dot11OperationModeCapability.uOpModeCapability = 
			DOT11_OPERATION_MODE_EXTENSIBLE_STATION |
			DOT11_OPERATION_MODE_NETWORK_MONITOR;

#if (NDIS_SUPPORT_NDIS620)
	if(pNdisCommon->NdisVersion > NDIS_VERSION_BASE_6_1)
	{
		Adapter->bStartVwifi = TRUE;

		if(!Adapter->pNdisCommon->bRegVWifiSupport)
		{
			Adapter->bStartVwifi = FALSE;
		}
		
		if(Adapter->bStartVwifi == TRUE)
		{
			pNdisCommon->dot11OperationModeCapability.uOpModeCapability |= 
				DOT11_OPERATION_MODE_EXTENSIBLE_AP;
		}
	}
#endif	

#if (NDIS_SUPPORT_NDIS630) 
		pNdisCommon->dot11OperationModeCapability.uOpModeCapability |= 
			DOT11_OPERATION_MODE_WFD_DEVICE| DOT11_OPERATION_MODE_WFD_GROUP_OWNER|DOT11_OPERATION_MODE_WFD_CLIENT;
#endif

#if FINE_TUNE_TH_8814A == 1
	pNdisCommon->dot11OperationModeCapability.uNumOfTXBuffers = 1024; // We can buffer as many packets as OS sends.
#else
	pNdisCommon->dot11OperationModeCapability.uNumOfTXBuffers = 0; // We can buffer as many packets as OS sends.
#endif
	pNdisCommon->dot11OperationModeCapability.uNumOfRXBuffers = Adapter->NumRxDesc[RX_MPDU_QUEUE];
	pNdisCommon->dot11OperationModeCapability.uMajorVersion = NATIVE_802_11_OPMODE_CAPABILITY_MAJOR_VERSION;
	pNdisCommon->dot11OperationModeCapability.uMinorVersion = NATIVE_802_11_OPMODE_CAPABILITY_MINOR_VERSION;
	pNdisCommon->dot11OperationModeCapability.uReserved = 0;

	//set  dot11MultiDomainCapabilityImplemented
	pNdisCommon->dot11MultiDomainCapabilityImplemented = MgntActQuery_MultiDomainImp(Adapter);

	// set support phy type
	pNdisCommon->pDot11SupportedPhyTypes->uTotalNumOfEntries = NATIVE_802_11_MAX_NUM_PHY_TYPES;
	pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries = 0; 


	WirelessModeCapa = HalGetSupportedWirelessMode(Adapter);
	
	if(WirelessModeCapa & WIRELESS_MODE_N_24G || (WirelessModeCapa & WIRELESS_MODE_AC_24G))
	{	
		//4 N mode		
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries ++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries -1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = dot11_phy_type_ht;
		// pDot11PhyMIBs.
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_ht; //6

		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 16;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 30;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 90;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[4] = 180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[5] = 240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[6] = (UCHAR)270;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[7] = (UCHAR)300;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[8] = (UCHAR)60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[9] = (UCHAR)120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[10] = (UCHAR)180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[11] = (UCHAR)240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[12] = (UCHAR)360;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[13] = (UCHAR)480;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[14] = (UCHAR)540;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[15] = (UCHAR)600;

		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);

		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);	//4 
	}

	if(WirelessModeCapa & WIRELESS_MODE_G)
	{
		//G mode		
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries ++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries -1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = dot11_phy_type_erp;
		// pDot11PhyMIBs.
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_erp; //6

		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 12;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 2;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 4;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 11;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 22;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[4] = 12;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[5] = 18;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[6] = 24;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[7] = 36;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[8] = 48;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[9] = 72;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[10] = 96;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[11] = 108;

		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = 
			MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);

	// <SC_TODO: basic rate and active rate ??>
		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		
	}
	
	if(WirelessModeCapa & WIRELESS_MODE_B) 
	{//Bmode
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries -1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = 
													dot11_phy_type_hrdsss;
		// pDot11PhyMIBs.
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_hrdsss; //5

		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 4;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 2;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 4;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 11;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 22;

		pNdisCommon->pDot11PhyMIBs[curPhyId].BasicRateSet.uRateSetLength = 4;
		pNdisCommon->pDot11PhyMIBs[curPhyId].BasicRateSet.ucRateSet[0] = 2;
		pNdisCommon->pDot11PhyMIBs[curPhyId].BasicRateSet.ucRateSet[1] = 4;
		pNdisCommon->pDot11PhyMIBs[curPhyId].BasicRateSet.ucRateSet[2] = 11;
		pNdisCommon->pDot11PhyMIBs[curPhyId].BasicRateSet.ucRateSet[3] = 22;
		
		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = 
			MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);
		
		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
	
	}
	
	RT_TRACE_F(COMP_INIT, DBG_LOUD, ("WirelessModeCapa 0x%x pNdisCommon->NdisVersion 0x%x\n", WirelessModeCapa, pNdisCommon->NdisVersion));

	if((WirelessModeCapa & WIRELESS_MODE_AC_5G) &&
		pNdisCommon->NdisVersion>=NDIS_VERSION_BASE_6_40)
	{	
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries ++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries - 1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = dot11_phy_type_vht;
	
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_vht;
	
	
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 16;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 30;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 90;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[4] = 180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[5] = 240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[6] = (UCHAR)270;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[7] = (UCHAR)300;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[8] = (UCHAR)60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[9] = (UCHAR)120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[10] = (UCHAR)180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[11] = (UCHAR)240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[12] = (UCHAR)360;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[13] = (UCHAR)480;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[14] = (UCHAR)540;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[15] = (UCHAR)600;

		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);
	
		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);	//4 
	}

	if((WirelessModeCapa & WIRELESS_MODE_N_5G) || (WirelessModeCapa & WIRELESS_MODE_AC_5G))
	{	
		//4 N 5g mode		
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries ++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries -1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = dot11_phy_type_ht;
	// pDot11PhyMIBs.
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_ht; //6


		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 16;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 30;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 90;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[4] = 180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[5] = 240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[6] = (UCHAR)270;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[7] = (UCHAR)300;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[8] = (UCHAR)60;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[9] = (UCHAR)120;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[10] = (UCHAR)180;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[11] = (UCHAR)240;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[12] = (UCHAR)360;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[13] = (UCHAR)480;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[14] = (UCHAR)540;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[15] = (UCHAR)600;

		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);

		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);	//4 
	}

	if(WirelessModeCapa& WIRELESS_MODE_A)
	{
		//A mode		
		pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries ++; 
		curPhyId = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries -1;
		pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[curPhyId] = dot11_phy_type_ofdm;
		// pDot11PhyMIBs.
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyID = curPhyId;
		pNdisCommon->pDot11PhyMIBs[curPhyId].PhyType = dot11_phy_type_ofdm; //4// 4


		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength = 8;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[0] = 12;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[1] = 18;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[2] = 24;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[3] = 36;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[4] = 48;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[5] = 72;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[6] = 96;
		pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet[7] = 108;

		pNdisCommon->pDot11PhyMIBs[curPhyId].ActiveRateSet.uRateSetLength = 0;
		pNdisCommon->pDot11PhyMIBs[curPhyId].Channel = 
			MgntActQuery_802_11_CHANNEL_NUMBER(Adapter);

		PlatformZeroMemory(
			&(pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedTxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
		PlatformMoveMemory(
			pNdisCommon->pDot11PhyMIBs[curPhyId].SupportedDataRatesValue.ucSupportedRxDataRatesValue,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.ucRateSet,
			pNdisCommon->pDot11PhyMIBs[curPhyId].OperationalRateSet.uRateSetLength);
	}
	
	
	pNdisCommon->dot11SelectedPhyId = 0;
	pNdisCommon->pDot11SelectedPhyMIB = pNdisCommon->pDot11PhyMIBs;

	// dot11SupportedPowerLevels.
	pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues[0] = 10;	//?? NEO tEST 123
	pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues[1] = 20;
	pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues[2] = 30;
	pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues[3] = 50;
	pNdisCommon->dot11SupportedPowerLevels.uNumOfSupportedPowerLevels = 4;

	// dot11CurrentTxPowerLevel.
	pNdisCommon->dot11CurrentTxPowerLevel = 1;

	//
	// Reset desired PHY ID list
	//
	pNdisCommon->staDesiredPhyCount = 1;
	pNdisCommon->staDesiredPhyList[0] = DOT11_PHY_ID_ANY;

	//
	// PCF and service class.
	//
	pNdisCommon->RegOptionalCapability.uReserved = 0;
	if(MgntActQuery_CfPollable(Adapter))
	{
		pNdisCommon->RegOptionalCapability.bDot11PCF = FALSE;
		pNdisCommon->RegOptionalCapability.bDot11PCFMPDUTransferToPC = FALSE;
	}
	else
	{
		pNdisCommon->RegOptionalCapability.bDot11PCF = FALSE;
		pNdisCommon->RegOptionalCapability.bDot11PCFMPDUTransferToPC = FALSE;
	}
	pNdisCommon->RegOptionalCapability.bStrictlyOrderedServiceClass = MgntActQuery_StrictlyOrderedImp(Adapter);

	PlatformMoveMemory(
		&(pNdisCommon->dot11CurrOptionalCapability), 
		&(pNdisCommon->RegOptionalCapability), 
		sizeof(DOT11_OPTIONAL_CAPABILITY));

	// bDot11MediaStreamingEnabled.
	pNdisCommon->bDot11MediaStreamingEnabled = FALSE;

	// dot11DesiredSSIDList.
	N6_ASSIGN_OBJECT_HEADER(
		pNdisCommon->dot11DesiredSSIDList.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_SSID_LIST_REVISION_1,
		sizeof(DOT11_SSID_LIST));
	pNdisCommon->dot11DesiredSSIDList.uNumOfEntries = 0;
	pNdisCommon->dot11DesiredSSIDList.uTotalNumOfEntries = 0;
	pNdisCommon->dot11DesiredSSIDListIndex = 0;

	// dot11DesiredSSIDList.
	N6_ASSIGN_OBJECT_HEADER(
		pNdisCommon->dot11DesiredSSIDListCopy.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_SSID_LIST_REVISION_1,
		sizeof(DOT11_SSID_LIST));
	pNdisCommon->dot11DesiredSSIDListCopy.uNumOfEntries = 0;
	pNdisCommon->dot11DesiredSSIDListCopy.uTotalNumOfEntries = 0;	

	//Ndis6 IBSS Parameters. For DTM Vista 1.0c, by Bruce, 2007-08-13.
	pNdisCommon->dot11IbssParams.bDot11IbssJoinOnly = FALSE;
	if(pNdisCommon->dot11IbssParams.AdditionalIEData)
	{
		PlatformFreeMemory(pNdisCommon->dot11IbssParams.AdditionalIEData, pNdisCommon->dot11IbssParams.AdditionalIESize);
	}
	pNdisCommon->dot11IbssParams.AdditionalIEData = NULL;
	pNdisCommon->dot11IbssParams.AdditionalIESize = 0;

	
	// dot11AutoConfigEnabled.
	pNdisCommon->dot11AutoConfigEnabled = 
		DOT11_PHY_AUTO_CONFIG_ENABLED_FLAG |
		DOT11_MAC_AUTO_CONFIG_ENABLED_FLAG;

	// dot11DesiredBSSIDList and NumDot11DesiredBSSIDList.
	N6_MAKE_WILDCARD_MAC_ADDRESS(pNdisCommon->dot11DesiredBSSIDList);
	pNdisCommon->NumDot11DesiredBSSIDList = 1;

	// dot11UnreachableDetectionThreshold.
	// <SC_TODO: revisit it>
	pNdisCommon->dot11UnreachableDetectionThreshold = 10;

	// dot11UnicastUseGroupEnabled.
	pNdisCommon->dot11UnicastUseGroupEnabled = TRUE;

	// dot11HiddenNetworkEnabled.
	pNdisCommon->dot11HiddenNetworkEnabled = FALSE;

	// dot11NetworkListOffload
	pNLOInfo->NumDot11OffloadNetwork = 0;
	pNdisCommon->Oidcounter = 0;

	// New indication engine. 2006.11.29, by shien chang.
	N6InitializeIndicateStateMachine(Adapter);

	// bWakeupAutoLinkInProgressing
	pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;

	// PrivacyExemptionList
	Adapter->pNdisCommon->PrivacyExemptionEntrieNum = 0;	
}

//
//	Description:
//		Open the file specifed and copy the file into an array, 
//
RT_STATUS
PlatformReadAndMapFile(
	IN		PVOID		Adapter,
	IN		ps1Byte		szFileName,
	IN OUT	pu1Byte		pOutFile,
	IN OUT	pu4Byte		UNALIGNED pFileSize
	)
{
	RT_STATUS				rtStatus = RT_STATUS_FAILURE;
	NDIS_STRING				NdisStrFileName;
	NDIS_PHYSICAL_ADDRESS	ndisPhyAddr;
	NDIS_STATUS				ndisStatus;
	NDIS_HANDLE				hFileHandle;
	UINT					ulFileLength;
	pu1Byte					pMappedFile = NULL;

	// Check input parameters.
	if(szFileName == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformReadAndMapFile(): szFileName should not be NULL!\n"));
		return rtStatus;
	}


	// Convert szFileName to NDIS_STRING.
	NdisInitializeString(&NdisStrFileName, (PUCHAR)szFileName);
	if(NdisStrFileName.Buffer != NULL && NdisStrFileName.Length > 0)
	{
		// Open the file specified.
		ndisPhyAddr.LowPart = ndisPhyAddr.HighPart = -1;
		NdisOpenFile(&ndisStatus,
					&hFileHandle,
					&ulFileLength,
					&NdisStrFileName,
					ndisPhyAddr);
		if(ndisStatus == NDIS_STATUS_SUCCESS)
		{
			// Map the file into memory.
			NdisMapFile(&ndisStatus, (PVOID *)(&pMappedFile), hFileHandle);
			if(ndisStatus == NDIS_STATUS_SUCCESS)
			{
				// Copy File into array
				PlatformMoveMemory(pOutFile, pMappedFile, ulFileLength);
				*pFileSize = ulFileLength;
	
				// Return Success only when Config Success
				rtStatus = RT_STATUS_SUCCESS; 
	
				// Relase the memory for mapping the file.
				NdisUnmapFile(hFileHandle);
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadAndMapFile(): failed to map the file, %s!, ndisStatus: %#X\n", szFileName, ndisStatus));
			}
	
			// Close the file.
			NdisCloseFile(hFileHandle);
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadAndMapFile(): failed to open the file, %s!, ndisStatus: %#X\n", szFileName, ndisStatus));
		}
	
		// Release the NDIS_STRING allocated via NdisInitializeString().
		NdisFreeString(NdisStrFileName);
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("PlatformReadAndMapFile(): NdisInitializeString() failed! szFileName: %s\n", szFileName));
	}

	return rtStatus;
}


//
//	Description:
//		Add NBL list into NBL wait queue.
//		
//	Assumption:
//		RT_BUFFER_SPINLOCK is acquired.
//
VOID
N6CAddNblWaitQueue(
	IN PRT_SINGLE_LIST_HEAD		pNBLWaitQueue,
	IN PNET_BUFFER_LIST		pNetBufferLists,
	IN BOOLEAN				bToHead
	)
{
	PRT_SINGLE_LIST_HEAD pList = pNBLWaitQueue;
	PRT_SINGLE_LIST_ENTRY pFirstEntry = (PRT_SINGLE_LIST_ENTRY)RT_GET_NBL_LINK(pNetBufferLists);
	PRT_SINGLE_LIST_ENTRY pLastEntry;

	RT_ASSERT(pFirstEntry != NULL, 
		("N6CAddNblWaitQueue(): pFirstEntry should not be NULL, pNetBufferLists(%p)\n !!!", pNetBufferLists));

	for(pLastEntry = pFirstEntry; pLastEntry->Next != NULL; pLastEntry = pLastEntry->Next)
		;

	if(bToHead)
	{
		if(!RTIsSListEmpty(pList))
		{
			pLastEntry->Next = pList->First;
			pList->First = pFirstEntry;
		}
		else
		{
			pLastEntry->Next = pList->First;
			pList->First = pFirstEntry;
			pList->Last = pLastEntry;
		}
	}
	else
	{
		if(!RTIsSListEmpty(pList))
		{
			pList->Last->Next = pFirstEntry; 
			pList->Last = pLastEntry;
		}
		else
		{
			pList->Last = pLastEntry;
			pList->First = pFirstEntry;
		}
	}
}

VOID
N6CConcatenateTwoList(
	IN PRT_SINGLE_LIST_ENTRY		pDest,
	IN PRT_SINGLE_LIST_ENTRY		pSource
	)
{
	PRT_SINGLE_LIST_ENTRY pDstLastEntry = NULL;
	PRT_SINGLE_LIST_ENTRY pSrcLastEntry = NULL;

	if( pDest == NULL )
		return;

	if( pSource == NULL )
		return;
	
	for(pDstLastEntry = pDest; pDstLastEntry->Next != NULL; pDstLastEntry = pDstLastEntry->Next)
		;
	
	pDstLastEntry->Next = pSource; 
}

//
//	Description:
//		Remove one NBL from head of list.
//		
//	Assumption:
//		RT_BUFFER_SPINLOCK is acquired.
//
PNET_BUFFER_LIST
N6CRemoveNblWaitQueue(
	IN PRT_SINGLE_LIST_HEAD		pNBLWaitQueue,
	IN BOOLEAN					bFromHead	
	)
{
	PRT_SINGLE_LIST_HEAD pList = pNBLWaitQueue;
	PRT_SINGLE_LIST_ENTRY pFirstEntry = NULL,	pLastEntry = NULL, pTempEntry = NULL;

	if( RTIsSListEmpty(pList) )
	{
		RT_ASSERT(FALSE, ("N6CRemvoeNblWaitQueue(): pList(%p) is empty!!!\n", pList));
		return NULL;
	}

	if(bFromHead)
	{
		PRT_SINGLE_LIST_ENTRY pEntry;

		pEntry = RTRemoveHeadSList(pList);
		return RT_GET_NBL_FROM_QUEUE_LINK(pEntry);
	}
	else
	{
		pFirstEntry = pList->First;

		for(pLastEntry = pFirstEntry; pLastEntry->Next != NULL; pLastEntry = pLastEntry->Next)
		{
			pTempEntry = pLastEntry->Next;
			if( pTempEntry->Next != NULL )
			{
				continue;
			}
			else
			{
				pLastEntry->Next = NULL;
				return RT_GET_NBL_FROM_QUEUE_LINK(pTempEntry);
			}
		}

		RTInitializeSListHead(pList);
		
		return RT_GET_NBL_FROM_QUEUE_LINK(pFirstEntry);
	}
}


//
//	Description:
//		This routine is called if miniport need to complete the NetBufferLists 
//		before associated with a TCB.
//
//	Assumption:
//		1. RT_TX_SPINLOCK is NOT acquired.
//
VOID
N6CompleteNetBufferLists(
	IN	PADAPTER		Adapter,
	IN	PNET_BUFFER_LIST	pNetBufferLists,
	IN	NDIS_STATUS			ndisStatus,
	IN	BOOLEAN				bDispatchLevel
	)
{
	PNDIS_ADAPTER_TYPE		pDevice = GET_NDIS_ADAPTER(Adapter);
	PNET_BUFFER_LIST		pCurrNetBufferList, pNextNetBufferList;

	for (pCurrNetBufferList = pNetBufferLists;
		pCurrNetBufferList != NULL;
		pCurrNetBufferList = pNextNetBufferList)
	{
		pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
		NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = ndisStatus;
	}


	NdisMSendNetBufferListsComplete(
		pDevice->hNdisAdapter, 
		pNetBufferLists, 
		(bDispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));
	
}


//
// Description:
// 	Indicate the current link (Tx/Rx) speed to Ndis6.
// Note:
//	Only call this function when the link status is associated.
// By Bruce, 2007-08-30.
//
VOID
N6IndicateLinkSpeed(
	PVOID			Adapter
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	NDIS_LINK_STATE		LinkState;

	N6_ASSIGN_OBJECT_HEADER(
		LinkState.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		NDIS_LINK_STATE_REVISION_1,
		sizeof(NDIS_LINK_STATE));
	
	LinkState.MediaConnectState = MediaConnectStateConnected;

/*
MediaDuplexStateHalf 
	The miniport adapter can transmit or receive but not both simultaneously. 
MediaDuplexStateFull 
	The miniport adapter can transmit and receive simultaneously. 
MediaDuplexStateUnknown 
	The duplex state of the miniport adapter is unknown. 
*/	
	LinkState.MediaDuplexState = MediaDuplexStateHalf;

	LinkState.RcvLinkSpeed = 300000000;
	LinkState.XmitLinkSpeed = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter , pMgntInfo->bForcedShowRxRate, FALSE)/2)*1000000;

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) && LinkState.XmitLinkSpeed < INTEL_HIGH_RESOLUTION_MIN_XMIT_SPEED)
	{
		// Note:
		//	Changing the transmit speed to upscale the Intel Miracast Graphics resolution 1920x1080.
		//	This is not a good solution and we will check this with Intel.
		//	Windows would not show the rate for P2P port on Windows UI.
		//	By Bruce, 2014-09-11.
		LinkState.XmitLinkSpeed = INTEL_HIGH_RESOLUTION_MIN_XMIT_SPEED;
	}
	RT_TRACE_F(COMP_INDIC, DBG_LOUD, ("XmitSpeed = %d RcvLinkSpeed = %d for adapter port = %d\n", (u4Byte)LinkState.XmitLinkSpeed, (u4Byte)LinkState.RcvLinkSpeed, GET_PORT_NUMBER(pAdapter)));
	LinkState.PauseFunctions = NdisPauseFunctionsUnsupported;
	LinkState.AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;
	
	N6IndicateStatus(
					pAdapter,
					NDIS_STATUS_LINK_STATE,
					&LinkState,
					sizeof(NDIS_LINK_STATE));
	
}


//
// Description:
//	Decide whether the BSSID of AP or peer address is agreed by desired list.
//	2006.11.14, by shien chang.
// 	Because it is Platform based function, I move it from "IsInDesiredBSSIDList in N6C_OidSet.c" to here.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	MacAddr -
//		The mac address to be verified if it is in the desired list.
// Rewritten by Bruce, 2008-05-27.
//
BOOLEAN
PlatformIsInDesiredBSSIDList(
	IN	PADAPTER		Adapter,
	IN	pu1Byte 		MacAddr
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte				index;

	if (pNdisCommon->NumDot11DesiredBSSIDList == 0)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("IsInDesiredBSSIDList(): NumDot11DesiredBSSIDList is zero\n"));
		return FALSE;
	}

	for (index=0; index<pNdisCommon->NumDot11DesiredBSSIDList; index++)
	{
		if ( N6Dot11AddrIsBcast( pNdisCommon->dot11DesiredBSSIDList[index] ))
		{
			return TRUE;
		}

		if ( eqNByte(
			(pu1Byte)(pNdisCommon->dot11DesiredBSSIDList[index]),
			MacAddr,
			sizeof(DOT11_MAC_ADDRESS)) )
		{
			return TRUE;
		}
	}

	return FALSE;
}


BOOLEAN
PlatformGetDesiredSSIDList(
	IN	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte				index = pNdisCommon->dot11DesiredSSIDListIndex;
	PDOT11_SSID_LIST	pSsidList = &(pNdisCommon->dot11DesiredSSIDList);
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;

	if (pSsidList->uNumOfEntries == 0 ||
		(u1Byte)pSsidList->uNumOfEntries <= pNdisCommon->dot11DesiredSSIDListIndex)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("PlatformCheckDesiredSSIDList(): uNumOfEntries %d dot11DesiredSSIDListIndex %d\n", pNdisCommon->dot11DesiredSSIDList.uNumOfEntries, pNdisCommon->dot11DesiredSSIDListIndex));
		return FALSE;
	}

	if(CompareSSID(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, pSsidList->SSIDs[index].ucSSID, (u2Byte)pSsidList->SSIDs[index].uSSIDLength))
		pNdisCommon->dot11DesiredSSIDListIndex++;

	if((u1Byte)pSsidList->uNumOfEntries <= pNdisCommon->dot11DesiredSSIDListIndex)
		return FALSE;

	index = pNdisCommon->dot11DesiredSSIDListIndex;
	
	CopyMem(pMgntInfo->Ssid.Octet, pSsidList->SSIDs[index].ucSSID, pSsidList->SSIDs[index].uSSIDLength);
	pMgntInfo->Ssid.Length =(u2Byte) pSsidList->SSIDs[index].uSSIDLength;	

	RT_PRINT_STR(COMP_MLME, DBG_LOUD, "===> PlatformGetDesiredSSIDList(): ", pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	pNdisCommon->dot11DesiredSSIDListIndex++;

	return TRUE;
}


//
// Descriotion:
//	Determine the conditions under Vista if the packets should be filtered and not to be indicated to Ndis 6.
// Adapter -
//	NIC adapter context.
// pRfd -
//	RFD buffer.
// Return -
//	TRUE if this packet should not be filtered. Or return FALSE.
// From 818xB, by Bruce, 2008-06-20.
//
BOOLEAN
N6ReceiveIndicateFilter(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING		pduOS;
	u4Byte				i;
	pu1Byte 			pRaddr;
	pu1Byte				pBssid;
	BOOLEAN				bAddrMatch = FALSE;
	BOOLEAN				bIsBroadcast = FALSE;
	BOOLEAN				bIsMulticast = FALSE;

	
	FillOctetString(pduOS, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->PacketLength);
	pRaddr = Frame_pRaddr(pduOS);
	pBssid = Frame_pBssid(pduOS);
	bAddrMatch = (PlatformCompareMemory(Adapter->CurrentAddress, pRaddr, ETHERNET_ADDRESS_LENGTH)) ? FALSE : TRUE;
	bIsBroadcast = (MacAddr_isBcst(pRaddr));
	bIsMulticast = (MacAddr_isMulticast(pRaddr));

	// Always indicate all packets in bNetMonitorMode.
	if(Adapter->MgntInfo.bNetMonitorMode)
	{
		return TRUE;
	}

	if(N6MatchPrivacyExemptList(Adapter, pRfd))
	{		
		return FALSE; 
	}

	// 
	// For DTM 1.2 Packetfilter_ext 
	// to filter data packet when CURRENT_PACKET_FILTER is set
	// 
	// (1) These standard packet filters are only applicable to 802.11 data packets
	//
	// (2) If NdisPacketFilter is not set (remains zero)
	//		 do not indicate any packets
	//
	// (3) For broadcast packet only indicate when
	//		 a) NDIS_PACKET_TYPE_BROADCAST is set in current packet filter
	//
	// (4) For multicast packet only indicate when
	//		 a) NDIS_PACKET_TYPE_ALL_MULTICAST is set in current packet filter or
	//		 b) NDIS_PACKET_TYPE_MULTICASTis set in current packet filter, and
	//			 DA belongs to one of the multicast address (set from OID_DOT11_MULTICAST_LIST)
	// 
	// Add by hpfan 2008.05.12
	//
	// For 802.11 specified filters respectively, by Bruce, 2008-06-26.
	//
	if(IsMgntFrame(pduOS.Octet))
	{
		if(bAddrMatch && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_802_11_DIRECTED_MGMT))
			return TRUE;
		if(bIsBroadcast && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_802_11_BROADCAST_MGMT))
			return TRUE;
		if(bIsMulticast && (pNdisCommon->NdisPacketFilter & (NDIS_PACKET_TYPE_802_11_MULTICAST_MGMT | 
								NDIS_PACKET_TYPE_802_11_ALL_MULTICAST_MGMT)))
			return TRUE;

		return FALSE;
	}
	else if(IsCtrlFrame(pduOS.Octet))
	{
		//
		// We do not implement yet, by Bruce, 2008-06-26.
		//
		return FALSE;
	}
	else if(IsDataFrame(pduOS.Octet))
	{
		//
		// Note:
		//	It is only valid for the miniport driver to enable the NDIS_PACKET_TYPE_PROMISCUOUS,
		//	NDIS_PACKET_TYPE_802_11_PROMISCUOUS_MGMT, or 
		//	NDIS_PACKET_TYPE_802_11_PROMISCUOUS_CTRL packet filters if the driver is 
		//	operating in Network Monitor (NetMon) mode.
		//
		if(bAddrMatch && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_DIRECTED))
			return TRUE;
		if(bIsBroadcast && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_BROADCAST))
            return TRUE;
		if(bIsMulticast && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST))
            return TRUE;

		// Do not filter data frame in IBSS mode.
		if(pMgntInfo->mIbss && !(PlatformCompareMemory(pMgntInfo->Bssid, pBssid, 6)))
		{
			return TRUE;	
		}
		if(bIsMulticast && (pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_MULTICAST))
		{
			if(Adapter->MCAddrCount == 0)
			{
				RT_PRINT_ADDR(COMP_RECV | COMP_MLME, DBG_LOUD, "N6ReceiveIndicateFilter(): filter multicast (MCAddrCount = 0) addr ", pRaddr);
				return FALSE;
			}
			for(i = 0; i < Adapter->MCAddrCount; i ++ )
			{
				if(i < MAX_MCAST_LIST_NUM)
				{
					if(!PlatformCompareMemory(Adapter->MCList[i], pRaddr, ETHERNET_ADDRESS_LENGTH) )
					{
						return TRUE;
					}
				}
			}
		}
		return FALSE;
	}
	RT_PRINT_ADDR(COMP_RECV | COMP_MLME, DBG_LOUD, "N6ReceiveIndicateFilter(): Filter this multicast not in MCL: ", pRaddr);
	return FALSE;
}

//
// Descriotion:
//	Try to Fulshing D0 Coalesing Queue !!
// Adapter -
//	NIC adapter context.
// bAcqRxLock -
//	Need to Acquire Rx SpinLock or NO
// bIndic - 
//	Drop or Indic 
//

VOID
N6FulshD0CoalescingQueue(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bAcqRxLock,
	IN	BOOLEAN					bIndic
	)
{
	PRT_NDIS6_COMMON		pNdisCommon = Adapter->pNdisCommon;
	PRT_RFD					*RfdArray;
	u2Byte					RfdCnt = 0;
	PRT_GEN_TEMP_BUFFER 	pGenBuf;
	PRT_RFD					TempRfd;

	RT_TRACE(COMP_TEST, DBG_LOUD, ("===> N6FulshD0CoalescingQueue() \n"));

	if(bAcqRxLock)
			PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);

	if( 	pNdisCommon->D0FilterState == RT_D0_FILTER_NONE ||  
		pNdisCommon->D0FilterState == RT_D0_FILTER_FLUSHING)
	{
		if(bAcqRxLock)
			PlatformReleaseSpinLock( Adapter, RT_RX_SPINLOCK);
		return;
	}

	if( RTIsListEmpty(&(pNdisCommon->D0FilterPktQueue)) )
	{
		RT_TRACE(COMP_TEST, DBG_LOUD, ("<=== N6FulshD0CoalescingQueue() : Flush NULL\n"));
		if(bAcqRxLock)
			PlatformReleaseSpinLock( Adapter, RT_RX_SPINLOCK);
		return;
	}

	if( bIndic )
	{
		RT_TRACE(COMP_TEST, DBG_LOUD, ("===> N6FulshD0CoalescingQueue() IndicatePackets !!\n"));
		if( !RTIsListEmpty(&(pNdisCommon->D0FilterPktQueue)) )
		{
			pNdisCommon->D0FilterState = RT_D0_FILTER_FLUSHING;
			RfdCnt = 0;
			pGenBuf = GetGenTempBuffer (Adapter, sizeof(PRT_RFD)*REORDER_WIN_SIZE);
			RfdArray = (PRT_RFD *)pGenBuf->Buffer.Ptr;
			
			while(!RTIsListEmpty(&pNdisCommon->D0FilterPktQueue))
			{
				TempRfd = (PRT_RFD)RTRemoveHeadList(&pNdisCommon->D0FilterPktQueue);
				RfdArray[RfdCnt] = TempRfd;
				RfdCnt = RfdCnt + 1;
			}
			
			DrvIFD0RxIndicatePackets(Adapter , RfdArray , RfdCnt);		
			ReturnGenTempBuffer(Adapter, pGenBuf);
		}
	
	}
	else
	{
		RT_TRACE(COMP_TEST, DBG_LOUD, ("===> N6FulshD0CoalescingQueue() ReturnRFDList !!\n"));
		if( !RTIsListEmpty(&(pNdisCommon->D0FilterPktQueue)) )
		{
			pNdisCommon->D0FilterState = RT_D0_FILTER_FLUSHING;

			while(!RTIsListEmpty(&pNdisCommon->D0FilterPktQueue))
			{
				TempRfd = (PRT_RFD)RTRemoveHeadList(&pNdisCommon->D0FilterPktQueue);
				ReturnRFDList(Adapter , TempRfd);
			}
		}
		
	}
	RT_TRACE(COMP_TEST, DBG_LOUD, ("<=== N6FulshD0CoalescingQueue() : Flush OK\n"));
	pNdisCommon->D0FilterState = RT_D0_FILTER_INIT;
	if(bAcqRxLock)
		PlatformReleaseSpinLock( Adapter, RT_RX_SPINLOCK);
	
}

//
// Descriotion:
//	Determine the packet to queue or let it go !!
// Adapter -
//	NIC adapter context.
// pRfd -
//	RFD buffer.
// Return 
//	True : be queue 
//	False: Not match !! 

BOOLEAN
N6ReceiveCoalescingFilter(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD					pRfd
	)
{
	PRT_NDIS6_COMMON			pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN						bBufferPkt = FALSE;
	PMGNT_INFO 					pMgntInfo = &Adapter->MgntInfo;
	u1Byte						ColseParIndex = 0;
	u1Byte						ColseFieldIndex = 0;
	BOOLEAN						bFiledMatch = TRUE;
	PRT_DO_COALESICING_FILTER_PARAMETER		pCurrFilterPar =NULL;
	PRT_DO_COALESICING_FIELD_INFO			pCurrFieldInfo = NULL;
	PRX_FILTER_INFO					pCurrRfdFilteInfo =NULL;
	PRT_DO_COALESICING_FILTER_PARAMETER		pMatchFilterPar =NULL;
	u4Byte						msDelay = 500;

//	OCTET_STRING								frame;
//	FillOctetString(frame, pRfd->Buffer.VirtualAddress+PLATFORM_GET_FRAGOFFSET(pRfd), pRfd->PacketLength);

	

	if(pRfd->nTotalSubframe > 0 )
		return bBufferPkt;

	if( pNdisCommon->D0FilterState == RT_D0_FILTER_NONE )
		return bBufferPkt;

	if( pNdisCommon->D0FilterState == RT_D0_FILTER_FLUSHING )  
		return bBufferPkt;

	if( pRfd->D0FilterCoalPktInfo.PacketType == RXPacketTypeUndefined)
		return bBufferPkt;

	if(!pMgntInfo->mRtD0ColesFilterInfo.bEnable)
		return bBufferPkt;

	pCurrRfdFilteInfo = &(pRfd->D0FilterCoalPktInfo);
	

	for( ColseParIndex = 0 ;  ColseParIndex < 10 && bBufferPkt == FALSE; ColseParIndex++)
	{
		bBufferPkt = FALSE;
		pCurrFilterPar = &(pMgntInfo->mRtD0ColesFilterInfo.dFilterArry[ColseParIndex]);

		if(pCurrFilterPar->FilterID == 0)
			continue;
		
		bFiledMatch =TRUE;

		for(ColseFieldIndex = 0 ; ColseFieldIndex < pCurrFilterPar->NumOfElem && bFiledMatch ==TRUE;  ColseFieldIndex++)
		{
			pCurrFieldInfo = &(pCurrFilterPar->dFieldArry[ColseFieldIndex]);
			
			switch(pCurrFieldInfo->FrameHead )
			{
				case RTFrameHeaderMac:
					if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestEqual)
					{
						if(pCurrFieldInfo->HeaderField.MacHeaderField == RTMacHeaderFieldPacketType )
						{
							if( pCurrFieldInfo->FieldValue.FieldByteValue  != pCurrRfdFilteInfo->PacketType )
							{
								bFiledMatch = FALSE;
							}
						}
						else if(pCurrFieldInfo->HeaderField.MacHeaderField == RTMacHeaderFieldProtocol)
						{
							if( PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldShortValue) , pCurrRfdFilteInfo->pEtherType , 2))
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							RT_TRACE(COMP_POWER, DBG_LOUD, ("===> RTFrameHeaderMac (%d)\n",pCurrFieldInfo->HeaderField.MacHeaderField));
							bFiledMatch = FALSE;
						}
					}
					else if( pCurrFieldInfo->FilterTest == RTReceiveFilterTestNotEqual )
					{
						bFiledMatch = FALSE;  // Now alway Equal 
					}
					else
					{
						bFiledMatch = FALSE;
					}
				 	break;
    				case RTFrameHeaderArp:
					if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestEqual)
					{
						if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldOperation)
						{
							if( PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldShortValue) , pCurrRfdFilteInfo->ARPOption , 2))
							{
								bFiledMatch = FALSE;
							}
						}
						else if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldSPA)
						{
							if( PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldLong64Value) , pCurrRfdFilteInfo->ARPSPA , 6))
							{
								bFiledMatch = FALSE;
							}
						}
						else if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldTPA)
						{
							if( PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldLong64Value) , pCurrRfdFilteInfo->ARPTPA , 6))
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestNotEqual)
					{
						if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldOperation)
						{
							if( !PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldShortValue) , pCurrRfdFilteInfo->ARPOption , 2))
							{
								bFiledMatch = FALSE;
							}
						}
						else if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldSPA)
						{
							if( !PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldLong64Value) , pCurrRfdFilteInfo->ARPSPA , 6))
							{
								bFiledMatch = FALSE;
							}
						}
						else if(pCurrFieldInfo->HeaderField.ArpHeaderField ==  RTARPHeaderFieldTPA)
						{
							if( !PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldLong64Value) , pCurrRfdFilteInfo->ARPTPA , 6))
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else
					{
						bFiledMatch = FALSE;
					}
					break;
				case RTFrameHeaderIPv4:
					if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestEqual)
					{
						if(pCurrFieldInfo->HeaderField.IPv4HeaderField ==  RTIPv4HeaderFieldProtocol)
						{
							if( pCurrFieldInfo->FieldValue.FieldByteValue  != pCurrRfdFilteInfo->Protocol )
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestNotEqual)
					{
						if(pCurrFieldInfo->HeaderField.IPv4HeaderField ==  RTIPv4HeaderFieldProtocol)
						{
							if( pCurrFieldInfo->FieldValue.FieldByteValue  == pCurrRfdFilteInfo->Protocol )
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else
					{						
						bFiledMatch = FALSE;
					}
				 	break;
    				case RTFrameHeaderIPv6:
					if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestEqual)
					{
						if(pCurrFieldInfo->HeaderField.IPv6HeaderField ==  RTIPv6HeaderFieldProtocol)
						{
							if( pCurrFieldInfo->FieldValue.FieldByteValue  != pCurrRfdFilteInfo->Protocol )
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestNotEqual)
					{
						if(pCurrFieldInfo->HeaderField.IPv6HeaderField ==  RTIPv6HeaderFieldProtocol)
						{
							if( pCurrFieldInfo->FieldValue.FieldByteValue  == pCurrRfdFilteInfo->Protocol )
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else
					{
						bFiledMatch = FALSE;
					}
					 break;
				case RTFrameHeaderUdp:
					if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestEqual)
					{
						if(pCurrFieldInfo->HeaderField.UdpHeaderField ==  RTUdpHeaderFieldDestinationPort)
						{
							if( PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldShortValue) , pCurrRfdFilteInfo->pDestinationPort , 2))
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else if(pCurrFieldInfo->FilterTest == RTReceiveFilterTestNotEqual)
					{
						if(pCurrFieldInfo->HeaderField.UdpHeaderField ==  RTUdpHeaderFieldDestinationPort)
						{
							if( !PlatformCompareMemory(&(pCurrFieldInfo->FieldValue.FieldShortValue) , pCurrRfdFilteInfo->pDestinationPort , 2))
							{
								bFiledMatch = FALSE;
							}
						}
						else
						{
							bFiledMatch = FALSE;
						}
					}
					else
					{
						bFiledMatch = FALSE;
					}
				 	break;
    				default:
					bFiledMatch = FALSE;
					break;
			}
		}

		if(bFiledMatch)
		{
			bBufferPkt = TRUE;
			pMatchFilterPar = pCurrFilterPar;
		}
		
	}

	
	if(bBufferPkt )
	{
		//RT_PRINT_DATA(COMP_TEST, DBG_LOUD, "===>bTestBufferPkt(true):\n", frame.Octet, frame.Length);
		RT_TRACE(COMP_TEST, DBG_LOUD,("===>bBufferPkt(true) !!\n"));
	}
	
	// Change state !!
	if(bBufferPkt)
	{
		RTInsertTailList(&pNdisCommon->D0FilterPktQueue, &pRfd->List);
		if(pNdisCommon->D0FilterState == RT_D0_FILTER_INIT)
		{
			if(pMatchFilterPar != NULL)
			{
				msDelay = pMatchFilterPar->Delaytime;
			}
			PlatformSetTimer( Adapter, &pNdisCommon->D0RxIndicatTimer, msDelay);
			pNdisCommon->D0FilterState =  RT_D0_FILTER_BUFFING;
		}
		else
		{
			RT_TRACE(COMP_TEST, DBG_LOUD,("bBufferPkt ==> pNdisCommon->D0FilterState(%d) !!\n",pNdisCommon->D0FilterState));
		}
	}
	else
	{
		if(pNdisCommon->D0FilterState ==  RT_D0_FILTER_BUFFING)
		{
			RT_TRACE(COMP_TEST, DBG_LOUD,("N6ReceiveCoalescingFilter(): N6FulshD0CoalescingQueue !!\n"));
			PlatformCancelTimer(Adapter , &pNdisCommon->D0RxIndicatTimer);
			//PlatformStallExecution(100);
			N6FulshD0CoalescingQueue( Adapter, FALSE, TRUE);
			pNdisCommon->D0FilterState = RT_D0_FILTER_INIT;
		}
	}

	return bBufferPkt;
}

VOID
N6Fill80211PhyAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PHY_ATTRIBUTES	pPhyAttributes;
	ULONG	i, index;
	UCHAR	rate;
	
	for (i=0, pPhyAttributes = pDot11Attributes->SupportedPhyAttributes ; 
		i<pDot11Attributes->NumSupportedPhys ;
		i++,pPhyAttributes++)
	{
		N6_ASSIGN_OBJECT_HEADER(
			pPhyAttributes->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PHY_ATTRIBUTES_REVISION_1,
			sizeof(DOT11_PHY_ATTRIBUTES));

		pPhyAttributes->PhyType = pNdisCommon->pDot11PhyMIBs[i].PhyType;
		pPhyAttributes->bHardwarePhyState = TRUE;
		pPhyAttributes->bSoftwarePhyState = TRUE;
		pPhyAttributes->bCFPollable = MgntActQuery_CfPollable(Adapter);
		pPhyAttributes->uMPDUMaxLength = NATIVE_802_11_MTU_SIZE;
		pPhyAttributes->TempType = dot11_temp_type_unknown;
		pPhyAttributes->DiversitySupport = dot11_diversity_support_dynamic;

		switch (pPhyAttributes->PhyType)
		{
		case dot11_phy_type_hrdsss:
			pPhyAttributes->HRDSSSAttributes.bShortPreambleOptionImplemented = FALSE;
			pPhyAttributes->HRDSSSAttributes.bPBCCOptionImplemented = FALSE;
			pPhyAttributes->HRDSSSAttributes.bChannelAgilityPresent = FALSE;
			pPhyAttributes->HRDSSSAttributes.uHRCCAModeSupported = FALSE;
			break;

		case dot11_phy_type_ofdm:
			pPhyAttributes->OFDMAttributes.uFrequencyBandsSupported = 
					DOT11_FREQUENCY_BANDS_LOWER | 
					DOT11_FREQUENCY_BANDS_MIDDLE;
			break;

		case dot11_phy_type_erp:
			pPhyAttributes->ERPAttributes.bShortPreambleOptionImplemented = TRUE;
			pPhyAttributes->ERPAttributes.bPBCCOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bChannelAgilityPresent = FALSE;
			pPhyAttributes->ERPAttributes.uHRCCAModeSupported = FALSE;
			pPhyAttributes->ERPAttributes.bERPPBCCOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bDSSSOFDMOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bShortSlotTimeOptionImplemented = TRUE;
			break;

		default:
			break;
		}

		// Tx Power Level.
		pPhyAttributes->uNumberSupportedPowerLevels = 
			pNdisCommon->dot11SupportedPowerLevels.uNumOfSupportedPowerLevels;
		PlatformMoveMemory(
			pPhyAttributes->TxPowerLevels,
			pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues,
			sizeof(pPhyAttributes->TxPowerLevels));
	
		// Data Rate.
		PlatformMoveMemory(
			&(pPhyAttributes->SupportedDataRatesValue),
			&(pNdisCommon->pDot11PhyMIBs[i].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		index = 0;
		while ( (rate = pNdisCommon->pDot11PhyMIBs[i].SupportedDataRatesValue.ucSupportedTxDataRatesValue[index]) != 0)
		{
			pPhyAttributes->DataRateMappingEntries[index].ucDataRateIndex = rate;
			pPhyAttributes->DataRateMappingEntries[index].ucDataRateFlag = 0;
			pPhyAttributes->DataRateMappingEntries[index].usDataRateValue = rate;

			index++;
		}
		pPhyAttributes->uNumDataRateMappingEntries = index;
	}
}

VOID
N6Fill80211ExtStaAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes,
	IN	PNIC_SUPPORTED_AUTH_CIPHER_PAIRS	pSupportedAuthCipherAlgs
	)
{
	PDOT11_EXTSTA_ATTRIBUTES	pExtStaAttributes = pDot11Attributes->ExtSTAAttributes;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	pu1Byte				Buffer = NULL;

	N6_ASSIGN_OBJECT_HEADER(
		pExtStaAttributes->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_EXTSTA_ATTRIBUTES_REVISION_3,
		sizeof(DOT11_EXTSTA_ATTRIBUTES));

	// For FIPS support indication.
	pExtStaAttributes->bSafeModeImplemented = DOT11_EXTSTA_ATTRIBUTES_SAFEMODE_OID_SUPPORTED;


	pExtStaAttributes->uScanSSIDListSize = NATIVE_802_11_MAX_SCAN_SSID;
	pExtStaAttributes->uDesiredBSSIDListSize = NATIVE_802_11_MAX_DESIRED_BSSID;
	pExtStaAttributes->uDesiredSSIDListSize = NATIVE_802_11_MAX_DESIRED_SSID;
	pExtStaAttributes->uExcludedMacAddressListSize = NATIVE_802_11_MAX_EXCLUDED_MACADDR;
	pExtStaAttributes->uPrivacyExemptionListSize = NATIVE_802_11_MAX_PRIVACY_EXEMPTION;
	pExtStaAttributes->uKeyMappingTableSize = NATIVE_802_11_MAX_KEY_MAPPING_ENTRY;
	pExtStaAttributes->uDefaultKeyTableSize = NATIVE_802_11_MAX_DEFAULT_KEY_ENTRY;
	pExtStaAttributes->uWEPKeyValueMaxLength = NATIVE_802_11_MAX_WEP_KEY_LENGTH;
	pExtStaAttributes->uPMKIDCacheSize = NATIVE_802_11_MAX_PMKID_CACHE;
	pExtStaAttributes->uMaxNumPerSTADefaultKeyTables = NATIVE_802_11_MAX_PER_STA_DEFAULT_KEY;
	pExtStaAttributes->bStrictlyOrderedServiceClassImplemented = FALSE;
	pExtStaAttributes->ucSupportedQoSProtocolFlags = 0;
	//pExtStaAttributes->bSafeModeImplemented = FALSE;
	pExtStaAttributes->uNumSupportedCountryOrRegionStrings = 0;
	pExtStaAttributes->pSupportedCountryOrRegionStrings = NULL;

	RT_TRACE( COMP_TEST,DBG_LOUD, ("bAutoPowerSaveMode   0000000\n"));
	pExtStaAttributes->uMaxNetworkOffloadListSize  = NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE;
	pExtStaAttributes->bAutoPowerSaveMode = TRUE;
	pExtStaAttributes->bMFPCapable = TRUE;


	pExtStaAttributes->uInfraNumSupportedMcastMgmtAlgoPairs = 
		pSupportedAuthCipherAlgs->pInfraSupportedMcastMgmtAlgoList->uNumOfEntries;	
	pExtStaAttributes->pInfraSupportedMcastMgmtAlgoPairs = 
		pSupportedAuthCipherAlgs->pInfraSupportedMcastMgmtAlgoList->AuthCipherPairs;	

	//
	// Authentication and Encryption algorithm.
	//
	pExtStaAttributes->uInfraNumSupportedUcastAlgoPairs = 
		pSupportedAuthCipherAlgs->pInfraUcastAuthCipherList->uNumOfEntries;
	pExtStaAttributes->pInfraSupportedUcastAlgoPairs =
		pSupportedAuthCipherAlgs->pInfraUcastAuthCipherList->AuthCipherPairs;

	pExtStaAttributes->uInfraNumSupportedMcastAlgoPairs =
		pSupportedAuthCipherAlgs->pInfraMcastAuthCipherList->uNumOfEntries;
	pExtStaAttributes->pInfraSupportedMcastAlgoPairs =
		pSupportedAuthCipherAlgs->pInfraMcastAuthCipherList->AuthCipherPairs;

	pExtStaAttributes->uAdhocNumSupportedUcastAlgoPairs =
		pSupportedAuthCipherAlgs->pAdhocUcastAuthCipherList->uNumOfEntries;
	pExtStaAttributes->pAdhocSupportedUcastAlgoPairs =
		pSupportedAuthCipherAlgs->pAdhocUcastAuthCipherList->AuthCipherPairs;

	pExtStaAttributes->uAdhocNumSupportedMcastAlgoPairs =
		pSupportedAuthCipherAlgs->pAdhocMcastAuthCipherList->uNumOfEntries;
	pExtStaAttributes->pAdhocSupportedMcastAlgoPairs =
		pSupportedAuthCipherAlgs->pAdhocMcastAuthCipherList->AuthCipherPairs;
	
}



NDIS_STATUS
N6CSetGeneralAttributes(
	IN	PADAPTER		Adapter
	)
{
	NDIS_MINIPORT_ADAPTER_ATTRIBUTES	MiniportAttributes;
	PRT_NDIS6_COMMON					pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO							pMgntInfo = &(Adapter->MgntInfo);
#if (NDIS_SUPPORT_NDIS620)
	NDIS_PM_CAPABILITIES				PmCapabilities;
#endif
	NDIS_PNP_CAPABILITIES				PnpCapabilities;


	PlatformZeroMemory(&MiniportAttributes, sizeof(NDIS_MINIPORT_ADAPTER_ATTRIBUTES));

	//
	// <Roger_Notes> We should fill up capability variable with zero value first to prevent 
	// unexpected NDIS parsing and IRQL_NOT_LESS_OR_EQUAL (a) bugcheck code occurs
	// after ndisMInitializeAdapter and NdisSetTimer is fired.
	// 2010.05.24.
	//

#if (NDIS_SUPPORT_NDIS620)
	N6_ASSIGN_OBJECT_HEADER(
		MiniportAttributes.GeneralAttributes.Header,
		NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES,
		NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2,
		sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));

#elif (NDIS_SUPPORT_NDIS6)
	N6_ASSIGN_OBJECT_HEADER(
		MiniportAttributes.GeneralAttributes.Header,
		NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES,
		NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1,
		sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));
#endif

	// By tynli for wowlan attribute "PowerManagementCapabilitiesEx" which only works on revision 2.

	MiniportAttributes.GeneralAttributes.MediaType = NATIVE_802_11_MEDIA_TYPE;
	MiniportAttributes.GeneralAttributes.PhysicalMediumType = NATIVE_802_11_PHYSICAL_MEDIA_TYPE;
	MiniportAttributes.GeneralAttributes.MtuSize = NATIVE_802_11_MTU_SIZE;
	// neo test 123 // this two should be query from HW supported limit
	MiniportAttributes.GeneralAttributes.MaxXmitLinkSpeed = NATIVE_802_11_MAX_XMIT_LINK_SPEED;
	MiniportAttributes.GeneralAttributes.MaxRcvLinkSpeed = NATIVE_802_11_MAX_RCV_LINK_SPEED;
	MiniportAttributes.GeneralAttributes.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	MiniportAttributes.GeneralAttributes.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	MiniportAttributes.GeneralAttributes.MediaConnectState = MediaConnectStateDisconnected;
	MiniportAttributes.GeneralAttributes.MediaDuplexState = MediaDuplexStateHalf;
	MiniportAttributes.GeneralAttributes.LookaheadSize = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;

#if (NDIS_SUPPORT_NDIS620)
	PlatformZeroMemory(&PmCapabilities, sizeof(NDIS_PM_CAPABILITIES));

	MiniportAttributes.GeneralAttributes.PowerManagementCapabilitiesEx = &PmCapabilities;
	FillPmCapabilities(Adapter, &PmCapabilities);

	MiniportAttributes.GeneralAttributes.PowerManagementCapabilities = NULL;
#else
	PlatformZeroMemory(&PnpCapabilities, sizeof(NDIS_PNP_CAPABILITIES));
	MiniportAttributes.GeneralAttributes.PowerManagementCapabilities = &PnpCapabilities;
	FillPnpCapabilities(Adapter, &PnpCapabilities);
#endif

	MiniportAttributes.GeneralAttributes.MacOptions = 
						NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
#if COALESCE_RECEIVED_PACKET
						NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
#endif
						NDIS_MAC_OPTION_NO_LOOPBACK;

	// Qos support. 2007.01.05, by shien chang.
	if (pMgntInfo->pStaQos->QosCapability > QOS_DISABLE)
	{
		MiniportAttributes.GeneralAttributes.MacOptions |= NDIS_MAC_OPTION_8021P_PRIORITY;
	}
	
	MiniportAttributes.GeneralAttributes.SupportedPacketFilters =
						NIC_SUPPORTED_FILTERS;
	
	MiniportAttributes.GeneralAttributes.MaxMulticastListSize = NATIVE_802_11_MAX_MULTICAST_LIST;
	MiniportAttributes.GeneralAttributes.MacAddressLength = ETH_LENGTH_OF_ADDRESS;

	PlatformMoveMemory(
		&(MiniportAttributes.GeneralAttributes.PermanentMacAddress),
		Adapter->PermanentAddress,
		ETH_LENGTH_OF_ADDRESS
		);

	PlatformMoveMemory(
		&(MiniportAttributes.GeneralAttributes.CurrentMacAddress),
		Adapter->CurrentAddress,
		ETH_LENGTH_OF_ADDRESS
		);

	MiniportAttributes.GeneralAttributes.RecvScaleCapabilities = NULL;
	MiniportAttributes.GeneralAttributes.AccessType = NET_IF_ACCESS_BROADCAST;
	MiniportAttributes.GeneralAttributes.DirectionType = NET_IF_DIRECTION_SENDRECEIVE;
	MiniportAttributes.GeneralAttributes.IfType = IF_TYPE_IEEE80211;
	MiniportAttributes.GeneralAttributes.IfConnectorPresent = TRUE;
	MiniportAttributes.GeneralAttributes.DataBackFillSize = NATIVE_802_11_DATA_BACK_FILL_SIZE;

	// Support OIDs.
	MiniportAttributes.GeneralAttributes.SupportedOidList = N6GetSupportedOids();
	MiniportAttributes.GeneralAttributes.SupportedOidListLength = N6GetSupportedOidsLength();
	
	return NdisMSetMiniportAttributes(Adapter->pNdisCommon->hNdisAdapter,
								&MiniportAttributes
								);
}


NDIS_STATUS
N6Set80211Attributes(
	IN	PADAPTER		Adapter
	)
{
	NDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES		Dot11Attributes;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS		ndisStatus;
	RT_STATUS		rtStatus;
	NIC_SUPPORTED_AUTH_CIPHER_PAIRS		SupportedAuthCipherAlgs;
	u4Byte	BytesWritten, BytesNeeded;
	u1Byte	tempAuthType = 0;

	// For single .sys support: 
	//	Point to types: DOT11_VWIFI_COMBINATION_V2 and DOT11_VWIFI_COMBINATION_V3
	PVOID	pCombination = NULL;



#if NDIS_SUPPORT_NDIS620
	PDOT11_VWIFI_ATTRIBUTES 	pVWiFiAttribs = NULL;
	ULONG	vwifiAttrSize = 0;
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;
#endif

#if NDIS_SUPPORT_NDIS630
	PDOT11_WFD_ATTRIBUTES   WFDAttributes;
    	BOOLEAN bAutoPowerSaveMode;
    	ULONG uMaxNetworkOffloadListSize;
#endif

	PlatformZeroMemory(&Dot11Attributes, sizeof(NDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES));

	if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_1)
	{
		N6_ASSIGN_OBJECT_HEADER(
			Dot11Attributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_802_11_ATTRIBUTES_REVISION_1,
			NDIS_SIZEOF_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES_REVISION_1);
	}
#if NDIS_SUPPORT_NDIS620
	else if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_20)
	{
		N6_ASSIGN_OBJECT_HEADER(
			Dot11Attributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_802_11_ATTRIBUTES_REVISION_2,
			NDIS_SIZEOF_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES_REVISION_2);
	}
#endif
#if NDIS_SUPPORT_NDIS630
	else if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_40)
	{
	
		N6_ASSIGN_OBJECT_HEADER(
			Dot11Attributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_802_11_ATTRIBUTES_REVISION_3,
			NDIS_SIZEOF_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES_REVISION_3);
	}
#endif

	Dot11Attributes.OpModeCapability = pNdisCommon->dot11OperationModeCapability.uOpModeCapability;
	Dot11Attributes.NumOfTXBuffers = pNdisCommon->dot11OperationModeCapability.uNumOfTXBuffers;
	Dot11Attributes.NumOfRXBuffers = pNdisCommon->dot11OperationModeCapability.uNumOfRXBuffers;
	Dot11Attributes.MultiDomainCapabilityImplemented = pNdisCommon->dot11MultiDomainCapabilityImplemented;
	Dot11Attributes.NumSupportedPhys = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries;
	
	//
	// SupportedPhyAttributes attributes.
	//
	if (Dot11Attributes.NumSupportedPhys)
	{
		rtStatus = PlatformAllocateMemory(
			Adapter, 
			&(Dot11Attributes.SupportedPhyAttributes),
			Dot11Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES)
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, 
				("N6Set80211Attributes(): failed to allocate memory for SupportedPhyAttributes\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}

		PlatformZeroMemory(Dot11Attributes.SupportedPhyAttributes, 
				Dot11Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES));

		N6Fill80211PhyAttributes(Adapter, &Dot11Attributes);
	}

	//
	// ExtSTAAttributes attributes.
	//
	rtStatus = PlatformAllocateMemory(
		Adapter,
		&(Dot11Attributes.ExtSTAAttributes),
		sizeof(DOT11_EXTSTA_ATTRIBUTES)
		);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): failed to allocate memory for ExtSTAAttributes\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	PlatformZeroMemory(Dot11Attributes.ExtSTAAttributes,
						sizeof(DOT11_EXTSTA_ATTRIBUTES));
	
	//
	// Get the supported authentication and cipher algorithm.
	//
	PlatformZeroMemory(&SupportedAuthCipherAlgs, sizeof(NIC_SUPPORTED_AUTH_CIPHER_PAIRS));
	
	// Store current mode and switch to infrastructure mode.
	// <SC_TODO:>
	
	tempAuthType = Adapter->MgntInfo.Regdot11networktype;
	Adapter->MgntInfo.Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;
	
	// Unicast & Infrastructure.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				Adapter, 
				&(SupportedAuthCipherAlgs.pInfraUcastAuthCipherList), 
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, 
			("N6Set80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pInfraUcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): failed to query unicast auth cipher pairs under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Multicast & Infrastructure.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				Adapter,
				&(SupportedAuthCipherAlgs.pInfraMcastAuthCipherList),
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR under infra mode.\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pInfraMcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): failed to query multicast auth cipher pairs under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Switch to adhoc mode.
	// <SC_TODO:>
	Adapter->MgntInfo.Regdot11networktype = RT_JOIN_NETWORKTYPE_ADHOC;
	// Unicast & Adhoc.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				Adapter, 
				&(SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList), 
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, 
			("N6Set80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): failed to query unicast auth cipher pairs under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Multicast & Adhoc.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				Adapter,
				&(SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList),
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR under adhoc mode.\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				Adapter,
				SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6Set80211Attributes(): failed to query multicast auth cipher pairs under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	{
		static DOT11_AUTH_CIPHER_PAIR  McastMFPCipherList[] = 
		{
			{DOT11_AUTH_ALGO_RSNA_PSK,			DOT11_CIPHER_ALGO_BIP},	// 0
			{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_BIP},	// 1
		};

		u4Byte	ulNumOfPairSupported = sizeof(McastMFPCipherList)/sizeof(DOT11_AUTH_CIPHER_PAIR);
		u4Byte	TotalLength = sizeof(McastMFPCipherList) + FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		u4Byte	FillLength = 0;
		PDOT11_AUTH_CIPHER_PAIR_LIST pMFPAUTHList = NULL;
		u4Byte	i;

		rtStatus = PlatformAllocateMemory(
					Adapter,
					&(SupportedAuthCipherAlgs.pInfraSupportedMcastMgmtAlgoList),
					TotalLength);

		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS,
				("N6Set80211Attributes(): failed to query multicast MFP cipher pairs \n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}

		// Fill DOT11_AUTH_CIPHER_PAIR_LIST !!
		pMFPAUTHList = SupportedAuthCipherAlgs.pInfraSupportedMcastMgmtAlgoList;

		PlatformZeroMemory( pMFPAUTHList, sizeof(DOT11_AUTH_CIPHER_PAIR_LIST) );

		N6_ASSIGN_OBJECT_HEADER(
				pMFPAUTHList->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_AUTH_CIPHER_PAIR_LIST_REVISION_1,
				sizeof(DOT11_AUTH_CIPHER_PAIR_LIST));
		FillLength += FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs);
		
		pMFPAUTHList->uNumOfEntries = 0;
		pMFPAUTHList->uTotalNumOfEntries = ulNumOfPairSupported;
		for (i=0; i<ulNumOfPairSupported; i++)
		{
			if (FillLength + sizeof(DOT11_AUTH_CIPHER_PAIR) > TotalLength )
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, (" <- [Error]N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
				ndisStatus = NDIS_STATUS_FAILURE;
				goto Exit;
			}
		
			pMFPAUTHList->uNumOfEntries ++;
			FillLength += sizeof(DOT11_AUTH_CIPHER_PAIR);
		}

		CopyMem(pMFPAUTHList->AuthCipherPairs,
				McastMFPCipherList,
				sizeof(DOT11_AUTH_CIPHER_PAIR)*pMFPAUTHList->uNumOfEntries);

		RT_TRACE( COMP_TEST , DBG_LOUD , ("===> TotalLength = %d\n",TotalLength) );
		RT_TRACE( COMP_TEST , DBG_LOUD , ("===> FillLength = %d\n",FillLength) );
	}

	// Switch back to origin mode.
	// <SC_TODO:>
	Adapter->MgntInfo.Regdot11networktype = tempAuthType;

	N6Fill80211ExtStaAttributes(Adapter, &Dot11Attributes, &SupportedAuthCipherAlgs);

#if NDIS_SUPPORT_NDIS620
	if(pNdisCommon->NdisVersion > NDIS_VERSION_BASE_6_1)
	{
	//4	add vwifi attributes
#if NDIS_SUPPORT_NDIS630
		if(pNdisCommon->NdisVersion > NDIS_VERSION_BASE_6_20)
		        vwifiAttrSize = FIELD_OFFSET(DOT11_VWIFI_ATTRIBUTES, Combinations)  + 
		                    (NUM_SUPPORTED_VWIFI_COMBINATIONS) * DOT11_SIZEOF_VWIFI_COMBINATION_REVISION_3;

		else 			
#endif
		        vwifiAttrSize = sizeof(DOT11_VWIFI_ATTRIBUTES) + 
		                    (NUM_SUPPORTED_VWIFI_COMBINATIONS) * DOT11_SIZEOF_VWIFI_COMBINATION_REVISION_2;

		rtStatus = PlatformAllocateMemory(
			Adapter,
			&(Dot11Attributes.VWiFiAttributes),
			vwifiAttrSize
			);
		if (rtStatus != RT_STATUS_SUCCESS)
			
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS,
				("failed to allocate memory for VWiFiAttributes\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}

		PlatformZeroMemory(Dot11Attributes.VWiFiAttributes, vwifiAttrSize);
		N6_ASSIGN_OBJECT_HEADER(
	            	Dot11Attributes.VWiFiAttributes->Header, 
	           	NDIS_OBJECT_TYPE_DEFAULT,
	            	DOT11_VWIFI_ATTRIBUTES_REVISION_1,
		        sizeof(DOT11_VWIFI_ATTRIBUTES));

		Dot11Attributes.VWiFiAttributes->uTotalNumOfEntries = NUM_SUPPORTED_VWIFI_COMBINATIONS;

#if 1		        
        if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_20)
       	{
			pCombination = (PVOID) &(Dot11Attributes.VWiFiAttributes->Combinations[0]);
			
		        N6_ASSIGN_OBJECT_HEADER(
					((PDOT11_VWIFI_COMBINATION_V2) pCombination)->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_VWIFI_COMBINATION_REVISION_2,
					DOT11_SIZEOF_VWIFI_COMBINATION_REVISION_2
				);
				
			((PDOT11_VWIFI_COMBINATION_V2) pCombination)->uNumInfrastructure = 1;
			((PDOT11_VWIFI_COMBINATION_V2) pCombination)->uNumSoftAP = 1;
			if(!Adapter->bInHctTest)
				((PDOT11_VWIFI_COMBINATION_V2) pCombination)->uNumVirtualStation = 1;
		}
#endif			 
#if NDIS_SUPPORT_NDIS630
		else if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_40)
		{
			pCombination = (PVOID) &Dot11Attributes.VWiFiAttributes->Combinations[0];

			N6_ASSIGN_OBJECT_HEADER(
				((PDOT11_VWIFI_COMBINATION_V3) pCombination)->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_VWIFI_COMBINATION_REVISION_3,
				DOT11_SIZEOF_VWIFI_COMBINATION_REVISION_3
			);
			 
			((PDOT11_VWIFI_COMBINATION_V3) pCombination)->uNumInfrastructure = 1;
			((PDOT11_VWIFI_COMBINATION_V3) pCombination)->uNumSoftAP = 1;
			((PDOT11_VWIFI_COMBINATION_V3) pCombination)->uNumWFDGroup = 2;				
			((PDOT11_VWIFI_COMBINATION_V3) pCombination)->uNumVirtualStation = 0;
		}
#endif
		//4    add extap attributes	
		if(Adapter->bStartVwifi == TRUE)
		{
		        Dot11Attributes.OpModeCapability |= DOT11_OPERATION_MODE_EXTENSIBLE_AP;

			PlatformAllocateMemory(Adapter, 
				&(Dot11Attributes.ExtAPAttributes), 
				sizeof(DOT11_EXTAP_ATTRIBUTES));

			if(Dot11Attributes.ExtAPAttributes == NULL)
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("Allocate Ndis 6.2 ExtAPAttributes Fail\n"));
				ndisStatus = NDIS_STATUS_FAILURE;
				goto Exit;
			}

			PlatformZeroMemory(Dot11Attributes.ExtAPAttributes, sizeof(DOT11_EXTAP_ATTRIBUTES));

			N6_ASSIGN_OBJECT_HEADER(
		            Dot11Attributes.ExtAPAttributes->Header, 
		            NDIS_OBJECT_TYPE_DEFAULT,
		            DOT11_EXTAP_ATTRIBUTES_REVISION_1,
		            sizeof(DOT11_EXTAP_ATTRIBUTES));

		        Dot11Attributes.ExtAPAttributes->uScanSSIDListSize = AP_SCAN_SSID_LIST_MAX_SIZE;
		        Dot11Attributes.ExtAPAttributes->uPrivacyExemptionListSize = NATIVE_802_11_MAX_PRIVACY_EXEMPTION;
			 // TODO: Get the real Size Some day
		        Dot11Attributes.ExtAPAttributes->uDefaultKeyTableSize = DOT11_MAX_NUM_DEFAULT_KEY;//VNic11DefaultKeyTableSize(vnic);
		        Dot11Attributes.ExtAPAttributes->uWEPKeyValueMaxLength = ( 104 / 8);//VNic11WEP104Implemented(vnic) ? 
		                                                // 104 / 8 : (VNic11WEP40Implemented(vnic) ? 40 / 8 : 0);

		        Dot11Attributes.ExtAPAttributes->uDesiredSSIDListSize = AP_DESIRED_SSID_LIST_MAX_SIZE;
		        Dot11Attributes.ExtAPAttributes->bStrictlyOrderedServiceClassImplemented = AP_STRICTLY_ORDERED_SERVICE_CLASS_IMPLEMENTED;
		        Dot11Attributes.ExtAPAttributes->uAssociationTableSize = AP_DEFAULT_ALLOWED_ASSOCIATION_COUNT;

		        //
		        // 11d stuff.
		        //
		        Dot11Attributes.ExtAPAttributes->uNumSupportedCountryOrRegionStrings = 0;
		        Dot11Attributes.ExtAPAttributes->pSupportedCountryOrRegionStrings = NULL;
				
			// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
			// TODO: Win 7 , We should support more security in here for Win 7 V3 AP Mode
			if(Adapter->bInHctTest)
			{
				// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
				Dot11Attributes.ExtAPAttributes->uInfraNumSupportedUcastAlgoPairs = 2;
				Dot11Attributes.ExtAPAttributes->pInfraSupportedUcastAlgoPairs =
					SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->AuthCipherPairs;

				Dot11Attributes.ExtAPAttributes->uInfraNumSupportedMcastAlgoPairs = 2;
				Dot11Attributes.ExtAPAttributes->pInfraSupportedMcastAlgoPairs =
					SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->AuthCipherPairs;
			}
			else
			{
				// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
				Dot11Attributes.ExtAPAttributes->uInfraNumSupportedUcastAlgoPairs = 
					SupportedAuthCipherAlgs.pInfraUcastAuthCipherList	->uNumOfEntries;
				Dot11Attributes.ExtAPAttributes->pInfraSupportedUcastAlgoPairs =
					SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->AuthCipherPairs;

				Dot11Attributes.ExtAPAttributes->uInfraNumSupportedMcastAlgoPairs = 
					SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->uNumOfEntries;
				Dot11Attributes.ExtAPAttributes->pInfraSupportedMcastAlgoPairs =
					SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->AuthCipherPairs;
			}
			
		}		
		

	}
#endif	// NDIS_SUPPORT_NDIS620

#if NDIS_SUPPORT_NDIS630
{
	PDOT11_WFD_ATTRIBUTES	pWFDAttributes = NULL;
	DOT11_COUNTRY_OR_REGION_STRING	dcors[1] = {{0,0,0}};
		
	Dot11Attributes.OpModeCapability |= 
		DOT11_OPERATION_MODE_WFD_DEVICE | 
		DOT11_OPERATION_MODE_WFD_GROUP_OWNER | 
		DOT11_OPERATION_MODE_WFD_CLIENT;
		
	PlatformAllocateMemory(Adapter, 
		&(Dot11Attributes.WFDAttributes), 
		DOT11_SIZEOF_WFD_ATTRIBUTES_REVISION_1);

	if(Dot11Attributes.WFDAttributes == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Allocate Ndis 6.30 WFDAttributes Fail\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
		
	pWFDAttributes = Dot11Attributes.WFDAttributes;
		
	PlatformZeroMemory(Dot11Attributes.WFDAttributes, DOT11_SIZEOF_WFD_ATTRIBUTES_REVISION_1);
	{
		N6_ASSIGN_OBJECT_HEADER(
				pWFDAttributes->Header, 
				NDIS_OBJECT_TYPE_DEFAULT,
				DOT11_WFD_ATTRIBUTES_REVISION_1,
				DOT11_SIZEOF_WFD_ATTRIBUTES_REVISION_1
			);
			
		pWFDAttributes->uNumConcurrentGORole = 1;
		pWFDAttributes->uNumConcurrentClientRole = 1;
		pWFDAttributes->WPSVersionsSupported = DOT11_WPS_VERSION_2_0;
		pWFDAttributes->uMaxSecondaryDeviceTypeListSize = 8;
		pWFDAttributes->bServiceDiscoverySupported = FALSE;
		pWFDAttributes->bClientDiscoverabilitySupported = FALSE;
		pWFDAttributes->bInfrastructureManagementSupported = FALSE;

		// Win8: Let the device port use the locally-administered MAC address ------------------------------------------------
		cpMacAddr(pWFDAttributes->DeviceAddress, Adapter->PermanentAddress);
		pWFDAttributes->DeviceAddress[0] |= BIT1;
		RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, "Win8 Registered Device Address: ", pWFDAttributes->DeviceAddress);
		// ----------------------------------------------------------------------------------------------------------

		// Win8: Fill the interface addresses ------------------------------------------------------------------------------
		pWFDAttributes->uInterfaceAddressListCount = 2 ;

		PlatformAllocateMemory(
				Adapter, 
				(PVOID*) &pWFDAttributes->pInterfaceAddressList,
				sizeof(DOT11_MAC_ADDRESS) * pWFDAttributes->uInterfaceAddressListCount
			);

		if(pWFDAttributes->pInterfaceAddressList == NULL)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Allocate Ndis 6.30 pInterfaceAddressList Failure! \n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}
		
		cpMacAddr(pWFDAttributes->pInterfaceAddressList[0], Adapter->CurrentAddress);
		RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, "Win8 Registered Interface Address [0]: ", pWFDAttributes->pInterfaceAddressList[0]);

		cpMacAddr(pWFDAttributes->pInterfaceAddressList[1], Adapter->CurrentAddress);
		RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, "Win8 Registered Interface Address [1]: ", pWFDAttributes->pInterfaceAddressList[1]);
		// -----------------------------------------------------------------------------------------------------------

		pWFDAttributes->uNumSupportedCountryOrRegionStrings = 0;
		pWFDAttributes->pSupportedCountryOrRegionStrings = &dcors[0];
		pWFDAttributes->uDiscoveryFilterListSize = AP_SCAN_SSID_LIST_MAX_SIZE;
		pWFDAttributes->uGORoleClientTableSize = P2P_MAX_P2P_CLIENT;	
	}
}
#endif

	//
	// Ok, now register the attributes.
	//
	ndisStatus = NdisMSetMiniportAttributes(Adapter->pNdisCommon->hNdisAdapter,
								(PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&Dot11Attributes
								);	

	RT_TRACE(COMP_INIT, DBG_LOUD, ("NdisMSetMiniportAttributes: ndisStatu: %d\n", ndisStatus));

Exit:
	// Memory cleanup.

	if (SupportedAuthCipherAlgs.pInfraUcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
			SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (SupportedAuthCipherAlgs.pInfraMcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
			SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}	

	if (SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
			SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
			SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (SupportedAuthCipherAlgs.pInfraSupportedMcastMgmtAlgoList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pInfraSupportedMcastMgmtAlgoList,
			SupportedAuthCipherAlgs.pInfraSupportedMcastMgmtAlgoList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (Dot11Attributes.ExtSTAAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.ExtSTAAttributes,
						sizeof(DOT11_EXTSTA_ATTRIBUTES));
	}
	
	if (Dot11Attributes.VWiFiAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.VWiFiAttributes,vwifiAttrSize);
	}


	if (Dot11Attributes.ExtAPAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.ExtAPAttributes,
						sizeof(DOT11_EXTAP_ATTRIBUTES));
	}

	if (Dot11Attributes.WFDAttributes != NULL)
	{
		if(Dot11Attributes.WFDAttributes->pInterfaceAddressList!=NULL)
		{
			PlatformFreeMemory(
					Dot11Attributes.WFDAttributes->pInterfaceAddressList,
					sizeof(DOT11_MAC_ADDRESS) * Dot11Attributes.WFDAttributes->uInterfaceAddressListCount
				);
		}

		PlatformFreeMemory(Dot11Attributes.WFDAttributes,sizeof(DOT11_WFD_ATTRIBUTES));
	}

	if (Dot11Attributes.NumSupportedPhys && Dot11Attributes.SupportedPhyAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.SupportedPhyAttributes,
					Dot11Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES));
	}

	return ndisStatus;
}

u4Byte
N6CGetTCPCheckFormNetBuffLisInfo(
	IN	PADAPTER		pAdapter,
	IN  PNET_BUFFER_LIST	pNetBufferList
)
{
	return 0;
}


VOID
N6CSetTCPCheckToNetBuffLisInfo(
	IN	PADAPTER			pAdapter,
	IN	PRT_RFD				pRfd,
	IN  	PNET_BUFFER_LIST	pNetBufferList
)
{
	return;
}

VOID
N6FreeAdapter(
	IN	PADAPTER	Adapter
	)
{

	RT_TRACE(COMP_INIT, DBG_LOUD, ("===>N6FreeAdapter()\n"));
	// free CtxWorkItem and ndis62Common.
	
	MgntFreeMemory(Adapter);
	PlatformFreeMemory(Adapter->pNdisCommon, sizeof(RT_NDIS6_COMMON));
	PlatformFreeMemory(Adapter, sizeof(ADAPTER));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("<===N6FreeAdapter()\n"));
}

//
// Description:
//	Compare the packet's privacy field, the EatherType and the packet destination type with the exemption list,
//	and return TRUE if this packet should be dropped, otherwise return FALSE.
// Note:
//	This function should be called after removing 802.11 Qos and Security fileds.
//
BOOLEAN
N6MatchPrivacyExemptList(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	OCTET_STRING		pduOS;
	u2Byte				TypeLength; // EatherType
	u1Byte				TypeLengthBuf[2];
	u4Byte				i;
	USHORT				usEtherType;
	USHORT				usExemptionPacketType;
	USHORT				usExemptionActionType;

	pduOS.Octet = pRfd->Buffer.VirtualAddress + pRfd->FragOffset;
	pduOS.Length = pRfd->PacketLength;

	//
	// Get Type Length.
	//
	if(pduOS.Length < 32) 
		return FALSE;

	//
	// Get EatherType from the IEEE 802.11packet without Qos and Security(IV) fields.
	// The Ndis sets the Exemption list with EatherTypes in "Big-Endian", thus transfer
	// the EatherType filed from "Little-Endian" to "Big-Endian".
	//
	TypeLengthBuf[0] = pduOS.Octet[30];
	TypeLengthBuf[1] = pduOS.Octet[31];
	TypeLength= (((u2Byte)EF1Byte(TypeLengthBuf[0]))<<8) + EF1Byte(TypeLengthBuf[1]); // host order.

	//
	// Check against each privacy exemption entry.
	//
	for(i = 0; i < pNdisCommon->PrivacyExemptionEntrieNum; i++)
	{
		usEtherType = pNdisCommon->PrivacyExemptionEntries[i].usEtherType;
		usExemptionActionType = pNdisCommon->PrivacyExemptionEntries[i].usExemptionActionType;
		usExemptionPacketType = pNdisCommon->PrivacyExemptionEntries[i].usExemptionPacketType;
		
		if(usEtherType != N2H2BYTE(TypeLength)) // TypeLength is in network order.
			continue; // check next
		
		if(usExemptionPacketType == DOT11_EXEMPT_MULTICAST)
		{
			if(!MacAddr_isMulticast(Frame_pDaddr(pduOS)))
				continue; // check next
		}
		else if(usExemptionPacketType == DOT11_EXEMPT_UNICAST)
		{
			if(MacAddr_isMulticast(Frame_pDaddr(pduOS)))
				continue; // check next
		}
		else if(usExemptionPacketType == DOT11_EXEMPT_BOTH)
		{
			// go down
		}
		else
		{// not recognized
			continue; // check next
		}

		if(usExemptionActionType == DOT11_EXEMPT_NO_EXEMPTION)
		{
			continue; // check next
		}
		else if(usExemptionActionType == DOT11_EXEMPT_ALWAYS)
		{
			if(Frame_WEP(pduOS))
			{ // the packet is encrypted, but the list alwyas discards the packets with privacy bit set.
				return TRUE;
			}

		}
		else if(usExemptionActionType == DOT11_EXEMPT_ON_KEY_MAPPING_KEY_UNAVAILABLE)
		{
			//
			// If we have a key mapping key corresponding to the transmitter address and 
			// the packet is in plain text then drop the packet.
			// If the key is unavailable, and the packet without privacy bit set, we should indicate this packet.
			//
			if(!Frame_WEP(pduOS)) // the packe is in plain text
			{
				PRT_WLAN_STA pEntry;
				PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;

				pEntry = AsocEntry_GetEntry(pMgntInfo, Frame_pTaddr(pduOS));
				if(!pEntry)
				{
					// The key is unavailable and this packet is not encrypted.
				}
				else
				{
					static u1Byte EmptyPTK[PTK_LEN];
					PlatformZeroMemory(EmptyPTK, PTK_LEN);
			if(PlatformCompareMemory(pEntry->perSTAKeyInfo.PTK, EmptyPTK, PTK_LEN) != 0)
					{// plain text and key mapping key is unavailable, exempt this frame by returning TRUE
						return TRUE;
					}
					
				}
			}
			else
				continue; // check next
		}
		else
		{// not recognized
			continue; // check next
		}
			
	}	

	return FALSE;
	
}



VOID
InitializeAdapterTimerCallback(
	PRT_TIMER pTimer
	)
{
	PADAPTER Adapter = (PADAPTER)pTimer->Adapter;
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;

	PlatformScheduleWorkItem(&(pNdisCommon->InitializeAdapterWorkItem));

	return;
}

// Add by CCW. 20130416
VOID
PNPReConnentTimerCallback(
	PRT_TIMER pTimer
	)
{
	PADAPTER Adapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	// Refent to  http://msdn.microsoft.com/en-us/library/windows/hardware/ff559418(v=vs.85).aspx
	// For a miniport adapter in the Pausing state, the miniport driver:
	// Can provide status indications with the NdisMIndicateStatusEx function.
#if 0
	if(N6C_GET_MP_DRIVER_STATE(Adapter) != MINIPORT_RUNNING && pNdisCommon->PNPconnentCout < MAX_PNP_RECONNENT_COUNTER )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PNPReConnentTimerCallback(): Driver state(%d) != MINIPORT_RUNNING, Reset timer!\n", N6C_GET_MP_DRIVER_STATE(Adapter)));
		pNdisCommon->PNPconnentCout++;
		PlatformSetTimer( Adapter, &pNdisCommon->PNPReConnentTimer, 20);
		return;
	}

	if(pNdisCommon->PNPconnentCout == MAX_PNP_RECONNENT_COUNTER) // timeout for 500ms, the timeout value can be tuned by different platform or test cases.
	{
		DrvIFIndicateDisassociation(Adapter , unspec_reason, pMgntInfo->mAId + 1);
		return;
		// ?? try to dissaco AP !!
	}
#endif


	if(Adapter->MgntInfo.bSetPnpPwrInProgress)
	{
		// Add delay to prevent from being returned in MgntLinkRetry() by bSetPnpPwrInProgress flag.
		PlatformSetTimer( Adapter, &pNdisCommon->PNPReConnentTimer, 10);
		return;
	}

	//
	// <Roger_Notes> We indicate roaming start here while S3/S4 resume if we dissociated before sleep.
	// If we indicate roaming start before entering S4 state, 
	// which could suffers from roaming failure due to roaming timeout.
	// Added by Roger, 2007.11.07.
	//
	// Move from PNP wake flow to PNPReConnentTimerCallback(). 2013.04.
	DrvIFIndicateRoamingStart(Adapter);

	if(pNdisCommon->bDissociateBeforeSleep || !Adapter->MgntInfo.bMediaConnect)
	{
		MgntActSet_802_11_SSID(Adapter, Adapter->MgntInfo.Ssid.Octet, Adapter->MgntInfo.Ssid.Length, TRUE );
	}
	else
	{
		// If we do not disconnect to AP before PNP sleep, just call MgntLinkRetry() to roam to the original AP.
		PlatformMoveMemory(Adapter->MgntInfo.APaddrbeforeRoaming, Adapter->MgntInfo.Bssid, 6);
		Dot11d_Reset(Adapter);
		RemoveAllTS(Adapter); // Do this to reset TS data structure.
		MgntLinkRetry(Adapter, TRUE);
	}

	pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
	pNdisCommon->bDissociateBeforeSleep = FALSE;
}

VOID
D0RxIndicatTimerCallback(
	PRT_TIMER pTimer
	)
{
	PADAPTER 				Adapter = (PADAPTER)pTimer->Adapter;
	PRT_NDIS6_COMMON 		pNdisCommon = Adapter->pNdisCommon;
	PRT_RFD					*RfdArray;
	u2Byte					RfdCnt = 0;
	PRT_GEN_TEMP_BUFFER 	pGenBuf;
	PRT_RFD					TempRfd;

	RT_TRACE(COMP_TEST, DBG_LOUD, ("===> D0RxIndicatTimerCallback()\n"));

	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);

	if( pNdisCommon->D0FilterState == RT_D0_FILTER_NONE ||  
		pNdisCommon->D0FilterState == RT_D0_FILTER_FLUSHING)
	{
		PlatformReleaseSpinLock( Adapter, RT_RX_SPINLOCK);
		return;
	}
	
	if( !RTIsListEmpty(&(pNdisCommon->D0FilterPktQueue)) )
	{
		pNdisCommon->D0FilterState = RT_D0_FILTER_FLUSHING;
		RfdCnt = 0;
		pGenBuf = GetGenTempBuffer (Adapter, sizeof(PRT_RFD)*REORDER_WIN_SIZE);
		RfdArray = (PRT_RFD *)pGenBuf->Buffer.Ptr;
		
		while(!RTIsListEmpty(&pNdisCommon->D0FilterPktQueue))
		{
			TempRfd = (PRT_RFD)RTRemoveHeadList(&pNdisCommon->D0FilterPktQueue);
			RfdArray[RfdCnt] = TempRfd;
			RfdCnt = RfdCnt + 1;
		}
		
		DrvIFD0RxIndicatePackets(Adapter , RfdArray , RfdCnt);
		
		ReturnGenTempBuffer(Adapter, pGenBuf);
	}
	RT_TRACE(COMP_TEST, DBG_LOUD, ("<=== D0RxIndicatTimerCallback() : Flush OK\n"));
	pNdisCommon->D0FilterState = RT_D0_FILTER_INIT;
	PlatformReleaseSpinLock( Adapter, RT_RX_SPINLOCK);
}

//
// To faster NIC init time for OS.
//
VOID
InitializeAdapterWorkItemCallback(
	IN PVOID			pContext
	)
{
	PADAPTER pAdapter = (PADAPTER)pContext;

	InitializeAdapterCommon(pAdapter);
}


VOID 
InitializeAdapterThread(
    IN PVOID                    pContext
    )
{
	InitializeAdapterCommon(((PRT_THREAD)pContext)->Adapter);
}

//
// Gather same code for initialize adapter workitem and callback API.
//
#define	MAX_INIT_RETRY_CNT	10

BOOLEAN
MatchInitPendingOid(
	IN	PNDIS_OID_REQUEST	NdisRequest
	)
{
	u1Byte index = 0;

	for(index=0; index< (sizeof(NdisInitPendingOid)/sizeof(OID)); index++)
	{

		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("MatchInitPendingOid OID (0x%x) type (0x%x) \n", NdisInitPendingOid[index].Oid, NdisInitPendingOid[index].Type));
	
		if(NdisInitPendingOid[index].Type == NdisRequest->RequestType &&
			NdisInitPendingOid[index].Oid == NdisRequest->DATA.METHOD_INFORMATION.Oid)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN
InitializePendingOIDBeforeReady(
	IN	PADAPTER				Adapter,
	IN	PNDIS_OID_REQUEST	NdisRequest,
	OUT	PNDIS_STATUS			pRetVal
	)
{
	
	if(N6_INIT_READY(Adapter))
	{
		return FALSE;
	}
	else	
	{

		RT_TRACE( COMP_INIT, DBG_LOUD, ("InitializePendingOIDBeforeReady(): QUERY_INFORMATION OID(%#x) when initializing.\n", NdisRequest->DATA.QUERY_INFORMATION.Oid));
		RT_TRACE( COMP_INIT, DBG_LOUD, ("InitializePendingOIDBeforeReady(): METHOD_INFORMATION OID(%#x) when initializing.\n", NdisRequest->DATA.METHOD_INFORMATION.Oid));

		N6C_DOT11_DUMP_OID(NdisRequest->DATA.METHOD_INFORMATION.Oid);

		if(MatchInitPendingOid(NdisRequest))
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("InitializePendingOIDBeforeReady(): OID when initializing. return PENDING\n"));
			*pRetVal = NDIS_STATUS_PENDING;
			return TRUE;
		}

		// Reset request is skip during initialization.
		if(NdisRequest->RequestType==NdisRequestMethod|| NdisRequest->RequestType==NdisRequestSetInformation)
		{
		
			if(NdisRequest->DATA.METHOD_INFORMATION.Oid==OID_DOT11_RESET_REQUEST)
			{		
			
				//
				// <Roger_Notes> We need to check whether OutputBufferLength is valid prior to following status indication.
				// 2014.05.02.
				//
				if ( NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength >= sizeof(DOT11_STATUS_INDICATION) )
				{
				
					PDOT11_STATUS_INDICATION pDot11StatusIndication = NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

					PlatformZeroMemory(pDot11StatusIndication, sizeof(DOT11_STATUS_INDICATION));
					pDot11StatusIndication->uStatusType = DOT11_STATUS_RESET_CONFIRM;
					pDot11StatusIndication->ndisStatus = NDIS_STATUS_SUCCESS;
					NdisRequest->DATA.METHOD_INFORMATION.BytesRead = sizeof(DOT11_RESET_REQUEST);// After reset operation was completed, return the status indication. 

					RT_TRACE( COMP_INIT, DBG_LOUD, ("InitializePendingOIDBeforeReady(): OID_DOT11_RESET_REQUEST when initializing. return SUCCESS\n"));
					
					*pRetVal = NDIS_STATUS_SUCCESS;
					return TRUE;
				}				
			}
		}

		// No specific handle in initialization.
		return FALSE;
	}
}

InitializeCompleteOID(
	IN 	PADAPTER 	pAdapter
)
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	FunctionIn(COMP_MLME);

	PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	if(pNdisCommon->PendedRequest!= NULL)
	{
		if(MatchInitPendingOid(pNdisCommon->PendedRequest))
		{
			 RT_TRACE(COMP_INIT, DBG_LOUD, 
					("InitializeAdapterCommon(): (1) complete PendedRequest(%p) call N6PciOidRequest()!!!++++++++\n", pNdisCommon->PendedRequest));
			
			N6C_DOT11_DUMP_OID(pNdisCommon->PendedRequest->DATA.METHOD_INFORMATION.Oid);

			RT_TRACE(COMP_INIT, DBG_LOUD, 
					("InitializeAdapterCommon(): complete PendedRequest(%p) ++++++++\n", pNdisCommon->PendedRequest));
						
			PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);

			N6CompletePendedOID(pAdapter, RT_PENDED_OID_DONT_CARE, 
			N6SdioOidRequest((NDIS_HANDLE)pAdapter, pNdisCommon->PendedRequest));
		}
		else
		{
			PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
			RT_TRACE(COMP_INIT, DBG_LOUD, ("InitializeAdapterCommon(): No matched OID complete ++++++++\n"));
		}
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	}
}


static	VOID
InitializeAdapterCommon(
	IN 	PADAPTER 	pAdapter
	)
{
	PMGNT_INFO 	pMgntInfo = &pAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = NULL;
	RT_STATUS	RTStatus = RT_STATUS_SUCCESS;
	BOOLEAN		bInitFail = FALSE;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("-->InitializeAdapterCommon\n"));

	if(pAdapter->bDriverIsGoingToUnload)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("bDriverIsGoingToUnload before\n"));
		goto Exit;
	}

	while(!pAdapter->initfinish)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("pAdapter->initfinish %d \n", pAdapter->initfinish));
		delay_ms(1);
	}
	delay_ms(1);	// this delay is use toe avoid this wokritem interrupt into n6pciinitialize!!!!
	
	if(pAdapter->bDriverIsGoingToUnload)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("bDriverIsGoingToUnload after\n"));
		goto Exit;		
	}
	
	pAdapter->bInitializeInProgress=TRUE;

	// 2015/02/25 MH Merge from merge temp, illegal methid to support velocity initialize.
	if(pMgntInfo->RegPreInitMem)
		RTStatus = RT_STATUS_SUCCESS;
	else
		RTStatus = NicIFAllocateMemory(pAdapter);
	if(RTStatus != RT_STATUS_SUCCESS)
	{
		bInitFail = TRUE; // Fail to perform the init process.
		N6WriteEventLog(pAdapter, RT_INIT_FAIL, 0);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("NicIFAllocateMemory fail\n"));
		goto Exit;
	}

	PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
	PrepareAllRxDescBuffer(pAdapter);
	PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);

	// move allocate memory here to reduce the time at the platform HP95 Toshiba.!! neo
	// need to check return value if allocate memory fail.
	ADAPTER_SET_STATUS_FLAG(pAdapter, ADAPTER_STATUS_FIRST_INIT);

	RTStatus = pAdapter->HalFunc.InitializeAdapterHandler(pAdapter, pAdapter->MgntInfo.dot11CurrentChannelNumber);

	if(RTStatus != RT_STATUS_SUCCESS)
	{
		N6WriteEventLog(pAdapter, RT_INIT_FAIL, 0);
		NicIFResetMemory(pAdapter);
		bInitFail = TRUE;
		goto Exit;
	}
	
	PlatformSetCheckForHangTimer(pAdapter);
	
	ADAPTER_CLEAR_STATUS_FLAG(pAdapter, ADAPTER_STATUS_FIRST_INIT);

	MultiPortSetAllPortsHWReadyStatus(pAdapter, TRUE);
	
	//
	// <Roger_Notes> We shall restore default ASPM settings If Non-ATI solution.
	// 2009.03.23.
	//
	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	if(pPSC->RegRfPsLevel & RT_RF_PS_LEVEL_ALWAYS_ASPM)
	{
		// Always enable ASPM & Clock Req.
		RT_ENABLE_ASPM(pAdapter);
		RT_SET_PS_LEVEL(pAdapter, RT_RF_PS_LEVEL_ALWAYS_ASPM);
	}

	//DrvIFIndicateCurrentPhyStatus(pAdapter);

Exit:
	
	pAdapter->bInitializeInProgress=FALSE;

	// 
	// <Roger_Notes> If we had already returned control for the InitializeHandlerEx routine 
	// and we can removes the specified miniport driver adapter that the miniport driver 
	// has determined is unrecoverable from the system.
	// 2010.06.24.
	//
	if(pAdapter->initfinish && bInitFail)
	{
		N6CompletePendedOID(pAdapter, RT_PENDED_OID_DONT_CARE,  NDIS_STATUS_FAILURE);
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("InitializeAdapterCommon(): Fail to initialize NIC, remove it from the system!!\n"));
		if(!pAdapter->bDriverIsGoingToUnload)
		NdisMRemoveMiniport(pAdapter->pNdisCommon->hNdisAdapter);
	}
	else
	{

		// Clear interrupt before enable interrupt.
		NicIFClearInterrupt(pAdapter);
		NicIFEnableInterrupt(pAdapter);	
	
		// 2012/03/27 hpfan Add for win8 DTM DPC ISR test
		if(pAdapter->bUseThreadHandleInterrupt)
		{
			PlatformSetEventTrigerThread(
				pAdapter,
				&pAdapter->InterruptThread,
				PASSIVE_LEVEL,
				NULL);	
		}

		InitializeCompleteOID(pAdapter);
		
		pAdapter->bInitComplete = TRUE;		

		RT_SET_DRV_STATE(pAdapter, DrvStateHwInitialized);
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<--InitializeAdapterCommon\n"));
	
}	// InitializeAdapterCommon

PVOID 
GetTwoPortSharedResource(
	IN		PADAPTER	Adapter,
	IN		u1Byte		Type_ToGet,
	IN OUT	PVOID		pVariable_ToGet, // For more readable on caller side.
	OUT		pu1Byte		pTwoPortStatus // For convinient (pu1Byte)NULL is OK.
	)
{
	// XP/Vista not yet take into consideration, Use Platform
	PADAPTER pDefaultAdapter	= GetDefaultAdapter(Adapter);
	PADAPTER ExtAdapter	= GetFirstAPAdapter(Adapter);
	u1Byte 	 TwoPortStatus = (u1Byte)TWO_PORT_STATUS__DEFAULT_ONLY;
		
	//(1.)  ======  Decide TWO_PORT_STATUS  ======
	if(ExtAdapter == NULL || ExtAdapter == pDefaultAdapter)
	{
		if(pDefaultAdapter->MgntInfo.mAssoc==TRUE)
			TwoPortStatus = TWO_PORT_STATUS__DEFAULT_ONLY;
		else
			TwoPortStatus = TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE;
	}
	else if( //(Adapter==ExtAdapter)&&(ExtAdapter->MgntInfo.mAssoc_start==TRUE)
		(pDefaultAdapter->MgntInfo.mAssoc==TRUE)
		&&(ExtAdapter->MgntInfo.OpMode==RT_OP_MODE_AP) // a dangerous way depend on compiler
		&&(pDefaultAdapter->MgntInfo.dot11CurrentWirelessMode&WIRELESS_MODE_G)
		&&(IS_WIRELESS_MODE_N(ExtAdapter)) )
	{
		TwoPortStatus = TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20;
	}
	else if( //(Adapter==ExtAdapter)&&(ExtAdapter->MgntInfo.mAssoc_start==TRUE)
		(pDefaultAdapter->MgntInfo.mAssoc==TRUE)
		&&(ExtAdapter->MgntInfo.OpMode==RT_OP_MODE_AP) // a dangerous way depend on compiler
		&&(pDefaultAdapter->MgntInfo.dot11CurrentWirelessMode&WIRELESS_MODE_A)
		&&(IS_WIRELESS_MODE_N(ExtAdapter)))
	{		
		TwoPortStatus = TWO_PORT_STATUS__DEFAULT_A_EXTENSION_N20;
	}
	else if(
		(pDefaultAdapter->MgntInfo.mAssoc==TRUE)
		&&(ExtAdapter->MgntInfo.OpMode==RT_OP_MODE_AP) )
	{
		TwoPortStatus = TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT;
	}
	else if(
		(pDefaultAdapter->MgntInfo.mAssoc==TRUE)&&(ExtAdapter->MgntInfo.OpMode!=RT_OP_MODE_AP)
		)
	{
		TwoPortStatus = TWO_PORT_STATUS__DEFAULT_ONLY;
	}
	else if(
		(pDefaultAdapter->MgntInfo.mAssoc==FALSE)&&(ExtAdapter->MgntInfo.OpMode==RT_OP_MODE_AP)
		)
	{
		TwoPortStatus = TWO_PORT_STATUS__EXTENSION_ONLY;
	}
	else if(
		(pDefaultAdapter->MgntInfo.mAssoc==FALSE)&&(pDefaultAdapter->MgntInfo.mIbss==TRUE)
	)
	{
		TwoPortStatus = TWO_PORT_STATUS__ADHOC;
	}
	else
	{
		TwoPortStatus = TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE;
	}
	
	if(pTwoPortStatus != NULL)
		*pTwoPortStatus = (u1Byte)TwoPortStatus;
	
	//(2.)  ======  For Each Type of Share Resources Deside its Adapter Source  ======
	switch (Type_ToGet)
	{
	//===================================================================
	case TWO_PORT_SHARED_OBJECT__STATUS:
		return pTwoPortStatus;		
	break;
	//===================================================================		
	case TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos_WMMParamEle:
	{		
		switch(TwoPortStatus)
		{
		case TWO_PORT_STATUS__EXTENSION_ONLY:
			return (pu1Byte)ExtAdapter->MgntInfo.pStaQos->WMMParamEle;// how to handle the pointer?
			break;
		default:			
			return (pu1Byte)pDefaultAdapter->MgntInfo.pStaQos->WMMParamEle;// how to handle the pointer?
			break;
		}
	}
	break;
	//===================================================================
	case TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos:
	{
		switch(TwoPortStatus)
		{
		case TWO_PORT_STATUS__EXTENSION_ONLY:
			*(PQOS_MODE)pVariable_ToGet = ExtAdapter->MgntInfo.pStaQos->CurrentQosMode;
			break;
		default:			
			*(PQOS_MODE)pVariable_ToGet = pDefaultAdapter->MgntInfo.pStaQos->CurrentQosMode;
			break;
		}
		break;
	}
	//===================================================================
	// In this case, TwoPortStatus will be treated as BW to output.
	case TWO_PORT_SHARED_OBJECT__BW:
	{
		switch(TwoPortStatus)
		{
		case TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20:
		case TWO_PORT_STATUS__DEFAULT_A_EXTENSION_N20:
		case TWO_PORT_STATUS__ADHOC:	
			if (pTwoPortStatus != NULL)
			{
				*pTwoPortStatus = (u1Byte)CHANNEL_WIDTH_20;// cast issue
			}
			break;
			
		case TWO_PORT_STATUS__EXTENSION_ONLY:
			if (pTwoPortStatus != NULL)
			{
				*pTwoPortStatus = (u1Byte)ExtAdapter->MgntInfo.dot11CurrentChannelBandWidth;
			}
			break;
			
		case TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT:	
		case TWO_PORT_STATUS__DEFAULT_ONLY:
			if (pTwoPortStatus != NULL)
			{
				*pTwoPortStatus = (u1Byte)pDefaultAdapter->MgntInfo.dot11CurrentChannelBandWidth;
			}
			break;

		case TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE:
			if (pTwoPortStatus != NULL)
			{
				if (Adapter->MgntInfo.pVHTInfo->bEnableVHT)
					*pTwoPortStatus = CHNL_GetRegBWSupport(Adapter);// cast issue
				else
					*pTwoPortStatus = CHNL_GetRegBWSupport(Adapter) ? CHANNEL_WIDTH_40 : CHANNEL_WIDTH_20;// cast issue
			}
			break;
		}
	}
	break;
	//===================================================================	
	default:
		return NULL;
	break;
	//===================================================================
		
	}
	return NULL;
}

VOID
N6WriteEventLog(

	IN PADAPTER Adapter, 
	IN RT_EVENTLOG_TYPE type,
	IN	ULONG	Num
	)
{
	PWCHAR ErrorString; 	

	// 20101025 Joseph: Merge with 91SE code base. This prevent the system unknown log shown on event view.
	// Without this log, customer may not be confused.
	return;
	
	switch(type)
	{
	case RT_FW_DOWLOAD_F:
		ErrorString = L"FW_DL_F";
		break;
	case RT_INIT_HANDLER_FAIL:
		ErrorString = L"RT_INIT_HANDLER_FAIL";
		break;
	case RT_INIT_FAIL:
		ErrorString = L"RT_INIT_FAIL";
		break;
	case RT_MUTUAL_AUTHENTICATION_FAIL:
		ErrorString = L" RT_MUTUAL_AUTHENTICATION_FAIL";
		break;
	case RT_INIT_OK:
		ErrorString = L"RT_INIT_OK";
		break;
	case RT_SCAN_START:
		ErrorString = L"OID_SCAN_REQUEST";
		break;
	case RT_SCAN_COMPLETE:
		ErrorString = L"OID_SCAN_COMPLETE";
		break;
	case RT_SCAN_NUM:
		ErrorString = L"OID_SCAN_NUM";
		break;
	case RT_TKIP_MIC_ERROR:
		ErrorString = L"TKIP_MIC_ERROR";
		break;
	case RT_HW_AIRPLANE_MODE_TRIGGERED:
		ErrorString = L"RT_HW_AIRPLANE_MODE_TRIGGERED";
		break;
	default:
		ErrorString = L"NO_EVENT_INDICATED";
		break;
	}

	NdisWriteEventLogEntry(
		GlobalRtDriverContext.NdisContext.pDriverObject,
		NDIS_STATUS_SUCCESS, 
		2, 
		1,	 
		&ErrorString, 
		sizeof(Num), 
		&Num);
}

VOID
N6CWriteEventLogEntry(
	IN  ADAPTER					*pAdapter,
	IN  NDIS_STATUS				ndisStatus,
	IN  ULONG					dataLen,
	IN  u1Byte					*data,
	IN  WCHAR					*wcFormat,
	IN  ...
	)
{
#if DBG

	va_list						argPtr;
	RT_GEN_TEMP_BUFFER			*buf = NULL;

	va_start(argPtr, wcFormat);

	do
	{
		if(NULL == (buf = GetGenTempBuffer(pAdapter, GEN_TEMP_BUFFER_SIZE)))
			break;

		if(vswprintf_s(
			(wchar_t *)buf->Buffer.Ptr, // buffer
			buf->Buffer.Length / sizeof(wchar_t), // numberOfElements
			wcFormat, // format
			argPtr // argptr
			))
		{
			NdisWriteEventLogEntry(
				GlobalRtDriverContext.NdisContext.pDriverObject,
				ndisStatus, 
				2, 
				1,	 
				&buf->Buffer.Ptr, 
				dataLen, 
				data);
		}
	}while(FALSE);

	if(buf)
		ReturnGenTempBuffer(pAdapter, buf);

	va_end(argPtr);
	
#endif

	return;
}


VOID
N6CSendSingleNetBufferListTimerCallback(
		IN	PRT_TIMER	pTimer)
{

	PADAPTER		Adapter = pTimer->Adapter;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PNET_BUFFER		pCurrNetBuffer = NULL;
	PNET_BUFFER_LIST	pCurrNetBufferList = NULL;

	if(PlatformAtomicExchange(&Adapter->IntrNBLRefCount, TRUE)==TRUE)
		return;
	
	PlatformAcquireSpinLock( Adapter, RT_TX_SPINLOCK);
	if(pDevice->SendingNetBufferList == NULL)
	{
		while(TRUE)
		{
			PlatformAcquireSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			if(N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue))
			{
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
				break;
			}

			if(!pNdisCommon->bReleaseNblWaitQueueInProgress)
			{
				pCurrNetBufferList = N6CGetHeadNblWaitQueue(pNdisCommon->TxNBLWaitQueue);
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);

				if( N6C_CANNOT_TX(Adapter) )
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("break for CANNOT_TX()\n"));
					break;
				}

				RT_TRACE(COMP_SEND, DBG_TRACE, (" handle the NBL buffered(%p)\n", pCurrNetBufferList ));

				if( !N6SdioSendSingleNetBufferList(
						Adapter, 
						pCurrNetBufferList,
						TRUE) ) // bFromQueue
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("N6CSendSingleNetBufferList() returns FALSE\n"));
					break;
				}
			}
			else
			{
				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6CSendSingleNetBufferList(): bReleaseNblWaitQueueInProgress\n"));
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			}
		}
	}
	PlatformReleaseSpinLock( Adapter, RT_TX_SPINLOCK);

	PlatformAtomicExchange(&Adapter->IntrNBLRefCount, FALSE);
	return;
}

BOOLEAN
N6CQueryPhyIdReady(
	IN	PADAPTER			Adapter)
{

	BOOLEAN 	bResult = TRUE;
	PADAPTER 	pLoopAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pMgntInfo = &(pLoopAdapter->MgntInfo);
	
	//
	// <Roger_Notes> The following configuration is for airplane mode synchronization problem on Win8 or later version
	// 2013.06.07.
	//	
	while(pLoopAdapter)
	{
		// We only take first two ports into consideration
		if((GET_PORT_NUMBER(pLoopAdapter) == 0) ||
			(GET_PORT_NUMBER(pLoopAdapter) == 1))
		{			
			if(pLoopAdapter->pNdis62Common->bDot11SetPhyIdReady == FALSE)
			{
				RT_TRACE(COMP_RF, DBG_TRACE, ("N6CQueryPhyIdReady(): pLoopAdapter(%p), PortType(%d), bDot11SetPhyIdReady(%d)\n", 
					pLoopAdapter, pLoopAdapter->pNdis62Common->PortType, pLoopAdapter->pNdis62Common->bDot11SetPhyIdReady));
				bResult = FALSE;
				break;
			}			
		}	
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);	
	}

	return bResult;
}

#if DRV_LOG_REGISTRY

//
//	Description: 
//		Save current driver state in specific registry
//
//	Assumption: 
//		1. Running at PASSIVE_LEVEL
//
//	Added by Roger, 2013.12.13.
//
NDIS_STATUS
N6WriteRegDriverState(
	IN	PADAPTER		Adapter,
	IN	ULONG			State)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("DrvState");
	u1Byte				TimeOut = 100;


	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{
		RT_TRACE(COMP_INIT,DBG_WARNING, ("N6WriteRegDriverState(): Current IRQL>PASSIVE_LEVEL!!\n"));
		return NDIS_STATUS_FAILURE;
	}

	if(Adapter->bDriverStopped)
		return NDIS_STATUS_FAILURE;

	while(PlatformAtomicExchange(&Adapter->WriteRegRefCount, TRUE) == TRUE) 
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("N6WriteRegDriverState(): Waiting for previous process...\n"));
		PlatformSleepUs(100);
		TimeOut--;
		if(TimeOut <= 0)
		{
			RT_TRACE(COMP_INIT,DBG_WARNING, ("N6WriteRegDriverState(): Wait too long!!\n"));
			return NDIS_STATUS_FAILURE;
		}
	}
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT,DBG_TRACE, ("N6WriteRegDriverState(): Status=%x\n", Status));
		return Status;
	}
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = State;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);

	PlatformAtomicExchange(&Adapter->WriteRegRefCount, FALSE);
	
	return Status;
}

#endif

//
//	Description:
//		Write "DbgMonitor" registry value
//	2014.05.09, by Cosa.
//
NDIS_STATUS
N6WriteRegDbgMonitor(
	IN	PADAPTER		Adapter,
	IN	u4Byte			DbgMonitor
	)
{
	NDIS_HANDLE			ConfigurationHandle;
	NDIS_STATUS			Status;
	NDIS_CONFIGURATION_PARAMETER	ConfigParam;
	NDIS_STRING			KeyName = NDIS_STRING_CONST("DbgMonitor");
	
	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle(Adapter, &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE_EX(COMP_DBG_MON, DBG_LOUD, ("N6WriteRegDbgMonitor(): Fail!!!, Status=%x\n", Status));
		return Status;
	}
	
	ConfigParam.ParameterType = NdisParameterInteger;
	ConfigParam.ParameterData.IntegerData = DbgMonitor;
	
	NdisWriteConfiguration(
		&Status,
		ConfigurationHandle,
		&KeyName,
		&ConfigParam);

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);
	RT_TRACE_EX(COMP_DBG_MON, DBG_LOUD, ("N6WriteRegDbgMonitor(): Success!!!, DbgMonitor=%x\n", DbgMonitor));
	return Status;
}

VOID
NdisOIDHistoryInit(
	IN PADAPTER pAdapter)
{
	PRT_NDIS_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("NdisOIDHistoryInit() ====> \n"));
	
	NdisAllocateSpinLock(&(pNdisCommon->OidHistorySpinLock));
	RTInitializeListHead(&pNdisCommon->OidHistoryList);
	pNdisCommon->OidHistoryCount = 0;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("NdisOIDHistoryInit() <==== \n"));
}

VOID
NdisOIDHistoryDeInit(
	IN PADAPTER pAdapter)
{
	PRT_NDIS_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	PRT_OID_HISTORY_ENTRY	pOidHistoryEntry = NULL;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("====> NdisOIDHistoryDeInit()\n"));

	NdisAcquireSpinLock(&(pNdisCommon->OidHistorySpinLock));

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));
	
	if( pNdisCommon->OidHistoryCount > 0 )
	{
		do{
			pOidHistoryEntry = (PRT_OID_HISTORY_ENTRY)RTRemoveHeadListWithCnt(&pNdisCommon->OidHistoryList, &pNdisCommon->OidHistoryCount);
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));

			if( pOidHistoryEntry->InformationBuffer != NULL )
			{
				RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: Free information buffer %p, length: %d\n", __FUNCTION__, pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength));
				RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE, ("Free information buffer:"), pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength);
				PlatformFreeMemory(pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength);
				pOidHistoryEntry->InformationBuffer = NULL;
				pOidHistoryEntry->InformationBufferLength = 0;
			}

			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: Free OID history entry.\n", __FUNCTION__));
			PlatformFreeMemory(pOidHistoryEntry, sizeof(RT_OID_HISTORY_ENTRY));
		}while( RTIsListNotEmpty(&pNdisCommon->OidHistoryList) );
	}

	NdisReleaseSpinLock(&(pNdisCommon->OidHistorySpinLock));
	NdisFreeSpinLock(&(pNdisCommon->OidHistorySpinLock));
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("<==== NdisOIDHistoryDeInit()\n"));
}

VOID
NdisOIDHistoryUpdate(
	IN	PADAPTER			pAdapter,
	IN	PNDIS_OID_REQUEST	pNdisRequest,
	IN	RT_OID_HISTORY_STATE		OidHistoryState)
{
	PRT_NDIS_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PRT_OID_HISTORY_ENTRY	pOidHistoryEntry = NULL;
	PWDI_MESSAGE_HEADER	pWdiHeader = (PWDI_MESSAGE_HEADER)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("====> %s\n", __FUNCTION__));

	do
	{
		if( pAdapter == NULL )
			break;
		
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));
		
		if( pNdisCommon->OidHistoryCount >= OID_HISTORY_MAX_NUM )
		{
			NdisAcquireSpinLock(&(pNdisCommon->OidHistorySpinLock));
			pOidHistoryEntry = (PRT_OID_HISTORY_ENTRY)RTRemoveHeadListWithCnt(&pNdisCommon->OidHistoryList, &pNdisCommon->OidHistoryCount);
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: remove entry from head, pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));
			NdisReleaseSpinLock(&(pNdisCommon->OidHistorySpinLock));

			if( pOidHistoryEntry->InformationBuffer != NULL )
			{
				PlatformFreeMemory(pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength);
				pOidHistoryEntry->InformationBuffer = NULL;
				pOidHistoryEntry->InformationBufferLength = 0;
			}
			pOidHistoryEntry->CurrentState = OidHistoryState;
			pOidHistoryEntry->CurrentTime = PlatformGetCurrentTime();
			pOidHistoryEntry->Oid = pNdisRequest->DATA.METHOD_INFORMATION.Oid;
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->Oid: 0x%X\r\n", pOidHistoryEntry->Oid));
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->CurrentState: %d\r\n", pOidHistoryEntry->CurrentState));
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->CurrentTime: %I64d\r\n", pOidHistoryEntry->CurrentTime));
			
			pOidHistoryEntry->RequestType = pNdisRequest->RequestType;
			switch (pOidHistoryEntry->RequestType)
			{
				// Query
				case NdisRequestQueryInformation:			
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;	
					break;
					
				// Set
				case NdisRequestSetInformation:
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
					break;

				// Method
				case NdisRequestMethod:
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
					break;
			}
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->InformationBufferLength: %#x\r\n", pOidHistoryEntry->InformationBufferLength));

			if( pOidHistoryEntry->InformationBufferLength > 0 )
			{
				if( PlatformAllocateMemory(pAdapter, &pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength) == RT_STATUS_SUCCESS )
				{
					switch (pOidHistoryEntry->RequestType)
					{
						// Query
						case NdisRequestQueryInformation:				
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer, pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength);
							break;
						// Set
						case NdisRequestSetInformation:
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.SET_INFORMATION.InformationBuffer, pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength);
							break;

						// Method
						case NdisRequestMethod:
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);
							break;
					}
				}
				else
				{
					RT_TRACE(COMP_OID_SET, DBG_TRACE, ("[Full]Fail to allocate memory to store information buffer.\r\n"));
					pOidHistoryEntry->InformationBuffer = NULL;
					pOidHistoryEntry->InformationBufferLength = 0;
				}
			}
			if( pOidHistoryEntry->InformationBuffer != NULL )
				RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->InformationBuffer:\n"), pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength);
			
			pOidHistoryEntry->TransactionId = pWdiHeader->TransactionId;

			NdisAcquireSpinLock(&(pNdisCommon->OidHistorySpinLock));
			RTInsertTailListWithCnt(&pNdisCommon->OidHistoryList, &(pOidHistoryEntry->List), &pNdisCommon->OidHistoryCount);
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: insert entry to tail, pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));
			NdisReleaseSpinLock(&(pNdisCommon->OidHistorySpinLock));
		}
		else
		{
			if( PlatformAllocateMemory(pAdapter, &pOidHistoryEntry, sizeof(RT_OID_HISTORY_ENTRY)) != RT_STATUS_SUCCESS )
			{
				RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("Allocate OID_HISTORY_ENTRY failure\r\n"));
				break;
			}
			pOidHistoryEntry->InformationBuffer = NULL;
			pOidHistoryEntry->InformationBufferLength = 0;
			pOidHistoryEntry->CurrentState = OidHistoryState;
			pOidHistoryEntry->CurrentTime = PlatformGetCurrentTime();
			pOidHistoryEntry->Oid = pNdisRequest->DATA.METHOD_INFORMATION.Oid;
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->Oid: 0x%X\r\n", pOidHistoryEntry->Oid));
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->CurrentState: %d\r\n", pOidHistoryEntry->CurrentState));
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->CurrentTime: %I64d\r\n", pOidHistoryEntry->CurrentTime));

			pOidHistoryEntry->RequestType = pNdisRequest->RequestType;
			switch (pOidHistoryEntry->RequestType)
			{
				// Query
				case NdisRequestQueryInformation:				
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
					break;
				// Set
				case NdisRequestSetInformation:
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
					break;

				// Method
				case NdisRequestMethod:
					pOidHistoryEntry->InformationBufferLength = pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
					break;
			}
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->InformationBufferLength: %#x\r\n", pOidHistoryEntry->InformationBufferLength));

			if( pOidHistoryEntry->InformationBufferLength > 0 )
			{
				if( PlatformAllocateMemory(pAdapter, &pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength) == RT_STATUS_SUCCESS )
				{
					switch (pOidHistoryEntry->RequestType)
					{
						// Query
						case NdisRequestQueryInformation:				
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer, pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength);
							break;
						// Set
						case NdisRequestSetInformation:
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.SET_INFORMATION.InformationBuffer, pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength);
							break;

						// Method
						case NdisRequestMethod:
							PlatformMoveMemory(pOidHistoryEntry->InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);
							break;
					}
				}
				else
				{
					RT_TRACE(COMP_OID_SET, DBG_TRACE, ("[New]Fail to allocate memory to store information buffer.\r\n"));
					pOidHistoryEntry->InformationBuffer = NULL;
					pOidHistoryEntry->InformationBufferLength = 0;
				}
			}
			
			if( pOidHistoryEntry->InformationBuffer != NULL )
				RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE, ("pOidHistoryEntry->InformationBuffer:\n"), pOidHistoryEntry->InformationBuffer, pOidHistoryEntry->InformationBufferLength);

			pOidHistoryEntry->TransactionId = pWdiHeader->TransactionId;

			NdisAcquireSpinLock(&(pNdisCommon->OidHistorySpinLock));
			RTInsertTailListWithCnt(&pNdisCommon->OidHistoryList, &(pOidHistoryEntry->List), &pNdisCommon->OidHistoryCount);
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("%s: insert entry to tail, pNdisCommon->OidHistoryCount %d\n", __FUNCTION__, pNdisCommon->OidHistoryCount));
			NdisReleaseSpinLock(&(pNdisCommon->OidHistorySpinLock));
		}
	}while(FALSE);
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("<==== %s\n", __FUNCTION__));
}

VOID
N6CReleaseDataFrameQueuedWorkItemCallback(
	IN PVOID		pContext
)
{
	PADAPTER				pAdapter = (PADAPTER)pContext;
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PNET_BUFFER_LIST		pNetBufferList;
	PNDIS_ADAPTER_TYPE		pDevice = GET_NDIS_ADAPTER(pAdapter);
	PNET_BUFFER_LIST		pCurrNetBufferList, pNextNetBufferList;
	KIRQL			OldIrql = KeGetCurrentIrql();

	RT_TRACE(COMP_SEND, DBG_LOUD, ("N6CReleaseDataFrameQueuedWorkItemCallback()\n"));

	PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
	pNdisCommon->bReleaseNblWaitQueueInProgress = TRUE;
	PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
	
	while(TRUE)
	{
		
		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
		
		//
		// Wait SendingNetBufferList completed.
		//
		if( pDevice->SendingNetBufferList != NULL )
		{
			pDevice->bWaitingSendingNBL = TRUE;
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		
			RT_TRACE(COMP_SEND, DBG_LOUD, ("N6CReleaseDataFrameQueuedWorkItemCallback(): wait SendingNetBufferList...\n"));
			// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
			// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
			// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
			NdisWaitEvent(&(pDevice->evtSendingNBLCompleted), 0);
			NdisResetEvent(&(pDevice->evtSendingNBLCompleted));
		
			PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
		}
		RT_ASSERT(pDevice->SendingNetBufferList == NULL, ("N6CReleaseDataFrameQueuedWorkItemCallback(): SendingNetBufferList(%p) should be NULL!!!\n", pDevice->SendingNetBufferList));
		
		//
		// Remove pending NBLs in wait queue.
		// Note that, we must make sure: 
		// 1. SendingNetBufferList is NULL.
		// 2. No one can call N6SdioSendSingleNetBufferList() now.
		// 3. All TCB and related resource are completed.
		//
		
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
	
		PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
		if(N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue))
		{
			PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
			break;
		}
		pNetBufferList = N6CRemoveNblWaitQueue(&pNdisCommon->TxNBLWaitQueue, TRUE);

		if(pNetBufferList != NULL)
			RT_NBL_SET_REF_CNT(pNetBufferList, 0);   //YJ,test for MPE,120221

		for (pCurrNetBufferList = pNetBufferList;
			pCurrNetBufferList != NULL;
			pCurrNetBufferList = pNextNetBufferList)
		{
			pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
			NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = NDIS_STATUS_SUCCESS;
		}
		PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

		RT_TRACE(COMP_SEND, DBG_LOUD, ("N6CReleaseDataFrameQueuedWorkItemCallback(): NdisMSendNetBufferListsComplete\n"));
		// Prefast warning C6387: 'pNetBufferList' could be '0':  this does not adhere to the specification for the function 'NdisMSendNetBufferListsComplete'.
		if (pNetBufferList != NULL)
		{
			NdisMSendNetBufferListsComplete(
				pDevice->hNdisAdapter,
				pNetBufferList,
				((NDIS_CURRENT_IRQL() == DISPATCH_LEVEL) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));
		}
	}

	PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
	pNdisCommon->bReleaseNblWaitQueueInProgress = FALSE;
	PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
}


//
//	Description: 
//		Timer resource management for SoC off on Win10 Mobile platform
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
VOID
N6CTimerResourceInit(
	IN	PADAPTER	pAdapter
)
{
	PPORT_COMMON_INFO pPortCommonInfo = pAdapter->pPortCommonInfo;

	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	RTInitializeListHead(&(pPortCommonInfo->TimerResourceList));
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
}


//
//	Description: 
//		Timer resource management for SoC off on Win10 Mobile platform
//
//	Assumption:
//		TimerLock is acquired before calling this function
//
//	2016.01.15, created by Roger.
//
VOID
N6CTimerResourceInsert(
	IN	PADAPTER 	pAdapter,
	IN	PRT_TIMER 	pTimer
)
{

	PPORT_COMMON_INFO pPortCommonInfo = pAdapter->pPortCommonInfo;
	
	RTInsertTailListWithCnt(&(pPortCommonInfo->TimerResourceList), &(pTimer->Handle.List), &pPortCommonInfo->uNumberOfInsertedTimerRes);

	RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("N6CTimerResourceInsert(): uNumberOfInsertedTimerRes(%d), Timer: %s\n", pPortCommonInfo->uNumberOfInsertedTimerRes, pTimer->szID));
}

//
//	Description: 
//		Timer resource management for SoC off on Win10 Mobile platform
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
BOOLEAN
N6CTimerResourceRemove(
	IN	PADAPTER	pAdapter,
	IN	PRT_TIMER	pTimer
)
{

	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;
	PRT_LIST_ENTRY	pList = NULL;
	PRT_TIMER	pThisTimer = NULL;
	u4Byte	uNumberOfTimerHandled = 0;	
	BOOLEAN		bFound = FALSE;

	
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	pList = RTGetHeadList(&pPortCommonInfo->TimerResourceList);

	while(!RTIsListHead(&pPortCommonInfo->TimerResourceList, pList))
	{
		pThisTimer = (PRT_TIMER) CONTAINING_RECORD(pList, RT_TIMER, Handle.List);	

		if(pThisTimer->Handle.NdisTimerHandle == pTimer->Handle.NdisTimerHandle){
			
			RTRemoveEntryListWithCnt(pList, &pPortCommonInfo->uNumberOfInsertedTimerRes);
			bFound = TRUE;
			
			RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("N6CTimerResourceRemove(): Timer Found! uNumberOfInsertedTimerRes(%d), Timer: %s\n", 
				pPortCommonInfo->uNumberOfInsertedTimerRes, pThisTimer->szID));

			break;
		}		
		pList = RTNextEntryList(&pThisTimer->Handle.List);
	}
	
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));

	RT_ASSERT(bFound, ("N6CTimerResourceRemove(): Fail to remove specific Timer resource(%s)\n", pTimer->szID));
	
	return bFound;
}


//
//	Description: 
//		Timer resource management for SoC off on Win10 Mobile platform, we can use this routine to confirm 
//		whether all timer resources are released when driver is going to unload or halt.
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
VOID
N6CTimerResourceDump(
	IN	PADAPTER	pAdapter
)
{

	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;
	PRT_LIST_ENTRY	pList = NULL;
	PRT_TIMER	pTimer = NULL;
	u4Byte	uNumberOfTimerHandled = 0;	
	
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	pList = RTGetHeadList(&pPortCommonInfo->TimerResourceList);

	while(!RTIsListHead(&pPortCommonInfo->TimerResourceList, pList))
	{
		pTimer = (PRT_TIMER) CONTAINING_RECORD(pList, RT_TIMER, Handle.List);
		uNumberOfTimerHandled++;
		
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("N6CTimerResourceDump(): uNumberOfTimerHandled(%d), Timer: %s\n", uNumberOfTimerHandled, pTimer->szID));
		
		pList = RTNextEntryList(&pTimer->Handle.List);
	}

	RT_ASSERT(uNumberOfTimerHandled == pPortCommonInfo->uNumberOfInsertedTimerRes, ("N6CTimerResourceDump(): uNumberOfTimerHandled is not match!\n"));
	
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));

}


//
//	Description: 
//		Timer resource management for SoC off on Win10 Mobile platform
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
VOID
N6CTimerResourceAction(
	IN	PADAPTER	pAdapter,
	IN	RT_TIMER_RESOURCE_ACTION		Action
)
{

	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;
	PRT_LIST_ENTRY	pList = NULL;
	PRT_TIMER	pTimer = NULL;
	u4Byte	uNumberOfTimerHandled = 0;	
	
	
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("--->N6CTimerResourceAction(): Action(%d)\n", Action));
	
	pList = RTGetHeadList(&pPortCommonInfo->TimerResourceList);

	while(!RTIsListHead(&pPortCommonInfo->TimerResourceList, pList))
	{

		pTimer = (PRT_TIMER) CONTAINING_RECORD(pList, RT_TIMER, Handle.List);
		uNumberOfTimerHandled++;	

		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));

		if(Action == RT_TIMER_RESOURCE_ACTION_SUSPEND)
			PlatformSuspendTimer(pAdapter, pTimer);
		else if(Action == RT_TIMER_RESOURCE_ACTION_RESUME)
			PlatformResumeTimer(pAdapter, pTimer);
		else{
			// TODO:
		}
		
		NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
		
		pList = RTNextEntryList(&pTimer->Handle.List);
	}

	RT_ASSERT(uNumberOfTimerHandled == pPortCommonInfo->uNumberOfInsertedTimerRes, ("N6CTimerResourceAction(): uNumberOfTimerHandled is not match!\n"));
	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("<---N6CTimerResourceAction(): Action(%d)\n", Action));
	
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));	
}


