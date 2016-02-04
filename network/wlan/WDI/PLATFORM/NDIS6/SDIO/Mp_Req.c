#include "Mp_Precomp.h"
#include "802_11_OID.h"
#include "CustomOid.h"

NDIS_STATUS
InterfaceSetInformationHandleCustomizedOriginalMPSetOid(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	PADAPTER				Adapter = (PADAPTER)MiniportAdapterContext;
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PMGNT_INFO      			pMgntInfo = &(Adapter->MgntInfo);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

    case OID_802_11_NUMBER_OF_ANTENNAS:
			//TODO:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_NUMBER_OF_ANTENNAS: \n"));
			break;

    case OID_802_11_RX_ANTENNA_SELECTED:
			//TODO:
			if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(ULONG)) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG);
				goto set_oid_exit;
			}	
			else
			{
				*BytesRead = sizeof(ULONG);
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_RX_ANTENNA_SELECTED: \n"));
			break;

    case OID_802_11_TX_ANTENNA_SELECTED:
			//TODO:
			if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(ULONG)) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG);
				goto set_oid_exit;
			}	
			else
			{
				*BytesRead = sizeof(ULONG);
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_TX_ANTENNA_SELECTED: \n"));
			break;

	case OID_802_11_BSSID_LIST_SCAN:
			//
			// Note! this OID is also implemented in N6CSetInformation(). 
			// Since, we had some special requirement (which may not be necessary) for 8187, 
			// we implement it here instead of using that in N6CSetInformation().
			// 2005.10.16, by rcnjko.
			//

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_802_11_BSSID_LIST_SCAN.\n"));

			// Sometimes we also need site survey when chariot running, comment out by Bruce, 2007-06-28.
			// 061010, by rcnjko.
			// if(pMgntInfo->LinkDetectInfo.bBusyTraffic && pMgntInfo->bMediaConnect)
			//{ 
			//	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_802_11_BSSID_LIST_SCAN (Immediate return because traffic is busy now)\n"));
			//	goto set_oid_exit;
			// }

			MgntActSet_802_11_BSSID_LIST_SCAN( Adapter );



			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_802_11_BSSID_LIST_SCAN.\n"));
			break;
			
    case OID_RT_SCAN_AVAILABLE_BSSID:
			//
			// Note! this OID is also implemented in N6CSetInformation(). 
			// Since, we had some special requirement (which may not be necessary) for 8187, 
			// we implement it here instead of using that in N6CSetInformation().
			// 2005.10.16, by rcnjko.
			//
			
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SCAN_AVAILABLE_BSSID.\n"));

			// 061010, by rcnjko.
			if(pMgntInfo->LinkDetectInfo.bBusyTraffic && pMgntInfo->bMediaConnect)
			{ 
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SCAN_AVAILABLE_BSSID (Immediate return because traffic is busy now)\n"));
				goto set_oid_exit;
			}

			MgntActSet_802_11_BSSID_LIST_SCAN( Adapter );

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SCAN_AVAILABLE_BSSID.\n"));
			break;	

	case OID_RT_PRO_READ_REGISTRY:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_READ_REGISTRY.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			PRT_NDIS_DBG_CONTEXT pDbgCtx = &(Adapter->ndisDbgCtx);

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;

				goto set_oid_exit;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			// Read MAC register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgReadMacReg(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_READ_REGISTRY.\n"));
		break;

	case OID_RT_PRO_WRITE_REGISTRY:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_WRITE_REGISTRY.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;
				
				goto set_oid_exit;
			}
			// Get offset, data width, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write MAC register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteMacReg(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_WRITE_REGISTRY.\n"));
		break;


	case OID_RT_PRO_READ_BB_REG:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_READ_BB_REG.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulBeOFDM;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;

				goto set_oid_exit;
			}
			// Get offset, and type of BB register to read.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulBeOFDM = *((ULONG*)InformationBuffer+1);
			// Write BB register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgReadBbReg(GetDefaultAdapter(Adapter), ulRegOffset, ulBeOFDM))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_READ_BB_REG.\n"));
		break;

	case OID_RT_PRO_WRITE_BB_REG:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_WRITE_BB_REG.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulBeOFDM;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;
				
				goto set_oid_exit;
			}
			// Get offset, type of BB register, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulBeOFDM = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write BB register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteBbReg(GetDefaultAdapter(Adapter), ulRegOffset, ulBeOFDM, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_WRITE_BB_REG.\n"));
		break;	

	case OID_RT_PRO_RF_READ_REGISTRY:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_RF_READ_REGISTRY.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;
				
				goto set_oid_exit;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			// Read RF register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgReadRfReg(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_RF_READ_REGISTRY.\n"));
		break;

	case OID_RT_PRO_RF_WRITE_REGISTRY:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_RF_WRITE_REGISTRY.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;

				goto set_oid_exit;
			}
			// Get offset, data width, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write RF register asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteRfReg(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_RF_WRITE_REGISTRY.\n"));
		break;

	case OID_RT_PRO_READ_EEPROM:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_READ_EEPROM.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;
				
				goto set_oid_exit;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			// Read EEPROM asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgReadEeprom(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_READ_EEPROM.\n"));
		break;

	case OID_RT_PRO_WRITE_EEPROM:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_WRITE_EEPROM.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;

				goto set_oid_exit;
			}
			// Get offset, data width, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write EEPROM asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteEeprom(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_WRITE_EEPROM.\n"));
		break;

	case OID_RT_PRO_READ_EFUSE:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_READ_EFUSE.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;
				
				goto set_oid_exit;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			// Read EFUSE asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgReadEFuse(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("<=== Set OID_RT_PRO_READ_EFUSE.\n"));
		break;

	case OID_RT_PRO_WRITE_EFUSE:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_WRITE_EFUSE.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;

				goto set_oid_exit;
			}
			// Get offset, data width, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write EFUSE asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteEFuse(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("<=== Set OID_RT_PRO_WRITE_EFUSE.\n"));
		break;

	case OID_RT_PRO_UPDATE_EFUSE:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_UPDATE_EFUSE.\n"));
		
		if(!DbgUpdateEFuse(Adapter))
		{
			Status = NDIS_STATUS_NOT_ACCEPTED;
		}
		break;

	case OID_RT_PRO_READ_EFUSE_BT:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_READ_EFUSE_BT.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;
				
				goto set_oid_exit;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			// Read EFUSE asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgReadBTEFuse(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("<=== Set OID_RT_PRO_READ_EFUSE_BT.\n"));
		break;

	case OID_RT_PRO_WRITE_EFUSE_BT:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_WRITE_EFUSE_BT.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	ulRegValue;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;

				goto set_oid_exit;
			}
			// Get offset, data width, and value to write.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			ulRegValue = *((ULONG*)InformationBuffer+2);
			// Write EFUSE asynchronously.
			// Caller should use OID_RT_PRO8187_WI_POLL to make sure IO is completed. 
			if(!DbgWriteBTEFuse(GetDefaultAdapter(Adapter), ulRegOffset, ulRegDataWidth, ulRegValue))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("<=== Set OID_RT_PRO_WRITE_EFUSE_BT.\n"));
		break;

	case OID_RT_PRO_UPDATE_EFUSE_BT:
		RT_TRACE(COMP_OID_SET|COMP_EFUSE, DBG_LOUD, ("===> Set OID_RT_PRO_UPDATE_EFUSE_BT.\n"));
		
		if(!DbgUpdateBTEFuse(Adapter))
		{
			Status = NDIS_STATUS_NOT_ACCEPTED;
		}
		break;

	case OID_RT_11N_USB_TX_AGGR_NUM:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_11N_USB_TX_AGGR_NUM.\n"));
#if TX_AGGREGATION
		{
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			u1Byte					numValue;
			u4Byte	value32;
			numValue = *((u1Byte *)InformationBuffer);
			if ( numValue> 0x0f){
				Status = NDIS_STATUS_INVALID_DATA;
				goto set_oid_exit;
			}
			
			pHalData->UsbTxAggDescNum = numValue;
			

			value32 = PlatformEFIORead4Byte(Adapter, REG_TDECTRL);
			value32 = value32 & ~(BLK_DESC_NUM_MASK << BLK_DESC_NUM_SHIFT);
			value32 |= ((pHalData->UsbTxAggDescNum & BLK_DESC_NUM_MASK) << BLK_DESC_NUM_SHIFT);
		
			PlatformEFIOWrite4Byte(Adapter, REG_TDECTRL, value32);
		
		}
#endif
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_11N_USB_TX_AGGR_NUM.\n"));
		break;

	case OID_RT_PRO_SET_ANTENNA_BB:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_SET_ANTENNA_BB.\n"));
		{
			u1Byte	antennaIndex;
			
			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*1)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*1;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_SET_ANTENNA_BB. return!!\n"));
				return Status;				
			}
			
			// Get antenna index.
			antennaIndex = *((UCHAR*)InformationBuffer);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_PRO_SET_ANTENNA_BB, antennaIndex(%#x)\n", antennaIndex));
			if(!DbgSetTxAntenna(Adapter, antennaIndex))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
			
			// Initialize antenna test 
			pMgntInfo->AntennaTest = 1;
			Adapter->RxStats.PWDBAllCnt = Adapter->RxStats.PWDBAllOldCnt = 0;
			Adapter->RxStats.RssiCalculateCnt = Adapter->RxStats.RssiOldCalculateCnt = 0;
	
			Adapter->HalFunc.SetTxAntennaHandler(Adapter, antennaIndex);
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_SET_ANTENNA_BB.\n"));
		break;

	case OID_RT_SDIO_REG_CTRL:
		{
			u1Byte	SdioRegCtrl = *(pu1Byte)InformationBuffer;
			
			pDevice->SdioRegDbgCtrl = SdioRegCtrl & SDIO_REG_CTRL_MASK; 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SDIO_REG_CTRL: SdioRegCtrl(%d)\n", pDevice->SdioRegDbgCtrl));
		}
		break;
	}

set_oid_exit:

	return Status;
}
