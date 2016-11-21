#ifndef __INC_RMTYPE_H
#define __INC_RMTYPE_H

#define	CCX_RM_CONFG_DELAY		20 // in ms, determine the needed time to config HW time before performing RM reuest.

//
// Radio Measurement Requests.
//
#define	MAX_RM_REQ_ELES_BUF	1600

#define TSM_TIMER_PERIOD	100 // in ms

#define CCX4_TSM_MIN_INTERVAL 977 // in TU, Ref CCX4 S56.5.1,  1 sec = 977 TU.
#define CCX4_TSM_MAX_INTERVAL 9766 // in TU, Ref CCX4 S56.5.1,  10 sec = 9766 TU.
#define	DOT11K_NUM_RPI	11

//
// RM basic type. 11k or CCX.
//
typedef enum _RT_RM_BASIC_TYPE{
	RT_RM_BASIC_CCX,	// CCX type
	RT_RM_BASIC_11K,	// 11k type
	RT_RM_BASIC_LOCAL,	// Local perform 11k rm
}RT_RM_BASIC_TYPE;

typedef struct _RT_RM_REQUESTS{
	// RM basic type (11k or CCX)
	RT_RM_BASIC_TYPE	RmBasicType;

	// Source address from the request
	u1Byte				SrcAddr[6];
	
	// Destination of the Radio Measurement Request frame. 
	u1Byte				DestAddr[6];	

	// Maximal measurement duration in these requests, in TU.
	u2Byte				MaxDuration;

	// Relative start time in us. 
	u4Byte				StartTime;
	 
	// Dialog Token.
	u2Byte				DialogToken;

	// Buffer of Measurement Request elements.
	u1Byte				MeasReqElesBuf[MAX_RM_REQ_ELES_BUF];
	OCTET_STRING		osMeasReqEles;

	// # of Repeatitions of measurements of dot11k RM
	u2Byte				Repetitions;
}RT_RM_REQUESTS, *PRT_RM_REQUESTS;


//
// Radio Measurement Reports.
//
#define	MAX_RM_RPT_ELES_BUF	1500 
typedef struct _RT_RM_REPORTS{
	// This rm report is valid
	BOOLEAN				bValid;

	// Autonomous measurement report or not.
	BOOLEAN				bAutonomous;

	// Dialog Token.
	u2Byte				DialogToken;

	// Buffer of Measurement Request elements.
	u1Byte				MeasRptElesBuf[MAX_RM_RPT_ELES_BUF];
	OCTET_STRING		osMeasRptEles;
}RT_RM_REPORTS, *PRT_RM_REPORTS;

typedef struct _TSM_INFO{
	u1Byte				State; // 1: Enable, 0: disable.
	u2Byte				MeasurementInterval; // in TU.
	u2Byte				TimeOutTickNum; // # of tick before timeout, unit of tick is TSM_TIMER_PERIOD. 
	u2Byte				TickCounter; // A counter to keep tick passed since last timeout event.
}TSM_INFO, *PTSM_INFO;

//
// Wrapper class of Radio Measurement related stuff.
//
typedef struct _RT_RM_INFO{
	DECLARE_RT_OBJECT(RT_RM_INFO);

	// TRUE if we are CCX RM capable, FALSE o.w.. 
	BOOLEAN					bRegCcxRm;

	// TRUE if current BSS is CCX Radio Measurement Enable, FALSE o.w..	
	BOOLEAN					bBssCcxRmEnable;

	// TRUE if we are doing one of request in RmRequests, FALSE otherwise. 
	// Note that, it is protected by RT_RM_SPINLOCK.
	BOOLEAN					bGoingOn;	

	// Postive value means upper limit of non-serving channel measurment duration,
	// in unit of TU. 0 means unlimited, postive value 
	u2Byte					OffLineDurUpLimit;

	// Radio Measurement Requests to handle.
	RT_RM_REQUESTS			RmRequests; 

	// Radio Measurement Reports to RmRequests.
	RT_RM_REPORTS			RmReports;

	
	// Hash table to keep frame report result.
	// Note that, hFrameReportTable and related operation are protected by RT_RM_SPINLOCK.
	RT_HASH_TABLE_HANDLE	hFrameReportTable;

	// TRUE if we have to update singal strength of the received frame.
	BOOLEAN					bMonitorStaPower;

	// Traffic Stream Metrics related.
	TSM_INFO				TsmInfo[MAX_STA_TS_COUNT]; // Information about TSM for each TS.
	RT_TIMER				TsmTimer; // TSM timer, see TSM_TIMER_PERIOD for its timeout period.
	u1Byte					TsmTimerRefCnt; // Number of TS using TsmTimer.  
	RT_RM_REPORTS			TsmReport; // Buffer and setting for the IAPP frame with TSM report.

	// CCX Radio Measurement /dot11k RM workitem
	RT_WORK_ITEM			RmWorkitem;

	// Dot11k max measurment duration
	u2Byte					RrmMaxOpChnlDur;		// RRM Operating Channel Max Measurement Duration
	u2Byte					RrmMaxNonOpChnlDur;	// RRM Non-Operating Channel Max Measurement Duration.
}RT_RM_INFO, *PRT_RM_INFO;


// Parameters for HAL to perform a Radio Measurement.
typedef struct _RT_RM_REQ_PARAM{
	// TRUE to clear previous result before starting measurement;
	// FALSE to accumulate measurement result.
	BOOLEAN				bToClear; 

	// Measurement duration in TU.
	u1Byte				Duration;
}RT_RM_REQ_PARAM, *PRT_RM_REQ_PARAM;

// Ref: CCX 2 spec, Table S36-5
#define CCX_RM_STATE_NORMAL 1

// Ref: CCX 2 spec, Table S36-7
#define	BCN_REQ_PASSIVE_SCAN	0
#define	BCN_REQ_ACTIVE_SCAN		1
#define	BCN_REQ_BEACON_TABLE	2


// Ref: CCX 2 spec, Figure S36-6
typedef struct _BEACON_REQUEST{
	u1Byte				ChannelNumber;
	u1Byte				ScanMode;
	u2Byte				MeasurementDuration; // In TU.
}BEACON_REQUEST, *PBEACON_REQUEST;


// Ref: CCX 2 spec, Table 36-9
#define	BCN_RPT_PHY_FH				1
#define	BCN_RPT_PHY_DSS				2
#define	BCN_RPT_PHY_UNUSED			3
#define	BCN_RPT_PHY_OFDM			4
#define	BCN_RPT_PHY_HIGH_RATE_DSS	5
#define	BCN_RPT_PHY_ERP				6

//
// Max number of STA to monitor, it is bound to size of frame report:
// (1514 - 12 - 24 - 8 - 4) / 14 = 104.14. 
// I pick 64 for computaion time and memory space consideration.
// 070608, by rcnjko.
//
#define MAX_NUM_FRPT_ELEMENT 64 


// Ref: CCX 2 spec, Figure S36-14.
typedef struct _FRPT_ELEMENT{
	u1Byte				TA[6]; // MAC address of transmitter STA.
	u1Byte				Bssid[6]; // BSSID of the transmitter STA.
	s1Byte				AvgRxPower; // In dBm. Average signal strength of the 802.11 frame recevied fro mthe transmitter STA.
	u1Byte				NumFrames; // Number of frames received from the transmitter STA.
}FRPT_ELEMENT, *PFRPT_ELEMENT;

// Ref: CCX 2 spec, Figure S36-7.
typedef struct _FRAME_REQUEST{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.
}FRAME_REQUEST, *PFRAME_REQUEST;



typedef struct _FRAME_REPORT{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.
	FRPT_ELEMENT		Element[1];
}FRAME_REPORT, *PFRAME_REPORT;

//
// Signal strength of a STA.
// It is a value object used in the hash table, hFrameReportTable. 
//
typedef struct _RT_RM_STA_POWER{
	//
	// We will use TA and BSSID as key to hash.
	//
	DECLARE_RT_HASH_ENTRY;
	
	int					RxSignalSum; // in dBm, sum of signal power of frames received.
	int					NumFrames; // number of frames received.
}RT_RM_STA_POWER, *PRT_RM_STA_POWER;

//
// Hash function associated with RT_RM_STA_POWER.
//
#define RM_STA_RX_POWER_KEY_SIZE 12



// Ref: CCX 2 spec, Figure S36-8
typedef struct _CHANNEL_LOAD_REQUEST{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.
}CHANNEL_LOAD_REQUEST, *PCHANNEL_LOAD_REQUEST;


// Ref: CCX2 spec, Figure S36-15
typedef struct _CHANNEL_LOAD_REPORT{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.
	u1Byte				CcaBusyFraction; // 0 - 255.
}CHANNEL_LOAD_REPORT, *PCHANNEL_LOAD_REPORT;


// Ref: CCX 2 spec, Figure S36-9
typedef struct _NOISE_HISTOGRAM_REQUEST{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.
}NOISE_HISTOGRAM_REQUEST, *PNOISE_HISTOGRAM_REQUEST;


// Ref: CCX 2 spec, Table 36-11
#define	CCX_RPI_0_POWER		-87
#define	CCX_RPI_1_POWER		-82
#define	CCX_RPI_2_POWER		-77
#define	CCX_RPI_3_POWER		-72
#define	CCX_RPI_4_POWER		-67
#define	CCX_RPI_5_POWER		-62
#define	CCX_RPI_6_POWER		-57


// Ref: CCX 2 spec, Figure S36-16
#define NUM_RPI			8
typedef struct _NOISE_HISTOGRAM_REPORT{
	u1Byte				ChannelNumber;
	u1Byte				Spare;
	u2Byte				MeasurementDuration; // In TU.

	// Each entry is an index ranged from 0 to 255.
	u1Byte				RpiDensity[NUM_RPI];
}NOISE_HISTOGRAM_REPORT, *PNOISE_HISTOGRAM_REPORT;


// Ref: CCX 2 spec, Figure S36-5 
#define	RM_REQ_MODE_PARALLEL	BIT0
#define	RM_REQ_MODE_ENABLE		BIT1
#define	RM_REQ_MODE_REPORT		BIT3


// Ref: CCX 2 spec, Table S36-6.
#define	RM_TYPE_UNUSED			0
#define	RM_TYPE_CHANNEL_LOAD	1
#define	RM_TYPE_NOISE_HISTOGRAM	2
#define	RM_TYPE_BEACON			3
#define	RM_TYPE_FRAME			4
#define	RM_HIDDEN_NODE			5
#define RM_TYPE_TSM_V4			6
#define RM_TYPE_TSM_V5			8
#define	RM_TYPE_STATISTICS		10	// CCXv5 S66-Roaming and Real-time Diagnostics Event Log for STA statistics


//
// Measurement Request Element.
// Ref: CCX 2 spec, S36.7
//
typedef struct _MEASUREMENT_REQUEST_ELEMENT{
	u2Byte				MeasurementToken;
	u1Byte				MeasurementMode;
	u1Byte				MeasurementType;
	u1Byte				MeasurementRequest[1];
}MEASUREMENT_REQUEST_ELEMENT, *PMEASUREMENT_REQUEST_ELEMENT;


// Ref: CCX 2 spec, Figure S36-12 
#define	RM_RPT_MODE_PARALLEL	BIT0
#define	RM_RPT_MODE_INCAPABLE	BIT1
#define	RM_RPT_MODE_REFUSED		BIT2


//
// Measurement Report Element.
// Ref: CCX 2 spec, S36.13
// Measurement Report format:
//	|| Element ID || Length || Measurement Token || Measurement Mode || Measurement Type || Measurement Report ||
//	||  2 Byte	|| 2 Byte ||  2 Byte                   ||  1 Byte                 ||  1 Byte                ||  veriable                 ||
//
typedef struct _MEASUREMENT_REPORT_ELEMENT{
	u2Byte				MeasurementToken;
	u1Byte				MeasurementMode;
	u1Byte				MeasurementType;
	u1Byte				MeasurementReport[1];
}MEASUREMENT_REPORT_ELEMENT, *PMEASUREMENT_REPORT_ELEMENT;

//
// Traffic Stream Metrics IE.
// Ref: CCX 4 spec, S56.5.1.
//
typedef struct _TSM_IE{
	u1Byte				TSID;
	u1Byte				State; // 1: Enable, 0: disable.
	u2Byte				MeasurementInterval; // in TU.
}TSM_IE, *PTSM_IE;

#define TSM_IE_STATE_DISABLE	0
#define TSM_IE_STATE_ENABLE		1

#define CISCO_AIRONET_SNAP_LENGTH 8

//
// Radio Measurement Request frame body.
// Ref: CCX 2 spec, Figure S36-1.
//
typedef struct _RM_CCX_REQUEST_PACKET{
	u1Byte				SnapHeader[CISCO_AIRONET_SNAP_LENGTH]; // Cisco Aironet SNAP header: AAAA 0300 4096 0000
	u2Byte				IappIdLen; // 0x0nnn, 0x0: IAPP control message, 0xnnn: length from IAPP ID & Length to end of the packet.
	u1Byte				IappType; // 0x32 indicates Radio Measurement.
	u1Byte				IappSubtype; // 0x01 indicates request.
	u1Byte				DstAddr[6]; // Unused, it should be all zeros.
	u1Byte				SrcAddr[6]; // Unused, it should be all zeros.
	u2Byte				DialogToken; // Non-zero value to identify request/report transaction.
	u1Byte				ActivationDelay; // # of TBTTs until the interval specified by the Measurement Offset field starts.
	u1Byte				MeasurementOffset; // TUs after the Activation Delay.
	u1Byte				Elements[1]; // Actual length is ((IappIdLen & 0x0FFF) - 20) 
}RM_CCX_REQUEST_PACKET, *PRM_CCX_REQUEST_PACKET;


//
// Radio Measurement Report frame body.
// Ref: CCX 2 spec, Figure S36-2.
//
typedef struct _RM_REPORT_PACKET{
	u1Byte				SnapHeader[CISCO_AIRONET_SNAP_LENGTH]; // Cisco Aironet SNAP header: AAAA 0300 4096 0000
	u2Byte				IappIdLen; // 0x0nnn, 0x0: IAPP control message, 0xnnn: length from IAPP ID & Length to end of the packet.
	u1Byte				IappType; // 0x32 indicates Radio Measurement.
	u1Byte				IappSubtype; // 0x81 indicates response.
	u1Byte				DstAddr[6]; // Unused, it should be all zeros.
	u1Byte				SrcAddr[6]; // Reporting STA's MAC address. 
	u2Byte				DialogToken; // Non-zero value to identify request/report transaction.
	u1Byte				Elements[1]; // Actual length is ((IappIdLen & 0x0FFF) - 18) 
}RM_REPORT_PACKET, *PRM_REPORT_PACKET;

//
// Parameters for HAL to perform a Traffic Stream Metrics.
//
typedef enum _RT_TSM_ACTION_E{
	TSM_ACTION_NONE,
	TSM_ACTION_START,
	TSM_ACTION_RESET,
	TSM_ACTION_STOP,
}RT_TSM_ACTION_E;

typedef struct _RT_TSM_REQ_PARAM{
	u1Byte				TSID;
	RT_TSM_ACTION_E		Action;
}RT_TSM_REQ_PARAM, *PRT_TSM_REQ_PARAM;

// The fileds in TSM Report are placed in Big Endian.
#define	SET_CCX_TSM_RPT_PTK_QUEUE_DELAY_AVERAGE(__Rpt, __Delay)		(*((pu2Byte)(__Rpt + 8)) = H2N2BYTE(__Delay))
#define	SET_CCX_TSM_RPT_PTK_QUEUE_DELAY_HIS_i(__Rpt, __Delay, __i)		(*((pu2Byte)(__Rpt + 10 + (2*__i))) = H2N2BYTE(__Delay[__i]))
#define	SET_CCX_TSM_RPT_PTK_TRANS_DELAY(__Rpt, __Delay)		(*((pu4Byte)(__Rpt + 18)) = H2N4BYTE(__Delay))
#define	SET_CCX_TSM_RPT_PTK_LOSS(__Rpt, __Delay)		(*((pu2Byte)(__Rpt + 22)) = H2N2BYTE(__Delay))
#define	SET_CCX_TSM_RPT_PTK_CNT(__Rpt, __Delay)		(*((pu2Byte)(__Rpt + 24)) = H2N2BYTE(__Delay))
#define	SET_CCX_TSM_ROAMING_CNT(__Rpt, Cnt)		(*(__Rpt + 26) = H2N1BYTE(Cnt))
#define	SET_CCX_TSM_ROAMING_DELAY(__Rpt, Cnt)		(*((pu2Byte)(__Rpt + 27)) = H2N2BYTE(Cnt))
#define	SET_CCX_TSM_USED_TIME(__Rpt, Cnt)		(*((pu2Byte)(__Rpt + 29)) = H2N2BYTE(Cnt))
#define	SET_CCX_TSM_TSID(__Rpt, Cnt)		(*(__Rpt + 31) = H2N1BYTE(Cnt))
//
// Parameters for HAL to report a Traffic Stream Metrics.
//
typedef struct _RT_TSM_RPT_PARAM{
	u1Byte				TSID;
	RT_TSM_ACTION_E		Action;

	u4Byte				MediaDelay; // in us.
	u2Byte				MediaDelayHistogram[4];
	u2Byte				PacketLossCount;
	u2Byte				PacketCount;
	u2Byte				UsedTime;
}RT_TSM_RPT_PARAM, *PRT_TSM_RPT_PARAM;

//
// Macro to manipulate IAPP Header of RM Request.
//
#define SET_RMREQ_IAPPIDLEN(_pRmReqPkt, _IappIdLen)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappIdLen = H2N2BYTE(_IappIdLen) )
#define GET_RMREQ_IAPPIDLEN(_pRmReqPkt)	( N2H2BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappIdLen) )

#define SET_RMREQ_IAPPTYPE(_pRmReqPkt, _IappType)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappType = H2N1BYTE(_IappType) )
#define GET_RMREQ_IAPPTYPE(_pRmReqPkt)	( N2H1BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappType) )

#define SET_RMREQ_IAPPSUBTYPE(_pRmReqPkt, _IappSubtype)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappSubtype = H2N1BYTE(_IappSubtype) )
#define GET_RMREQ_IAPPSUBTYPE(_pRmReqPkt)	( N2H1BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->IappSubtype) )

#define SET_RMREQ_DIALOGTOKEN(_pRmReqPkt, _DialogToken)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->DialogToken = H2N2BYTE(_DialogToken) )
#define GET_RMREQ_DIALOGTOKEN(_pRmReqPkt)	( N2H2BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->DialogToken) )

#define SET_RMREQ_ACTIVATIONDELAY(_pRmReqPkt, _ActivationDelay)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->ActivationDelay = H2N1BYTE(_ActivationDelay) )
#define GET_RMREQ_ACTIVATIONDELAY(_pRmReqPkt)	( N2H1BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->ActivationDelay) )

#define SET_RMREQ_MEASUREMENTOFFSET(_pRmReqPkt, _MeasurementOffset)	( ((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->MeasurementOffset = H2N1BYTE(_MeasurementOffset) )
#define GET_RMREQ_MEASUREMENTOFFSET(_pRmReqPkt)	( N2H1BYTE(((PRM_CCX_REQUEST_PACKET)_pRmReqPkt)->MeasurementOffset) )

//
// Macro to manipulate IAPP Header of RM Report.
//
#define SET_RMRPT_IAPPIDLEN(_pRmRptPkt, _IappIdLen)	( ((PRM_REPORT_PACKET)_pRmRptPkt)->IappIdLen = H2N2BYTE(_IappIdLen) )
#define GET_RMRPT_IAPPIDLEN(_pRmRptPkt)	( N2H2BYTE(((PRM_REPORT_PACKET)_pRmRptPkt)->IappIdLen) )

#define SET_RMRPT_IAPPTYPE(_pRmRptPkt, _IappType)	( ((PRM_REPORT_PACKET)_pRmRptPkt)->IappType = H2N1BYTE(_IappType) )
#define GET_RMRPT_IAPPTYPE(_pRmRptPkt)	( N2H1BYTE(((PRM_REPORT_PACKET)_pRmRptPkt)->IappType) )

#define SET_RMRPT_IAPPSUBTYPE(_pRmRptPkt, _IappSubtype)	( ((PRM_REPORT_PACKET)_pRmRptPkt)->IappSubtype = H2N1BYTE(_IappSubtype) )
#define GET_RMRPT_IAPPSUBTYPE(_pRmRptPkt)	( N2H1BYTE(((PRM_REPORT_PACKET)_pRmRptPkt)->IappSubtype) )

#define SET_RMRPT_DIALOGTOKEN(_pRmRptPkt, _DialogToken)	( ((PRM_REPORT_PACKET)_pRmRptPkt)->DialogToken = H2N2BYTE(_DialogToken) )
#define GET_RMRPT_DIALOGTOKEN(_pRmRptPkt)	( N2H2BYTE(((PRM_REPORT_PACKET)_pRmRptPkt)->DialogToken) )

// Dot11k RM request mode
#define	DOT11K_RM_REQ_MODE_PARALLEL		BIT0	// TRUE: Multiple measurements are started at the same time.
#define	DOT11K_RM_REQ_MODE_ENABLE		BIT1	// Indicate the Request or Report information.
#define	DOT11K_RM_REQ_MODE_REQUEST		BIT2	// Request is enabled.
#define	DOT11K_RM_REQ_MODE_REPORT		BIT3	// Report is enabled.
#define	DOT11K_RM_REQ_MODE_MAN_DUR		BIT4	// The measurement duaration is mandatory or shorter.

// Dot11k RM report mode
#define	DOT11K_RM_RPT_MODE_LATE			BIT0	// The request was too late.
#define	DOT11K_RM_RPT_MODE_INCAPABLE	BIT1	// The capabilities of current setting is incapable to perform this RM.
#define	DOT11K_RM_RPT_MODE_REFUSED		BIT2	// The request rm condition is not acceptable for current threshold.

//
// Dot11k action frame for rm request
//
#define	GET_DOT11K_ACT_RM_REQ_DIALOGTOKEN(pduOS)	EF1Byte( *(	(Frame_FrameBody(pduOS) + 2) ) )
#define	GET_DOT11K_ACT_RM_REQ_REPEAT(pduOS)	EF2Byte( *(	(UNALIGNED pu2Byte)(Frame_FrameBody(pduOS) + 3) ) )

//
// Dot11k rm request IE variables (excluding the header of Element ID and Length)
//
#define	GET_DOT11K_RM_REQ_MRE_TOkEN(pduOS)	EF1Byte(*((pduOS).Octet))
#define	GET_DOT11K_RM_REQ_MRE_MODE(pduOS)	EF1Byte( *((pduOS).Octet + 1) )
#define	GET_DOT11K_RM_REQ_MRE_TYPE(pduOS)	EF1Byte( *((pduOS).Octet + 2) )

//
// Dot11k Channel Load fields, the address is started at Measurement Element (excluding the header of Element ID and Length)
//
#define	GET_DOT11K_RM_REQ_CHNL_LOAD_REG_CLASS(pduOS)	EF1Byte(*((pduOS).Octet + 3))	// Regulatory Class of RM Channel Load
#define	GET_DOT11K_RM_REQ_CHNL_LOAD_CHANNEL(pduOS)	EF1Byte(*((pduOS).Octet + 3 + 1))	// Channel of RM Channel Load
#define	GET_DOT11K_RM_REQ_CHNL_LOAD_RANDOM_INTVL(pduOS)	EF2Byte(*((UNALIGNED pu2Byte)(pduOS).Octet + 3 + 2))	// Randomization Interval of RM Channel Load
#define	GET_DOT11K_RM_REQ_CHNL_LOAD_DURATION(pduOS)	EF2Byte(*((UNALIGNED pu2Byte)(pduOS).Octet + 3 + 4))	// Randomization Interval of RM Channel Load

//
// Dot11k Noise Histogram fields, the address is started at Measurement Element (excluding the header of Element ID and Length)
//
#define	GET_DOT11K_RM_REQ_NHM_REG_CLASS(pduOS)	EF1Byte(*((pduOS).Octet + 3))	// Regulatory Class of RM Channel Load
#define	GET_DOT11K_RM_REQ_NHM_CHANNEL(pduOS)	EF1Byte(*((pduOS).Octet + 3 + 1))	// Channel of RM Channel Load
#define	GET_DOT11K_RM_REQ_NHM_RANDOM_INTVL(pduOS)	EF2Byte(*((UNALIGNED pu2Byte)(pduOS).Octet + 3 + 2))	// Randomization Interval of RM Channel Load
#define	GET_DOT11K_RM_REQ_NHM_DURATION(pduOS)	EF2Byte(*((UNALIGNED pu2Byte)(pduOS).Octet + 3 + 4))	// Randomization Interval of RM Channel Load

//
// Dot11k rm request IE variables (including the header of Element ID and Length)
//
#define	SET_DOT11K_RM_REQ_MRE_EID(pHeader, _val)	WriteEF1Byte((pHeader), _val)
#define	SET_DOT11K_RM_REQ_MRE_LEN(pHeader, _val)	WriteEF1Byte(((pHeader) + 1), _val)
#define	SET_DOT11K_RM_REQ_MRE_TOKEN(pHeader, _val)	WriteEF1Byte(((pHeader) + 2), _val)
#define	SET_DOT11K_RM_REQ_MRE_MODE(pHeader, _val)	WriteEF1Byte(((pHeader) + 3), _val)
#define	SET_DOT11K_RM_REQ_MRE_TYPE(pHeader, _val)	WriteEF1Byte(((pHeader) + 4), _val)

//
// Dot11k Channel Load request fields, the address is started at Measurement Element (including the header of Element ID and Length)
//
#define	SET_DOT11K_RM_REQ_CHNL_LOAD_REG_CLASS(pHeader, _val)	WriteEF1Byte(((pHeader) + 5), _val)	// Regulatory Class
#define	SET_DOT11K_RM_REQ_CHNL_LOAD_CHANNEL(pHeader, _val)	WriteEF1Byte(((pHeader) + 6), _val)	// Channel Number
#define	SET_DOT11K_RM_REQ_CHNL_LOAD_RANDOM_INTVL(pHeader, _val)	WriteEF2Byte(((pHeader) + 7), _val)	// Random Interval
#define	SET_DOT11K_RM_REQ_CHNL_LOAD_DURATION(pHeader, _val)	WriteEF2Byte(((pHeader) + 9), _val)	// Duration

//
// Dot11k Noise Histogram fields, the address is started at Measurement Element (excluding the header of Element ID and Length)
//
#define	SET_DOT11K_RM_REQ_NHM_REG_CLASS(pHeader, _val)	WriteEF1Byte(((pHeader) + 5), _val)	// Regulatory Class
#define	SET_DOT11K_RM_REQ_NHM_CHANNEL(pHeader, _val)	WriteEF1Byte(((pHeader) + 6), _val)	// Channel Number
#define	SET_DOT11K_RM_REQ_NHM_RANDOM_INTVL(pHeader, _val)	WriteEF2Byte(((pHeader) + 7), _val)		// Random Interval
#define	SET_DOT11K_RM_REQ_NHM_DURATION(pHeader, _val)	WriteEF2Byte(((pHeader) + 9), _val)	// Duration


//
// Dot11k rm report IE variables (including the header of Element ID and Length)
//
#define	GET_DOT11K_RM_RPT_MRE_EID(pduOS)	EF1Byte(*((pduOS).Octet))
#define	GET_DOT11K_RM_RPT_MRE_LEN(pduOS)	EF1Byte(*((pduOS).Octet + 1))
#define	GET_DOT11K_RM_RPT_MRE_TOKEN(pduOS)	EF1Byte(*((pduOS).Octet + 2))
#define	GET_DOT11K_RM_RPT_MRE_MODE(pduOS)	EF1Byte( *((pduOS).Octet + 3) )
#define	GET_DOT11K_RM_RPT_MRE_TYPE(pduOS)	EF1Byte( *((pduOS).Octet + 4) )

//
// Dot11k Channel Load report fields, the address is started at Measurement Element (including the header of Element ID and Length)
//
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_REG_CLASS(pduOS)	EF1Byte( *((pduOS).Octet + 5) )
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_CHANNEL(pduOS)	EF1Byte( *((pduOS).Octet + 6) )
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_STARTTIME_LOW(pduOS)	EF4Byte( *(UNALIGNED pu4Byte)((pduOS).Octet + 7) )
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_STARTTIME_HIGH(pduOS)	EF4Byte( *(UNALIGNED pu4Byte)((pduOS).Octet + 11) )
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_DURATION(pduOS)	EF2Byte( *(UNALIGNED pu2Byte)((pduOS).Octet + 15) )
#define	GET_DOT11K_RM_RPT_CHNL_LOAD_LOAD(pduOS)	EF1Byte( *((pduOS).Octet + 17) )

//
// Dot11k Noise Histogram report fields, the address is started at Measurement Element (including the header of Element ID and Length)
//
#define	GET_DOT11K_RM_RPT_NHM_REG_CLASS(pduOS)	EF1Byte( *((pduOS).Octet + 5) )
#define	GET_DOT11K_RM_RPT_NHM_CHANNEL(pduOS)	EF1Byte( *((pduOS).Octet + 6) )
#define	GET_DOT11K_RM_RPT_NHM_STARTTIME_LOW(pduOS)	EF4Byte( *(UNALIGNED pu4Byte)((pduOS).Octet + 7) )
#define	GET_DOT11K_RM_RPT_NHM_STARTTIME_HIGH(pduOS)	EF4Byte( *(UNALIGNED pu4Byte)((pduOS).Octet + 11) )
#define	GET_DOT11K_RM_RPT_NHM_DURATION(pduOS)	EF2Byte( *(UNALIGNED pu2Byte)((pduOS).Octet + 15) )
#define	GET_DOT11K_RM_RPT_NHM_ANT_ID(pduOS)	EF1Byte( *((pduOS).Octet + 17) )
#define	GET_DOT11K_RM_RPT_NHM_ANPI(pduOS)	EF1Byte( *((pduOS).Octet + 18) )
#define	GET_DOT11K_RM_RPT_NHM_IPI_DENSITY(pduOS, _pIPI)	PlatformMoveMemory(_pIPI, ((pduOS).Octet + 19), DOT11K_NUM_RPI) // The IPI density



//
// Dot11k rm report IE variables (including the header of Element ID and Length)
//
#define	SET_DOT11K_RM_RPT_MRE_EID(pHeader, _val)	WriteEF1Byte((pHeader), _val)
#define	SET_DOT11K_RM_RPT_MRE_LEN(pHeader, _val)	WriteEF1Byte(((pHeader) + 1), _val)
#define	SET_DOT11K_RM_RPT_MRE_TOKEN(pHeader, _val)	WriteEF1Byte(((pHeader) + 2), _val)
#define	SET_DOT11K_RM_RPT_MRE_MODE(pHeader, _val)	WriteEF1Byte(((pHeader) + 3), _val)
#define	SET_DOT11K_RM_RPT_MRE_TYPE(pHeader, _val)	WriteEF1Byte(((pHeader) + 4), _val)

//
// Dot11k Channel Load report fields, the address is started at Measurement Element (including the header of Element ID and Length)
//
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_REG_CLASS(pHeader, _val)	WriteEF1Byte(((pHeader) + 5), _val)	// Regulatory Class
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_CHANNEL(pHeader, _val)	WriteEF1Byte(((pHeader) + 6), _val)	// Channel Number
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_STARTTIME_LOW(pHeader, _val)	WriteEF4Byte(((pHeader) + 7), _val)	// Start time low
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_STARTTIME_HIGH(pHeader, _val)	WriteEF4Byte(((pHeader) + 11), _val)	// Start time high
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_DURATION(pHeader, _val)	WriteEF2Byte(((pHeader) + 15), _val)	// Measurement Duration
#define	SET_DOT11K_RM_RPT_CHNL_LOAD_LOAD(pHeader, _val)	WriteEF1Byte(((pHeader) + 17), _val)	// Channel Load

//
// Dot11k Noise Histogram report fields, the address is started at Measurement Element (including the header of Element ID and Length)
//
#define	SET_DOT11K_RM_RPT_NHM_REG_CLASS(pHeader, _val)	WriteEF1Byte(((pHeader) + 5), _val)	// Regulatory Class
#define	SET_DOT11K_RM_RPT_NHM_CHANNEL(pHeader, _val)	WriteEF1Byte(((pHeader) + 6), _val)	// Channel Number
#define	SET_DOT11K_RM_RPT_NHM_STARTTIME_LOW(pHeader, _val)	WriteEF4Byte(((pHeader) + 7), _val)	// Start time low
#define	SET_DOT11K_RM_RPT_NHM_STARTTIME_HIGH(pHeader, _val)	WriteEF4Byte(((pHeader) + 11), _val)	// Start time high
#define	SET_DOT11K_RM_RPT_NHM_DURATION(pHeader, _val)	WriteEF2Byte(((pHeader) + 15), _val)	// Measurement Duration
#define	SET_DOT11K_RM_RPT_NHM_ANT_ID(pHeader, _val)	WriteEF1Byte(((pHeader) + 17), _val)	// Antenna ID
#define	SET_DOT11K_RM_RPT_NHM_ANPI(pHeader, _val)	WriteEF1Byte(((pHeader) + 18), _val)	// The average noise power indicator
#define	SET_DOT11K_RM_RPT_NHM_IPI_DENSITY(pHeader, _pVal)	PlatformMoveMemory(((pHeader) + 19), _pVal, DOT11K_NUM_RPI) // The IPI density

//
// CCX rm report element variables (including the header of Element ID and Length)
//
#define	SET_CCX_RM_RPT_MRE_EID(pHeader, _val)	WriteEF2Byte((pHeader), _val)
#define	SET_CCX_RM_RPT_MRE_LEN(pHeader, _val)	WriteEF2Byte(((pHeader) + 2), _val)
#define	SET_CCX_RM_RPT_MRE_TOKEN(pHeader, _val)	WriteEF2Byte(((pHeader) + 4), _val)
#define	SET_CCX_RM_RPT_MRE_MODE(pHeader, _val)	WriteEF1Byte(((pHeader) + 6), _val)
#define	SET_CCX_RM_RPT_MRE_TYPE(pHeader, _val)	WriteEF1Byte(((pHeader) + 7), _val)

//
// CCX rm beacon report element variables
//
#define	SET_CCX_RM_RPT_CHANNEL(pHeader, _val)	WriteEF1Byte(((pHeader)), _val)
#define	SET_CCX_RM_RPT_SPARE(pHeader, _val)	WriteEF1Byte(((pHeader) + 1), _val)
#define	SET_CCX_RM_RPT_MEASURE_DUR(pHeader, _val)	WriteEF2Byte(((pHeader) + 2), _val)
#define	SET_CCX_RM_RPT_PHY_TYPE(pHeader, _val)	WriteEF1Byte(((pHeader) + 4), _val)
#define	SET_CCX_RM_RPT_SIGNAL_PWR(pHeader, _val)	WriteEF1Byte(((pHeader) + 5), _val)
#define	SET_CCX_RM_RPT_BSSID(pHeader, _val)	PlatformMoveMemory((UNALIGNED pu1Byte)(pHeader + 6), _val, 6)
#define	SET_CCX_RM_RPT_PARENT_TSF(pHeader, _val)	WriteEF4Byte(((pHeader) + 12), _val)
#define	SET_CCX_RM_RPT_TARGET_TSF_LOW(pHeader, _val)	WriteEF4Byte(((pHeader) + 16), _val)
#define	SET_CCX_RM_RPT_TARGET_TSF_HIGH(pHeader, _val)	WriteEF4Byte(((pHeader) + 20), _val)
#define	SET_CCX_RM_RPT_BCN_INTERVAL(pHeader, _val)	WriteEF2Byte(((pHeader) + 24), _val)
#define	SET_CCX_RM_RPT_CAP(pHeader, _val)	WriteEF2Byte(((pHeader) + 26), _val)

//
// CCX rm request element variables
//
#define	GET_CCX_RM_REQ_MRE_TOKEN(pduOS)	EF2Byte(*((pduOS).Octet))
#define	GET_CCX_RM_REQ_MRE_MODE(pduOS)	EF1Byte( *((pduOS).Octet + 2) )
#define	GET_CCX_RM_REQ_MRE_TYPE(pduOS)	EF1Byte( *((pduOS).Octet + 3) )

//
// CCX rm beacon request variables
//
#define	GET_CCX_RM_REQ_MRE_DUR(pduOS)	EF2Byte(*((pduOS).Octet + 2))
#define	GET_CCX_RM_RPT_SIGNAL_PWR(pHeader)	EF1Byte(*((pHeader) + 5))

// Dot11k measurement request type
typedef	enum _DOT11K_RM_TYPE{
	DOT11K_RM_TYPE_CHANNEL_LOAD = 3,
	DOT11K_RM_TYPE_NOISE_HISTOGRAM = 4,
}DOT11K_RM_TYPE, *PDOT11K_RM_TYPE;

unsigned int
RmStaRxPowerHash(
	IN RT_HASH_KEY			Key
	);


#endif // #ifndef __INC_RMTYPE_H

