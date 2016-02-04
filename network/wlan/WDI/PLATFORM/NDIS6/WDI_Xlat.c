////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_Xlat.c
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"
#include "WDI_Xlat.h"

#if WPP_SOFTWARE_TRACE
#include "WDI_Xlat.tmh"
#endif

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
static
BOOLEAN
wdi_xlat_Zalloc(
	IN  VOID					**pp,
	IN  UINT 					len
	)
{
	// Note: Attempt to allocate pool with a tag that does not contain any 
	// letters or digits would irritate driver verifier.
	// The use of 'XLAT' is called multicharacter constant.
	if(NDIS_STATUS_SUCCESS == NdisAllocateMemoryWithTag(pp, len, 'XLAT'))
	{
		NdisZeroMemory(*pp, len);
		return TRUE;
	}

	return FALSE;
}

static
VOID
wdi_xlat_Free(
	IN  VOID					*p,
	IN  UINT 					len
	)
{
	NdisFreeMemory(p, len, 0);
}

static
WDI_TASK_PARAM *
wdi_xlat_ParamConstructor(
	IN  u1Byte					*buf,
	IN  RT_OID_HANDLER			*hTask,
	IN  UINT					allocSize
	)
{
	WDI_TASK_PARAM				*param = (WDI_TASK_PARAM *)(buf + allocSize - sizeof(WDI_TASK_PARAM));
	
	param->hTask = hTask;
	param->allocSize = allocSize;

	return param;
}

static
NDIS_PORT_NUMBER
wdi_xlat_PortId(
	IN  WDI_MESSAGE_HEADER		*hdr
	)
{
	return (NDIS_PORT_NUMBER)hdr->PortId;
}

static
VOID
wdi_xlat_OperationMode(
	IN  WDI_OPERATION_MODE		wdiOpMode,
	OUT DOT11_CURRENT_OPERATION_MODE *opMode
	)
{
	opMode->uReserved = 0;
	if(WDI_OPERATION_MODE_STA == wdiOpMode)
		opMode->uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_STATION;
	//else if(WDI_OPERATION_MODE_AP == wdiOpMode)
	//	opMode->uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_AP;
	//else if(WDI_OPERATION_MODE_NETMON == wdiOpMode)
	//	opMode->uCurrentOpMode = DOT11_OPERATION_MODE_NETWORK_MONITOR;
	else if(WDI_OPERATION_MODE_P2P_DEVICE == wdiOpMode)
		opMode->uCurrentOpMode = DOT11_OPERATION_MODE_WFD_DEVICE;
	else if(WDI_OPERATION_MODE_P2P_CLIENT == wdiOpMode)
		opMode->uCurrentOpMode = DOT11_OPERATION_MODE_WFD_CLIENT;
	else if(WDI_OPERATION_MODE_P2P_GO == wdiOpMode)
		opMode->uCurrentOpMode = DOT11_OPERATION_MODE_WFD_GROUP_OWNER;
	//else if(WDI_OPERATION_MODE_MANUFACTURING == wdiOpMode)
	//	opMode->uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_STATION;
	else
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("unknown WDI op mode: %u, use ext sta instead\n", wdiOpMode));
		opMode->uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_STATION;
	}

	return;
}

static
VOID
wdi_xlat_P2PDiscoverType(
	IN  WDI_P2P_DISCOVER_TYPE	wdiType,
	IN  BOOLEAN					bForced,
	OUT DOT11_WFD_DISCOVER_TYPE	*type
	)
{
	if(WDI_P2P_DISCOVER_TYPE_SCAN_ONLY == wdiType)
		*type = dot11_wfd_discover_type_scan_only;
	else if(WDI_P2P_DISCOVER_TYPE_FIND_ONLY == wdiType)
		*type = dot11_wfd_discover_type_find_only;
	else if(WDI_P2P_DISCOVER_TYPE_AUTO == wdiType)
		*type = dot11_wfd_discover_type_auto;
	else if(WDI_P2P_DISCOVER_TYPE_SCAN_SOCIAL_CHANNELS == wdiType)
		*type = dot11_wfd_discover_type_scan_social_channels;
	else
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("unknown WDI P2P discover type: %u, use auto instead\n", wdiType));
		*type = dot11_wfd_discover_type_auto;
	}

	if(bForced)
		SET_FLAG(*type, dot11_wfd_discover_type_forced);

	return;
}

static
VOID
wdi_xlat_P2PScanType(
	IN  WDI_P2P_SCAN_TYPE		wdiType,
	OUT DOT11_WFD_SCAN_TYPE		*type
	)
{
	if(WDI_P2P_SCAN_TYPE_ACTIVE == wdiType)
		*type = dot11_wfd_scan_type_active;
	else if(WDI_P2P_SCAN_TYPE_PASSIVE == wdiType)
		*type = dot11_wfd_scan_type_passive;
	else if(WDI_P2P_SCAN_TYPE_AUTO == wdiType)
		*type = dot11_wfd_scan_type_auto;
	else
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("unknown WDI P2P scan type: %u, use passive instead\n", wdiType));
		*type = dot11_wfd_scan_type_passive;
	}
		
	return;
}


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

DOT11_AUTH_ALGORITHM
Wdi_Xlat_AuthMode(
	IN  WDI_AUTH_ALGORITHM		algo
	)
{
	if(WDI_AUTH_ALGO_80211_OPEN == algo)
		return DOT11_AUTH_ALGO_80211_OPEN;
	else if(WDI_AUTH_ALGO_80211_SHARED_KEY == algo)
		return DOT11_AUTH_ALGO_80211_SHARED_KEY;
	else if(WDI_AUTH_ALGO_WPA == algo)
		return DOT11_AUTH_ALGO_WPA;
	else if(WDI_AUTH_ALGO_WPA_PSK == algo)
		return DOT11_AUTH_ALGO_WPA_PSK;
	else if(WDI_AUTH_ALGO_WPA_NONE == algo)
		return DOT11_AUTH_ALGO_WPA_NONE;
	else if(WDI_AUTH_ALGO_RSNA == algo)
		return DOT11_AUTH_ALGO_RSNA;
	else if(WDI_AUTH_ALGO_RSNA_PSK == algo)
		return DOT11_AUTH_ALGO_RSNA_PSK;
	else if(WDI_AUTH_ALGO_IHV_START == algo)
		return DOT11_AUTH_ALGO_IHV_START;
	else if(WDI_AUTH_ALGO_IHV_END == algo)
		return DOT11_AUTH_ALGO_IHV_END;
	else
	{
		RT_ASSERT(FALSE, ("%s(): invalid auth algo: %u\n", __FUNCTION__, algo));
		return DOT11_AUTH_ALGO_80211_OPEN;
	}
}

DOT11_CIPHER_ALGORITHM
Wdi_Xlat_CipherAlgo(
	IN  WDI_CIPHER_ALGORITHM	algo
	)
{
	if(WDI_CIPHER_ALGO_NONE == algo)
		return DOT11_CIPHER_ALGO_NONE;
	else if(WDI_CIPHER_ALGO_WEP40 == algo)
		return DOT11_CIPHER_ALGO_WEP40;
	else if(WDI_CIPHER_ALGO_TKIP == algo)
		return DOT11_CIPHER_ALGO_TKIP;
	else if(WDI_CIPHER_ALGO_CCMP == algo)
		return DOT11_CIPHER_ALGO_CCMP;
	else if(WDI_CIPHER_ALGO_WEP104 == algo)
		return DOT11_CIPHER_ALGO_WEP104;
	else if(WDI_CIPHER_ALGO_BIP == algo)
		return DOT11_CIPHER_ALGO_BIP;
	else if(WDI_CIPHER_ALGO_WPA_USE_GROUP == algo)
		return DOT11_CIPHER_ALGO_WPA_USE_GROUP;
	else if(WDI_CIPHER_ALGO_RSN_USE_GROUP == algo)
		return DOT11_CIPHER_ALGO_RSN_USE_GROUP;
	else if(WDI_CIPHER_ALGO_WEP == algo)
		return DOT11_CIPHER_ALGO_WEP;
	else if(WDI_CIPHER_ALGO_IHV_START == algo)
		return DOT11_CIPHER_ALGO_IHV_START;
	else if(WDI_CIPHER_ALGO_IHV_END == algo)
		return DOT11_CIPHER_ALGO_IHV_END;
	else
	{
		RT_ASSERT(FALSE, ("%s(): invalid cipher algo: %u\n", __FUNCTION__, algo));
		return DOT11_CIPHER_ALGO_NONE;
	}
}

VOID
Wdi_Xlat_FreeOid(
	IN  NDIS_OID_REQUEST		*req
	)
{
	WDI_TASK_PARAM				*param = (WDI_TASK_PARAM *)WDI_XLAT_GET_OID_MP_RSVD_0(req);
	
	wdi_xlat_Free(req, param->allocSize);

	return;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocCreateMacOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_CREATE_PORT_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_MAC_INFO)			// the output buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N62C_OID_DOT11_CREATE_MAC, for output
	req->RequestType = NdisRequestMethod;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_CREATE_MAC;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = 0;
	req->DATA.METHOD_INFORMATION.OutputBufferLength = sizeof(DOT11_MAC_INFO);

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDeleteMacOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_DELETE_PORT_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_MAC_INFO)			// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_MAC_INFO				*macInfo = NULL;

	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N62C_OID_DOT11_DELETE_MAC
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.SET_INFORMATION.Oid = OID_DOT11_DELETE_MAC;
	req->DATA.SET_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.SET_INFORMATION.InformationBufferLength  = sizeof(DOT11_MAC_INFO);

	// Prep MAC info, don't have mac addr here
	macInfo = (DOT11_MAC_INFO *)req->DATA.SET_INFORMATION.InformationBuffer;
	macInfo->uNdisPortNumber = (ULONG)param->DeletePortParameters.PortNumber;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocStartApReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_START_AP_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(WDI_TASK_PARAM)
									;

	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_START_GO_REQUEST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.SET_INFORMATION.Oid = OID_DOT11_WFD_START_GO_REQUEST;
	req->DATA.SET_INFORMATION.InformationBuffer = NULL;
	req->DATA.SET_INFORMATION.InformationBufferLength  = 0;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSetDesiredSsidListOid(
	IN	RT_OID_HANDLER			*hTask,
	IN	WDI_SSID				*param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST)	// the NDIS REQUEST
									+ sizeof(DOT11_SSID_LIST)
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SSID_LIST				*ssidlist = NULL;

	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_START_GO_REQUEST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.SET_INFORMATION.Oid = OID_DOT11_DESIRED_SSID_LIST;
	req->DATA.SET_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.SET_INFORMATION.InformationBufferLength = sizeof(DOT11_SSID_LIST);

	// Prep SSID list
	ssidlist = (DOT11_SSID_LIST *)req->DATA.SET_INFORMATION.InformationBuffer;
	ssidlist->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	ssidlist->Header.Revision = DOT11_SSID_LIST_REVISION_1;
	ssidlist->Header.Size = sizeof(DOT11_SSID_LIST);
	ssidlist->uNumOfEntries = 1;
	ssidlist->uTotalNumOfEntries = 1;
	ssidlist->SSIDs[0].uSSIDLength = param->ElementCount;
	PlatformMoveMemory(ssidlist->SSIDs[0].ucSSID, param->pElements, param->ElementCount);

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocResetOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_DOT11_RESET_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_RESET_REQUEST)		// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_RESET_REQUEST			*resetReq = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N6C_OID_DOT11_RESET_REQUEST
	// Note: assume sizeof DOT11_RESET_REQUEST > DOT11_STATUS_INDICATION
	req->RequestType = NdisRequestMethod;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_RESET_REQUEST;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_RESET_REQUEST);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = sizeof(DOT11_STATUS_INDICATION);

	// Prep reset req, ignore resetReq->dot11MacAddress here
	resetReq = (DOT11_RESET_REQUEST *)req->DATA.SET_INFORMATION.InformationBuffer;
	resetReq->dot11ResetType = dot11_reset_type_phy_and_mac;
	resetReq->bSetDefaultMIB = (0 != param->Dot11ResetParameters.SetDefaultMIB);

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocChangeOpModeOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_CHANGE_OPERATION_MODE_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 		// the NDIS REQUEST
									+ sizeof(DOT11_CURRENT_OPERATION_MODE) 	// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_CURRENT_OPERATION_MODE *opMode = NULL;
	
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N62C_OID_DOT11_DELETE_MAC
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.SET_INFORMATION.Oid = OID_DOT11_CURRENT_OPERATION_MODE;
	req->DATA.SET_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.SET_INFORMATION.InformationBufferLength  = sizeof(*opMode);

	// Prep op mode
	wdi_xlat_OperationMode(
		param->OperationMode, 
		(DOT11_CURRENT_OPERATION_MODE *)req->DATA.SET_INFORMATION.InformationBuffer);
	
	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocWfdDiscoverOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_DISCOVER_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 		// the NDIS REQUEST
									+ sizeof(DOT11_WFD_DISCOVER_REQUEST) 	// the input buffer
									+ param->VendorIEs.ElementCount			// vendor IE
									+ sizeof(WDI_TASK_PARAM)
									;
	
	DOT11_WFD_DISCOVER_REQUEST	*discReq = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_OID_DOT11_WFD_DISCOVER_REQUEST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.SET_INFORMATION.Oid = OID_DOT11_WFD_DISCOVER_REQUEST;
	req->DATA.SET_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.SET_INFORMATION.InformationBufferLength = sizeof(*discReq);

	// Prep reset req, ignore resetReq->dot11MacAddress here
	discReq = (DOT11_WFD_DISCOVER_REQUEST *)req->DATA.SET_INFORMATION.InformationBuffer;
	{		
		// discover type
		wdi_xlat_P2PDiscoverType(
			param->DiscoverMode.DiscoveryType, 
			(0 != param->DiscoverMode.ForcedDiscovery), 
			&discReq->DiscoverType);

		// scan type
		wdi_xlat_P2PScanType(param->DiscoverMode.ScanType, &discReq->ScanType);

		// TODO: ScanRepeatCount

		// TODO: TimeBetweenScans

		// TODO: DwellTime

		// TODO: BandChannelList

		// TODO: SSIDList: ArrayOfElementsOfWIFI_SSID

		// VendorIEs
		if(param->VendorIEs.ElementCount)
		{
			discReq->uIEsLength = param->VendorIEs.ElementCount;
			discReq->uIEsOffset = sizeof(*discReq);
			NdisMoveMemory(
				(u1Byte *)discReq + discReq->uIEsOffset, 
				param->VendorIEs.pElements, param->VendorIEs.ElementCount);
		}
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDevInfoOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_DEVICE_INFO_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_WFD_DEVICE_INFO)		// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_WFD_DEVICE_INFO		*devInfo = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_DEVICE_INFO
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_ADDITIONAL_IE;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_WFD_DEVICE_INFO);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	devInfo = (DOT11_WFD_DEVICE_INFO *)req->DATA.SET_INFORMATION.InformationBuffer;
	devInfo->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	devInfo->Header.Revision = DOT11_WFD_DEVICE_INFO_REVISION_1;
	devInfo->Header.Size = sizeof(DOT11_WFD_DEVICE_INFO);
	cpMacAddr(devInfo->DeviceAddress, param->DeviceInfoParameters.DeviceAddress);
	devInfo->ConfigMethods = param->DeviceInfoParameters.ConfigurationMethods;
	devInfo->PrimaryDeviceType.CategoryID = param->DeviceInfoParameters.DeviceType.CategoryID;
	devInfo->PrimaryDeviceType.SubCategoryID = param->DeviceInfoParameters.DeviceType.SubcategoryID;
	PlatformMoveMemory(devInfo->PrimaryDeviceType.OUI, 
		param->DeviceInfoParameters.DeviceType.OUI,
		sizeof(param->DeviceInfoParameters.DeviceType.OUI));

	devInfo->DeviceName.uDeviceNameLength = param->DeviceName.ElementCount;
	PlatformMoveMemory(devInfo->DeviceName.ucDeviceName,
		param->DeviceName.pElements,
		param->DeviceName.ElementCount);

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDevCapOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_DEVICE_CAPABILITY_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_WFD_DEVICE_CAPABILITY_CONFIG) // the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_WFD_DEVICE_CAPABILITY_CONFIG *devCap = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_DEVICE_CAPABILITY
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_DEVICE_CAPABILITY;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_WFD_DEVICE_CAPABILITY_CONFIG);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	devCap = (DOT11_WFD_DEVICE_CAPABILITY_CONFIG *)req->DATA.SET_INFORMATION.InformationBuffer;
	devCap->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	devCap->Header.Revision = DOT11_WFD_DEVICE_CAPABILITY_CONFIG_REVISION_1;
	devCap->Header.Size = sizeof(DOT11_WFD_DEVICE_CAPABILITY_CONFIG);
	devCap->bServiceDiscoveryEnabled = TEST_FLAG(param->DeviceCapabilityBitmap , dcServiceDiscovery);
	devCap->bClientDiscoverabilityEnabled = TEST_FLAG(param->DeviceCapabilityBitmap , dcP2PClientDiscoverability);
	devCap->bConcurrentOperationSupported = TEST_FLAG(param->DeviceCapabilityBitmap , dcConcurrentOperation);
	devCap->bInfrastructureManagementEnabled = TEST_FLAG(param->DeviceCapabilityBitmap , dcP2PInfrastructureManaged);
	devCap->bDeviceLimitReached = TEST_FLAG(param->DeviceCapabilityBitmap , dcP2PDeviceLimit);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("[%s] devCap: %02X \n", "p2pattr", param->DeviceCapabilityBitmap ));
	
	devCap->bInvitationProcedureEnabled = TEST_FLAG(param->DeviceCapabilityBitmap , dcP2PInvitationProcedure);
	devCap->WPSVersionsEnabled = param->WPSVersionsEnabled;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocGrpCapOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_GROUP_OWNER_CAPABILITY_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG) // the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG *grpCap = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_DEVICE_CAPABILITY;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_WFD_DEVICE_CAPABILITY_CONFIG);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	grpCap = (DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG *)req->DATA.SET_INFORMATION.InformationBuffer;
	grpCap->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	grpCap->Header.Revision = DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG_REVISION_1;
	grpCap->Header.Size = sizeof(DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG);
	grpCap->bPersistentGroupEnabled = TEST_FLAG(param->GroupCapabilityBitmap, gcPersistentP2PGroup);
	grpCap->bIntraBSSDistributionSupported = TEST_FLAG(param->GroupCapabilityBitmap, gcIntraBSSDistribution);
	grpCap->bCrossConnectionSupported = TEST_FLAG(param->GroupCapabilityBitmap, gcCrossConnection);
	grpCap->bPersistentReconnectSupported = TEST_FLAG(param->GroupCapabilityBitmap, gcPersistentReconnect);
	grpCap->bGroupFormationEnabled = TEST_FLAG(param->GroupCapabilityBitmap, gcGroupFormation);
	grpCap->uMaximumGroupLimit = param->MaximumGroupLimit;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSecDevTypeOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_SECONDARY_DEVICE_TYPE_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST) + sizeof(DOT11_WFD_DEVICE_TYPE) * param->ElementCount // the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;
	ULONG						it = 0;

	DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST *secDevTypeList = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_DEVICE_CAPABILITY;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_WFD_DEVICE_CAPABILITY_CONFIG);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	secDevTypeList = (DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST *)req->DATA.SET_INFORMATION.InformationBuffer;
	secDevTypeList->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	secDevTypeList->Header.Revision = DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST_REVISION_1;
	secDevTypeList->Header.Size = sizeof(DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST);
	secDevTypeList->uNumOfEntries = param->ElementCount;
	secDevTypeList->uTotalNumOfEntries = param->ElementCount;
	for(it = 0; it < param->ElementCount; it++)
	{
		secDevTypeList->SecondaryDeviceTypes[it].CategoryID = param->pElements[it].CategoryID;

		secDevTypeList->SecondaryDeviceTypes[it].SubCategoryID = param->pElements[it].SubcategoryID;
		PlatformMoveMemory(secDevTypeList->SecondaryDeviceTypes[it].OUI, 
			param->pElements[it].OUI,
			sizeof(param->pElements[it].OUI));
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocListenStateDiscoverabilityOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_LISTEN_STATE_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = (WDI_MESSAGE_HEADER *)hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(ULONG) 					// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	ULONG 						*listenStateDiscoverability = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(ULONG);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	listenStateDiscoverability = (ULONG *)req->DATA.SET_INFORMATION.InformationBuffer;
	if(WDI_P2P_LISTEN_STATE_OFF == *param)
		*listenStateDiscoverability = DOT11_WFD_DEVICE_NOT_DISCOVERABLE;
	else if(WDI_P2P_LISTEN_STATE_PASSIVE_AVAILABILITY == *param) // TODO: what's passive availability?
		*listenStateDiscoverability = DOT11_WFD_DEVICE_NOT_DISCOVERABLE;
	else if(WDI_P2P_LISTEN_STATE_AUTOMATIC_AVAILABILITY == *param)
		*listenStateDiscoverability = DOT11_WFD_DEVICE_AUTO_AVAILABILITY;
	else if(WDI_P2P_LISTEN_STATE_HIGH_AVAILABILITY == *param)
		*listenStateDiscoverability = DOT11_WFD_DEVICE_HIGH_AVAILABILITY;
	else
		*listenStateDiscoverability = DOT11_WFD_DEVICE_AUTO_AVAILABILITY;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocListenChannelOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_CHANNEL_CONTAINER *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = (WDI_MESSAGE_HEADER *)hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(ULONG) 					// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_WFD_DEVICE_LISTEN_CHANNEL *listenChnl = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_WFD_DEVICE_LISTEN_CHANNEL);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	listenChnl = (DOT11_WFD_DEVICE_LISTEN_CHANNEL *)req->DATA.SET_INFORMATION.InformationBuffer;
	listenChnl->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	listenChnl->Header.Revision = DOT11_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1;
	listenChnl->Header.Size = sizeof(DOT11_WFD_DEVICE_LISTEN_CHANNEL);
	listenChnl->ChannelNumber = (UCHAR)param->ChannelNumber;

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocGetDialogTokenOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN_PARAMETERS *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = (WDI_MESSAGE_HEADER *)hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(UCHAR) 					// the output buffer
									+ sizeof(WDI_TASK_PARAM)
									;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_QUERY_OID_DOT11_WFD_GET_DIALOG_TOKEN
	req->RequestType = NdisRequestQueryInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_GET_DIALOG_TOKEN;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = 0;
	req->DATA.METHOD_INFORMATION.OutputBufferLength = sizeof(UCHAR);

	return req;

}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegReqReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST)	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *reqParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req
	reqParam = (DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	reqParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	reqParam->Header.Revision = DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS_REVISION_1;
	reqParam->Header.Size = DOT11_SIZEOF_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS_REVISION_1;
	reqParam->DialogToken = param->RequestParams.DialogToken;
	cpMacAddr(reqParam->PeerDeviceAddress, param->RequestParams.PeerDeviceAddress.Address);
	reqParam->uSendTimeout = param->RequestParams.SendTimeout;

	if(param->Optional.GONegotiationRequestInfo_IsPresent)
	{
		reqParam->GroupOwnerIntent.Intent = (UCHAR)param->GONegotiationRequestInfo.RequestParams.GOIntent;
		reqParam->GroupOwnerIntent.TieBreaker = param->GONegotiationRequestInfo.RequestParams.TieBreaker;

		reqParam->MinimumConfigTimeout.GOTimeout = (UCHAR)param->GONegotiationRequestInfo.RequestParams.GOConfigTimeout;
		reqParam->MinimumConfigTimeout.ClientTimeout = (UCHAR)param->GONegotiationRequestInfo.RequestParams.ClientConfigTimeout;

		cpMacAddr(reqParam->IntendedInterfaceAddress, param->GONegotiationRequestInfo.RequestParams.IntendedInterfaceAddress.Address);

		reqParam->GroupCapability = param->GONegotiationRequestInfo.RequestParams.GroupCapability;
	}
	
	if(param->VendorIEs.ElementCount)
	{
		reqParam->uIEsLength = param->VendorIEs.ElementCount;
		reqParam->uIEsOffset = sizeof(*reqParam);
		PlatformMoveMemory((u1Byte *)reqParam + sizeof(*reqParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendInvitationReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_INVITATION_REQUEST_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_INVITATION_REQUEST_PARAMETERS *reqParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
	// Note: assume sizeof DOT11_RESET_REQUEST > DOT11_STATUS_INDICATION
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_INVITATION_REQUEST;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_INVITATION_REQUEST_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req, ignore resetReq->dot11MacAddress here
	reqParam = (DOT11_SEND_INVITATION_REQUEST_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	reqParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	reqParam->Header.Revision = DOT11_SEND_INVITATION_REQUEST_PARAMETERS_REVISION_1;
	reqParam->Header.Size = DOT11_SIZEOF_SEND_INVITATION_REQUEST_PARAMETERS_REVISION_1;
	reqParam->DialogToken = param->RequestParams.DialogToken;
	cpMacAddr(reqParam->PeerDeviceAddress, param->RequestParams.PeerDeviceAddress.Address);
	reqParam->uSendTimeout = param->RequestParams.SendTimeout;

	if(param->Optional.InvitationRequestInfo_IsPresent)
	{
		reqParam->MinimumConfigTimeout.ClientTimeout = (UCHAR)param->InvitationRequestInfo.RequestParams.ClientConfigTimeout;
		reqParam->MinimumConfigTimeout.GOTimeout = (UCHAR)param->InvitationRequestInfo.RequestParams.GOConfigTimeout;
		reqParam->InvitationFlags.InvitationType = TEST_FLAG(param->InvitationRequestInfo.RequestParams.InvitationFlags, BIT0);
		if(param->InvitationRequestInfo.Optional.GroupBSSID_IsPresent)
		{
			reqParam->bUseGroupBSSID = TRUE;
			cpMacAddr(reqParam->GroupBSSID, param->InvitationRequestInfo.GroupBSSID.Address);
		}
		if(param->InvitationRequestInfo.Optional.OperatingChannel_IsPresent)
		{
			reqParam->bUseSpecifiedOperatingChannel = TRUE;
			reqParam->OperatingChannel.ChannelNumber = (UCHAR)param->InvitationRequestInfo.OperatingChannel.ChannelNumber;
			reqParam->OperatingChannel.OperatingClass = param->InvitationRequestInfo.OperatingChannel.OperatingClass;
			PlatformMoveMemory(reqParam->OperatingChannel.CountryRegionString, 
				param->InvitationRequestInfo.OperatingChannel.CountryOrRegionString,
				sizeof(param->InvitationRequestInfo.OperatingChannel.CountryOrRegionString));
		}
		cpMacAddr(reqParam->GroupID.DeviceAddress, param->InvitationRequestInfo.GroupID.DeviceAddress.Address);
		reqParam->GroupID.SSID.uSSIDLength = param->InvitationRequestInfo.GroupID.GroupSSID.ElementCount;
		PlatformMoveMemory(reqParam->GroupID.SSID.ucSSID, 
			param->InvitationRequestInfo.GroupID.GroupSSID.pElements, 
			param->InvitationRequestInfo.GroupID.GroupSSID.ElementCount);
		reqParam->bLocalGO = param->InvitationRequestInfo.RequestParams.IsLocalGO;
	}
	
	if(param->VendorIEs.ElementCount)
	{
		reqParam->uIEsLength = param->VendorIEs.ElementCount;
		reqParam->uIEsOffset = sizeof(*reqParam);
		PlatformMoveMemory((u1Byte *)reqParam + sizeof(*reqParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendPDReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *reqParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req
	reqParam = (DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	reqParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	reqParam->Header.Revision = DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS_REVISION_1;
	reqParam->Header.Size = DOT11_SIZEOF_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS_REVISION_1;
	reqParam->DialogToken = param->RequestParams.DialogToken;
	cpMacAddr(reqParam->PeerDeviceAddress, param->RequestParams.PeerDeviceAddress.Address);
	reqParam->uSendTimeout = param->RequestParams.SendTimeout;

	if(param->Optional.ProvisionDiscoveryRequestInfo_IsPresent)
	{
		reqParam->GroupCapability = param->ProvisionDiscoveryRequestInfo.RequestParams.GroupCapability;
		
		if(param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent)
		{
			cpMacAddr(reqParam->GroupID.DeviceAddress, param->ProvisionDiscoveryRequestInfo.GroupID.DeviceAddress.Address);
			reqParam->GroupID.SSID.uSSIDLength = param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.ElementCount;
			PlatformMoveMemory(reqParam->GroupID.SSID.ucSSID, 
				param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.pElements, 
				param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.ElementCount);
			reqParam->bUseGroupID = TRUE;
		}

		if(param->ProvisionDiscoveryRequestInfo.Optional.ServiceAttributes_IsPresent)
		{
			// TODO: WFDS
		}

		if(param->ProvisionDiscoveryRequestInfo.Optional.PersistentGroupID_IsPresent)
		{
			// TODO: WFDS
		}

		if(param->ProvisionDiscoveryRequestInfo.Optional.ServiceSessionInfo_IsPresent)
		{
			// TODO: WFDS
		}
	}

	if(param->VendorIEs.ElementCount)
	{
		reqParam->uIEsLength = param->VendorIEs.ElementCount;
		reqParam->uIEsOffset = sizeof(*reqParam);
		PlatformMoveMemory((u1Byte *)reqParam + sizeof(*reqParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegResponseOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *rspParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req
	rspParam = (DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	rspParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	rspParam->Header.Revision = DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS_REVISION_1;
	rspParam->Header.Size = DOT11_SIZEOF_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS_REVISION_1;
	rspParam->DialogToken = param->ResponseParams.DialogToken;
	cpMacAddr(rspParam->PeerDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);
	rspParam->uSendTimeout = param->ResponseParams.SendTimeout;

	if(param->Optional.GONegotiationResponseInfo_IsPresent)
	{
		rspParam->Status = param->GONegotiationResponseInfo.ResponseParams.StatusCode;
		rspParam->GroupOwnerIntent.Intent = (UCHAR)param->GONegotiationResponseInfo.ResponseParams.GOIntent;
		rspParam->GroupOwnerIntent.TieBreaker = param->GONegotiationResponseInfo.ResponseParams.TieBreaker;
		rspParam->MinimumConfigTimeout.ClientTimeout = (UCHAR)param->GONegotiationResponseInfo.ResponseParams.ClientConfigTimeout;
		rspParam->MinimumConfigTimeout.GOTimeout = (UCHAR)param->GONegotiationResponseInfo.ResponseParams.GOConfigTimeout;
		cpMacAddr(rspParam->IntendedInterfaceAddress, param->GONegotiationResponseInfo.ResponseParams.IntendedInterfaceAddress.Address);
		rspParam->GroupCapability = param->GONegotiationResponseInfo.ResponseParams.GroupCapability;

		if(param->GONegotiationResponseInfo.Optional.GroupID_IsPresent)
		{
			cpMacAddr(rspParam->GroupID.DeviceAddress, param->GONegotiationResponseInfo.GroupID.DeviceAddress.Address);
			rspParam->GroupID.SSID.uSSIDLength = param->GONegotiationResponseInfo.GroupID.GroupSSID.ElementCount;
			PlatformMoveMemory(rspParam->GroupID.SSID.ucSSID, 
				param->GONegotiationResponseInfo.GroupID.GroupSSID.pElements, 
				param->GONegotiationResponseInfo.GroupID.GroupSSID.ElementCount);
			rspParam->bUseGroupID = TRUE;
		}
	}

	if(param->VendorIEs.ElementCount)
	{
		rspParam->uIEsLength = param->VendorIEs.ElementCount;
		rspParam->uIEsOffset = sizeof(*rspParam);
		PlatformMoveMemory((u1Byte *)rspParam + sizeof(*rspParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}


NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegConfirmOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *rspParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req
	rspParam = (DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	rspParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	rspParam->Header.Revision = DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS_REVISION_1;
	rspParam->Header.Size = DOT11_SIZEOF_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS_REVISION_1;
	rspParam->DialogToken = param->ResponseParams.DialogToken;
	cpMacAddr(rspParam->PeerDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);
	rspParam->uSendTimeout = param->ResponseParams.SendTimeout;

	if(param->Optional.GONegotiationConfirmationInfo_IsPresent)
	{
		rspParam->Status = param->GONegotiationConfirmationInfo.ConfirmationParams.StatusCode;
		rspParam->GroupCapability = param->GONegotiationConfirmationInfo.ConfirmationParams.GroupCapability;

		if(param->GONegotiationConfirmationInfo.Optional.GroupID_IsPresent)
		{
			cpMacAddr(rspParam->GroupID.DeviceAddress, param->GONegotiationConfirmationInfo.GroupID.DeviceAddress.Address);
			rspParam->GroupID.SSID.uSSIDLength = param->GONegotiationConfirmationInfo.GroupID.GroupSSID.ElementCount;
			PlatformMoveMemory(rspParam->GroupID.SSID.ucSSID, 
				param->GONegotiationConfirmationInfo.GroupID.GroupSSID.pElements, 
				param->GONegotiationConfirmationInfo.GroupID.GroupSSID.ElementCount);
			rspParam->bUseGroupID = TRUE;
		}
	}

	if(param->VendorIEs.ElementCount)
	{
		rspParam->uIEsLength = param->VendorIEs.ElementCount;
		rspParam->uIEsOffset = sizeof(*rspParam);
		PlatformMoveMemory((u1Byte *)rspParam + sizeof(*rspParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendInvitationRspOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_INVITATION_RESPONSE_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *rspParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_INVITATION_RESPONSE
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_INVITATION_RESPONSE;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_INVITATION_RESPONSE_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	rspParam = (DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	rspParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	rspParam->Header.Revision = DOT11_SEND_INVITATION_RESPONSE_PARAMETERS_REVISION_1;
	rspParam->Header.Size = DOT11_SIZEOF_SEND_INVITATION_RESPONSE_PARAMETERS_REVISION_1;
	cpMacAddr(rspParam->ReceiverDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);
	rspParam->DialogToken = param->ResponseParams.DialogToken;
	rspParam->RequestContext = NULL;
	rspParam->uSendTimeout = param->ResponseParams.SendTimeout;

	if(param->Optional.InvitationResponseInfo_IsPresent)
	{
		rspParam->Status = param->InvitationResponseInfo.ResponseParams.StatusCode;
		rspParam->MinimumConfigTimeout.ClientTimeout = (UCHAR)param->InvitationResponseInfo.ResponseParams.ClientConfigTimeout;
		rspParam->MinimumConfigTimeout.GOTimeout = (UCHAR)param->InvitationResponseInfo.ResponseParams.GOConfigTimeout;
		
		if(param->InvitationResponseInfo.Optional.GroupBSSID_IsPresent)
		{
			cpMacAddr(rspParam->GroupBSSID, param->InvitationResponseInfo.GroupBSSID.Address);
			rspParam->bUseGroupBSSID = TRUE;
		}

		if(param->InvitationResponseInfo.Optional.OperatingChannel_IsPresent)
		{
			rspParam->bUseSpecifiedOperatingChannel = TRUE;
			rspParam->OperatingChannel.ChannelNumber = (UCHAR)param->InvitationResponseInfo.OperatingChannel.ChannelNumber;
			rspParam->OperatingChannel.OperatingClass = param->InvitationResponseInfo.OperatingChannel.OperatingClass;
			PlatformMoveMemory(rspParam->OperatingChannel.CountryRegionString, 
				param->InvitationResponseInfo.OperatingChannel.CountryOrRegionString,
				sizeof(param->InvitationResponseInfo.OperatingChannel.CountryOrRegionString));
		}
	}
	
	if(param->VendorIEs.ElementCount)
	{
		rspParam->uIEsLength = param->VendorIEs.ElementCount;
		rspParam->uIEsOffset = sizeof(*rspParam);
		PlatformMoveMemory((u1Byte *)rspParam + sizeof(*rspParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}


NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendPDRspOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *rspParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE
	// Note: assume sizeof DOT11_RESET_REQUEST > DOT11_STATUS_INDICATION
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep reset req, ignore resetReq->dot11MacAddress here
	rspParam = (DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *)req->DATA.SET_INFORMATION.InformationBuffer;
	rspParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	rspParam->Header.Revision = DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS_REVISION_1;
	rspParam->Header.Size = DOT11_SIZEOF_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS_REVISION_1;
	cpMacAddr(rspParam->ReceiverDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);
	rspParam->DialogToken = param->ResponseParams.DialogToken;
	rspParam->RequestContext = NULL;
	rspParam->uSendTimeout = param->ResponseParams.SendTimeout;
	
	if(param->VendorIEs.ElementCount)
	{
		rspParam->uIEsLength = param->VendorIEs.ElementCount;
		rspParam->uIEsOffset = sizeof(*rspParam);
		PlatformMoveMemory((u1Byte *)rspParam + sizeof(*rspParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocIncomingAssociationDecisionOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE_PARAMETERS *param
	)
{
	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(DOT11_INCOMING_ASSOC_DECISION_V2) + param->VendorIEs.ElementCount// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	DOT11_INCOMING_ASSOC_DECISION_V2 *xlatdParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	// Fill in minimum info required by N62C_SET_OID_DOT11_INCOMING_ASSOCIATION_DECISION
	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId((WDI_MESSAGE_HEADER *)hTask->pInputBuffer);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_INCOMING_ASSOCIATION_DECISION;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(*xlatdParam);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	xlatdParam = (DOT11_INCOMING_ASSOC_DECISION_V2 *)req->DATA.SET_INFORMATION.InformationBuffer;
	xlatdParam->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	xlatdParam->Header.Revision = DOT11_INCOMING_ASSOC_DECISION_REVISION_2;
	xlatdParam->Header.Size = DOT11_SIZEOF_INCOMING_ASSOC_DECISION_REVISION_2;
	cpMacAddr(xlatdParam->PeerMacAddr, param->IncomingRequestInfo.AssocRequestParams.PeerMacAddress.Address);
	xlatdParam->bAccept = (0 != param->AssocResponseParameters.AcceptRequest);
	xlatdParam->usReasonCode = param->AssocResponseParameters.ReasonCode;
	if(param->VendorIEs.ElementCount)
	{
		xlatdParam->uAssocResponseIEsLength = param->VendorIEs.ElementCount;
		xlatdParam->uAssocResponseIEsOffset = sizeof(*xlatdParam);
		PlatformMoveMemory((u1Byte *)xlatdParam + sizeof(*xlatdParam),
			param->VendorIEs.pElements,
			param->VendorIEs.ElementCount);
	}

	return req;
}

NDIS_OID_REQUEST *
Wdi_Xlat_AllocWpsEnabledOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_SET_P2P_WPS_ENABLED_PARAMETERS *param
	)
{
	WDI_MESSAGE_HEADER			*wdiHdr = (WDI_MESSAGE_HEADER *)hTask->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	u1Byte						*buf = NULL;
	NDIS_OID_REQUEST			*req = NULL;
	WDI_TASK_PARAM				*taskParam = NULL;
	UINT						allocSize = sizeof(NDIS_OID_REQUEST) 	// the NDIS REQUEST
									+ sizeof(BOOLEAN)					// the input buffer
									+ sizeof(WDI_TASK_PARAM)
									;

	// Ref N62C_SET_OID_DOT11_WPS_ENABLED

	BOOLEAN						*xlatdParam = NULL;
	
	if(!wdi_xlat_Zalloc(&buf, allocSize))
		return NULL;

	req = (NDIS_OID_REQUEST *)buf;
	taskParam = wdi_xlat_ParamConstructor(buf, hTask, allocSize);
	WDI_XLAT_SET_OID_MP_RSVD_0(req, taskParam);

	req->RequestType = NdisRequestSetInformation;
	req->PortNumber = wdi_xlat_PortId(wdiHdr);
	req->DATA.METHOD_INFORMATION.Oid = OID_DOT11_WPS_ENABLED;
	req->DATA.METHOD_INFORMATION.InformationBuffer = buf + sizeof(*req);
	req->DATA.METHOD_INFORMATION.InputBufferLength = sizeof(*xlatdParam);
	req->DATA.METHOD_INFORMATION.OutputBufferLength = 0;

	// Prep req
	xlatdParam = (BOOLEAN *)req->DATA.SET_INFORMATION.InformationBuffer;
	*xlatdParam = param->WPSEnabled ? TRUE : FALSE;

	return req;
}

