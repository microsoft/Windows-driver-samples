/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    PrecompInc.h

Abstract:
    Header fils to include. Note that, this file is only intend to 
	be included by Precomp.h
    
Major Change History:
      When        Who	What
    ----------	------	----------------------------------------------
    2006-09-26	Rcnjko	Created
    2006-11-06	Rcnjko	Add bus concept.
    2010-12-17	Roger	Add SDIO bus related header included requirements.

Notes:

--*/

#ifndef __INC_PRECOMPINC_H
#define __INC_PRECOMPINC_H

//
// Common defintion for every platform.
//
#define WPA2 1

#ifdef DBG
#undef  WPP_SOFTWARE_TRACE
#define WPP_SOFTWARE_TRACE 		0
#endif


#ifndef DBG_CMD
	#define DBG_CMD					1
#endif
//=============================================

//
// Header files required by PlatformDef.h.
//
#include "GeneralDef.h"
#include "LinkList.h"
#include "StatusCode.h"
#include "Object.h"
#include "HashTable.h"


//Isaiah for MacOS 
#ifndef	_MACHINE_ENDIAN_H
#define	__MACHINE_LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	__MACHINE_BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net, ppc */
#define	__MACHINE_PDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#endif /* _MACHINE_ENDIAN_H */


#define INTEL_WIDI_SUPPORT			0

//
// Platform dependent header files.
//	
	#ifndef	BYTE_ORDER
	#define BYTE_ORDER	__MACHINE_LITTLE_ENDIAN
	#endif
	
	#define USE_WORKITEM 1

	// This flag shall only be turned on for DTM testing.
	#define POWER_MAN	1

		#define VISTA_USB_TEMP	0
		#define USE_WDF_SDIO 0
		#define N6_FAKE_AP_MODE 1 // 061229, rcnjko: Make Native WiFi IM driver to take us an IBSS when we are working at AP mode.


		#define POWER_MAN	1

		//
		// <Roger_Notes> For SDIO compilation consideration, we should incluede system initguid.h header first before ntddsd.h is included 
		// to prevent unresolved external symbol _GUID_SDBUS_INTERFACE_STANDARD error occurs in linking process.
		// 2012.01.17.
		//		
		#include <ndis.h>
		#include <windef.h>
		#include <usbdi.h>

	#include <dot11wdi.h>				// defined in wdk
	#include <wditypes.hpp> 			// defined in wdk
	#include <wlan\1.0\TLVGeneratorParser.hpp>	// defined in wdk

		#include <initguid.h>
		#include <wdf.h>
		#include <wdfusb.h>
		#include <WdfMiniport.h>
		#include <wdftimer.h>
		#include <ntddsd.h>

		#include "N6C_PlatformDef.h"
		#include "PlatformDef.h"
		#include "NdisComm.h"
		#include "NdisDbg.h"
		#include "NdisOid.h"

		#include "N6Sdio_dbg.h"
		#include "N6Sdio_typedef.h"
		#include "N6SdioPlatformWindows.h"
		#include "N6Sdio_info.h"
		#include "N6Sdio_main.h"
		#include "CCX_Extension.h"
		#include "CCX_Predef.h"
		#include "Ndis6Common.h"
		#include "N6C_Init.h"
		#include "N6C_Req.h"
		#include "N6C_OidQuery.h"
		#include "N6C_OidSet.h"
		#include "N6C_Oids.h"
		#include "WDI_Common.h"

		#include "ntintsafe.h"
		#include "N62C_def.h"	
		#include "N62C_Init.h"
		#include "N62C_Port.h"
		#include "N62C_QueryOid.h"
		#include "N62C_SetOid.h"
		#include "N62C_Oids.h"
		#include "N62C_QuerySetOid.h"
		#include "N62C_AP_Def.h"
		#include "N62C_AP.h"
		
		#include "N63C_SendAction.h"
		#include "N63C_Oids.h"
	
	
		#include "WDI_def.h"
		#include "N6Sdio_WdiMain.h"
		#include "WDI_SendAction.h"
		#include "WDI_Cmds.h"
		#include "WDI_Extension.h"
		#include "WDI_Xlat.h"

	
	#include "Ndis_OID.h"
	
	#include <Ntstrsafe.h>

#define	MS_SUPPORT		0


//
// Platform indenpent header files.
//

#include "ActionTimer.h"
#include "Protocol802_11.h"
#include "Wapi.h"
#include "ChannelType.h"
#include "ChannelInfo.h"
#include "HTType.h"
#include "Dot11d.h"
#include "Ethernet.h"
#include "GeneralFunc.h"
#include "EndianFree.h"
#include "HalDef.h"
#include "HalTxFeedback.h"
#include "HalMacID.h"
#include "SecurityType.h"
#include "QoSType.h"
//#include "BT.h"
#include "RtChnlPlan.h"
#include "Frame_Buf.h"
#include "Pool.h"
#include "CustomizedScan.h"
#include "OffChnlTx.h"
#include "P2P_AdditionalIe.h"
#include "P2P_TempPublic.h"
#include "P2P.h"
#include "P2P_Public.h"
#include "P2PSvc.h"
#include "Hotspot20.h"
#include "DrvLogImp.h"
#include "MimoPs.h"
#include "BAType.h"
#include "TDLSType.h"
#include "TSType.h"
#include "BAGen.h"
#include "TDLSGen.h"
#include "TcpCheck.h"
#include "TCPOFFLOAD_Typt.h"
//#include "IntelClassRoom.h"
#include "MultiPorts.h"
#include "MultiChannels.h"
#include "WPS.h"
#include "VHTGen.h"
#include "Debug.h"
#include "DbgMon.h"
#include "UsbModeSwitch.h"
#include "DFS.h"
#include "SMBios.h"
#include "TypeDef.h"
#include "HalEfuse.h"
//#include "TxShortcut.h"
//#include "RxShortcut.h"
#include "HTGen.h"
#include "AMSDU_Aggregation.h"
#include "TSGen.h"
#include "Widi.h"
#include "WiDiType.h"
#include "Debug.h"
#include "HalEEPROM.h"

#include "HalPG.h"
#include "QosGen.h"
#include "DriverInterfaceDef.h"
#include "TransmitDesc.h"
#include "Transmit.h"
#include "Receive.h"
#include "MgntGen.h"
#include "MgntConstructPacket.h"
#include "MgntSendPacket.h"
#include "MgntLink.h"
#include "MgntEngine.h"
#include "Defrag.h"
#include "SecurityGen.h"
#include "RC4.h"
#include "ApEngine.h"
#include "Authenticator.h"
#include "IOTGen.h"

#include "MgntActQueryParam.h"
#include "MgntActSetParam.h"
#include "HalCam.h"

#include "1x_rc4.h"
#include "1x_md5c.h"
#include "1x_kmsm_eapolkey.h"
#include "1x_eapol.h"

#include "RmDef.h"
#include "RadioMeasurement.h"

#include "ROGUEAP.h"

#include "PowerSave.h"

#include "DbgCmd.h"

#include "CcxType.h"
#include "CcxGen.h"
#include "BssCoexistence.h"
#include "AES_OCB.h"
#include "AES_rijndael.h"
#include "Supplicant.h"
#include "TCPOFFLOADGen.h"
#include "RxReorder.h"

#include "ParserGen.h"

#include "WOLPattern.h"

#include "sha256.h"
#include "Dot11RAES.h"

#include "HalSIC.h"
#include "HalPwrSeqCmd.h"
#include "HalFw.h"

#include "HalPhy.h"

#include "phydm_precomp.h"
#include "phydm.h"


#include "HalComDef.h"
#include "HalComDesc.h"
#include "HalComPhyCfg.h"
#include "HalComTxPwrCfg.h"
#include "HalComTxPwrLmtCfg.h"
#include "HalADCSampling.h"


#include "HalComLed.h"
#include "HalComReg.h"
#include "HalComFirmware.h"
#include "HalPowerSave.h"
#include "HalWoWLAN.h"
#include "HalH2cIOCmd.h"

#include "HalIcGlue.h"

#if (RTL8723B_SUPPORT == 1)
#include "Hal8723BEfuse.h"
#include "Hal8723BPhyCfg.h"
#include "Hal8723BPhyReg.h"
#include "Hal8723BPwrSeq.h"
#include "Hal8723BDesc.h"
#include "Hal8723BDefCom.h"
#include "Hal8723BFirmware.h"
#include "Hal8723BRf.h"

	#include "Hal8723BSdioLed.h"
	#include "Hal8723BSdioDef.h"

#endif

#include "HalJaguarPhyReg.h"




//
// Hal Bus dependent.
//
// How about rtl8192S series ??
// For ODM structure, we need to move HAL interace.h to lower position. Because
// it need to use odm structure declared in odm.h
//
		
	#include "PlatformSdio.h"


#include "HalHWImg.h"
#include "HalDM.h"
#include "halCmdPkt.h"
#include "halDbgCmd.h"


#include "HalSdio.h"



#include "HalPhyRf_WIN.h"
//#include "HalPhyRf_8188e_WIN.h"
//#include "HalPhyRf_8812A_WIN.h"	
//#include "HalPhyRf_8821A_WIN.h"
//#include "HalPhyRf_8821B.h"
//#include "HalPhyRf_8192e_WIN.h"
//#include "HalPhyRf_8814a_WIN.h"
#include "HalPhyRf_8723B_WIN.h"
//#include "HalPhyRf_8703B.h"
//#include "HalPhyRf_8723D.h"


#include "phydm_debug.h"

#if SOFTWARE_TRACE_LOGGING
//TraceLogging
#include <evntrace.h>
#include <TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(g_hProvider);
#endif


#endif //end of #ifndef __INC_PRECOMPINC_H
