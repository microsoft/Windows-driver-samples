#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Receive.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported: public
//-----------------------------------------------------------------------------

BOOLEAN
p2p_validate_Beacon(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;
	
	do
	{
		if(!FrameBuf_Length(&msg->p2pAttributes))
		{
			break;
		}
		
		if(!msg->_devId) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev id attr\n"));
			break;
		}

		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		bValid = TRUE;
	}while(FALSE);

	return bValid;
}

BOOLEAN
p2p_validate_ProbeReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!FrameBuf_Length(&msg->p2pAttributes))
		{
			break;
		}
		
		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		if(!msg->_opChannel && TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER))
		{
			// PF #2, some vender don't send ProbeReq with op channel
			// do not filter
		}
		
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_ProbeRsp(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!FrameBuf_Length(&msg->p2pAttributes))
		{
			break;
		}
		
		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}
		
		if(!msg->_devInfo) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			break;
		}

		if(!msg->_dsParam) // from P2P_OnProbersp
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no ds param ie\n"));
			break;
		}

		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_AssocReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}
		
		if(!msg->_devInfo) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			break;
		}

		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_AssocRsp(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		/*if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}*/
			
		/*if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}*/
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_Deauth(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;
	
	do
	{
		/*if(!msg->_minorReasonCode) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no minor reason code attr\n"));
			break;
		}*/
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_GoNegReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		if(!msg->_goIntent) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no go intent attr\n"));
			break;
		}

		if(!msg->_configTimeout) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no config timeout attr\n"));
			break;
		}

		if(!msg->_listenChannel) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no listen channel attr\n"));
			break;
		}

		if(!msg->_intendedIntfAddr) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no intended interface address attr\n"));
			break;
		}

		if(!msg->_channelList) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no channel list attr\n"));
			break;
		}

		if(!msg->_devInfo) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			break;
		}

		if(!msg->_opChannel) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no op channel attr\n"));
			break;
		}

		if(!msg->_wpsDevPasswordId)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no WPS device password id attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_GoNegRsp(
	IN  const P2P_INFO			*info,
	IN  P2P_MESSAGE				*msg
	)
{
	int 						go = 0;

	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}

		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		if(!msg->_goIntent) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no go intent attr\n"));
			break;
		}

		go = p2p_go_det(info->GOIntent, msg->goIntent);

		if(!msg->_intendedIntfAddr) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no intended interface address attr\n"));
			break;
		}

		if(!msg->_channelList) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no channel list attr\n"));
			break;
		}

		if(!msg->_devInfo) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			break;
		}

		if(!msg->_grpId && !go)
		{// peer is GO and no group id
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group id attr, self intent: %u, peer intent: %u\n", info->GOIntent, msg->goIntent));
			break;
		}

		if(!msg->_opChannel && !go)
		{// peer is GO and no op channel
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no op channel attr, self intent: %u, peer intent: %u\n", info->GOIntent, msg->goIntent));
			break;
		}

		if(P2P_SC_SUCCESS == msg->status && !msg->_wpsDevPasswordId)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("status is success but no WPS device password id attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_GoNegConfirm(
	IN  BOOLEAN					go,
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}

		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		if(!msg->_channelList) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no channel list attr\n"));
			break;
		}

		if(!msg->_grpId && !go)
		{// peer is GO and no group id
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group id attr when peer will be go\n"));
			break;
		}

		if(!msg->_opChannel) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no op channel attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_InvitationReq(
	IN  P2P_MESSAGE				*msg
	)
{
	P2P_ROLE					invitorRole;
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_configTimeout) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no config timeout attr\n"));
			break;
		}

		if(!msg->_grpId) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group id attr\n"));
			break;
		}

		invitorRole = (eqMacAddr(msg->grpDevAddr, msg->sa)) ? (P2P_GO) : (P2P_CLIENT);

		if(!msg->_grpBssid && P2P_GO == invitorRole)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group bssid attr\n"));
			break;
		}

		if(!msg->_channelList) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no channel list attr\n"));
			break;
		}

		if(!msg->_devInfo) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			break;
		}

		if(!msg->_grpId) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group id attr\n"));
			break;
		}

		if(!msg->_opChannel && P2P_GO == invitorRole) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no op channel attr when invitor role is GO\n"));
			break;
		}

		if(!msg->_invitationFlags) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no invitation flags attr\n"));
			break;
		}

		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_InvitationRsp(
	IN  P2P_ROLE				invitorRole,
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}

		if(!msg->_channelList) 
		{
			// Although the channel list attribute is mandatory for the successful case of
			// invitation response, actually the information is not important because the
			// pairing devices can go to next association by the OP channel attribute.
			// Then, we defined it as optional and this would fix the invitation failure
			// with Netgear PTV3000 (FW:2.4.12 or later) under Win8.
			// By Bruce, 2013-09-03.
		}

		if(!msg->_opChannel && P2P_CLIENT == invitorRole && 0 == msg->status)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no op channel attr when invitor role is GO\n"));
			break;
		}

		if(!msg->_grpBssid && P2P_CLIENT == invitorRole && 0 == msg->status)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group bssid attr when invitor role is GO\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_DevDiscReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_devId) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no device id attr\n"));
			break;
		}

		if(!msg->_grpId) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no group id attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_DevDiscRsp(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_PDReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_capability) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no capability attr\n"));
			break;
		}

		if(!msg->_devInfo) 
		{
			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr\n"));
			//break;
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dev info attr, use sa as devAddr\n"));
			msg->devAddr = msg->sa;
		}

		if(!msg->_wpsConfigMethods)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no WPS config method attr\n"));
			break;
		}

		// TODO: if this is a PD req to join our GO, group id is mandatory
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_PDRsp(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_wpsConfigMethods)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no WPS config method attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_PresenceReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_noa)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no noa attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_PresenceRsp(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
		
		if(!msg->_status) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no status attr\n"));
			break;
		}

		if(!msg->_noa)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no noa attr\n"));
			break;
		}
	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

BOOLEAN
p2p_validate_GoDiscoverabilityReq(
	IN  P2P_MESSAGE				*msg
	)
{
	BOOLEAN 					bValid = FALSE;

	do
	{
		if(!msg->_dialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("no dialog token\n"));
			break;
		}
	
	// Thter is no Elements field in a GO Discoverability Request frame

	
		bValid = TRUE;
	}while(FALSE);
	
	return bValid;
}

#endif
