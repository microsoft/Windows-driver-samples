#ifndef __INC_BSSCOEXISTENCE_H
#define __INC_BSSCOEXISTENCE_H

// Normally, the OS (windows but not including win7 or later) scans per 60 seconds,
// and we don't want the period smaller than the windows period to affect the throughput.
// In WiFi 11n 5.2.48 BSS coexistence test, the scan interval is 180 seconds.
#define	RT_OBSS_MIN_TRIGGER_SCAN_INTERVAL		70		// 70 Seconds

#define SET_BSS_COEXISTENCE_ELE_INFO_REQUEST(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart), 0, 1, _val)
#define SET_BSS_COEXISTENCE_ELE_FORTY_INTOLERANT(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart), 1, 1, _val)
#define SET_BSS_COEXISTENCE_ELE_20_WIDTH_REQ(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE((_pEleStart), 2, 1, _val)
#define SET_BSS_COEXISTENCE_ELE_OBSS_EXEMPTION_GRT(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart), 4, 1, _val)
#define GET_BSS_COEXISTENCE_ELE_FORTY_INTOLERANT(_pEleStart)			LE_BITS_TO_1BYTE((_pEleStart), 1, 1)	
#define GET_BSS_COEXISTENCE_ELE_20_WIDTH_REQ(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart), 2, 1)
#define GET_BSS_COEXISTENCE_ELE_OBSS_EXEMPTION_GRT(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart), 4, 1)

#define SET_BSS_INTOLERANT_ELE_REG_CLASS(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart), 0, 8, _val)
#define SET_BSS_INTOLERANT_ELE_CHANNEL(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 0, 8, _val)

#define SET_EXT_CAPABILITY_ELE_BSS_COEXIST(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart), 0, 1, _val)
#define GET_EXT_CAPABILITY_ELE_BSS_COEXIST(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart), 0, 1)

#define GET_OBSS_PARAM_ELE_SCAN_INTERVAL(_pEleStart)					ReadEF2Byte(((_pEleStart)+4))
VOID
BSS_ParsingOBSSInfoElement(
	PADAPTER		Adapter,
	OCTET_STRING	osFrame,
	PRT_WLAN_BSS	pBssDesc
	);

VOID
BSS_ParsingBSSCoexistElement(
	PADAPTER		Adapter,
	OCTET_STRING	osFrame,
	PRT_WLAN_BSS	pBssDesc
	);

VOID
BSS_AppendExentedCapElement(
	PADAPTER		pAdapter,
	POCTET_STRING	posFrame
	);

RT_STATUS
OnBssCoexistence(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

VOID
BSS_OnScanComplete(
	IN	PADAPTER		Adapter
	);

VOID
BSS_IdleScanWatchDog(
	IN	PADAPTER		pAdapter
	);
#endif
