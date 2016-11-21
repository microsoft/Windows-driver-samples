#ifndef __INC_ROGUEAP_H
#define __INC_ROGUEAP_H


typedef struct _ROGUEAP_REPORT_PACKET{
	u1Byte				SnapHeader[8]; // Cisco Aironet SNAP header: AAAA 0300 4096 0000
	u2Byte				ROGUEAPLen;    //always 40 bytes
	u1Byte				message_type; // 0x40.
	u1Byte				function_code; // 0x8e.
	u1Byte				DstAddr[6]; // MAC address of the AP.
	u1Byte				SrcAddr[6]; // Reporting STA's MAC address. 
	u2Byte				Failure_Reason;
	u1Byte                          RogueAPAddr[6]; //MAC address of the client reporting the AP.
	u1Byte				RogueAPname[16]; // All Null if not known
}ROGUEAP_REPORT_PACKET, *PROGUEAP_REPORT_PACKET;


typedef struct _MH_CCX_ROGUE_AP_STATUS{
	u4Byte length;
	u4Byte version;
	u4Byte code;
	u1Byte bssid[6];

} MH_CCX_ROGUE_AP_STATUS,*PMH_CCX_ROGUE_AP_STATUS;

#define ROGUEAP_message_type  0x40
#define ROGUEAP_function_code 0x8e



#define MH_CCX_ROGUE_AP_STATUS_LENGTH                           18
#define MH_CCX_ROGUE_AP_STATUS_VERSION                           1

#define MH_CCX_ROGUE_AP_STATUS_AuthTimeoutFailure           1
#define MH_CCX_ROGUE_AP_STATUS_ChallengeFromApFailure    2
#define MH_CCX_ROGUE_AP_STATUS_ChallengeToApFailure        3
#define MH_CCX_ROGUE_AP_STATUS_AuthSuccess                     4
#define MH_CCX_ROGUE_AP_STATUS_ClearRogues                     5
#define MH_CCX_ROGUE_AP_STATUS_Invalidauthenticationtype 6

	
//-----------------------------------------------------
//name : CCX_Add_ROGUE_AP_Entry
//function : Add an Entry to ROGUE AP List
//Input : ROGUEAP => Aegis offer data about ROGUE AP
//-----------------------------------------------------
void
CCX_Add_ROGUE_AP_Entry( 
	PADAPTER Adapter,
	MH_CCX_ROGUE_AP_STATUS *ROGUEAP );

//-----------------------------------------------------
//name : CCX_Add_ROGUE_AP_Authfail_Entry
//function : Add an Entry to ROGUE AP List for fail code : 0x00 0x01
//Input : 
//-----------------------------------------------------
void
CCX_Add_ROGUE_AP_Authfail_Entry( 
	PADAPTER Adapter  );

//-----------------------------------------------------
//name : CCX_Del_ROGUE_AP_Entry
//function : Del an Entry to ROGUE AP List When successful connent to AP
//Input : ROGUEAP => Aegis offer data about ROGUE AP
//-----------------------------------------------------
void
CCX_Del_ROGUE_AP_Entry( 
	PADAPTER Adapter,
	MH_CCX_ROGUE_AP_STATUS *ROGUEAP );
	
//-----------------------------------------------------
//name : CCX_Clear_ROGUE_AP_All_Entry
//function : Del an Entry to ROGUE AP List When all AP connent fail.
//Input : 
//-----------------------------------------------------
void
CCX_Clear_ROGUE_AP_All_Entry( 
	PADAPTER Adapter );

//-----------------------------------------------------
//name : SendCcxROGUEAPReport
//function : Send all ROGUE AP report in list to AP ( which connected successful )
//Input : 
//-----------------------------------------------------
void
SendCcxROGUEAPReport(
	PADAPTER Adapter );
	
#endif
