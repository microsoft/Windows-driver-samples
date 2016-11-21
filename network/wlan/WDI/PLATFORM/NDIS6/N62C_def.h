/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    N62C_def.h

Abstract:
    Contains Port specific defines
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    05-12-2007    Created

Notes:

--*/

#pragma once

#ifndef _N62_DEF__H
#define _N62_DEF__H

/** 
 * Gets access to the MP_EXTSTA_PORT from the MP_PORT
 */
#define IS_ALLOCATED_PORT_NUMBER(_PortNumber)   (_PortNumber != DEFAULT_NDIS_PORT_NUMBER && _PortNumber != HELPER_PORT_PORT_NUMBER)




//
// (Ported from MS's code, Maddest, 2008-06-13.) 
// Macros for assigning and verifying NDIS_OBJECT_HEADER
//
#define MP_VERIFY_NDIS_OBJECT_HEADER_DEFAULT(_header, _type, _revision, _size) \
    (((_header).Type == _type) && \
     ((_header).Revision >= _revision) && \
     ((_header).Size >= _size))

typedef enum _RT_MUTEX_TYPE{
	RT_RESET_PNP_MUTEX = 1,
}RT_MUTEX_TYPE;

typedef enum _MP_PORT_OP_STATE
{
    INIT_STATE = 0,
    OP_STATE
} MP_PORT_OP_STATE, *PMP_PORT_OP_STATE;

typedef enum _MP_PORT_TYPE
{
    HELPER_PORT = 0,
    EXTSTA_PORT,
    EXTAP_PORT,
    EXT_P2P_DEVICE_PORT,
    EXT_P2P_ROLE_PORT
} MP_PORT_TYPE, *PMP_PORT_TYPE;


/** 
 * Keeps tracks of the current state of association attempt by the driver. Note
 * the some states are set just before we start an action to avoid timing windows.
 */
typedef enum _STA_ASSOC_STATE
{
    /** 
     * Start state when we are not associated and have not started association process 
     */
    ASSOC_STATE_NOT_ASSOCIATED = 0,
    
    /** 
     * When set we are ready to start the association process and are either
     * about to start the association or have failed a previous association and 
     * would be restarting the association. Roaming and connect request code
     * are synchronized after this state.
     */
    ASSOC_STATE_READY_TO_ASSOCIATE,

    /** 
     * Have started the association process. This is set just after we have selected
     * an access point to start the association attempt. This would only be set for
     * the duration of the association process
     */
    ASSOC_STATE_STARTED_ASSOCIATION,
    
    /** 
     * Have asked the hardware functions to synchronize with the access point 
     * and are waiting for hardware functions to return
     */ 
    ASSOC_STATE_WAITING_FOR_JOIN,
    
    /**
     * Hardware functions have returned after successful synchronization
     * with the access point
     */
    ASSOC_STATE_JOINED,
    
    /** 
     * Special state set when we get deauthenticate packet from 
     * the access point while we still havent completed association. This is
     * not set on normal code path. It is used to ensure that we dont 
     * complete the association successfully even if we received a
     * deauth packet from the access point
     */
    ASSOC_STATE_REMOTELY_DEAUTHENTICATED,
    
    /** 
     * We are waiting for an authentication packet from the accesspoint.
     * Currently driver only uses open authentication or shared key authentication
     * so this is set when we are waiting for packet with sequence number 2 or 4
     */
    ASSOC_STATE_WAITING_FOR_AUTHENTICATE,

    /** 
     * Received successful authentication response from the access point
     */
    ASSOC_STATE_RECEIVED_AUTHENTICATE,
    
    /** 
     * Special state set when we get disassociate request from 
     * the access point while we still havent completed association. This
     * is again not set on normal code paths. It is used to ensure that
     * we dont complete the association successfully even if we received
     * a disassociate packet from the access point
     */
    ASSOC_STATE_REMOTELY_DISASSOCIATED,
    
    /** 
     * Waiting for Association response from the access point
     */
    ASSOC_STATE_WAITING_FOR_ASSOCIATE,
    
    /** 
     * Received successful association response from the access point 
     */
    ASSOC_STATE_RECEIVED_ASSOCIATE,
    
    /** 
     * Associated process completed successfully & status indicated. After this
     * is set, any disassociate/deauthenticate packet from the accesspoint
     * would case disassociation status indication
     */
    ASSOC_STATE_ASSOCIATED
    
} STA_ASSOC_STATE, *PSTA_ASSOC_STATE  ;

/** 
 * State of the connection attempt as per the operating system. This is 
 * used to keep track of what the operating system is expecting the driver
 * to do with regards to connection to an 802.11 network
 */
typedef enum _STA_CONNECT_STATE{
    /** 
     * Disconnected: Driver should not attempt to associate and if associated
     * should disconnect cleanly. This is set on receiving DISCONNECT OID and the
     * driver terminates any existing connection.
     */
    CONN_STATE_DISCONNECTED,

    /** 
     * In reset. Operating system is reseting the adapter (either by OID or 
     * NdisReset) or halting the adapter, etc. The driver waits
     * for on going connection to complete and then resets back to 
     * disconnected state
     */
    CONN_STATE_IN_RESET,

    /** 
     * Okay to connect. The operating system expects the driver to start connecting. The
     * driver would attempt to find and associate with candidate access point. This
     * is set on receiving CONNECT OID and is maintained until we have made a connection
     * attempt.
     */
    CONN_STATE_READY_TO_CONNECT,

    /** 
     * Okay to roam. The operating system expects the driver to stay connected a
     * and if connection is lost for some reason, to roam to a new access point.
     * In this state, any connection attempt is because we want to roam
     */
    CONN_STATE_READY_TO_ROAM
    
} STA_CONNECT_STATE, *PSTA_CONNECT_STATE;

//
// Management Frame
//

typedef union {
    struct {
        USHORT          ESS: 1;
        USHORT          IBSS: 1;
        USHORT          CFPollable: 1;
        USHORT          CFPollRequest: 1;
        USHORT          Privacy: 1;
        USHORT          ShortPreamble: 1;
        USHORT          PBCC: 1;
        USHORT          ChannelAgility: 1;
        USHORT          Reserved: 2;
        USHORT          ShortSlotTime:1;
        USHORT          Reserved2: 2;
        USHORT          DSSSOFDM: 1;
        USHORT          Reserved3: 2;
    };

    USHORT usValue;

} DOT11_CAPABILITY, * PDOT11_CAPABILITY;

typedef struct _QUEUE_ENTRY
{
    struct _QUEUE_ENTRY *Next;
} QUEUE_ENTRY, *PQUEUE_ENTRY;

typedef struct _MP_PACKET_QUEUE
{
    PQUEUE_ENTRY        Head;
    PQUEUE_ENTRY        Tail;
    ULONG               Count;
} MP_PACKET_QUEUE, *PMP_PACKET_QUEUE;

BOOLEAN
IsAPModeExist(
	PADAPTER	Adapter
	);

BOOLEAN
IsExtAPModeExist(
	PADAPTER	Adapter
	);

BOOLEAN
IsSendingBeacon(
	PADAPTER	Adapter
	);
PADAPTER
GetAdapterByPortNum(PADAPTER	Adapter, u1Byte PortNum);

#endif ////_N62_DEF__H
