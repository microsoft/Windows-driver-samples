#ifndef __INC_QOS_TYPE_H
#define __INC_QOS_TYPE_H


#define	MAX_WMMELE_LENGTH	64

typedef u4Byte QOS_MODE, *PQOS_MODE;
#define QOS_DISABLE			0
#define QOS_WMM			1
#define QOS_WMMSA			2
#define QOS_EDCA			4
#define QOS_HCCA			8
#define QOS_WMM_UAPSD		16   //WMM Power Save, 2006-06-14 Isaiah 

#define WMM_PARAM_ELE_BODY_LEN	18

#define MAX_STA_TS_COUNT			16
#define MAX_AP_TS_COUNT				32
#define QOS_TSTREAM_KEY_SIZE		13 // 1-byte TID | 6-byte RA | 6-byte TA

#define	WMM_ACTION_CATEGORY_CODE	17
#define WMM_PARAM_ELE_BODY_LEN		18

//
// TSID definition.
// Write your own specific TSID here. TSID 0~15 is Qos specific.
//
#define MAX_TSPEC_TSID				15
#define SESSION_REJECT_TSID			0xfe
#define DEFAULT_TSID				0xff

#define ADDTS_TIME_SLOT				100		// Time interval of per ADDTS reuest

#define	ACM_TIMEOUT					1000	// Time interval of ACM update
#define	SESSION_REJECT_TIMEOUT		60000

//
// QoS ACK Policy Field Values
// Ref: WMM spec 2.1.6: QoS Control Field, p.10.
//
typedef	enum _ACK_POLICY{
	eAckPlc0_ACK		= 0x00,
	eAckPlc1_NoACK		= 0x01,
}ACK_POLICY,*PACK_POLICY;

#define FRAME_OFFSET_QOS_CTRL(_pStart) 				(24 + 6*GET_80211_HDR_FROM_DS(_pStart)*GET_80211_HDR_TO_DS(_pStart))

#define GET_QOS_CTRL(_pStart)					ReadEF2Byte((pu1Byte)(_pStart) + FRAME_OFFSET_QOS_CTRL(_pStart))
#define SET_QOS_CTRL(_pStart, _value)				WriteEF2Byte((pu1Byte)(_pStart) + FRAME_OFFSET_QOS_CTRL(_pStart), _value)

// WMM control field.
#define GET_QOS_CTRL_WMM_UP(_pStart)				((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 3))
#define SET_QOS_CTRL_WMM_UP(_pStart, _value)			SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 3, (u1Byte)(_value))

#define GET_QOS_CTRL_WMM_EOSP(_pStart)				((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1))
#define SET_QOS_CTRL_WMM_EOSP(_pStart, _value)			SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_WMM_ACK_POLICY(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2))
#define SET_QOS_CTRL_WMM_ACK_POLICY(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2, (u1Byte)(_value))

// 802.11e control field (by STA, data)
#define GET_QOS_CTRL_STA_DATA_TID(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4))
#define SET_QOS_CTRL_STA_DATA_TID(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4, (u1Byte)(_value))

#define GET_QOS_CTRL_STA_DATA_QSIZE_FLAG(_pStart)		((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1))
#define SET_QOS_CTRL_STA_DATA_QSIZE_FLAG(_pStart, _value)	SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_STA_DATA_ACK_POLICY(_pStart)		((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2))
#define SET_QOS_CTRL_STA_DATA_ACK_POLICY(_pStart, _value)	SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2, (u1Byte)(_value))

#define GET_QOS_CTRL_STA_DATA_AMSDU(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 7, 1))
#define SET_QOS_CTRL_STA_DATA_AMSDU(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 7, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_STA_DATA_TXOP(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8))
#define SET_QOS_CTRL_STA_DATA_TXOP(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8, (u1Byte)(_value))

#define GET_QOS_CTRL_STA_DATA_QSIZE(_pStart)	GET_QOS_CTRL_STA_DATA_TXOP(_pStart)
#define SET_QOS_CTRL_STA_DATA_QSIZE(_pStart, _value)	SET_QOS_CTRL_STA_DATA_TXOP(_pStart)

// 802.11e control field (by HC, data)
#define GET_QOS_CTRL_HC_DATA_TID(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4))
#define SET_QOS_CTRL_HC_DATA_TID(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_DATA_EOSP(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1))
#define SET_QOS_CTRL_HC_DATA_EOSP(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_DATA_ACK_POLICY(_pStart)		((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2))
#define SET_QOS_CTRL_HC_DATA_ACK_POLICY(_pStart, _value)	SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_DATA_PS_BUFSTATE(_pStart)		((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8))
#define SET_QOS_CTRL_HC_DATA_PS_BUFSTATE(_pStart, _value)	SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8, (u1Byte)(_value))

// 802.11e control field (by HC, CFP)
#define GET_QOS_CTRL_HC_CFP_TID(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4))
#define SET_QOS_CTRL_HC_CFP_TID(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 0, 4, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_CFP_EOSP(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1))
#define SET_QOS_CTRL_HC_CFP_EOSP(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 4, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_CFP_ACK_POLICY(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2))
#define SET_QOS_CTRL_HC_CFP_ACK_POLICY(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 5, 2, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_CFP_USRSVD(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 7, 1))
#define SET_QOS_CTRL_HC_CFP_USRSVD(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 7, 1, (u1Byte)(_value))

#define GET_QOS_CTRL_HC_CFP_TXOP_LIMIT(_pStart)			((u1Byte)LE_BITS_TO_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8))
#define SET_QOS_CTRL_HC_CFP_TXOP_LIMIT(_pStart, _value)		SET_BITS_TO_LE_2BYTE((pu1Byte)(_pStart)+FRAME_OFFSET_QOS_CTRL(_pStart), 8, 8, (u1Byte)(_value))




#define SET_WMM_QOS_INFO_FIELD(_pStart, _val)	WriteEF1Byte(_pStart, _val)

#define GET_WMM_QOS_INFO_FIELD_PARAMETERSET_COUNT(_pStart)	LE_BITS_TO_1BYTE(_pStart, 0, 4)
#define SET_WMM_QOS_INFO_FIELD_PARAMETERSET_COUNT(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 0, 4, _val)

#define GET_WMM_QOS_INFO_FIELD_AP_UAPSD(_pStart)	LE_BITS_TO_1BYTE(_pStart, 7, 1)
#define SET_WMM_QOS_INFO_FIELD_AP_UAPSD(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 7, 1, _val)

#define GET_WMM_QOS_INFO_FIELD_STA_AC_VO_UAPSD(_pStart)	LE_BITS_TO_1BYTE(_pStart, 0, 1)
#define SET_WMM_QOS_INFO_FIELD_STA_AC_VO_UAPSD(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 0, 1, _val)

#define GET_WMM_QOS_INFO_FIELD_STA_AC_VI_UAPSD(_pStart)	LE_BITS_TO_1BYTE(_pStart, 1, 1)
#define SET_WMM_QOS_INFO_FIELD_STA_AC_VI_UAPSD(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 1, 1, _val)

#define GET_WMM_QOS_INFO_FIELD_STA_AC_BE_UAPSD(_pStart)	LE_BITS_TO_1BYTE(_pStart, 2, 1)
#define SET_WMM_QOS_INFO_FIELD_STA_AC_BE_UAPSD(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 2, 1, _val)
	
#define GET_WMM_QOS_INFO_FIELD_STA_AC_BK_UAPSD(_pStart)	LE_BITS_TO_1BYTE(_pStart, 3, 1)
#define SET_WMM_QOS_INFO_FIELD_STA_AC_BK_UAPSD(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 3, 1, _val)

#define GET_WMM_QOS_INFO_FIELD_STA_MAX_SP_LEN(_pStart)	LE_BITS_TO_1BYTE(_pStart, 5, 2)
#define SET_WMM_QOS_INFO_FIELD_STA_MAX_SP_LEN(_pStart, _val)	SET_BITS_TO_LE_1BYTE(_pStart, 5, 2, _val)
		

#define WMM_INFO_ELEMENT_SIZE	7

#define GET_WMM_INFO_ELE_OUI(_pStart)	((pu1Byte)(_pStart))
#define SET_WMM_INFO_ELE_OUI(_pStart, _pVal)	PlatformMoveMemory(_pStart, _pVal, 3);

#define GET_WMM_INFO_ELE_OUI_TYPE(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+3) ) )
#define SET_WMM_INFO_ELE_OUI_TYPE(_pStart, _val)	( *((pu1Byte)(_pStart)+3) = EF1Byte(_val) )

#define GET_WMM_INFO_ELE_OUI_SUBTYPE(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+4) ) )
#define SET_WMM_INFO_ELE_OUI_SUBTYPE(_pStart, _val)	( *((pu1Byte)(_pStart)+4) = EF1Byte(_val) )

#define GET_WMM_INFO_ELE_VERSION(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+5) ) )
#define SET_WMM_INFO_ELE_VERSION(_pStart, _val)	( *((pu1Byte)(_pStart)+5) = EF1Byte(_val) )

#define GET_WMM_INFO_ELE_QOS_INFO_FIELD(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+6) ) )
#define SET_WMM_INFO_ELE_QOS_INFO_FIELD(_pStart, _val)	( *((pu1Byte)(_pStart)+6) = EF1Byte(_val) )

// WMM TSPEC Element content
#define CLEAR_TSPEC(_tspec)	( PlatformZeroMemory((_tspec), TSPEC_SIZE) )

#define GET_TSPEC_ID(_tspec) EF1Byte( (_tspec)[0] )
#define SET_TSPEC_ID(_tspec, _value) ( (_tspec)[0] = EF1Byte(_value) )

#define GET_TSPEC_LENGTH(_tspec) EF1Byte( (_tspec)[1] )
#define SET_TSPEC_LENGTH(_tspec, _value) ( (_tspec)[1] = EF1Byte(_value) )

#define GET_TSPEC_OUI(_tspec, _value) \
	{ (_value)[0]=(_tspec)[2];  (_value)[1]=(_tspec)[3];  (_value)[2]=(_tspec)[4]; }
#define SET_TSPEC_OUI(_tspec, _value) \
	{ (_tspec)[2]=(_value)[0];  (_tspec)[3]=(_value)[1];  (_tspec)[4]=(_value)[2]; }

#define GET_TSPEC_OUI_TYPE(_tspec) EF1Byte( (_tspec)[5] )
#define SET_TSPEC_OUI_TYPE(_tspec, _value) ( (_tspec)[5] = EF1Byte(_value) )

#define GET_TSPEC_OUI_SUBTYPE(_tspec) EF1Byte( (_tspec)[6] )
#define SET_TSPEC_OUI_SUBTYPE(_tspec, _value) ( (_tspec)[6] = EF1Byte(_value) )

#define GET_TSPEC_VERSION(_tspec) EF1Byte( (_tspec)[7] )
#define SET_TSPEC_VERSION(_tspec, _value) ( (_tspec)[7] = EF1Byte(_value) )


#define GET_TSPEC_TSINFO_TRAFFIC_TYPE(_tspec) LE_BITS_TO_1BYTE( (_tspec)+8, 0, 1)
#define SET_TSPEC_TSINFO_TRAFFIC_TYPE(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+8, 0, 1, (_value) )

#define GET_TSPEC_TSINFO_TSID(_tspec) LE_BITS_TO_1BYTE( (_tspec)+8, 1, 4)
#define SET_TSPEC_TSINFO_TSID(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+8, 1, 4, (_value) )

#define GET_TSPEC_TSINFO_DIRECTION(_tspec) LE_BITS_TO_1BYTE( (_tspec)+8, 5, 2)
#define SET_TSPEC_TSINFO_DIRECTION(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+8, 5, 2, (_value) )

#define GET_TSPEC_TSINFO_ACCESS_POLICY_BIT0(_tspec) LE_BITS_TO_1BYTE( (_tspec)+8, 7, 1)
#define SET_TSPEC_TSINFO_ACCESS_POLICY_BIT0(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+8, 7, 1, (_value) )


#define GET_TSPEC_TSINFO_ACCESS_POLICY_BIT1(_tspec) LE_BITS_TO_1BYTE( (_tspec)+9, 0, 1)
#define SET_TSPEC_TSINFO_ACCESS_POLICY_BIT1(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+9, 0, 1, (_value) )

#define GET_TSPEC_TSINFO_AGGREGATION(_tspec) LE_BITS_TO_1BYTE( (_tspec)+9, 1, 1)
#define SET_TSPEC_TSINFO_AGGREGATION(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+9, 1, 1, (_value) )

#define GET_TSPEC_TSINFO_PSB(_tspec) LE_BITS_TO_1BYTE( (_tspec)+9, 2, 1)
#define SET_TSPEC_TSINFO_PSB(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+9, 2, 1, (_value) )

#define GET_TSPEC_TSINFO_UP(_tspec) LE_BITS_TO_1BYTE( (_tspec)+9, 3, 3)
#define SET_TSPEC_TSINFO_UP(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+9, 3, 3, (_value) )

#define GET_TSPEC_TSINFO_ACK_POLICY(_tspec) LE_BITS_TO_1BYTE( (_tspec)+9, 6, 2)
#define SET_TSPEC_TSINFO_ACK_POLICY(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+9, 6, 2, (_value) )


#define GET_TSPEC_TSINFO_SCHEDULE(_tspec) LE_BITS_TO_1BYTE( (_tspec)+10, 0, 1)
#define SET_TSPEC_TSINFO_SCHEDULE(_tspec, _value) SET_BITS_TO_LE_1BYTE( (_tspec)+10, 0, 1, (_value) )


#define GET_TSPEC_NOMINAL_MSDU_SIZE(_tspec) LE_BITS_TO_2BYTE( (_tspec)+11, 0, 16)
#define SET_TSPEC_NOMINAL_MSDU_SIZE(_tspec, _value) SET_BITS_TO_LE_2BYTE( (_tspec)+11, 0, 16, (_value) )

#define GET_TSPEC_MAX_MSDU_SIZE(_tspec) LE_BITS_TO_2BYTE( (_tspec)+13, 0, 16)
#define SET_TSPEC_MAX_MSDU_SIZE(_tspec, _value) SET_BITS_TO_LE_2BYTE( (_tspec)+13, 0, 16, (_value) )

#define GET_TSPEC_MIN_SERVICE_INTERVAL(_tspec) LE_BITS_TO_4BYTE( (_tspec)+15, 0, 32)
#define SET_TSPEC_MIN_SERVICE_INTERVAL(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+15, 0, 32, (_value) )

#define GET_TSPEC_MAX_SERVICE_INTERVAL(_tspec) LE_BITS_TO_4BYTE( (_tspec)+19, 0, 32)
#define SET_TSPEC_MAX_SERVICE_INTERVAL(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+19, 0, 32, (_value) )

#define GET_TSPEC_INACTIVITY_INTERVAL(_tspec) LE_BITS_TO_4BYTE( (_tspec)+23, 0, 32)
#define SET_TSPEC_INACTIVITY_INTERVAL(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+23, 0, 32, (_value) )

#define GET_TSPEC_SUSPENSION_INTERVAL(_tspec) LE_BITS_TO_4BYTE( (_tspec)+27, 0, 32)
#define SET_TSPEC_SUSPENSION_INTERVAL(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+27, 0, 32, (_value) )

#define GET_TSPEC_SERVICE_START_TIME(_tspec) LE_BITS_TO_4BYTE( (_tspec)+31, 0, 32)
#define SET_TSPEC_SERVICE_START_TIME(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+31, 0, 32, (_value) )

#define GET_TSPEC_MIN_DATA_RATE(_tspec) LE_BITS_TO_4BYTE( (_tspec)+35, 0, 32)
#define SET_TSPEC_MIN_DATA_RATE(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+35, 0, 32, (_value) )

#define GET_TSPEC_MEAN_DATA_RATE(_tspec) LE_BITS_TO_4BYTE( (_tspec)+39, 0, 32)
#define SET_TSPEC_MEAN_DATA_RATE(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+39, 0, 32, (_value) )

#define GET_TSPEC_PEAK_DATA_RATE(_tspec) LE_BITS_TO_4BYTE( (_tspec)+43, 0, 32)
#define SET_TSPEC_PEAK_DATA_RATE(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+43, 0, 32, (_value) )

#define GET_TSPEC_MAX_BURST_SIZE(_tspec) LE_BITS_TO_4BYTE( (_tspec)+47, 0, 32)
#define SET_TSPEC_MAX_BURST_SIZE(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+47, 0, 32, (_value) )

#define GET_TSPEC_DELAY_BOUND(_tspec) LE_BITS_TO_4BYTE( (_tspec)+51, 0, 32)
#define SET_TSPEC_DELAY_BOUND(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+51, 0, 32, (_value) )

#define GET_TSPEC_MIN_PHY_RATE(_tspec) LE_BITS_TO_4BYTE( (_tspec)+55, 0, 32)
#define SET_TSPEC_MIN_PHY_RATE(_tspec, _value) SET_BITS_TO_LE_4BYTE( (_tspec)+55, 0, 32, (_value) )

#define GET_TSPEC_SURPLUS_BANDWITH_ALLOWANCE(_tspec) LE_BITS_TO_2BYTE( (_tspec)+59, 0, 16)
#define SET_TSPEC_SURPLUS_BANDWITH_ALLOWANCE(_tspec, _value) SET_BITS_TO_LE_2BYTE( (_tspec)+59, 0, 16, (_value) )

#define GET_TSPEC_MEDIUM_TIME(_tspec) LE_BITS_TO_2BYTE( (_tspec)+61, 0, 16)
#define SET_TSPEC_MEDIUM_TIME(_tspec, _value) SET_BITS_TO_LE_2BYTE( (_tspec)+61, 0, 16, (_value) )

#define GET_TSPEC_TSINFO_ACCESS_POLICY(_tspec) \
	( (GET_TSPEC_TSINFO_ACCESS_POLICY_BIT1(_tspec) << 1) | GET_TSPEC_TSINFO_ACCESS_POLICY_BIT0(_tspec) )
#define SET_TSPEC_TSINFO_ACCESS_POLICY(_tspec, _value) \
	{\
		SET_TSPEC_TSINFO_ACCESS_POLICY_BIT0((_tspec), (_value)&0x01);\
		SET_TSPEC_TSINFO_ACCESS_POLICY_BIT1((_tspec), (_value)&0x02);\
	}


// TS Info of the WMM TSPEC Body
#define GET_TSINFO_TRAFFIC_TYPE(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo), 0, 1)
#define SET_TSINFO_TRAFFIC_TYPE(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo), 0, 1, (_value) )

#define GET_TSINFO_TSID(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo), 1, 4)
#define SET_TSINFO_TSID(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo), 1, 4, (_value) )

#define GET_TSINFO_DIRECTION(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo), 5, 2)
#define SET_TSINFO_DIRECTION(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo), 5, 2, (_value) )

#define GET_TSINFO_ACCESS_POLICY_BIT0(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo), 7, 1)
#define SET_TSINFO_ACCESS_POLICY_BIT0(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo), 7, 1, (_value) )


#define GET_TSINFO_ACCESS_POLICY_BIT1(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 1, 0, 1)
#define SET_TSINFO_ACCESS_POLICY_BIT1(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 1, 0, 1, (_value) )

#define GET_TSINFO_AGGREGATION(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 1, 1, 1)
#define SET_TSINFO_AGGREGATION(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 1, 1, 1, (_value) )

#define GET_TSINFO_PSB(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 1, 2, 1)
#define SET_TSINFO_PSB(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 1, 2, 1, (_value) )

#define GET_TSINFO_UP(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 1, 3, 3)
#define SET_TSINFO_UP(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 1, 3, 3, (_value) )

#define GET_TSINFO_ACK_POLICY(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 1, 6, 2)
#define SET_TSINFO_ACK_POLICY(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 1, 6, 2, (_value) )

#define GET_TSINFO_SCHEDULE(_tsinfo) LE_BITS_TO_1BYTE( (_tsinfo) + 2, 0, 1)
#define SET_TSINFO_SCHEDULE(_tsinfo, _value) SET_BITS_TO_LE_1BYTE( (_tsinfo) + 2, 0, 1, (_value) )


#define QOS_RATE_TO_BPS(_rate) ( (u4Byte)( ((_rate)*1000*1000)/2 ) )
#define QOS_BPS_TO_RATE(_bps)  ( (u1Byte)( ((_bps) * 2)/(1000*1000))) 



typedef enum {
	QOSIE_SRC_ADDTSREQ,
	QOSIE_SRC_ADDTSRSP,
	QOSIE_SRC_REASOCREQ,
	QOSIE_SRC_REASOCRSP,
	QOSIE_SRC_DELTS,
} QOSIE_SOURCE;


typedef u4Byte AC_CODING;
#define AC0_BE	0		// ACI: 0x00	// Best Effort
#define AC1_BK	1		// ACI: 0x01	// Background
#define AC2_VI	2		// ACI: 0x10	// Video
#define AC3_VO	3		// ACI: 0x11	// Voice
#define AC_MAX	4		// Max: define total number; Should not to be used as a real enum.

	
#define AC_PARAM_SIZE	4

#define GET_WMM_AC_PARAM_AIFSN(_pStart)	( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 0, 4) )
#define SET_WMM_AC_PARAM_AIFSN(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 0, 4, _val)

#define GET_WMM_AC_PARAM_ACM(_pStart)	( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 4, 1) )
#define SET_WMM_AC_PARAM_ACM(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 4, 1, _val)

#define GET_WMM_AC_PARAM_ACI(_pStart)		( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 5, 2) )
#define SET_WMM_AC_PARAM_ACI(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 5, 2, _val)

#define GET_WMM_AC_PARAM_ACI_AIFSN(_pStart)	( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 0, 8) )
#define SET_WMM_AC_PARAM_ACI_AIFSN(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 0, 8, _val)

#define GET_WMM_AC_PARAM_ECWMIN(_pStart)	( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 8, 4) )
#define SET_WMM_AC_PARAM_ECWMIN(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 8, 4, _val)

#define GET_WMM_AC_PARAM_ECWMAX(_pStart)	( (u1Byte)LE_BITS_TO_4BYTE(_pStart, 12, 4) )
#define SET_WMM_AC_PARAM_ECWMAX(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 12, 4, _val)

#define GET_WMM_AC_PARAM_TXOP_LIMIT(_pStart)		( (u2Byte)LE_BITS_TO_4BYTE(_pStart, 16, 16) )
#define SET_WMM_AC_PARAM_TXOP_LIMIT(_pStart, _val)	SET_BITS_TO_LE_4BYTE(_pStart, 16, 16, _val)



#define WMM_PARAM_ELEMENT_SIZE	(8+(4*AC_PARAM_SIZE))

#define GET_WMM_PARAM_ELE_OUI(_pStart)	((pu1Byte)(_pStart))
#define SET_WMM_PARAM_ELE_OUI(_pStart, _pVal)	PlatformMoveMemory(_pStart, _pVal, 3)

#define GET_WMM_PARAM_ELE_OUI_TYPE(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+3) ) )
#define SET_WMM_PARAM_ELE_OUI_TYPE(_pStart, _val)	( *((pu1Byte)(_pStart)+3) = EF1Byte(_val) )

#define GET_WMM_PARAM_ELE_OUI_SUBTYPE(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+4) ) )
#define SET_WMM_PARAM_ELE_OUI_SUBTYPE(_pStart, _val)	( *((pu1Byte)(_pStart)+4) = EF1Byte(_val) )

#define GET_WMM_PARAM_ELE_VERSION(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+5) ) )
#define SET_WMM_PARAM_ELE_VERSION(_pStart, _val)	( *((pu1Byte)(_pStart)+5) = EF1Byte(_val) )

#define GET_WMM_PARAM_ELE_QOS_INFO_FIELD(_pStart)	( EF1Byte( *((pu1Byte)(_pStart)+6) ) )
#define SET_WMM_PARAM_ELE_QOS_INFO_FIELD(_pStart, _val)	( *((pu1Byte)(_pStart)+6) = EF1Byte(_val) )

#define GET_WMM_PARAM_ELE_AC_PARAMS(_pStart)		( (pu1Byte)(_pStart)+8 )
#define SET_WMM_PARAM_ELE_AC_PARAMS(_pStart, _pVal) 	PlatformMoveMemory((_pStart)+8, _pVal, 16)

#define GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(_pStart, acIdx)		( ((pu1Byte)(_pStart))+8+ acIdx*4 )
#define SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(_pStart, acIdx, _pVal)	PlatformMoveMemory( ((pu1Byte)(_pStart))+8+ acIdx*4, _pVal, 4)

#define GET_WMM_PARAM_ELE_AC_PARAM(_pStart)	( (pu1Byte)(_pStart)+8 )
#define SET_WMM_PARAM_ELE_AC_PARAM(_pStart, _pVal) PlatformMoveMemory((_pStart)+8, _pVal, 16)


//
// QoS element subtype
//
typedef	enum _QOS_ELE_SUBTYPE{
	QOSELE_TYPE_INFO		= 0x00,		// 0x00: Information element
	QOSELE_TYPE_PARAM	= 0x01,		// 0x01: parameter element
}QOS_ELE_SUBTYPE,*PQOS_ELE_SUBTYPE;


//
// Direction Field Values.
// Ref: WMM spec 2.2.11: WME TSPEC Element, p.18.
//
typedef	enum _DIRECTION_VALUE{
	DIR_UP			= 0,		// 0x00	// UpLink
	DIR_DOWN		= 1,		// 0x01	// DownLink
	DIR_DIRECT		= 2,		// 0x10	// DirectLink
	DIR_BI_DIR		= 3,		// 0x11	// Bi-Direction
}DIRECTION_VALUE,*PDIRECTION_VALUE;


//
// TS Info field in WMM TSPEC Element.
// Ref:
//	1. WMM spec 2.2.11: WME TSPEC Element, p.18.
//	2. 8185 QoS code: QOS_TSINFO [def. in QoS_mp.h]
//

// Get/Set Value from Tspec Body
#define GET_TSPEC_BODY_TSINFO_TRAFFIC_TYPE(_TSpecBody)				LE_BITS_TO_1BYTE( (_TSpecBody), 0, 1)
#define SET_TSPEC_BODY_TSINFO_TRAFFIC_TYPE(_TSpecBody, _value)			SET_BITS_TO_LE_1BYTE( (_TSpecBody), 0, 1 , (_value))

#define GET_TSPEC_BODY_TSINFO_TSID(_TSpecBody)					LE_BITS_TO_1BYTE( (_TSpecBody), 1, 4)
#define SET_TSPEC_BODY_TSINFO_TSID(_TSpecBody, _value)				SET_BITS_TO_LE_1BYTE( (_TSpecBody), 1, 4 , (_value))

#define GET_TSPEC_BODY_TSINFO_DIRECTION(_TSpecBody)				LE_BITS_TO_1BYTE( (_TSpecBody), 5, 2)
#define SET_TSPEC_BODY_TSINFO_DIRECTION(_TSpecBody, _value)			SET_BITS_TO_LE_1BYTE( (_TSpecBody), 5, 2 , (_value))

#define GET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT0(_TSpecBody)			LE_BITS_TO_1BYTE( (_TSpecBody), 7, 1)
#define SET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT0(_TSpecBody, _value)		SET_BITS_TO_LE_1BYTE( (_TSpecBody), 7, 1 , (_value))

#define GET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT1(_TSpecBody)			LE_BITS_TO_1BYTE( (_TSpecBody)+1, 0, 1)
#define SET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT1(_TSpecBody, _value)		SET_BITS_TO_LE_1BYTE( (_TSpecBody)+1, 0, 1 , (_value))

#define GET_TSPEC_BODY_TSINFO_ACCESS_POLICY(_TSpecBody) \
		((GET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT1(_TSpecBody) << 1 ) | (GET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT0(_TSpecBody) ))
#define SET_TSPEC_BODY_TSINFO_ACCESS_POLICY(_TSpecBody, _value) \
		{\
			SET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT0((_TSpecBody), (_value) & 0x01); \
			SET_TSPEC_BODY_TSINFO_ACCESS_POLICY_BIT1((_TSpecBody), (_value) & 0x02); \
		}

#define GET_TSPEC_BODY_TSINFO_AGGREGATION(_TSpecBody)				LE_BITS_TO_1BYTE( (_TSpecBody)+1, 1, 1)
#define SET_TSPEC_BODY_TSINFO_AGGREGATION(_TSpecBody, _value)			SET_BITS_TO_LE_1BYTE( (_TSpecBody)+1, 1, 1 , (_value))
		
#define GET_TSPEC_BODY_TSINFO_PSB(_TSpecBody)					LE_BITS_TO_1BYTE( (_TSpecBody)+1, 2, 1)
#define SET_TSPEC_BODY_TSINFO_PSB(_TSpecBody, _value)				SET_BITS_TO_LE_1BYTE( (_TSpecBody)+1, 2, 1 , (_value))

#define GET_TSPEC_BODY_TSINFO_UP(_TSpecBody)					LE_BITS_TO_1BYTE( (_TSpecBody)+1, 3, 3)
#define SET_TSPEC_BODY_TSINFO_UP(_TSpecBody, _value)				SET_BITS_TO_LE_1BYTE( (_TSpecBody)+1, 3, 3 , (_value))

#define GET_TSPEC_BODY_TSINFO_ACK_POLICY(_TSpecBody)				LE_BITS_TO_1BYTE( (_TSpecBody)+1, 6, 2)
#define SET_TSPEC_BODY_TSINFO_ACK_POLICY(_TSpecBody, _value)			SET_BITS_TO_LE_1BYTE( (_TSpecBody)+1, 6, 2 , (_value))

#define GET_TSPEC_BODY_TSINFO_SCHEDULE(_TSpecBody)				LE_BITS_TO_1BYTE( (_TSpecBody)+2, 0, 1)
#define SET_TSPEC_BODY_TSINFO_SCHEDULE(_TSpecBody, _value)			SET_BITS_TO_LE_1BYTE( (_TSpecBody)+2, 0, 1 , (_value))



//
// WMM TSPEC Element.
// Ref: WMM spec 2.2.11: WME TSPEC Element, p.16.
//
//
// WMM TSPEC Element Format:
// | ID 		| Length 	| OUI 	| OUI Type 	| OUI SubType 	| Version 	| WMM TSPEC body 	|
// | 1 Byte 	| 1 Byte 	| 3 Bytes 	| 1 Byte 		| 1 Byte 		| 1 Byte 	| 55 Bytes 			|
//
// Note:
//	We never use the stucture allocated in the procedure because we do not ensure that the size of the allocated
//	memory exactly matches our demand. In general, when allocating memory with the size of odd nubmber for the
//	stuct or union operations, the OS always reserves 1 byte appended to the stucture. Packets manipulation
//	by filling memory with the stucure is dangerous. By Bruce, 2008-03-15.
//

#define TSPEC_SIZE		(2+6+55)
typedef u1Byte			WMM_TSPEC[TSPEC_SIZE], *PWMM_TSPEC;


//
// ACM implementation method.
// Annie, 2005-12-13.
//
typedef	enum _ACM_METHOD{
	eAcmWay0_SwAndHw		= 0,		// By SW and HW.
	eAcmWay1_HW				= 1,		// By HW.
	eAcmWay2_SW				= 2,		// By SW.
}ACM_METHOD,*PACM_METHOD;


typedef struct _ACM{
//	BOOLEAN		RegEnableACM;
	u8Byte		UsedTime;
	u8Byte		MediumTime;
	BOOLEAN		HwAcmCtl;	// TRUE: UsedTime exceed => Do NOT USE this AC. It wll be written to ACM_CONTROL(0xBF BIT 0/1/2 in 8185B).
}ACM, *PACM;

typedef	u1Byte		AC_UAPSD, *PAC_UAPSD;

#define	GET_VO_UAPSD(_apsd) ((_apsd) & BIT0)
#define	SET_VO_UAPSD(_apsd) ((_apsd) |= BIT0)
	
#define	GET_VI_UAPSD(_apsd) ((_apsd) & BIT1)
#define	SET_VI_UAPSD(_apsd) ((_apsd) |= BIT1)
	
#define	GET_BK_UAPSD(_apsd) ((_apsd) & BIT2)
#define	SET_BK_UAPSD(_apsd) ((_apsd) |= BIT2)

#define	GET_BE_UAPSD(_apsd) ((_apsd) & BIT3)
#define	SET_BE_UAPSD(_apsd) ((_apsd) |= BIT3)
	
typedef	u1Byte		AC_NOACK, *PAC_NOACK;

// Maps of AC NoAck and Bits
#define	GET_VO_NOACK(_AC) ((_AC) & BIT0)
#define	SET_VO_NOACK(_AC) ((_AC) |= BIT0)
	
#define	GET_VI_NOACK(_AC) ((_AC) & BIT1)
#define	SET_VI_NOACK(_AC) ((_AC) |= BIT1)
	
#define	GET_BK_NOACK(_AC) ((_AC) & BIT2)
#define	SET_BK_NOACK(_AC) ((_AC) |= BIT2)

#define	GET_BE_NOACK(_AC) ((_AC) & BIT3)
#define	SET_BE_NOACK(_AC) ((_AC) |= BIT3)

// Translate WMM(QOS) UP to RT defined AC BITs
#define WMMUP_TO_RT_AC_BIT(_UP)	\
	((6 == (_UP) || 7 == (_UP)) ? BIT0 :	\
	(4 == (_UP) || 5 == (_UP)) ? BIT1 :	\
	(1 == (_UP) || 2 == (_UP)) ? BIT2 :	\
	(0 == (_UP) || 3 == (_UP)) ? BIT3 : 0)

#ifdef REMOVE_PACK
#pragma pack(1) 
#endif

//typedef struct _TCLASS{
// TODO
//} TCLASS, *PTCLASS;
typedef union _QOS_TCLAS{

	struct _TYPE_GENERAL{
		u1Byte		Priority;
		u1Byte 		ClassifierType;
		u1Byte 		Mask;
	} TYPE_GENERAL;

	struct _TYPE0_ETH{
		u1Byte		Priority;
		u1Byte 		ClassifierType;
		u1Byte 		Mask;
		u1Byte		SrcAddr[6];
		u1Byte		DstAddr[6];
		u2Byte		Type;
	} TYPE0_ETH;

	struct _TYPE1_IPV4{
		u1Byte		Priority;
		u1Byte 		ClassifierType;
		u1Byte 		Mask;
		u1Byte 		Version;
		u1Byte		SrcIP[4];
		u1Byte		DstIP[4];
		u2Byte		SrcPort;
		u2Byte		DstPort;
		u1Byte		DSCP;
		u1Byte		Protocol;
		u1Byte		Reserved;
	} TYPE1_IPV4;

	struct _TYPE1_IPV6{
		u1Byte		Priority;
		u1Byte 		ClassifierType;
		u1Byte 		Mask;
		u1Byte 		Version;
		u1Byte		SrcIP[16];
		u1Byte		DstIP[16];
		u2Byte		SrcPort;
		u2Byte		DstPort;
		u1Byte		FlowLabel[3];
	} TYPE1_IPV6;

	struct _TYPE2_8021Q{
		u1Byte		Priority;
		u1Byte 		ClassifierType;
		u1Byte 		Mask;
		u2Byte		TagType;
	} TYPE2_8021Q;
} QOS_TCLAS, *PQOS_TCLAS;

#ifdef REMOVE_PACK
#pragma pack() 
#endif

//
// Information about a traffic stream
//
typedef struct _QOS_TSTREAM{
	//
	// We will use TSID, RA and TA as key to hash.
	//
	DECLARE_RT_HASH_ENTRY;

	BOOLEAN			bUsed;
	u1Byte			UserPriority;
	RT_LIST_ENTRY	BufferedPacketList;
	u2Byte			MsduLifetime;
	BOOLEAN			bEstablishing;
	u1Byte			TimeSlotCount;
	u1Byte			DialogToken;
	WMM_TSPEC		TSpec;
	WMM_TSPEC		OutStandingTSpec;
	u1Byte			NominalPhyRate;
} QOS_TSTREAM, *PQOS_TSTREAM;


//
// STA QoS data.
// Ref: DOT11_QOS in 8185 code. [def. in QoS_mp.h]
//
typedef struct _STA_QOS{
	DECLARE_RT_OBJECT(STA_QOS);

	u1Byte					WMMIEBuf[MAX_WMMELE_LENGTH];
	OCTET_STRING			WMMIE;

	// Part 1. Self QoS Mode.
	QOS_MODE				QosCapability; //QoS Capability, 2006-06-14 Isaiah 
	QOS_MODE				QosCapabilityBackup; //QoS CapabilityBackup

	QOS_MODE				CurrentQosMode;

	// For WMM Power Save Mode : 
	// ACs are trigger/delivery enabled or legacy power save enabled. 2006-06-13 Isaiah
	AC_UAPSD				b4ac_Uapsd;  //VoUapsd(bit0), ViUapsd(bit1),  BkUapsd(bit2), BeUapsd(bit3),
	AC_UAPSD				Curr4acUapsd;
	BOOLEAN					bInServicePeriod;
	BOOLEAN					bWmmMoreData; // Mark as the final trigger-enabled qos data from the AP.
	u1Byte					MaxSPLength;
	
	// Part 2. EDCA Parameter (perAC)
	pu1Byte					pWMMInfoEle;
	u1Byte					WMMParamEle[WMM_PARAM_ELEMENT_SIZE];


	// Part 3. ACM
	ACM						acm[4];
	ACM_METHOD				AcmMethod;
	RT_TIMER				ACMTimer;

	// Part 4. Per TID (Part 5: TCLASS will be described by TStream)
	QOS_TSTREAM				StaTsArray[MAX_STA_TS_COUNT]; // Traffic Stream objects for STA mode.
	RT_HASH_TABLE_HANDLE	hApTsTable; // Traffic Stream objects for AP mode. 
	u1Byte					DialogToken;
	WMM_TSPEC				TSpec;
	
	u1Byte					QBssWirelessMode;

	// No Ack Setting
	AC_NOACK				AcNoAck;

	//For admission control.
	RT_TIMER				AddTsTimer;
}STA_QOS, *PSTA_QOS;


//
// Ref: 802.11e 7.3.2.13, Figure 46.1
//
#define QBSS_LOAD_SIZE 5
#define GET_QBSS_LOAD_STA_COUNT(__pStart) ReadEF2Byte(__pStart)
#define SET_QBSS_LOAD_STA_COUNT(__pStart, __Value) WriteEF2Byte(__pStart, __Value)
#define GET_QBSS_LOAD_CHNL_UTILIZATION(__pStart) ReadEF1Byte((pu1Byte)(__pStart) + 2)
#define SET_QBSS_LOAD_CHNL_UTILIZATION(__pStart, __Value) WriteEF1Byte((pu1Byte)(__pStart) + 2, __Value)
#define GET_QBSS_LOAD_AVAILABLE_CAPACITY(__pStart) ReadEF2Byte((pu1Byte)(__pStart) + 3)
#define SET_QBSS_LOAD_AVAILABLE_CAPACITY(__pStart, __Value) WriteEF2Byte((pu1Byte)(__pStart) + 3, __Value)


//
// BSS QOS data.
// Ref: BssDscr in 8185 code. [def. in BssDscr.h]
//
typedef struct _BSS_QOS{

	// Part 0. Ref. 8185 QoS code (From Emily)
	QOS_MODE				bdQoSMode;
	u1Byte					bdWMMIEBuf[MAX_WMMELE_LENGTH];
	OCTET_STRING			bdWMMIE;

	QOS_ELE_SUBTYPE		EleSubType;

	// Part 2. EDCA Parameter (perAC)
	pu1Byte					pWMMInfoEle;
	pu1Byte					pWMMParamEle;

	// QBSS Load.
	u1Byte					QBssLoad[QBSS_LOAD_SIZE];
	BOOLEAN					bQBssLoadValid;
}BSS_QOS, *PBSS_QOS;


#define 	sQoSCtlLng	2
#define	QOS_CTRL_LEN(_QosMode)		( (_QosMode > QOS_DISABLE)? sQoSCtlLng : 0 )


//Added by joseph
#define UP2AC(up)	( (up<3) ? ((up==0)?1:0) : (up>>1) )
#define IsACValid(ac)		( ( ac>=0 && ac<=7 )? TRUE : FALSE )

#endif // #ifndef __INC_QOS_TYPE_H
