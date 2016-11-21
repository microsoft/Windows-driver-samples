#ifndef __INC_BATYPE_H
#define __INC_BATYPE_H

#define 	TOTAL_TXBA_NUM	16
#define	TOTAL_RXBA_NUM	16

//temp set from 200 ms to 800 ms, for rental house test. by Maddest 20130105
#define	BA_SETUP_TIMEOUT		800

#define	BA_INACT_TIMEOUT		60000

#define	BA_POLICY_DELAYED		0
#define	BA_POLICY_IMMEDIATE		1

#define	ADDBA_STATUS_SUCCESS		0
#define	ADDBA_STATUS_REFUSED		37
#define	ADDBA_STATUS_INVALID_PARAM	38

#define	DELBA_REASON_QSTA_LEAVING	36
#define	DELBA_REASON_END_BA		37
#define	DELBA_REASON_UNKNOWN_BA		38
#define	DELBA_REASON_TIMEOUT		39

//3 BA Dialog Token
#define FRAME_OFFSET_BA_DIALOG_TOKEN					(sMacHdrLng+2)
#define GET_BA_FRAME_DIALOG_TOKEN(_pStart)				( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_DIALOG_TOKEN)
#define GET_BA_FRAME_STATUS_CODE(_pStart)				( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_DIALOG_TOKEN+1)

//3 Block Ack Parameter Set

#define FRAME_OFFSET_BA_PARAM_SET(_pStart)				(sMacHdrLng+(( GET_ACTFRAME_ACTION_CODE(_pStart) == 0)? 3:5) )  // BAReq:BARsp

#define GET_BA_FRAME_PARAM_SET(_pStart)					( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart))

#define GET_BA_FRAME_PARAM_SET_AMSDU_SUPPORT(_pStart)			((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 0, 1))
#define SET_BA_FRAME_PARAM_SET_AMSDU_SUPPORT(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 0, 1, (u1Byte)(_value))

#define GET_BA_FRAME_PARAM_SET_BA_POLICY(_pStart)			((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 1, 1))
#define SET_BA_FRAME_PARAM_SET_BA_POLICY(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 1, 1, (u1Byte)(_value))

#define GET_BA_FRAME_PARAM_SET_TID(_pStart)				((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 2, 4))
#define SET_BA_FRAME_PARAM_SET_TID(_pStart, _value)			SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 2, 4, (u1Byte)(_value))

#define GET_BA_FRAME_PARAM_SET_BUF_SIZE(_pStart)			((u2Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 6, 10))
#define SET_BA_FRAME_PARAM_SET_BUF_SIZE(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart), 6, 10, (u2Byte)(_value))

// Block Ack Timeout Value
#define GET_BA_FRAME_TIMEOUT_VALUE(_pStart)				( ((pu1Byte)(_pStart))+FRAME_OFFSET_BA_PARAM_SET(_pStart)+2)

//BA Starting Sequence Control Field
#define GET_BAREQ_FRAME_START_SQECTRL(_pStart)				( ((pu1Byte)(_pStart))+sMacHdrLng+7)

#define GET_BA_START_SQECTRL_FIELD_SEQ_NUM(_pStart)			((u2Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart)), 4, 12))
#define SET_BA_START_SQECTRL_FIELD_SEQ_NUM(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart)), 4, 12, (u2Byte)(_value))

//2 GET/SET BA Parameter Set Field
#define GET_BA_PARAM_SET_FIELD_AMSDU_SUPPORT(_pStart)			((u1Byte)LE_BITS_TO_2BYTE(_pStart, 0, 1))
#define SET_BA_PARAM_SET_FIELD_AMSDU_SUPPORT(_pStart, _value)		SET_BITS_TO_LE_2BYTE( _pStart, 0, 1, _value)

#define GET_BA_PARAM_SET_FIELD_BA_POLICY(_pStart)			((u1Byte)LE_BITS_TO_2BYTE(_pStart, 1, 1))
#define SET_BA_PARAM_SET_FIELD_BA_POLICY(_pStart, _value)		SET_BITS_TO_LE_2BYTE(_pStart, 1, 1, _value)

#define GET_BA_PARAM_SET_FIELD_TID(_pStart)				((u1Byte)LE_BITS_TO_2BYTE(_pStart, 2, 4))
#define SET_BA_PARAM_SET_FIELD_TID(_pStart, _value)			SET_BITS_TO_LE_2BYTE( _pStart, 2, 4, _value)

#define GET_BA_PARAM_SET_FIELD_BUF_SIZE(_pStart)			((u2Byte)LE_BITS_TO_2BYTE( _pStart, 6, 10))
#define SET_BA_PARAM_SET_FIELD_BUF_SIZE(_pStart, _value)		SET_BITS_TO_LE_2BYTE( _pStart, 6, 10, (u2Byte)(_value))

//3 DELBA Parameter Set
#define GET_DELBA_FRAME_PARAM_SET(_pStart)				ReadEF2Byte( ((pu1Byte)(_pStart))+sMacHdrLng+2)
#define SET_DELBA_FRAME_PARAM_SET(_pStart, _value)			WriteEF2Byte( ((pu1Byte)(_pStart))+sMacHdrLng+2, _value)

#define GET_DELBA_FRAME_PARAM_SET_INITIATOR(_pStart)			((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 11, 1))
#define SET_DELBA_FRAME_PARAM_SET_INITIATOR(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 11, 1, (u1Byte)(_value))

#define GET_DELBA_FRAME_PARAM_SET_TID(_pStart)				((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 12, 4))
#define SET_DELBA_FRAME_PARAM_SET_TID(_pStart, _value)			SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 12, 4, (u1Byte)(_value))

//2 GET/SET DELBA Parameter Set Field
#define GET_DELBA_PARAM_SET_FIELD_INITIATOR(_pStart)			((u1Byte)LE_BITS_TO_2BYTE(_pStart, 11, 1))
#define SET_DELBA_PARAM_SET_FIELD_INITIATOR(_pStart, _value)		SET_BITS_TO_LE_2BYTE( _pStart, 11, 1, _value)

#define GET_DELBA_PARAM_SET_FIELD_TID(_pStart)				((u1Byte)LE_BITS_TO_2BYTE(_pStart, 12, 4))
#define SET_DELBA_PARAM_SET_FIELD_TID(_pStart, _value)			SET_BITS_TO_LE_2BYTE(_pStart, 12, 4, _value)

// BlockAckReq frame parameters
#define GET_BAR_PARAM_CTRL_FIELD_BAR_ACK_POLICY(_pStart)				((u1Byte)LE_BITS_TO_2BYTE(_pStart, 0, 1))
#define GET_BAR_PARAM_CTRL_FIELD_MULTI_TID(_pStart)					((u1Byte)LE_BITS_TO_2BYTE(_pStart, 1, 1))
#define GET_BAR_PARAM_CTRL_FIELD_COMPRESSED_BITMAP(_pStart)			((u1Byte)LE_BITS_TO_2BYTE(_pStart, 2, 1))
#define GET_BAR_PARAM_CTRL_FIELD_GCR(_pStart)							((u1Byte)LE_BITS_TO_2BYTE(_pStart, 3, 1))
#define GET_BAR_PARAM_CTRL_FIELD_TID_INFO(_pStart)						((u1Byte)LE_BITS_TO_2BYTE(_pStart, 12, 4))

#define GET_BAR_PARAM_INFO_FIELD_FRAG_NUM(_pStart)					((u1Byte)LE_BITS_TO_2BYTE(_pStart, 0, 4))
#define GET_BAR_PARAM_INFO_FIELD_STARTING_SEQ_NUM(_pStart)			((u2Byte)LE_BITS_TO_2BYTE(_pStart, 4, 12))

typedef enum _BAR_ACK_POLICY{
	BAR_NORMAL_ACK = 0,
	BAR_NO_ACK = 1,
}BAR_ACK_POLICY;

typedef enum _BAR_TYPE{
	BAR_TYPE_BASIC_BAR,
	BAR_TYPE_COMPRESSED_BAR,
	BAR_TYPE_EXT_COMPRESSED_BAR,
	BAR_TYPE_MULTI_TID_BAR,
	BAR_TYPE_GCR_BAR,
}BAR_TYPE;

typedef struct _BA_RECORD {
	RT_TIMER			Timer;
	BOOLEAN				bValid;
	u1Byte				DialogToken;
	u2Byte				BaParamSet;
	u2Byte				BaTimeoutValue;
	u2Byte				BaStartSeqCtrl;
	u2Byte				BufferSize;	// The max number of packets(MPDUs) in an AMPDU
} BA_RECORD, *PBA_RECORD;


#endif
