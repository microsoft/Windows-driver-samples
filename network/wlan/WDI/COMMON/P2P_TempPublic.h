//------------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//------------------------------------------------------------------------------
//
// Description:
//		Definitions shall be internal but put as public because P2P_INFO
//		refers to them.
//		Note that P2P_INFO shall also be internal.
//

#ifndef __INC_TEMP_P2P_PUBLIC_H
#define __INC_TEMP_P2P_PUBLIC_H

#include "P2P_Channel.h" // to be removed because this shall be private
#include "P2P_Synch.h"

typedef struct _P2P_MESSAGE
{
	/* MAC header */
	u1Byte			da[6];
	u1Byte			sa[6];
	u1Byte			bssid[6];

	u1Byte			*devAddr;
	
	/* IE raw data */
	FRAME_BUF		p2pAttributes;			// concatenated P2P attributes
	FRAME_BUF		wpsAttributes;			// concatenated WPS attributes

	/* Action frame specific */
	const u1Byte	*_dialogToken;
	u1Byte			dialogTokenBuf[1];
	u1Byte			dialogToken;

	/* P2P IE, pointers into the raw IE data */
	const u1Byte	*_status;
	u1Byte			status;
	
	const u1Byte	*_minorReasonCode;
	u1Byte			minorReasonCode;
	
	const u1Byte	*_capability;
	u1Byte			grpCap;
	u1Byte			devCap;
	
	const u1Byte	*_devId;
	const u1Byte	*devIdDevAddr;
	
	const u1Byte	*_goIntent;
	u1Byte			goIntent;
	
	const u1Byte	*_configTimeout;
	u1Byte			goConfigTimeout;
	u1Byte			cliConfigTimeout;
	
	const u1Byte	*_listenChannel;
	u1Byte			listenChannel;
	
	const u1Byte	*_grpBssid;
	const u1Byte	*grpBssid;
	
	const u1Byte	*_extListenTiming;
	u2Byte			extListenPeriod;
	u2Byte			extListenInterval;
	
	const u1Byte	*_intendedIntfAddr;
	const u1Byte	*intfAddr;
	
	const u1Byte	*_manageability;
	u1Byte			manageability;
	
	const u1Byte	*_channelList;
	u2Byte			channelListLen;
	const u1Byte	*channelListCountry;
	u2Byte			channelEntryListLen;
	const u1Byte	*channelEntryList;
	
	const u1Byte	*_noa;
	u2Byte			noaLen;
	u1Byte			noaIndex;
	u1Byte			noaCtWindowAndOppPsParam;
	u2Byte			noaDescriptorLen;
	const u1Byte	*noaDescriptors;
	
	const u1Byte	*_devInfo;
	const u1Byte	*devInfoDevAddr;
	u2Byte			configMethod;
	const u1Byte	*primDevType;
	u1Byte			numSecDevType;
	const u1Byte	*secDevTypeList;
	u1Byte			devNameLen;
	const u1Byte	*devName;
	
	const u1Byte	*_grpInfo;
	u2Byte			cliInfoDescListLen;
	const u1Byte	*cliInfoDescList;
	
	const u1Byte	*_grpId;
	const u1Byte	*grpDevAddr;
	u1Byte			grpSsidLen;
	const u1Byte	*grpSsid;
	
	const u1Byte	*_intf;
	const u1Byte	*intfDevAddr;
	u1Byte			intfAddrCount;
	const u1Byte	*intfAddrList;
	
	const u1Byte	*_opChannel;
	u1Byte			opChannel;
	
	const u1Byte	*_invitationFlags;
	u1Byte			invitationFlags;

	/* P2P IE, WFDS addendum */
	const u1Byte	*_svcHash;
	u2Byte			svcHashLen;
	u2Byte			svcHashNum;
	const u1Byte	*svcHashList;
	
	const u1Byte	*_sessionInfoDataInfo;
	u2Byte			sessionInfoLen;
	const u1Byte	*sessionInfo;
	
	const u1Byte	*_connCap;
	u1Byte			connCap;
	
	const u1Byte	*_advId;
	u4Byte			advId;
	const u1Byte	*svcMac;
	
	const u1Byte	*_advSvc;
	u2Byte			advSvcLen;
	u2Byte			advSvcNum;

	const u1Byte	*_sessionId;
	u4Byte			sessionId;
	const u1Byte	*sessionMac;
	
	const u1Byte	*_featureCap;			
	u2Byte			featureCap;

	/* WPS IE */
	const u1Byte	*_wpsConfigMethods; 	// WPS config methods
	u2Byte			wpsConfigMethods;

	const u1Byte	*_wpsDevName;
	u2Byte			wpsDevNameLen;
	
	const u1Byte	*_wpsDevPasswordId;		// Device password id
	u2Byte			wpsDevPasswordId;
	
	const u1Byte	*_wpsManufacturer;		// WPS manufacturer
	u2Byte			wpsManufacturerLen;
	
	const u1Byte	*_wpsModelName;			// WPS model name
	u2Byte			wpsModelNameLen;
	
	const u1Byte	*_wpsModelNumber;		// WPS model number
	u2Byte			wpsModelNumberLen;
	
	const u1Byte	*_wpsSerialNumber;		// WPS serial number
	u2Byte			wpsSerialNumberLen;

	const u1Byte	*_wpsPrimaryDevType;	// WPS primary device type
	u2Byte			wpsPrimaryDevTypeLen;

	const u1Byte	*_wpsSecDevTypeList;
	u2Byte 			wpsSecDevTypeListLen;
	u1Byte			wpsSecDevTypeNum;		// Number of secondary device type
	
	const u1Byte	*_wpsUuidE;
	u2Byte			wpsUuidELen;

	/* SSID IE */
	const u1Byte	*_ssid;
	u1Byte			ssidBuf[32];
	u1Byte			ssidLen;

	/* DS Parameter IE */
	const u1Byte	*_dsParam;
	u1Byte			dsParamBuf[1];
	u1Byte			dsParam;

	/* Debug */
	u4Byte			dbgLevel;
	RT_STATUS		rtStatus;
	
}P2P_MESSAGE;

#define P2P_MAX_DEV_LIST				256
#define P2P_MAX_P2P_DEV_INFO_POOL_ENTRIES 32
#define P2P_MAX_FRAME_POOL_ENTRIES		256	
#define P2P_MAX_FRAME_BUF_SIZE 			640
#define P2P_MAX_MSG_POOL_ENTRIES		32

typedef enum _P2P_DEV_TYPE
{
	P2P_DEV_TYPE_LEGACY,
	P2P_DEV_TYPE_GO,
	P2P_DEV_TYPE_DEV
}P2P_DEV_TYPE;

typedef enum _P2P_FRAME_TYPE
{
	P2P_FID_BEACON = 0,
	P2P_FID_PROBE_REQ,
	P2P_FID_PROBE_RSP,
	P2P_FID_ACTION_START,
	P2P_FID_GO_NEG_REQ = P2P_FID_ACTION_START,
	P2P_FID_GO_NEG_RSP,
	P2P_FID_GO_NEG_CONF,
	P2P_FID_INV_REQ,
	P2P_FID_INV_RSP,
	P2P_FID_PD_REQ,
	P2P_FID_PD_RSP,
	//--------------------------------------------------------------------------
	P2P_FID_MAX
}P2P_FRAME_TYPE;

typedef struct _P2P_FRAME_INFO
{
	RT_LIST_ENTRY		list;
	
	// Contains the raw frame 
	u2Byte				frameLen;
	u1Byte				frame[P2P_MAX_FRAME_BUF_SIZE];
	P2P_FRAME_TYPE 		type;

	// Dialog token, valid only when frame type is action
	u1Byte				token;

	// Parsed message, valid only when this is a rx frame
	P2P_MESSAGE			*msg;

	// Meta data
	u8Byte				time;
	u1Byte				channel;
	s4Byte				sigPower;
	u1Byte				sigStrength;
}P2P_FRAME_INFO;

typedef struct _P2P_DEV_INFO
{
	// Channel list
	P2P_CHANNELS		channels;
	P2P_CHANNELS		commonChannels;

	// Dialog token to be use for action frame tx
	u1Byte				dialogToken;

	// PD Info
	u2Byte				pdConfigMethod;
	u1Byte				pdStatus;
}P2P_DEV_INFO;

/*
 * Contains information of either a legacy AP, a GO or a P2P Device.
 */
typedef struct _P2P_DEV_LIST_ENTRY
{
	RT_LIST_ENTRY		list;

	// TA from the received frame, if from a P2P Device, it is the Device 
	// Address, otherwise it is either the Interface Address or the BSSID
	u1Byte				mac[6];

	// Type of the device
	P2P_DEV_TYPE		type;

	BOOLEAN				bDirty;

	// Rx frames
	P2P_FRAME_INFO 		*rxFrames[P2P_FID_MAX];
	RT_LIST_ENTRY		rxFrameQ;

	// Tx frames
	P2P_FRAME_INFO 		*txFrames[P2P_FID_MAX];

	// P2P Device/GO specific info, valid only when type is not legacy
	P2P_DEV_INFO		*p2p;
}P2P_DEV_LIST_ENTRY;

typedef struct _P2P_DEV_LIST
{
	u4Byte				count;
	RT_LIST_ENTRY		list;
	P2P_LOCK			lock;
	const char			*sig;

	POOL				entryPool;
	P2P_DEV_LIST_ENTRY	entryPoolRsvd[P2P_MAX_DEV_LIST];

	POOL				p2pDevInfoPool;
	P2P_DEV_INFO		p2pDevInfoPoolRsvd[P2P_MAX_P2P_DEV_INFO_POOL_ENTRIES];

	POOL				framePool;
	P2P_FRAME_INFO		framePoolRsvd[P2P_MAX_FRAME_POOL_ENTRIES];

	POOL				rxMsgPool;
	P2P_MESSAGE			rxMsgPoolRsvd[P2P_MAX_MSG_POOL_ENTRIES];
}P2P_DEV_LIST;

#endif	// #ifndef __INC_TEMP_P2P_PUBLIC_H
