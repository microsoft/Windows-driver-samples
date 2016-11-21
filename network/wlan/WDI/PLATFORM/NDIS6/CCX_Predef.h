#include <windot11.h>

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccxtype.h in Vsita CCX Header file
//
//--------------------------------------------------------------------

typedef unsigned char       CCXBOOLEAN;
typedef signed char         CCXINT8,    *PCCXINT8;
typedef unsigned char       CCXUINT8,   *PCCXUINT8;
typedef signed short        CCXINT16,   *PCCXINT16;
typedef unsigned short      CCXUINT16,  *PCCXUINT16;
typedef signed long         CCXINT32,   *PCCXINT32;
typedef unsigned long       CCXUINT32,  *PCCXUINT32;
typedef signed __int64      CCXINT64,   *PCCXINT64;
typedef unsigned __int64    CCXUINT64,  *PCCXUINT64;

typedef void                CCXVOID,    *PCCXVOID;
typedef void *              CCXHANDLE;
typedef CCXHANDLE           *PCCXHANDLE;

typedef char                CCXCHAR, *PCCXCHAR;

typedef unsigned short      CCXWCHAR;
typedef CCXWCHAR            *PCCXWCHAR;
typedef const CCXWCHAR      *PCCXCWSTR;

typedef CCXUINT32           CCX_OID,    *PCCX_OID;
typedef CCXINT32            CCX_ERROR,  *PCCX_ERROR;

#define CCX_802_11_MAC_ADDRESS_LENGTH   6
typedef CCXUINT8  CCX_802_11_MAC_ADDRESS[CCX_802_11_MAC_ADDRESS_LENGTH];

#define CCX_802_11_SSID_LENGTH  32
typedef struct _CCX_802_11_SSID
{
    CCXINT32   length;
    CCXUINT8   ssid[CCX_802_11_SSID_LENGTH];
} CCX_802_11_SSID, *PCCX_802_11_SSID;

typedef CCXUINT32   CCXLANGID;

#define CCX_MAX_COMPUTERNAME_LENGTH 31


typedef struct _CCXGUID {
    unsigned long  d1;
    unsigned short d2;
    unsigned short d3;
    unsigned char  d4[ 8 ];
} CCXGUID, *PCCXGUID;

#define CCXMAXUINT32    0xFFFFFFFF
#define CCXMAXUINT16    0xFFFF
#define CCXMAXUINT8     0xFF

#define CCX_ASSOCIATE_RELATED	0

//CCX OIDS
#define OID_CCX_ASSOC_INFO                  0xFF000003
#define OID_CCX_RM_REQUEST                  0xFF000004
#define OID_CCX_NEIGHBOR_LIST               0xFF000005
#define OID_CCX_ROAM                        0xFF000006
#define OID_CCX_MFP_STATISTICS              0xFF000007
#define OID_CCX_KEEP_ALIVE_REFRESH          0xFF000008
#define OID_CCX_STA_STATISTICS_2            0xFF00000A
#define OID_CCX_FW_VERSION                  0xFF00000B
#define OID_CCX_SERVICE_CAPABILITY          0xFF00000C
#define OID_CCX_ANTENNA_DATA                0xFF00000D
#define OID_CCX_MANUFACTURER_SERIAL_NUM     0xFF00000F
#define OID_CCX_MANUFACTURER_MODEL          0xFF000010
#define OID_CCX_MANUFACTURER_ID             0xFF000011
#define OID_CCX_MANUFACTURER_OUI            0xFF000012
#define OID_CCX_ENTERPRISE_PHONE_NUM        0xFF000013
#define OID_CCX_CELL_PHONE_NUM              0xFF000014
#define OID_CCX_RADIO_CHANNELS              0xFF000015
#define OID_CCX_DRIVER_VERSION              0xFF000016
#define OID_CCX_LAST_BCN_TIME               0xFF000017
#define OID_CCX_FRAME_LOGGING_MODE          0xFF000018
#define OID_CCX_TSF                         0xFF000019
#define OID_CCX_DIAGNOSTICS_MODE            0xFF00001A
#define OID_CCX_NUM_TX_BUFFER               0xFF00001B


#define CCX_ERROR_SUCCESS               0x00000000
#define CCX_ERROR_UNKNOWN_VERSION      (0x80000000 | 1305)          //ERROR_UNKNOWN_REVISION
#define CCX_ERROR_FAILED               (0x80000000 | 1627)          //ERROR_FUNCTION_FAILED
#define CCX_ERROR_INTERNAL             (0x80000000 | 1359)          //ERROR_INTERNAL_ERROR
#define CCX_ERROR_INVALID_PARAM        (0x80000000 | 87)            //ERROR_INVALID_PARAMETER
#define CCX_ERROR_INVALID_LENGTH       (0x80000000 | 24)            //ERROR_BAD_LENGTH
#define CCX_ERROR_INVALID_DATA         (0x80000000 | 13)            //ERROR_INVALID_DATA
#define CCX_ERROR_NOT_SUPPORTED        (0x80000000 | 50)            //ERROR_NOT_SUPPORTED
#define CCX_ERROR_BUFFER_TOO_SHORT     (0x80000000 | 122)           //ERROR_INSUFFICIENT_BUFFER
#define CCX_ERROR_TIMEOUT              (0x80000000 | 258)           //WAIT_TIMEOUT

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccxcommon.h in Vsita CCX Header file
//
//--------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Common Structures
//-----------------------------------------------------------------------------
typedef struct _CCX_VERSION_INFO
{
    CCXINT32    dwVerMin;
    CCXINT32    dwVerMax;
} CCX_VERSION_INFO, *PCCX_VERSION_INFO;

typedef struct _CCX_ADAPTER_INFO
{
    CCXGUID                     guid;                   //Guid which identifies the WLAN adapter
    CCXUINT8                    ccxVer;                 //Version of CCX the adapter supports
} CCX_ADAPTER_INFO, *PCCX_ADAPTER_INFO;  

typedef enum _CCX_AUTH_RESULT
{
    CCX_AUTH_SUCCESS,
    CCX_AUTH_FAILURE
} CCX_AUTH_RESULT, *PCCX_AUTH_RESULT;


typedef enum _CCX_ONEX_REASON
{
    CCX_ONEX_REASON_SUCCESS = 0,              //Success
    CCX_ONEX_REASON_PTK_HANDSHAKE_FAILED,     //PTK handshake failed
    CCX_ONEX_REASON_GTK_HANDSHAKE_FAILED,     //GTK handshake failed
    CCX_ONEX_REASON_NO_CCKMIE,                //(Re)Association Response did not contain a CCKMIE
    CCX_ONEX_REASON_AUTHENTICATOR_NOT_FOUND,  //Authenticator sent in EAPOL message is unknow to key managment
    CCX_ONEX_REASON_FAILED,                   //OneX failed not related to EAPOL Key messages
    CCX_ONEX_REASON_HANDSHAKE_FAILED,         //
    CCX_ONEX_REASON_UNKNOWN_AKM,
    CCX_ONEX_REASON_UNKNOWN_GROUP_CIPHER,
    CCX_ONEX_REASON_UNKNOWN_PAIRWISE_CIPHER,
    CCX_ONEX_REASON_EAP_KEYS_NOT_AVAILABLE
} CCX_ONEX_REASON, *PCCX_ONEX_REASON;

typedef struct _CCX_ASSOC_INFO
{
    CCXUINT32                       uSize;
    CCX_802_11_MAC_ADDRESS          ccxBSSID;
    u2Byte				  Aligment;
    CCXUINT32                       MaxTxPower;  // maximum transmit power (0 indicates no maximum)
    CCXUINT32                       AssocIEOffset;
    CCXUINT32                       AssocIELen;
} CCX_ASSOC_INFO, *PCCX_ASSOC_INFO;


typedef CCXUINT32   CCX_PACKET_TYPE;

#define CCX_PACKET_TYPE_NONE            0x00000000
#define CCX_PACKET_TYPE_802_11_FRAME    0x00001FFF          //A "raw", possibly fragmented, 802.11 frame (MPDU)
#define CCX_PACKET_TYPE_802_11          0x00002200          //A complete defragmented 802.11 encapsulated DATA packet (MSDU)
#define CCX_PACKET_TYPE_EAPOL           0x01000001          //An EAPOL packet of type CCX_EAPOL_PACKET


typedef struct _CCX_PACKET
{
    PCCXVOID                    ReservedIHV;                //Reserved for IHV use
    CCX_PACKET_TYPE             ccxPacketType;              //Type of packet
    CCXUINT32                   Reserved;                   //Reserved should be 0
    CCXUINT32                   uSize;                      //Size of packet
    CCXUINT8                    data[1];                    //start of data
} CCX_PACKET, *PCCX_PACKET;


typedef enum _CCX_PHY_TYPE {
    CCX_PHY_TYPE_UNKNOWN    = 0,
    CCX_PHY_TYPE_FHSS       = 1,
    CCX_PHY_TYPE_DSSS       = 2,
    CCX_PHY_TYPE_IRBASEBAND = 3,
    CCX_PHY_TYPE_OFDM       = 4,
    CCX_PHY_TYPE_HRDSSS     = 5,
    CCX_PHY_TYPE_ERP        = 6,
    CCX_PHY_TYPE_END                                        //Placeholder for the last diassoc reason
} CCX_PHY_TYPE, * PCCX_PHY_TYPE;

#define CCX_DATA_RATE_1         0x02
#define CCX_DATA_RATE_2         0x04
#define CCX_DATA_RATE_3         0x06
#define CCX_DATA_RATE_4_5       0x09
#define CCX_DATA_RATE_5_5       0x0B
#define CCX_DATA_RATE_6         0x0C
#define CCX_DATA_RATE_9         0x12
#define CCX_DATA_RATE_11        0x16
#define CCX_DATA_RATE_12        0x18
#define CCX_DATA_RATE_18        0x24
#define CCX_DATA_RATE_22        0x2C
#define CCX_DATA_RATE_24        0x30
#define CCX_DATA_RATE_27        0x36
#define CCX_DATA_RATE_33        0x42
#define CCX_DATA_RATE_36        0x48
#define CCX_DATA_RATE_48        0x60
#define CCX_DATA_RATE_54        0x6C

#define CCX_RX_FLAG_PACKET_FCS_FAILURE  0x00000001
typedef struct _RX_CHARACTERISTICS
{
    CCXUINT32               rxFlags;
    CCX_PHY_TYPE            phyID;
    CCXUINT32               frequency;
    CCXUINT16               numMPDU;
    CCXUINT32               dataRate;
    CCXUINT64               tsf;
    CCXINT32                rssi;
} RX_CHARACTERISTICS, *PRX_CHARACTERISTICS;


#define CCX_TX_FLAG_MAX_RETRY           0x00000001          //Max Retried pkt
#define CCX_TX_FLAG_CANCELLED           0x00000002          //Cancelled send do to roam - resubmit pkt
#define CCX_TX_FLAG_FAILED              0x00000004          //Failed to send pkt
typedef struct _TX_CHARACTERISTICS
{
    CCXUINT32               txFlags;
    CCX_PHY_TYPE            phyID;
    CCXUINT32               frequency;
    CCXUINT16               numMPDU;
    CCXUINT32               dataRate;
    CCXUINT64               tsf;       
    CCXUINT8                retryCount;
} TX_CHARACTERISTICS, *PTX_CHARACTERISTICS;


typedef enum _CCX_REQUEST_TYPE
{
    CCX_REQUEST_SET,
    CCX_REQUEST_QUERY,
    CCX_REQUEST_METHOD,
} CCX_REQUEST_TYPE, *PCCX_REQUEST_TYPE;


typedef union _CCX_REQUEST_DATA
{
    struct _QUERY_INFO
    {
        CCX_OID                 Oid;
        CCXUINT32               BytesWritten;
        CCXUINT32               BytesNeeded;
        CCXUINT32               InformationBufferLength;
        CCXUINT8                InformationBuffer[1];
    } QUERY_INFO;

    struct _SET_INFO
    {
        CCX_OID                 Oid;
        CCXUINT32               BytesRead;
        CCXUINT32               BytesNeeded;
        CCXUINT32               InformationBufferLength;
        CCXUINT8                InformationBuffer[1];
    } SET_INFO;

    struct _METHOD_INFO
    {
        CCX_OID                 Oid;
        CCXUINT32               BytesWritten;
        CCXUINT32               BytesNeeded;
        CCXUINT32               BytesRead;
        CCXUINT32               InputBufferLength;
        CCXUINT32               InformationBufferLength;
        CCXUINT8                InformationBuffer[1];
    } METHOD_INFO;

} CCX_REQUEST_DATA, *PCCX_REQUEST_DATA;


typedef struct _CCX_REQUEST
{
    CCX_REQUEST_TYPE            ccxRequestType;
    CCX_REQUEST_DATA            ccxData;
} CCX_REQUEST, *PCCX_REQUEST;


typedef struct _CCX_EAP_RESULTS
{
    CCXUINT32               uMPPSendKeyLen;
    PCCXUINT8               pMPPSendKey;
    CCXUINT32               uMPPRecvKeyLen;
    PCCXUINT8               pMPPRecvKey;
} CCX_EAP_RESULTS, *PCCX_EAP_RESULTS;

//Why nic is roaming                      
typedef enum _CCX_TRANS_REASON
{
    CCX_TRANS_UNSPECIFIED,                    // 0
    CCX_TRANS_POOR_LINK,                      // 1
    CCX_TRANS_LOAD_BALANCE,                   // 2
    CCX_TRANS_AP_CAPACITY,                    // 3
    CCX_TRANS_INFRASTRUCTURE_REQUESTED,       // 4
    CCX_TRANS_FIRST_ASSOCIATION,              // 5
    CCX_TRANS_INTO_WLAN,                      // 6
    CCX_TRANS_OUT_OF_WLAN,                    // 7
    CCX_TRANS_BETTER_AP_FOUND,                // 8   
    CCX_TRANS_AP_DISASSOCIATED,               // 9
    CCX_TRANS_FAILED_8021X_EAP_AUTHEN,        // 10   
    CCX_TRANS_FAILED_8021X_4WAY_HANDSHAKE,    // 11
    CCX_TRANS_REPLAY_COUNTER_FAILURES,        // 12
    CCX_TRANS_MIC_DATA_FAILURES,              // 13
    CCX_TRANS_MIC_MANAGEMENT_FAILURES,        // 14
    CCX_TRANS_MAX_RETRIES,                    // 15
    CCX_TRANS_EXCESS_NOISE,                   // 16
    CCX_TRANS_BROADCAST_DISASSOCIATIONS,      // 17
    CCX_TRANS_BROADCAST_DEAUTHENTICATIONS,    // 18
    CCX_TRANS_PREVIOUS_TRANSITION_FAILED      // 19
}  CCX_TRANS_REASON, *PCCX_TRANS_REASON;


//result of roam
typedef enum _CCX_TRANS_RESULT
{
    CCX_TRANS_SUCCESS,
    CCX_TRANS_PHY_DISABLED,
    CCX_TRANS_RESET,
    CCX_TRANS_CANDIDATE_LIST_EXHAUSTED

} CCX_TRANS_RESULT, *PCCX_TRANS_RESULT;


//result of associtation attempt w/ AP
typedef enum _CCX_ASSOC_RESULT
{
    CCX_ASSOC_SUCCESS = 0x00000000,
    CCX_ASSOC_NO_RESPONSE,
    CCX_ASSOC_PHY_DISABLED,
    CCX_ASSOC_RESET,

    CCX_ASSOC_FAILED  = 0x00030000
} CCX_ASSOC_RESULT, *PCCX_ASSOC_RESULT;


typedef enum _CCX_DISASSOC_REASON
{
    CCX_DISASSOC_REASON_BY_OS            = 0x00000007U,
    CCX_DISASSOC_REASON_INTERNAL_FAILURE = 0x00000009U,
    CCX_DISASSOC_REASON_PEER_DEAUTH      = 0x00010000U,
    CCX_DISASSOC_REASON_PEER_DISASSOC    = 0x00020000U,

    CCX_DISASSOC_REASON_END                         //Placeholder for the last diassoc reason
} CCX_DISASSOC_REASON, *PCCX_DISASSOC_REASON;


typedef enum _CCX_DIAG_TYPE
{
    CCX_DIAG_TYPE_START,
    CCX_DIAG_TYPE_STOP,
    CCX_DIAG_TYPE_END                        //Placeholder for the last diagnostic action type
} CCX_DIAG_TYPE, *PCCX_DIAG_TYPE;


typedef enum _CCX_NOTIFICATION_TYPE
{
    CCX_NOTIFICATION_TYPE_DIAG_MSG,                  //Diagnostic message notification
    CCX_NOTIFICATION_TYPE_END                        //Placeholder for the last notification type
} CCX_NOTIFICATION_TYPE, *PCCX_NOTIFICATION_TYPE;


typedef struct _CCX_NOTIFICATION
{
    CCX_NOTIFICATION_TYPE           ccxNotifyType;
    CCXUINT32                       uSize;
    CCXUINT8                        data[1];
} CCX_NOTIFICATION, *PCCX_NOTIFICATION;



//-----------------------------------------------------------------------------
//  Status Indications
//-----------------------------------------------------------------------------
#define    CCX_STATUS_TYPE_ASSOC_START          0x00000000
#define    CCX_STATUS_TYPE_ASSOC_COMPLETE       0x00000001
#define    CCX_STATUS_TYPE_ROAM_START           0x00000002
#define    CCX_STATUS_TYPE_ROAM_COMPLETE        0x00000003
#define    CCX_STATUS_TYPE_DISASSOCIATE         0x00000004
#define    CCX_STATUS_TYPE_ADAPTER_RESET        0x00000005
#define    CCX_STATUS_TYPE_ADAPTER_CONFIG       0x00000006
#define    CCX_STATUS_TYPE_EAP_RESULT           0x00000007
#define    CCX_STATUS_TYPE_MIC_FAILURE          0x00000008
#define    CCX_STATUS_TYPE_PMKID_LIST           0x00000009
#define    CCX_STATUS_TYPE_RM_REQUEST_COMPLETE  0x0000000A
#define    CCX_STATUS_TYPE_DIAG                 0x0000000B

typedef CCXUINT32           CCX_STATUS_TYPE;

typedef struct _CCX_STATUS_INDICATION
{
    CCX_STATUS_TYPE              ccxStatusType;
    PCCXVOID                     pStatus;
} CCX_STATUS_INDICATION, *PCCX_STATUS_INDICATION;


typedef struct _CCX_STATUS_ASSOC_START
{
    CCX_802_11_MAC_ADDRESS          ccxBSSID;
    CCX_802_11_SSID                 ccxSSID;

    CCXUINT32                       uProbeRespOffset;
    CCXUINT32                       uProbeRespSize;

} CCX_STATUS_ASSOC_START, *PCCX_STATUS_ASSOC_START;


typedef struct _CCX_STATUS_ASSOC_COMPLETE
{
    CCX_802_11_MAC_ADDRESS          ccxBSSID;
    CCX_ASSOC_RESULT                ccxAssocResult;
    CCXBOOLEAN                      reAssocReq;
    CCXBOOLEAN                      reAssocResp;
    CCXUINT32                       uAssocReqOffset;
    CCXUINT32                       uAssocReqSize;
    CCXUINT32                       uAssocRespOffset;
    CCXUINT32                       uAssocRespSize;
    CCXUINT32                       uProbeRespOffset;
    CCXUINT32                       uProbeRespSize;
} CCX_STATUS_ASSOC_COMPLETE, *PCCX_STATUS_ASSOC_COMPLETE;


typedef struct _CCX_STATUS_ROAM_START
{
    CCX_TRANS_REASON                ccxRoamReason;
} CCX_STATUS_ROAM_START, *PCCX_STATUS_ROAM_START;


typedef struct _CCX_STATUS_ROAM_COMPLETE
{
    CCX_TRANS_RESULT                ccxRoamResult;
} CCX_STATUS_ROAM_COMPLETE, *PCCX_STATUS_ROAM_COMPLETE;


typedef struct _CCX_STATUS_DISASSOC
{
    CCX_DISASSOC_REASON             uReason;
    CCXUINT16                       usReasonCode;
} CCX_STATUS_DISASSOC, *PCCX_STATUS_DISASSOC;


//typedef struct _CCX_STATUS_ADAPTER_RESET
//{
//} CCX_STATUS_ADAPTER_RESET, *PCCX_STATUS_ADAPTER_RESET;


typedef CCXUINT32 CCX_STATUS_ADAPTER_RESET, *PCCX_STATUS_ADAPTER_RESET;

typedef struct _CCX_STATUS_ADAPTER_CONFIG
{
    PCCXCWSTR                       pszXmlProfileSecurity;
    PCCXCWSTR                       pszXmlProfileConnectivity;
} CCX_STATUS_ADAPTER_CONFIG, *PCCX_STATUS_ADAPTER_CONFIG;


typedef struct _CCX_STATUS_EAP_AUTHENTICATED
{
    CCX_802_11_MAC_ADDRESS          ccxBSSID;    
    CCX_EAP_RESULTS                 ccxEapKeys;

} CCX_STATUS_EAP_AUTHENTICATED, *PCCX_STATUS_EAP_AUTHENTICATED;


typedef struct _CCX_STATUS_PMKID_LIST
{
    CCXUINT32                       uPMKIDListSize;
    CCXUINT32                       uPMKIDListOffset;   
} CCX_STATUS_PMKID_LIST, *PCCX_STATUS_PMKID_LIST;


typedef struct _CCX_STATUS_MIC_FAILURE
{
    CCXBOOLEAN                      bDefaultKeyFailure;
    CCXUINT32                       nKeyIndex;
    CCX_802_11_MAC_ADDRESS          ccxBSSID;
} CCX_STATUS_MIC_FAILURE, *PCCX_STATUS_MIC_FAILURE;


typedef struct _CCX_STATUS_DIAG_DATA
{
    CCX_DIAG_TYPE                   ccxDiagType;
    PCCXVOID                        pData;
} CCX_STATUS_DIAG_DATA, *PCCX_STATUS_DIAG_DATA;

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccxRM.h in Vsita CCX Header file
//
//--------------------------------------------------------------------
// 
// Measurement report mode field bit definitions.
// 
typedef enum _CCX_RM_RESULT {
    CCX_RM_RESULT_ACCEPTED = 0,
    CCX_RM_RESULT_INCAPABLE,
    CCX_RM_RESULT_REFUSED
} CCX_RM_RESULT, *PCCX_RM_RESULT;

// 
// Measurement type definitions (used for both requests and reports).
// 
typedef enum _CCX_RM_TYPE {
    CCX_RM_TYPE_CHANNEL_LOAD = 1,
    CCX_RM_TYPE_NOISE_HISTOGRAM,
    CCX_RM_TYPE_BEACON,
    CCX_RM_TYPE_FRAME
} CCX_RM_TYPE, *PCCX_RM_TYPE;

// 
// Scan mode definitions for beacon request element
// 
typedef enum _CCX_RM_SCAN_MODE {
    CCX_RM_MODE_PASSIVE = 0,
    CCX_RM_MODE_ACTIVE,
} CCX_RM_SCAN_MODE, *PCCX_RM_SCAN_MODE;

//------------------------------------------------------------------------------
// List of RM request passed by CCX Service Library to IHV via OID_CCX_RM_REQUEST
// The list will contain one or more RM requests defined below
//------------------------------------------------------------------------------
typedef struct _CCX_RM_REQUEST {
    CCXUINT16   NumRequests;
    CCXUINT8    RequestElements[1];      // one or more requests
} CCX_RM_REQUEST, *PCCX_RM_REQUEST;

//------------------------------------------------------------------------------
// Header struct for RM request 
//------------------------------------------------------------------------------
typedef struct _CCX_RM_REQ_IE_HDR {
    CCXUINT16               Token; 
    CCX_RM_TYPE             Type;
} CCX_RM_REQ_IE_HDR, *PCCX_RM_REQ_IE_HDR;

//------------------------------------------------------------------------------
// The RM requests 
//------------------------------------------------------------------------------
typedef struct _CCX_RM_CCA_REQ_IE {
    CCX_RM_REQ_IE_HDR       Header;         // type 1
    CCXUINT8                Channel;
    CCXUINT16               Duration;       
} CCX_RM_CCA_REQ_IE, *PCCX_RM_CCA_REQ_IE;

typedef struct _CCX_RM_RPI_REQ_IE {
    CCX_RM_REQ_IE_HDR       Header;         // type 2
    CCXUINT8                Channel;
    CCXUINT16               Duration;
} CCX_RM_RPI_REQ_IE, *PCCX_RM_RPI_REQ_IE;

typedef struct _CCX_RM_BEACON_REQ_IE {
    CCX_RM_REQ_IE_HDR       Header;         // type 3
    CCXUINT8                Channel;
    CCX_RM_SCAN_MODE        ScanMode;
    CCXUINT16               Duration;
} CCX_RM_BEACON_REQ_IE, *PCCX_RM_BEACON_REQ_IE;

typedef struct _CCX_RM_FRAME_REQ_IE {
    CCX_RM_REQ_IE_HDR       Header;         // type 4
    CCXUINT8                Channel;
    CCXUINT16               Duration;
    CCXUINT8                NumBSSIDs;
    CCX_802_11_MAC_ADDRESS  BSSID[1];
} CCX_RM_FRAME_REQ_IE, *PCCX_RM_FRAME_REQ_IE;

//------------------------------------------------------------------------------
// RM report sent by IHV Service to CCX Service Library in response to one or
// more RM requests.
//------------------------------------------------------------------------------
typedef struct _CCX_RM_REPORT {
    CCXUINT16   NumReports;
    CCXUINT8    ReportElements[1];         // one or more reports
} CCX_RM_REPORT, *PCCX_RM_REPORT;

//------------------------------------------------------------------------------
// Header struct for RM Report
//------------------------------------------------------------------------------
typedef struct _CCX_RM_RPT_IE_HDR {
    CCX_RM_RESULT           Result;
    CCXUINT16               Token;
    CCX_RM_TYPE             Type;
} CCX_RM_RPT_IE_HDR, *PCCX_RM_RPT_IE_HDR;

//------------------------------------------------------------------------------
// The RM Reports
//------------------------------------------------------------------------------
typedef struct _CCX_RM_CCA_RPT_IE {
    CCX_RM_RPT_IE_HDR       Header;         // type 1
    CCXUINT16               Duration;
    CCXUINT8                BusyFraction;
} CCX_RM_CCA_RPT_IE, *PCCX_RM_CCA_RPT_IE;

typedef struct _CCX_RM_RPI_RPT_IE {
    CCX_RM_RPT_IE_HDR       Header;         // type 2
    CCXUINT16               Duration;
    CCXUINT8                RPI[8];
} CCX_RM_RPI_RPT_IE, *PCCX_RM_RPI_RPT_IE;

typedef struct _CCX_RM_BEACON_RPT_IE {
    CCX_RM_RPT_IE_HDR       Header;         // type 3
    CCXUINT16               Duration;
} CCX_RM_BEACON_RPT_IE, *PCCX_RM_BEACON_RPT_IE;

// 
// Frame report data
// 
typedef struct _CCX_RM_FRAME_RPT_QUAD {
    CCX_802_11_MAC_ADDRESS  SA;
    CCX_802_11_MAC_ADDRESS  BSSID;
    CCXINT8                 RSSI;
    CCXUINT8                NumFrames;
} CCX_RM_FRAME_RPT_QUAD, *PCCX_RM_FRAME_RPT_QUAD;

typedef struct _CCX_RM_FRAME_RPT_IE {
    CCX_RM_RPT_IE_HDR       Header;         // type 4
    CCXUINT16               Duration;
    CCXUINT32               NumSTAs;
    CCX_RM_FRAME_RPT_QUAD   QuadList[1];
} CCX_RM_FRAME_RPT_IE, *PCCX_RM_FRAME_RPT_IE;


//
// structure used with OID_CCX_STA_STATISTICS_2 OID
//
typedef struct _CCX_STA_STATS_GRP_2 {
    CCXUINT32   RSNAStatsSelectedPairwiseCipher;
//TKIP Statistics
    CCXUINT32   RSNAStatsTKIPICVErrors;
    CCXUINT32   RSNAStatsTKIPLocalMICFailures;
    CCXUINT32   RSNAStatsTKIPReplays;
    CCXUINT32   RSNAMgmtStatsTKIPICVErrors;
    CCXUINT32   RSNAMgmtStatsTKIPLocalMICFailures;
    CCXUINT32   RSNAMgmtStatsTKIPReplays;
    CCXUINT32   RSNAMgmtStatsTKIPMHDRErrors;
    CCXUINT32   RSNAMgmtStatsTKIPNoEncryptErrors;
//CCMP Statistics
    CCXUINT32   RSNAStatsCCMPReplays;
    CCXUINT32   RSNAStatsCCMPDecryptErrors;
    CCXUINT32   RSNAMgmtStatsCCMPReplays;
    CCXUINT32   RSNAMgmtStatsCCMPDecryptErrors;
    CCXUINT32   RSNAMgmtStatsCCMPNoEncryptErrors;
// Broadcast Statistics
    CCXUINT32   RSNAStatsBroadcastDisassociateCount;
    CCXUINT32   RSNAStatsBroadcastDeauthenticateCount;
    CCXUINT32   RSNAStatsBroadcastActionFrameCount;
} CCX_STA_STATS_GRP_2, *PCCX_STA_STATS_GRP_2;

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccxDiag.h in Vsita CCX Header file
//
//--------------------------------------------------------------------
typedef struct _CCX_FRAME_LOGGING_MODE 
{
    CCXBOOLEAN                      bEnabled;
} CCX_FRAME_LOGGING_MODE, *PCCX_FRAME_LOGGING_MODE;

typedef enum _CCX_DIAG_MODE
{
    CCX_DIAG_MODE_OFF,
    CCX_DIAG_MODE_ON
} CCX_DIAG_MODE, *PCCX_DIAG_MODE;

typedef struct CCX_DIAG_ACTION_START
{
    PCCXCWSTR               profileName;
} CCX_DIAG_ACTION_START, *PCCX_DIAG_ACTION_START;


typedef enum _DIAG_NOTIFICATION_ID
{
    DIAG_NOTE_SSID_INVALID                          = 1,
    DIAG_NOTE_NETWORK_SETTING_INVALID               = 2,
    DIAG_NOTE_WLAN_CAP_MISMATCH                     = 3,
    DIAG_NOTE_INCORRECT_USER_CREDENTIAL             = 4,
    DIAG_NOTE_CALL_SUPPORT                          = 5,
    DIAG_NOTE_PROBLEM_RESOLVED                      = 6,
    DIAG_NOTE_PROBLEM_UNRESOLVED                    = 7,
    DIAG_NOTE_TRY_AGAIN_LATER                       = 8,
    DIAG_NOTE_CORRECT_INDICATED_PROBLEM             = 9,
    DIAG_NOTE_NETWORK_REFUSED_TROUBLESHOOTING       = 10,
    DIAG_NOTE_RETRIEVING_CLIENT_REPORTS             = 11,
    DIAG_NOTE_RETRIEVING_CLIENT_LOGS                = 12,
    DIAG_NOTE_RETRIEVAL_COMPLETE                    = 13,
    DIAG_NOTE_BEGIN_ASSOC_TEST                      = 14,
    DIAG_NOTE_BEGIN_DHCP_TEST                       = 15,
    DIAG_NOTE_BEGIN_GTW_PING_TEST                   = 16,       //This needs to be verified
    DIAG_NOTE_BEGIN_DNS_PING_TEST                   = 17,
    DIAG_NOTE_BEGIN_NAME_RESOLUTION_TEST            = 18,
    DIAG_NOTE_BEGIN_8021X_TEST                      = 19,
    DIAG_NOTE_PROFILE_REDIRECT                      = 20,
    DIAG_NOTE_TEST_COMPLETE                         = 21,
    DIAG_NOTE_TEST_PASSED                           = 22,
    DIAG_NOTE_TEST_FAILED                           = 23,
    DIAG_NOTE_END_DIAG_SELECT_PROFILE               = 24,       //We may want to comment this out as it doesn't make much sense
    DIAG_NOTE_CLIENT_REFUSED_LOG_RETRIEVAL          = 25,
    DIAG_NOTE_CLIENT_REFUSED_CLIENT_REPORT          = 26,
    DIAG_NOTE_CLIENT_REFUSED_TEST                   = 27,
    DIAG_NOTE_INVALID_IP_SETTING                    = 28,
    DIAG_NOTE_KNOWN_NETWORK_PROBLEM                 = 29,
    DIAG_NOTE_SCHEDULED_NETWORK_MAINTENANCE         = 30,
    DIAG_NOTE_WLAN_SECURITY_METHOD_INCORRECT        = 31,
    DIAG_NOTE_WLAN_ENCRYPTION_METHOD_INCORRECT      = 32,
    DIAG_NOTE_WLAN_AUTHENTICATION_METHOD_INCORRECT  = 33,


    DIAG_NOTE_CONNECT_TO_DIAG_CHANNEL_SUCCEEDED     = 0x80000000,
    DIAG_NOTE_FAILED_TO_CONNECT_TO_DIAG_CHANNEL,
    DIAG_NOTE_FAILED_TO_RECONNECT_TO_DIAG_CHANNEL,
    DIAG_NOTE_INTERNAL_ERROR,
	DIAG_NOTE_DIAG_FINISHED,
	DIAG_NOTE_DNS_PING_REQ_RECVD,
	DIAG_NOTE_GATEWAY_REQ_RECVD,
	DIAG_NOTE_DHCP_REQ_RECVD,
	DIAG_NOTE_DNS_RESOLVE_REQ_RECVD,
	DIAG_NOTE_ASSOC_REQ_RECVD,
	DIAG_NOTE_AUTH_REQ_RECVD,
	DIAG_NOTE_PERFOMING_TEST,
	DIAG_NOTE_REFUSED_TEST,
	DIAG_NOTE_INCAPABLE_TEST,
	DIAG_NOTE_SENT_TEST_RESPONSE,
	DIAG_NOTE_RECEIVED_TEST_ABORT,
	DIAG_NOTE_RECEIVED_PROFILE_REDIRECT

} DIAG_NOTIFICATION_ID;


typedef struct _CCX_NOTIFY_DIAG_MSG
{
    DIAG_NOTIFICATION_ID            msgId;
    CCXUINT32                       msgLen;
    CCXUINT32                       msgOffset; 
} CCX_NOTIFY_DIAG_MSG, *PCCX_NOTIFY_DIAG_MSG;

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccxNL.h in Vsita CCX Header file
//
//--------------------------------------------------------------------

// Index values for neighbor list entry subelements
// These are used to index into the subElemOffset array to retreive the
//       data offsets for each subelement type.  
// Note: These values do not match the subelement type values used in the 
//       TLV's received in the IAPP message.
typedef enum _CCX_NL_SUBELEM_INDEX
{
    CCX_NL_SUBELEM_INDEX_RF_PARAMS,
    CCX_NL_SUBELEM_INDEX_TSF_INFO,
    CCX_NL_SUBELEM_INDEX_ACE,

    CCX_NL_SUBELEM_INDEX_MAX
} CCX_NL_SUBELEM_INDEX, *PCCX_NL_SUBELEM_INDEX;

// RF parameters format for neighbor list subelement
//    descriptions of field values can be found in CCX V5, S51.2.5.1
typedef struct _CCX_NL_SUBELEM_RFPARAMS
{
    CCXUINT8 minRxSignalPower;
    CCXUINT8 APTxPower;
    CCXUINT8 clientTxPower;
    CCXUINT8 roamingHysteresis;
    CCXUINT8 adaptScanThreshold;
    CCXUINT8 transitionTime;
} CCX_NL_SUBELEM_RFPARAMS, *PCCX_NL_SUBELEM_RFPARAMS;

// TSF information format for neighbor list subelement
//     descriptions of field values can be found in CCX V5, S51.2.5.2
typedef struct _CCX_NL_SUBELEM_TSFINFO
{
    CCXUINT16 TSFOffset;
    CCXUINT16 beaconInterval;
} CCX_NL_SUBELEM_TSFINFO, *PCCX_NL_SUBELEM_TSFINFO;

// ACE parameter format for neighbor list subelement
//     descriptions of field values can be found in CCX V5, S51.2.5.3
typedef struct _CCX_NL_SUBELEM_ACE
{
    CCXUINT8 localSubnetInd;
    CCXUINT8 signalStrength;
    CCXUINT8 signalQuality;
    CCXUINT8 time;
    CCXUINT8 loadFactor;
} CCX_NL_SUBELEM_ACE, *PCCX_NL_SUBELEM_ACE;

// Neighbor list entry format.  
//  This is the format of the data for each entry in the neighbor list
typedef struct _CCX_NL_ELEMENT
{
    CCXUINT32               length;         // overall length of element in bytes
    CCX_802_11_MAC_ADDRESS  BSSID;          // BSSID of AP
    CCXUINT8                channel;        // current channel being used by AP
    CCXUINT8                channelBand;    // channel band being used by AP
    CCX_PHY_TYPE            phyType;        // phy type of AP

                                // subelem offsets are the offset from the start of this structure 
                                //  where the data for each subelement type is located.  Each 
                                //  subelement's data is formatted in it's struct type defined 
                                //  below.  If the offset for a subelement is 0, there is no 
                                //  data for that particular subelement.
    CCXUINT32               subElemOffset[CCX_NL_SUBELEM_INDEX_MAX];
} CCX_NL_ELEMENT, *PCCX_NL_ELEMENT;

// Neighbor list
//      Holds 1 or more neighbor list entries defined above
typedef struct _CCX_NEIGHBOR_LIST
{
    CCXUINT32           length;             // overall length of list in bytes
    CCXUINT32           numOfEntries;       // number of entries in the list
    CCX_NL_ELEMENT      ccxElement[1];      // list elements
} CCX_NEIGHBOR_LIST, *PCCX_NEIGHBOR_LIST;

//--------------------------------------------------------------------
//
// Here we define the predefine Parameter in ccx_common.h in Vsita CCX Header file
//
//--------------------------------------------------------------------

//---------------------------------------------------------------------------
// Custom auth algorithms
//---------------------------------------------------------------------------
#define DOT11_AUTH_ALGO_CCKM                            DOT11_AUTH_ALGO_IHV_START   + 0x00000001L

//---------------------------------------------------------------------------
// Custom cipher algorithms
//---------------------------------------------------------------------------
#define DOT11_CIPHER_ALGO_MFPCCMP                       DOT11_CIPHER_ALGO_IHV_START + 0x00000001L
#define DOT11_CIPHER_ALGO_MFPTKIP                       DOT11_CIPHER_ALGO_IHV_START + 0x00000002L

//---------------------------------------------------------------------------
// Custom roaming reasons
//---------------------------------------------------------------------------
#define DOT11_ROAMING_REASON_PEER_DEAUTHENTICATED       DOT11_ASSOC_STATUS_PEER_DEAUTHENTICATED
#define DOT11_ROAMING_REASON_PEER_DISASSOCIATED         DOT11_ASSOC_STATUS_PEER_DISASSOCIATED
#define DOT11_ROAMING_REASON_FAILED_8021X_EAP_AUTHEN    DOT11_ROAMING_REASON_IHV_START + 0x00000001L
#define DOT11_ROAMING_REASON_MAX_RETRIES                DOT11_ROAMING_REASON_IHV_START + 0x00000002L


//---------------------------------------------------------------------------
// Event codes
//---------------------------------------------------------------------------
#define CCX_EVENT_STATUS_INDICATION                 0x00000001L     // from driver
#define CCX_EVENT_PACKET_RECEIVED                   0x00000002L     // from driver
#define CCX_EVENT_PACKET_TRANSMITTED                0x00000003L     // from driver
#define CCX_EVENT_OID                               0x00000004L     // to driver
#define CCX_EVENT_OK_TO_ASSOCIATE                   0x00000005L     // to driver
#define CCX_EVENT_SEND_PACKET                       0x00000006L     // to driver

//---------------------------------------------------------------------------
// TLV codes associated with event codes
//---------------------------------------------------------------------------
#define CCX_TLV_802_11_PACKET                       0x00000001L
#define CCX_TLV_802_11_FRAME                        0x00000002L
#define CCX_TLV_BEACON_IHV_DATA                     0x00000003L
#define CCX_TLV_RM_REPORT                           0x00000004L
#define CCX_TLV_RX_CHARACTERISTICS                  0x00000005L
#define CCX_TLV_TX_CHARACTERISTICS                  0x00000006L
#define CCX_TLV_QUERY_DATA                          0x00000007L
#define CCX_TLV_SET_DATA                            0x00000008L
#define CCX_TLV_METHOD_DATA                         0x00000009L
#define CCX_TLV_ADAPTER_RESET                       0x0000000AL

// These tlv codes correspond to the like named DOT_11 status indications defined by Microsoft.
// The values are identical to the Microsoft defined status indications.
#ifndef NDIS_STATUS_DOT11_SCAN_CONFIRM
#define NDIS_STATUS_DOT11_SCAN_CONFIRM              0x40030000L
#define NDIS_STATUS_DOT11_MPDU_MAX_LENGTH_CHANGED   0x40030001L
#define NDIS_STATUS_DOT11_ASSOCIATION_START         0x40030002L
#define NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION    0x40030003L
#define NDIS_STATUS_DOT11_CONNECTION_START          0x40030004L
#define NDIS_STATUS_DOT11_CONNECTION_COMPLETION     0x40030005L
#define NDIS_STATUS_DOT11_ROAMING_START             0x40030006L
#define NDIS_STATUS_DOT11_ROAMING_COMPLETION        0x40030007L
#define NDIS_STATUS_DOT11_DISASSOCIATION            0x40030008L
#define NDIS_STATUS_DOT11_TKIPMIC_FAILURE           0x40030009L
#define NDIS_STATUS_DOT11_PMKID_CANDIDATE_LIST      0x4003000AL
#define NDIS_STATUS_DOT11_PHY_STATE_CHANGED         0x4003000BL
#define NDIS_STATUS_DOT11_LINK_QUALITY              0x4003000CL
#define NDIS_STATUS_DOT11_INCOMING_ASSOC_STARTED 0x4003000DL
#define NDIS_STATUS_DOT11_INCOMING_ASSOC_REQUEST_RECEIVED 0x4003000EL
#define NDIS_STATUS_DOT11_INCOMING_ASSOC_COMPLETION 0x4003000FL
#endif


//---------------------------------------------------------------------------
// Format of Profile Parameters  structure.
//---------------------------------------------------------------------------
typedef struct _PROFILE_PARAMS_ {
//    PDOT11EXT_IHV_CONNECTIVITY_PROFILE  pIhvConnProfile;
    BOOL                                bUseMSOnex;
    ULONG                               dot11AuthAlgorithm;
    DOT11_CIPHER_ALGORITHM              dot11CipherAlgorithm;
    BOOL                                bRadioMeasEnabled;
} PROFILE_PARAMS, *PPROFILE_PARAMS;

#pragma pack( 4 )

//---------------------------------------------------------------------------
// Format of TLV in CCX_NIC_SPECIFIC_EXTENSION structure.
//---------------------------------------------------------------------------
typedef struct _IHV_CCX_TLV {
    ULONG           type;       // TLV type code (see defined type codes below)
    ULONG           length;     // length of the data in value array (in bytes)
    UCHAR           value[1];   // data associated with the specified type code
} IHV_CCX_TLV, *PIHV_CCX_TLV;

#pragma pack()


//---------------------------------------------------------------------------
// Structure defining a count followed by array of TLV.
//---------------------------------------------------------------------------
typedef struct _IHV_CCX_TLV_DATA {
    ULONG           tlvCount;   // number of TLV in tlv array.
    IHV_CCX_TLV     tlv[1];     // variable number of variable length tlv
} IHV_CCX_TLV_DATA, *PIHV_CCX_TLV_DATA;

//---------------------------------------------------------------------------
// Format of CCX data passed to OID_DOT11_NIC_SPECIFIC_EXTENSION.
//---------------------------------------------------------------------------
typedef struct _CCX_NIC_SPECIFIC_EXTENSION {
    ULONG                   len;        // length of entire structure (excluding this field)
    UCHAR                   oui[4];     // vendor specific OUI value
    ULONG                   event;      // event code
    IHV_CCX_TLV_DATA        tlvData;    // tlv data
} CCX_NIC_SPECIFIC_EXTENSION, *PCCX_NIC_SPECIFIC_EXTENSION;


//---------------------------------------------------------------------------
// Format of CCX_EVENT_OID data when tlv is of type CCX_TLV_SET_DATA
// Note - This definition machtes the CCX_REQUEST_DATA's SET_INFO structure
//---------------------------------------------------------------------------
typedef struct _IHV_SET_INFO
{
    UINT32                  Oid;
    UINT32                  BytesRead;
    UINT32                  BytesNeeded;
    UINT32                  InformationBufferLength;
    UINT8                   InformationBuffer[1];
} IHV_SET_INFO, *PIHV_SET_INFO;


//---------------------------------------------------------------------------
// Format of CCX_EVENT_OID data when tlv is of type CCX_TLV_QUERY_DATA
// Note - This definition machtes the CCX_REQUEST_DATA's QUERY_INFO structure
//---------------------------------------------------------------------------
typedef struct _IHV_QUERY_INFO
{   
    UINT32                  Oid;
    UINT32                  BytesWritten;
    UINT32                  BytesNeeded;
    UINT32                  InformationBufferLength;
    UINT8                   InformationBuffer[1];
} IHV_QUERY_INFO, *PIHV_QUERY_INFO;


//---------------------------------------------------------------------------
// Format of CCX_EVENT_OID data when tlv is of type CCX_TLV_METHOD_DATA
// Note - This definition machtes the CCX_REQUEST_DATA's METHOD_INFO structure
//---------------------------------------------------------------------------
typedef struct _IHV_METHOD_INFO
{
    UINT32                  Oid;
    UINT32                  BytesWritten;
    UINT32                  BytesNeeded;
    UINT32                  BytesRead;
    UINT32                  InputBufferLength;
    UINT32                  InformationBufferLength;
    UINT8                   InformationBuffer[1];
} IHV_METHOD_INFO, *PIHV_METHOD_INFO;


//---------------------------------------------------------------------------
// Structure used to pass packet to driver.
//---------------------------------------------------------------------------
typedef struct _IHV_CCX_PACKET {
    HANDLE                  hIhvExtAdapter; // IHV adapter handle.
    HANDLE                  hTxComplete;    // handle needed to complete the packet back to service lib.
    UCHAR                   packet[1];      // packet data
} IHV_CCX_PACKET, *PIHV_CCX_PACKET;

//---------------------------------------------------------------------------
// Used to pass tlv(s) to CcxIndicateEvent
//---------------------------------------------------------------------------
typedef struct _TLV_STRUCT
{
    ULONG                   type;   
    ULONG                   length;
    PVOID                   value;
} TLV_STRUCT, *PTLV_STRUCT;

//---------------------------------------------------------------------------
// Receive characteristics (just use the DOT11 receive context).
//---------------------------------------------------------------------------
typedef DOT11_EXTSTA_RECV_CONTEXT   IHV_RX_CHARACTERISTICS, *PIHV_RX_CHARACTERISTICS;

//---------------------------------------------------------------------------
// Completion reason codes for packets passed from IHV service to driver for transmission.
// 
//  IHV_TX_COMPL_REASON_SUCCESS
//      -packet was successfully sent
// 
//  IHV_TX_COMPL_REASON_FAILED           
//      -MPSendNetBufferLists() failed to submit the packet for transmission
// 
//  IHV_TX_COMPL_REASON_CANCELLED   
//      -send was cancelled due to pause, reset, surprise removal.
// 
//  IHV_TX_COMPL_REASON_NDIS_CANCELLED   
//      -send was cancelled due to NDIS calling MPCancelSendNetBufferLists()
// 
//  IHV_TX_COMPL_REASON_ADAPTER_NOT_READY
//      -MPSendNetBufferLists() determined that it could not perform the send.
//       i.e. MP_ADAPTER_CANNOT_SEND_PACKETS() macro returns TRUE.
//
//  IHV_TX_COMPL_REASON_ROAMING
//      -send was cancelled because we're roaming.
//---------------------------------------------------------------------------
#define IHV_TX_COMPL_REASON_SUCCESS                     0L
#define IHV_TX_COMPL_REASON_FAILED                      1L
#define IHV_TX_COMPL_REASON_CANCELLED                   2L
#define IHV_TX_COMPL_REASON_NDIS_CANCELLED              3L
#define IHV_TX_COMPL_REASON_ADAPTER_NOT_READY           4L
#define IHV_TX_COMPL_REASON_ROAMING                     5L

//---------------------------------------------------------------------------
// Transmit characteristics
//---------------------------------------------------------------------------
typedef struct _IHV_TX_CHARACTERISTICS 
{
    HANDLE      hSrvLibCompletion;
    ULONG       txFlags;
    UCHAR       phyID;
    ULONG       frequency;
    USHORT      numMPDU;
    ULONG       dataRate;
    ULONGLONG   tsf;
    UCHAR       retryCount;
} IHV_TX_CHARACTERISTICS, *PIHV_TX_CHARACTERISTICS;

//****************************************************************************
//*
//*	Define in Sta.h in CCX Sample code 85 Driver
//*
//****************************************************************************

typedef struct _STA_EAP_SESSION {
    BOOLEAN         bSessionTimeout;            //Flag to indicate if session timeout occured
    BOOLEAN         bEapFailed;                 //set TRUE on receipt of EAP_CODE_FAILURE packet.
    BOOLEAN         bPrivacySet;                //Flag which says if EAPOL Request ID was Rx
                                                //unencrypted, if so then Tx EAPOL message
                                                //unencrypted
} STA_EAP_SESSION, *PEAP_SESSION;

// 
// Maximum number of request element we allow in a single request.
// 
#define MAX_RM_REQUEST_ELEMENTS    16

//
// MFP MIB variables.
//
typedef struct _MFP_MIB_VARS 
{
    LONG   dot11RSNAMgmtStatsTKIPICVErrors;
    LONG   dot11RSNAMgmtStatsTKIPLocalMICFailures;
    LONG   dot11RSNAMgmtStatsCCMPDecryptErrors;


    LONG   dot11RSNAMgmtStatsTKIPMHDRErrors;
    LONG   dot11RSNAMgmtStatsTKIPReplays;
    LONG   dot11RSNAMgmtStatsCCMPReplays;
    LONG   dot11RSNAMgmtStatsTKIPNoEncryptErrors;
    LONG   dot11RSNAMgmtStatsCCMPNoEncryptErrors;
    LONG   dot11RSNAStatsBroadcastDisassociateCount;
    LONG   dot11RSNAStatsBroadcastDeauthenticateCount;
    LONG   dot11RSNAStatsBroadcastActionFrameCount;
} MFP_MIB_VARS, *PMFP_MIB_VARS;

/**
 * Maintains CCX RM request info.
 */
typedef struct _STA_CCX_RM_CONTEXT {

    NDIS_HANDLE         RM_WorkItem;            // work item to handle RM requests.
    BOOLEAN             rmIndicateBeacons;      // if TRUE then indicate all incoming beacon frames to IHV service.
    BOOLEAN             rmIndicateProbeResps;   // if TRUE then indicate all incoming probe resp frames to IHV service.
    BOOLEAN             rmIndicateFrames;       // if TRUE then indicate all incoming frames to IHV service.

    NDIS_MINIPORT_TIMER timer;
    LONG                timerSync;
    ULONG               rmReqLen;
    PUCHAR              rmReq;
    
    ULONG               beaconCount;    // temporary
    ULONG               probeCount;     // temporary
    ULONG               frameCount;     // temporary

    USHORT              numReqElems;
    PCCX_RM_REQ_IE_HDR  reqElements[MAX_RM_REQUEST_ELEMENTS];
} STA_CCX_RM_CONTEXT, *PSTA_CCX_RM_CONTEXT;

/**
 * Maintains CCX diagnostics context
 */
typedef struct _STA_CCX_DIAG_CONTEXT 
{
//    PDIAG_CONTROLS  DiagCapCtl;             // Various settings for diag/capture mode Mark For Compile
    BOOLEAN         diagMode;               // if TRUE then diag mode is enabled.
    BOOLEAN         diagCaptureFrames;      // if TRUE then indicate all incoming/outgoing frames to IHV service.
} STA_CCX_DIAG_CONTEXT, *PSTA_CCX_DIAG_CONTEXT;

/**
 * Maintains CCX MFP context
 */
typedef struct _STA_CCX_MFP_CONTEXT 
{
    BOOLEAN             mfpActive;              // if TRUE then protect/validate class 3 management frames.
    ULONGLONG           mfpReplayCounter;       // watch for management frame replays
    MFP_MIB_VARS        mfpStats;               // MFP related counters.
//    DOT11_MGMT_HEADER   fragHeader;             // store header from first fragment so we can MFP validate 
                                                //    subsequent fragments against it.
//    PDOT11_MGMT_HEADER  pFirstFragHeader;       // non-NULL when there are more frags of an MSDU to come.
} STA_CCX_MFP_CONTEXT, *PSTA_CCX_MFP_CONTEXT;

/**
 * Maintains CCX diagnostics context
 */
typedef struct _STA_CCX_ASSOC_CONTEXT 
{
    BOOLEAN			Assoc_OID_Set;				//For set oid by DLL and go Normal assoc process
    NDIS_MINIPORT_TIMER OKToAssocTimeoutTimer;  // timer to timeout OK to assoc event from IHV service.
    LONG                OKToAssocTimerSync;     // 
    ULONG               AssocReqIESize;         // size of AssocReqIE buffer.
    PUCHAR              AssocReqIE;             // array of IE to include in association request.
    ULONG               txPowerLevel;           // Power level controlled by the AP.
} STA_CCX_ASSOC_CONTEXT, *PSTA_CCX_ASSOC_CONTEXT;

/**
 * Maintains roam context
 */
typedef struct _STA_CCX_ROAM_CONTEXT 
{
    BOOLEAN                 roamingActive;          // are we actively roaming?
    ULONG                   txFailsInRow;           // counter tracking successive TX failures
    BOOLEAN                 txFailLimitMet;         // the max number of successive tx failures has been hit - time to roam
} STA_CCX_ROAM_CONTEXT, *PSTA_CCX_ROAM_CONTEXT;


//---------------------------------------------------------------------------
// IAPP handling
//---------------------------------------------------------------------------

//
// Netbuffer reuse queue
//
typedef struct _IAPP_NET_BUFFER_QUEUE {
    NDIS_HANDLE       Iappbufferpoolhandle;
    NDIS_HANDLE       Iappbufferlistpoolhandle;

    //List of NBL which are free to use for Tx
    PNET_BUFFER_LIST  FreeTxNBL;
    UINT              FreeTxNBLCount;
    NDIS_SPIN_LOCK    TxQlock;
 
    PADAPTER          pAdapter;
} IAPP_BUFQUEUE, *PIAPP_BUFQUEUE;

/**
 * Maintains all CCX station specific state information
 */
typedef struct _Cisco_CCX_CONTEXT 
{

    STA_EAP_SESSION         eapSession;             // Structure containing info about EAP session

    PIAPP_BUFQUEUE          RecycleQueue;           // used for pool of unused NET_BUFFER_LISTS (IAPP handling)

    STA_CCX_ASSOC_CONTEXT   AssocContext;           // context infor for association 

    STA_CCX_DIAG_CONTEXT    DiagContext;            // context info for diagnostics

    STA_CCX_RM_CONTEXT      RMContext;              // context info for radio management

    STA_CCX_MFP_CONTEXT     MFPContext;             // context info for MFP

    STA_CCX_ROAM_CONTEXT    RoamContext;            // roaming context.

} Cisco_CCX_CONTEXT, *PCisco_CCX_CONTEXT;

