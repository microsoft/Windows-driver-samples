#ifndef	__CUSTOM_OID_H
#define __CUSTOM_OID_H


//3*****************************************************************************************************
//3 0xFF818000 - 0xFF81802F		RTL81xx Mass Production Kit
//3 0xFF818500 - 0xFF81850F		RTL8185 Setup Utility
//3 0xFF818580 - 0xFF81858F		RTL8185 Phy Status Utility
//3 0xFF818700 - 0xFF8187FF		RTL8187 related
//3 0xFF819000 - 0xFF8190FF		RTL8190 related

// by Owen for Production Kit
//3 For Production Kit with Agilent Equipments
//3 in order to make our custom oids hopefully somewhat unique
//3 we will use 0xFF (indicating implementation specific OID)
//3               		81(first byte of non zero Realtek unique identifier)
//3	        80 (second byte of non zero Realtek unique identifier)
//3               		XX (the custom OID number - providing 255 possible custom oids)

//3*****************************************************************************************************


//3--------------------------------------------------------------------
//3 			0xFF8180xx: Mass Prodution
//3--------------------------------------------------------------------
#define		OID_RT_PRO_RESET_DUT							0xFF818000
#define		OID_RT_PRO_SET_DATA_RATE						0xFF818001
#define		OID_RT_PRO_START_TEST							0xFF818002
#define		OID_RT_PRO_STOP_TEST    						0xFF818003
#define		OID_RT_PRO_SET_PREAMBLE							0xFF818004
#define		OID_RT_PRO_SET_TX_PMAC							0xFF818005
#define		OID_RT_PRO_SET_TX_TMAC							0xFF818006
#define		OID_RT_PRO_SET_MANUAL_DIVERSITY_BB				0xFF818007
#define		OID_RT_PRO_SET_CHANNEL_DIRECT_CALL				0xFF818008
#define		OID_RT_PRO_SET_SLEEP_MODE_DIRECT_CALL			0xFF818009
#define		OID_RT_PRO_SET_WAKE_MODE_DIRECT_CALL			0xFF81800A
#define		OID_RT_PRO_SET_TX_CONTINUOUS_DIRECT_CALL		0xFF81800B
#define		OID_RT_PRO_SET_SINGLE_CARRIER_TX_CONTINUOUS		0xFF81800C
#define		OID_RT_PRO_SET_TX_ANTENNA_BB					0xFF81800D
#define		OID_RT_PRO_SET_ANTENNA_BB						0xFF81800E
#define		OID_RT_PRO_SET_CR_SCRAMBLER						0xFF81800F
#define		OID_RT_PRO_SET_CR_NEW_FILTER					0xFF818010
#define		OID_RT_PRO_SET_TX_POWER_CONTROL					0xFF818011
#define		OID_RT_PRO_SET_CR_TX_CONFIG						0xFF818012
#define		OID_RT_PRO_GET_TX_POWER_CONTROL					0xFF818013
#define		OID_RT_PRO_GET_CR_SIGNAL_QUALITY				0xFF818014
#define		OID_RT_PRO_SET_CR_SETPOINT						0xFF818015
#define		OID_RT_PRO_SET_INTEGRATOR						0xFF818016
#define		OID_RT_PRO_SET_SIGNAL_QUALITY					0xFF818017
#define		OID_RT_PRO_GET_INTEGRATOR						0xFF818018
#define		OID_RT_PRO_GET_SIGNAL_QUALITY					0xFF818019
#define		OID_RT_PRO_QUERY_EEPROM_TYPE					0xFF81801A
#define		OID_RT_PRO_WRITE_MAC_ADDRESS					0xFF81801B
#define		OID_RT_PRO_READ_MAC_ADDRESS						0xFF81801C
#define		OID_RT_PRO_WRITE_CIS_DATA						0xFF81801D
#define		OID_RT_PRO_READ_CIS_DATA						0xFF81801E
#define		OID_RT_PRO_WRITE_POWER_CONTROL					0xFF81801F
#define		OID_RT_PRO_READ_POWER_CONTROL					0xFF818020
#define		OID_RT_PRO_WRITE_EEPROM							0xFF818021
#define		OID_RT_PRO_READ_EEPROM							0xFF818022
#define		OID_RT_PRO_RESET_TX_PACKET_SENT					0xFF818023
#define		OID_RT_PRO_QUERY_TX_PACKET_SENT					0xFF818024
#define		OID_RT_PRO_RESET_RX_PACKET_RECEIVED				0xFF818025 
#define		OID_RT_PRO_QUERY_RX_PACKET_RECEIVED				0xFF818026
#define		OID_RT_PRO_QUERY_RX_PACKET_CRC32_ERROR			0xFF818027
#define		OID_RT_PRO_QUERY_CURRENT_ADDRESS				0xFF818028
#define		OID_RT_PRO_QUERY_PERMANENT_ADDRESS				0xFF818029
#define		OID_RT_PRO_SET_PHILIPS_RF_PARAMETERS			0xFF81802A
#define		OID_RT_PRO_SET_CARRIER_SUPPRESSION_TX_CONTINUOUS	0xFF81802B
#define		OID_RT_PRO_RECEIVE_PACKET						0xFF81802C
// added by Owen on 04/08/03 for Cameo's request
#define		OID_RT_PRO_WRITE_EEPROM_BYTE					0xFF81802D
#define		OID_RT_PRO_READ_EEPROM_BYTE						0xFF81802E
#define		OID_RT_PRO_SET_MODULATION						0xFF81802F
//

// 200705/24 mh add for bandwitdh test of mp temporarily 
#define		OID_RT_PRO_SET_BANDWIDTH						0xFF818030
#define		OID_RT_PRO_QUERY_BANDWIDTH						0xFF818031
#define		OID_RT_PRO_SET_TX_AGC_OFFSET					0xFF818032
// 2007/08/22 cosa add for MP
#define		OID_RT_PRO_SET_CRYSTALCAP						0xFF818033
// 2008/01/03 cosa add for MP
#define		OID_RT_PRO_TRIGGER_RF_THERMAL_METER				0xFF818034
#define		OID_RT_PRO_READ_RF_THERMAL_METER				0xFF818035
#define		OID_RT_PRO_GET_TX_POWER_INDEX					0xFF818036
#define		OID_RT_PRO_GET_CRYSTALCAP						0xFF818037
#define		OID_RT_PRO_AUTOLOAD_STATUS						0xFF818038
#define		OID_RT_PRO_MP_TEST_START						0xFF818039
// 2009/02/04 cosa add for MP
#define		OID_RT_PRO_GET_TX_POWER_DIFF					0xFF81803A
#define		OID_RT_PRO_GET_PGFmtVer							0xFF81803B
#define		OID_RT_PRO_SET_LED_CONTROL						0xFF81803C
#define		OID_RT_PRO_READ_WPS_BUTTON_PUSHED				0xFF81803D
#define		OID_RT_PRO_GET_CALCULATED_TX_POWER_INDEX		0xFF81803E
#define		OID_RT_PRO_GENERAL_QUERY						0xFF81803F
/* 2007/05/30 MH Define OID for debug command between UI and driver. There are
   three steps between UI and driver as below. */
#define		OID_RT_PRO_DBGCMD_SEND							0xFF818040
#define		OID_RT_PRO_DBGCMD_CHECK							0xFF818041
#define		OID_RT_PRO_DBGCMD_RETURN						0xFF818042
#define		OID_RT_PRO_SET_SINGLE_TONE_TX                   0xFF818043
// Add EFUSE R/W after 92S 
#define		OID_RT_PRO_WRITE_EFUSE							0xFF818044
#define		OID_RT_PRO_READ_EFUSE							0xFF818045
#define		OID_RT_PRO_UPDATE_EFUSE							0xFF818046
#define		OID_RT_PRO_GET_EFUSE_UTILIZE					0xFF818047
#define		OID_RT_PRO_CHK_AUTOLOAD							0xFF818049
#define		OID_RT_PRO_TXPWR_TRACK							0xFF81804A
#define		OID_RT_PRO_1x1_RXCOMBINE						0xFF81804B
#define		OID_RT_PRO_UPDATE_EFUSE_MAP					0xFF81804D	//read real efuse content

// Add Efuse R/W for BT
#define		OID_RT_PRO_WRITE_EFUSE_BT						0xFF818050
#define		OID_RT_PRO_READ_EFUSE_BT							0xFF818051
#define		OID_RT_PRO_UPDATE_EFUSE_BT						0xFF818052
#define		OID_RT_PRO_GET_EFUSE_UTILIZE_BT					0xFF818053
#define		OID_RT_PRO_UPDATE_EFUSE_MAP_BT					0xFF818054	//read real efuse content
#define		OID_RT_PRO_READ_PHYSICAL_EFUSE					0xFF818055
#define		OID_RT_PRO_QUERY_LED_STATUS						0xFF818056


#define		OID_RT_PRO_DBG_CONTROL							0xFF81807F

//Sean		
#define		OID_RT_DRIVER_OPTION							0xFF818080
#define		OID_RT_RF_OFF									0xFF818081
#define		OID_RT_AUTH_STATUS								0xFF818082
#define		OID_RT_INACTIVE_PS								0xFF818083
#define		OID_RT_PRO_RF_PATH_SWITCH						0xFF818084
#define		OID_RT_QUERY_RF_MODE							0xFF818085
#define		OID_RT_PRO_GET_BOARD_TYPE 						0xFF818086
#define		OID_RT_PRO_QUERY_INTERFACE_INDEX				0xFF818087
#define		OID_RT_PRO_QUERY_SLAVE_DMSP						0xFF818088
#define		OID_RT_PRO_QUERY_RF_STATUS						0xFF818089
#define		OID_RT_PRO_QUERY_CHANNEL_SWITCH_STATUS			0xFF81808A
#define		OID_RT_PRO_DISABLE_IQK_LCK_BY_THERMAL			0xFF81808B
#define		OID_RT_PRO_QUERY_ISVS							0xFF81808C
#define		OID_RT_PRO_TRIGGER_DPK							0xFF81808D
#define		OID_RT_PRO_QUERY_ISVL							0xFF81808E
#define		OID_RT_PRO_RF_ON_OFF							0xFF818091
#define		OID_RT_PRO_TEST_FW_DL							0xFF818092
#define		OID_RT_PRO_QUERY_TX_PHY_PACKET_SENT				0xFF818093
#define		OID_RT_PRO_QUERY_RX_PHY_PACKET_RECEIVED			0xFF818094
#define		OID_RT_PRO_QUERY_RX_PHY_PACKET_CRC32_ERROR		0xFF818095
#define 	OID_RT_PRO_TRIGGER_MPT_DIG 						0xFF818096
#define 	OID_RT_PRO_POWER_LIMIT_TABLE_SEL 				0xFF818097
#define 	OID_RT_PRO_CUSTOMER_POWER_BY_RATE_TABLE_QUERY	0xFF818098
#define 	OID_RT_PRO_CUSTOMER_POWER_LIMIT_TABLE_QUERY 	0xFF818099
#define 	OID_RT_PRO_CUSTOMER_POWER_BY_RATE_TABLE_RESET	0xFF81809A
#define 	OID_RT_PRO_CUSTOMER_POWER_LIMIT_TABLE_RESET 	0xFF81809B
#define		OID_RT_PRO_QUERY_RX_MAC_PACKET_RECEIVED			0xFF81809C
#define		OID_RT_PRO_QUERY_RX_MAC_PACKET_CRC32_ERROR		0xFF81809D
#define		OID_RT_PRO_GET_EFUSE_MASK						0xFF81809E
#define		OID_RT_PRO_QUERY_LAST_ERROR_MP					0xFF81809F
#define 	OID_RT_PRO_CFO_TRACK            				0xFF8180A0
#define 	OID_RT_PRO_GET_CUT_VERSION            			0xFF8180A1
#define 	OID_RT_PRO_GET_PHY_PARAM_VERSION            	0xFF8180A2
#define		OID_RT_PRO_WRITE_PSEUDO_EFUSE					0xFF8180A3
#define		OID_RT_PRO_TRIGGER_LCK							0xFF8180A4
#define 	OID_RT_PRO_SET_TX_POWER_FOR_ALL_RATE			0xFF8180A5
#define 	OID_RT_QUERY_RF_PATH							0xFF8180A6
#define 	OID_RT_SEND_SPECIFIC_PACKET_MP					0xFF8180A7
#define 	OID_RT_SEND_ONE_PACKET_MP						0xFF8180A8
#define		OID_RT_QUERY_ONE_BYTE_ALIGNMENT					0xFF8180A9


//3--------------------------------------------------------------------
//3 		0xFF03xxxx~
//3--------------------------------------------------------------------

#define 	OID_RT_GET_CONNECT_STATE                		0xFF030001
#define 	OID_RT_RESCAN	                        		0xFF030002
#define 	OID_RT_SET_KEY_LENGTH                  			0xFF030003

#define 	OID_RT_SET_DEFAULT_KEY_ID              		 	0xFF030004
#define 	OID_RT_GET_DEFAULT_KEY_ID						0xFF030004 //Same defnition as OID_RT_SET_DEFAULT_KEY_ID but provide query


//3--------------------------------------------------------------------
//3 		0xFF01xxxx~
//3--------------------------------------------------------------------
#define 	OID_RT_SET_CHANNEL								0xFF010182
#define 	OID_RT_SET_SNIFFER_MODE                 		0xFF010183
#define 	OID_RT_GET_SIGNAL_QUALITY              	 		0xFF010184
#define 	OID_RT_GET_SMALL_PACKET_CRC						0xFF010185		
#define 	OID_RT_GET_MIDDLE_PACKET_CRC					0xFF010186
#define 	OID_RT_GET_LARGE_PACKET_CRC						0xFF010187
#define 	OID_RT_GET_TX_RETRY								0xFF010188
#define 	OID_RT_GET_RX_RETRY								0xFF010189
#define 	OID_RT_GET_RX_TOTAL_PACKET						0xFF010190
#define 	OID_RT_GET_TX_BEACON_OK							0xFF010191
#define 	OID_RT_GET_TX_BEACON_ERR						0xFF010192
#define 	OID_RT_GET_RX_ICV_ERR							0xFF010193
#define 	OID_RT_ENCRYPTION_ALGORITHM						0xFF010194
#define 	OID_RT_SET_SCAN_OPERATION						0xFF010195
#define 	OID_RT_GET_PREAMBLE_MODE						0xFF010196
#define 	OID_RT_GET_DRIVER_UP_DELTA_TIME					0xFF010197
#define 	OID_RT_GET_AP_IP								0xFF010198
#define 	OID_RT_GET_CHANNELPLAN							0xFF010199
#define 	OID_RT_SET_PREAMBLE_MODE						0xFF01019A
#define 	OID_RT_SET_BCN_INTVL							0xFF01019B
#define 	OID_RT_GET_BCN_INTVL							0xFF01019B //same definition as OID_RT_SET_BCN_INTVL but provide query
#define 	OID_RT_GET_RF_VENDER							0xFF01019C
#define 	OID_RT_DEDICATE_PROBE							0xFF01019D
#define 	OID_RT_PRO_RX_FILTER_PATTERN					0xFF01019E

#define 	OID_RT_BEAMFORMING_START						0xFF0101A0
#define 	OID_RT_BEAMFORMING_END							0xFF0101A1
#define 	OID_RT_BEAMFORMING_PERIOD						0xFF0101A2

#define 	OID_RT_ADCSMP_TRIG								0xFF0101A4
#define		OID_RT_ADCSMP_STOP								0xFF0101A5

#define 	OID_RT_GET_TOTAL_TX_BYTES						0xFF0101A7
#define 	OID_RT_GET_TOTAL_RX_BYTES						0xFF0101A8
#define 	OID_RT_CURRENT_TX_POWER_LEVEL					0xFF0101A9
#define 	OID_RT_GET_CHANNEL								0xFF0101AC
#define 	OID_RT_SET_CHANNELPLAN							0xFF0101AD
#define 	OID_RT_GET_HARDWARE_RADIO_OFF					0xFF0101AE
#define 	OID_RT_SCAN_AVAILABLE_BSSID						0xFF0101B0
#define 	OID_RT_GET_HARDWARE_VERSION						0xFF0101B1
#define 	OID_RT_RESET_LOG								0xFF0101B7
#define 	OID_RT_GET_LOG									0xFF0101B8
#define 	OID_RT_SET_INDICATE_HIDDEN_AP					0xFF0101B9
#define 	OID_RT_GET_HEADER_FAIL							0xFF0101BA
#define 	OID_RT_SUPPORTED_WIRELESS_MODE					0xFF0101BB
#define 	OID_RT_GET_CHANNEL_LIST							0xFF0101BC
#define 	OID_RT_GET_SCAN_IN_PROGRESS						0xFF0101BD
#define 	OID_RT_GET_TX_INFO								0xFF0101BE
#define 	OID_RT_IO_READ_WRITE_INFO						0xFF0101BF
#define 	OID_RT_IO_READ_WRITE							0xFF0101C0
// For Netgear request.
#define 	OID_RT_FORCED_DATA_RATE							0xFF0101C1	
// Auto-Config OID.
#define 	OID_RT_WIRELESS_MODE_FOR_SCAN_LIST          	0xFF0101C2
#define 	OID_RT_GET_BSS_WIRELESS_MODE			   		0xFF0101C3
// For AZ project.
#define 	OID_RT_SCAN_WITH_MAGIC_PACKET					0xFF0101C4
// Auto select channel of the BSS.
#define 	OID_RT_AUTO_SELECT_CHANNEL                      0xFF0101C6
//for AutoTurbomode
#define 	OID_RT_TURBOMODE								0xFF0101C7
// Send specific 802.11 frame for developing and verification purpose, 2005.12.23, by rcnjko.
#define 	OID_RT_SEND_SPECIFIC_PACKET   					0xFF0101C8
// For debug purpose.
#define 	OID_RT_DBG_COMPONENT           					0xFF0101C9
#define 	OID_RT_DBG_LEVEL                    			0xFF0101CA
#define 	OID_RT_DEVICE_ID_INFO							0xFF0101CB 	// To identify Realtek WLAN device, 2006.01.24, by rcnjko.

#define 	OID_RT_HIDDEN_SSID								0xFF0101CC
#define 	OID_RT_LOCKED_STA_ADDRESS						0xFF0101CD
#define 	OID_RT_PER_STA_DATA_RATE						0xFF0101CE

#define 	OID_RT_SimpleConfScan							0xFF0101D0
#define 	OID_RT_WPS_SET_IE_FRAGMENT					0xFF010250 // modified from 0xFF010210 to 0xFF010250 by haich, 2011.04.20
// For CCX test plan v3.61 4.3, 060927, by rcnjko.
#define 	OID_RT_TX_POWER									0xFF0101D2
#define 	OID_RT_MAC_FILTER_TYPE							0xFF0101D3
#define 	OID_RT_WEP_STATUS								0xFF0101D4
#define 	OID_RT_AUTHENTICATION_MODE						0xFF0101D5
#define 	OID_RT_ADD_KEY									0xFF0101D6
#define 	OID_RT_UI_ENABLE_HIGH_PRIORITY					0xFF0101D7
#define 	OID_RT_UI_DISABLE_HIGH_PRIORITY					0xFF0101D8
//For scan limit
#define 	OID_RT_SET_SCAN_LIMIT							0xFF0101D9
// 8187BMP for product string in eeprom. Added by Bruce, 2007-1-19.
#define 	OID_RT_PRO_CORRECT_PRODUCT_STRING				0xFF0101DA
#define 	OID_RT_PRO_CHECK_PRODUCT_STRING					0xFF0101DB
// For ASUS request
#define 	OID_RT_FILTER_STA_ADDRESS						0xFF0101DC

// The range in dBm for setting tx power.
#define		OID_RT_TX_POWER_RANGE							0xFF0101DD
// The TX power Level For Notebook.
#define		OID_RT_TX_POWER_LEVEL							0xFF0101DF

#define 	OID_RT_WPS_RECIEVE_PACKET						0xFF0101DE

// For PSP XLink status. 2007.01.12, by shien chang.
#define 	OID_RT_GET_PSP_XLINK_STATUS						0xFF0101CF
#define 	OID_RT_SET_PSP_XLINK_STATUS						0xFF0101E0

// For WMM and WMM-UAPSD, 2007.01.15, by shien chang.
#define 	OID_RT_GET_WMM_ENABLE							0xFF0101E1
#define 	OID_RT_SET_WMM_ENABLE							0xFF0101E2
#define 	OID_RT_GET_WMM_UAPSD_ENABLE						0xFF0101E3
#define 	OID_RT_SET_WMM_UAPSD_ENABLE						0xFF0101E4

// 070208, rcnjko: For 802.11d.
#define 	OID_RT_DOT11D									0xFF0101E5

// 070301, rcnjko: for driver log event mechanism.
#define 	OID_RT_GET_LOGV2_TYPE_LIST						0xFF0101E6
#define 	OID_RT_GET_LOGV2_ATTR_LIST						0xFF0101E7
#define 	OID_RT_GET_LOGV2_DATA_LIST						0xFF0101E8

//For CCX yest plane v3.61 4.4 , 061020 by CCW
#define 	OID_RT_ROAM_TO_SELECT_BSSID            		 	0xFF0101E9

// For WMM Admission Control, 2007.08.16, by shine chang.
#define 	OID_RT_SEND_WMM_ADDTS							0xFF0101EA
#define 	OID_RT_SEND_WMM_DELTS							0xFF0101EB

// Custom Mesh OIDs.
#define 	OID_RT_MESH_MODE								0xFF0101EC
#define 	OID_RT_MESH_ID									0xFF0101ED

// Custom Corega OID
#define 	OID_RT_CURRENT_WLRELESSMODE  					0xFF0101EE

// Send customized 802.11 frame for developing and verification purpose
#define 	OID_RT_SEND_CUSTOMIZED_PACKET					0xFF0101EF

// Dot11k Radio Measurement, by Bruce, 2009-08-05.
#define 	OID_RT_SET_DOT11K_CHANNEL_LOAD					0xFF0101F0
#define 	OID_RT_QUERY_DOT11K_CHANNEL_LOAD				0xFF0101F1
#define 	OID_RT_SET_DOT11K_NOISE_HISTOGRAM				0xFF0101F2
#define 	OID_RT_QUERY_DOT11K_NOISE_HISTOGRAM				0xFF0101F3

// For 11g protection mode!!
//#define 	OID_RT_FRAME_BURSTING							0xFF0101F5

// For Intel PROXIMITY mode
#define OID_RT_INTEL_PROXIMITY_MODE 							0xFF0101F6
#define OID_RT_INTEL_PROXIMITY_SEND							0xFF0101F7

// For Wi-Fi P2P
#define OID_RT_P2P_VERSION									0xFF0100EF
#define OID_RT_P2P_MAX_VERSION							0xFF0100F0
#define OID_RT_P2P_ACCEPT_INVITATION_REQ					0xFF0101F8
#define OID_RT_P2P_MODE										0xFF0101F9
#define OID_RT_P2P_DEVICE_DISCOVERY							0xFF0101FA
#define OID_RT_P2P_ENUM_SCAN_LIST							0xFF0101FB
#define OID_RT_P2P_CONNECT_REQUEST							0xFF0101FC
#define OID_RT_P2P_DISCONNECT_REQUEST						0xFF0101FD
#define OID_RT_P2P_PROVISION_IE								0xFF0101FE
#define OID_RT_P2P_FLUSH_SCAN_LIST							0xFF0101FF
#define OID_RT_P2P_PROVISIONING_RESULT						0xFF010200
#define OID_RT_P2P_INVITE_PEER								0xFF010201
#define OID_RT_P2P_GO_INTENT								0xFF010202
#define OID_RT_P2P_DEVICE_DISCOVERABILITY_REQ 				0xFF010203
#define OID_RT_P2P_PROVISION_DISCOVERY 						0xFF010204
#define OID_RT_P2P_CAPABILITY									0xFF010205
#define OID_RT_P2P_GO_SSID									0xFF010206
#define OID_RT_P2P_POWER_SAVE								0xFF010207
#define OID_RT_P2P_OP_CHANNEL								0xFF010208
#define OID_RT_P2P_LISTEN_CHANNEL							0xFF010209
#define OID_RT_P2P_EXTENDED_LISTEN_TIMING					0xFF01020A
#define OID_RT_P2P_SERVICE_DISCOVERY_REQ						0xFF01020B
#define OID_RT_P2P_SERVICE_DISCOVERY_RSP						0xFF01020C
#define OID_RT_P2P_GO_BEACON_INTERVAL 						0xFF01020D
#define OID_RT_P2P_GO_NEGO_RESULT							0xFF01020E
#define OID_RT_P2P_INTERFACE_ADDRESS						0xFF01020F
#define OID_RT_P2P_GO_PREPROVISIONING						0xFF010210
#define OID_RT_P2P_CLIENT_PREPROVISIONING 					0xFF010211
#define OID_RT_P2P_GO_SSID_POSTFIX							0xFF010212
#define OID_RT_P2P_SELF_DEV_DESC								0xFF010213
#define OID_RT_P2P_DEVICE_ADDRESS 							0xFF010214
#define OID_RT_P2P_CHANNEL_LIST								0xFF010215
#define OID_RT_CUSTOMIZED_SCAN								0xFF010216
#define	OID_RT_P2P_SERVICE_FRAG_THRESHOLD					0xFF010217

// For customized association IE
#define OID_RT_CUSTOMIZED_ASSOCIATION_PARAM				0xFF010218

// For P2PS
#define OID_RT_P2P_SERVICE_REQUEST							0xFF010240


// SUPPORT_WPS_INFO
#define OID_RT_WPS_INFORMATION								0xFF01021B

// Antenna detection info which was reported from single tone with open circuit measurement. Added by Roger, 2012.11.27.
#define OID_RT_ANTENNA_DETECTED_INFO						0xFF01021C

//For 360 Request Ken
#define OID_RT_FILTER_DEFAULT_PERMITED						0xFF01021D

// For Wi-Fi P2P
#define OID_RT_P2P_PROFILE_LIST							0xFF010230

#define	OID_RT_P2P_FULL_SSID								0xFF010231

// For NAN
#define	OID_RT_NAN_TEST 									0xFF010240
#define	OID_RT_NAN_SDF		 								0xFF010241
#define	OID_RT_NAN_INIT_TSF									0xFF010242
#define OID_RT_NAN_FURTHER_AVAILABILITY_TX					0xFF010243
#define OID_RT_NAN_FURTHER_AVAILABILITY_RX					0xFF010244
#define OID_RT_NAN_BEACON_SEND_TYPE							0xFF010245
#define OID_RT_NAN_OPER_CHNL_2G								0xFF010246
#define OID_RT_NAN_OPER_CHNL_5G								0xFF010247
#define OID_RT_NAN_GET_AVAILABILITY_INFO					0xFF010248
#define OID_RT_NAN_BEACON_AGE_TIMEOUT						0xFF010249

// Vincent 8185MP
#define 	OID_RT_PRO_RX_FILTER							0xFF0111C0
#define 	OID_RT_PRO_WRITE_REGISTRY						0xFF0111C1
#define 	OID_RT_PRO_READ_REGISTRY						0xFF0111C2
#define		OID_RT_PRO_SET_INITIAL_GAIN						0xFF0111C3
#define		OID_RT_PRO_SET_BB_RF_STANDBY_MODE				0xFF0111C4
#define		OID_RT_PRO_SET_BB_RF_SHUTDOWN_MODE				0xFF0111C5
#define		OID_RT_PRO_SET_TX_CHARGE_PUMP					0xFF0111C6
#define		OID_RT_PRO_SET_RX_CHARGE_PUMP					0xFF0111C7
#define 	OID_RT_PRO_RF_WRITE_REGISTRY					0xFF0111C8
#define 	OID_RT_PRO_RF_READ_REGISTRY						0xFF0111C9
#define		OID_RT_PRO_QUERY_RF_TYPE						0xFF0111CA
#define		OID_RT_PRO_SW_RF_READ_REGISTRY					0xFF0111CB  //SW Three Wire for SD3 Required.
#define 	OID_RT_PRO_SW_RF_WRITE_REGISTRY					0xFF0111CC  //SW Three Wire for SD3 Required.
#define		OID_RT_PRO_SEND_RAW_DATA						0xFF0111CD

// AP OID
#define 	OID_RT_AP_GET_ASSOCIATED_STATION_LIST			0xFF010300
#define 	OID_RT_AP_GET_CURRENT_TIME_STAMP				0xFF010301
#define 	OID_RT_AP_SWITCH_INTO_AP_MODE					0xFF010302

#define 	OID_RT_AP_SET_DTIM_PERIOD						0xFF010303
#define 	OID_RT_AP_GET_DTIM_PERIOD						0xFF010303 //Same definition as OID_RT_AP_SET_DTIM_PERIOD but provide query

#define 	OID_RT_AP_SUPPORTED								0xFF010304  // Determine if driver supports AP mode. 2004.08.27, by rcnjko.
#define 	OID_RT_AP_SET_PASSPHRASE						0xFF010305	// Set WPA-PSK passphrase into authenticator. 2005.07.08, byrcnjko.

#define 	OID_RT_AP_WDS_MODE								0xFF010306 // 0: WDS disabled, 1: WDS enabled. 2006.06.12, by rcnjko.
#define 	OID_RT_AP_WDS_AP_LIST							0xFF010307 // WDS AP address list. 
#define	OID_RT_AP_GET_VWIFI_STATUS						0xFF010308
#define	OID_RT_AP_GET_ASSOC_STA_COUNT					0xFF010310
#define	OID_RT_AP_GET_CLOUD_KEY_EX						0xFF010313   	// Get EFUSE 0x122-12F content,.
#define	OID_RT_AP_SET_BEACON_START						0xFF010311	// Start AP beacon after ICS & DHCP ready.
//

// 802.11 engineering page OID
// --802.11h
// --802.11d
#define 	OID_RT_SET_80211H_SWITCH_CHANNEL				0xFF010400

//BT related
#define	OID_RT_MOTOR_BT_802_11_PAL						0xFF010401


#define 	OID_RT_GET_ROAM_COUNT							0xFF010406
#define 	OID_RT_GET_DISASSOC_COUNT						0xFF010407
#define 	OID_RT_PRO_READ_REGISTRY_SIC					0xFF010408
#define 	OID_RT_PRO_WRITE_REGISTRY_SIC					0xFF010409
#define	OID_RT_BT_CONTROL									0xFF01040A
#define	OID_RT_HAL_CONTROL									0xFF01040B

#define 	OID_RT_FPGA_PHY_RDY								0xFF010410
#define 	OID_RT_HOST_SUSPEND_STATUS						0xFF010411
#define	OID_RT_SDIO_REG_CTRL								0xFF010412


//For MacOSX 10.4.0 "Network Utility" LinkSpeed workaround 
#define 	OID_RT_SET_TIGER_WORKAROUND						0xFF010413
//For MacOSX UI
#define 	OID_RT_COUNTRY_NOT_SUPPORT_AC					0xFF010414
#define 	OID_RT_SET_BEAMFORM_CAP							0xFF010415
#define 	OID_RT_GET_DRIVER_VERSION						0xFF010416
#define 	OID_RT_SET_PHY_ADAPTIVITY						0xFF010417
#define		OID_RT_NETWORK_STATUS							0xFF010418
#define		OID_RT_GET_BSS_NUMBER							0xFF010419
#define		OID_RT_SHAREDKEY_AUTHENTICATION					0xFF01041A
#define		OID_RT_SCAN_SSID_AND_LINK						0xFF01041B
//--------------------------------------------------------------------
//		0xFF0105xx: For Exta AP mode  
//--------------------------------------------------------------------
// Star or Stop Exta AP mode  
#define 	OID_RT_EXTAP_SWITCH_INTO_AP_MODE			0xFF010501 
#define 	OID_RT_EXTAP_GET_APMODE						0xFF010502
// Set or Get Auth mode : OPEN WPA  WPA2 
#define 	OID_RT_EXTAP_SET_AUTHENTICATION_MODE		0xFF010503
#define 	OID_RT_EXTAP_GET_AUTHENTICATION_MODE		0xFF010504
// Set or Get Encrypt Mode
#define 	OID_RT_EXTAP_SET_ENCRYPTION_ALGORITHM		0xFF010505
#define 	OID_RT_EXTAP_GET_ENCRYPTION_ALGORITHM		0xFF010506
// Set WPA or WPA2 PSK 
#define 	OID_RT_EXTAP_SET_PASSPHRASE					0xFF010507
// Set Key : Just for WEP key and Transmit Key index
#define 	OID_RT_EXTAP_SET_ADD_WEP_KEY					0xFF010508
#define 	OID_RT_EXTAP_SET_DEFAULT_KEY_ID				0xFF010509
// Set Exta AP SSID
#define 	OID_RT_EXTAP_SET_SSID						0xFF01050a
// Query Exta AP Station List 
#define	OID_RT_EXTAP_GET_ASSOCIATED_STATION_LIST	0xFF01050b


// WiDi related
#define		OID_RT_ROAM_FAKE_SIGNAL							0xFF010600

// For WFi Display Specified Request
#define	OID_RT_WFD_REQUEST							0xFF0106A0

// For NAN Specified Request
#define	OID_RT_NAN_REQUEST							0xFF0106A1

//
// Description:
//	The version of RT_OBJ_HDR_DRVIF_VERSION used in the current driver.
// Size:
//	ULONG
// Value:
//	Driver combines the minimal major and minor versions into single version.
// Direction:
//	Query
// Remark:
//	This REQ_ID supports query only.
//	
#define	OID_RT_OBJ_HDR_VERSION						0xFF0106A2

//
// Description:
//	The version of RT_OBJ_HDR_DRVIF_DATE used in the current driver.
// Size:
//	ULONG
// Value:
//	Driver combines the year and date into this field.
// Direction:
//	Query
// Remark:
//	This  supports query only.
//
#define	OID_RT_OBJ_HDR_DATE							0xFF0106A3



// Scope: 0xFF010801 ~ 0xFF0108FF
// For test tool. 2010.12.15. by tynli
#define	OID_RT_CONTROL_LPS							0xFF010800
#define	OID_RT_CONTROL_IPS							0xFF010801
#define	OID_RT_CLEAR_ANTENNA_TEST_VAL				0xFF010802
// For debug.
#define	OID_RT_PNP_POWER								0xff010803

#define 	OID_RT_FORCED_BUG_CHECK						0xff010804

// For LC 
#define 	OID_RT_LC_STOP_SCAN       						0xFF010901
#define	OID_RT_SYSTEM_POWER_STATE_SHUTDOWN		0xFF010902 // LC S5 WoWLAN

//For Comfast
#define OID_RT_GET_CUSTOMIZE_BSS_LIST				0xFF010910

//3--------------------------------------------------------------------
//3 			0xFF8185xx: 
//3--------------------------------------------------------------------
// by Owen for RTL8185 Phy Status Report Utility
#define		OID_RT_UTILITY_FALSE_ALARM_COUNTERS				0xFF818580
#define		OID_RT_UTILITY_SELECT_DEBUG_MODE				0xFF818581
#define		OID_RT_UTILITY_SELECT_SUBCARRIER_NUMBER			0xFF818582
#define		OID_RT_UTILITY_GET_RSSI_STATUS					0xFF818583
#define		OID_RT_UTILITY_GET_FRAME_DETECTION_STATUS		0xFF818584
#define		OID_RT_UTILITY_GET_AGC_AND_FREQUENCY_OFFSET_ESTIMATION_STATUS		0xFF818585
#define		OID_RT_UTILITY_GET_CHANNEL_ESTIMATION_STATUS	0xFF818586

//
// by Owen on 03/09/19-03/09/22 for RTL8185
#define		OID_RT_WIRELESS_MODE							0xFF818500
#define		OID_RT_SUPPORTED_RATES							0xFF818501
#define		OID_RT_DESIRED_RATES							0xFF818502
#define		OID_RT_WIRELESS_MODE_STARTING_ADHOC				0xFF818503
#define		OID_RT_ADHOC_DEFAULT_WIRELESS_MODE				0xFF818504



// Start for Real WoW v2 
#define 		OID_RT_CUSTOMER_ID_INFO						0xFF8168FA
#define 		OID_RT_CUSTOMER_WOW_WAKEUPCODE			0xFF8168FB
#define 		OID_RT_CUSTOMER_WOW_S5_SUPPORT			0xFF8168FC
#define 		OID_RT_CUSTOMER_WOW_S5_INFO				0xFF8168FD



//3 --------------------------------------------------------------------
//3 		0xFF8187xx: 8187 related
//3--------------------------------------------------------------------

// 8187MP. 2004.09.06, by rcnjko.
#define 	OID_RT_PRO8187_WI_POLL							0xFF818780
#define 	OID_RT_PRO_WRITE_BB_REG							0xFF818781
#define 	OID_RT_PRO_READ_BB_REG							0xFF818782
//

// 8187MP for KY's request. 2005.09.09, by rcnjko.
#define 	OID_RT_PRO_ENABLE_ACK_COUNTER					0xFF818783
#define 	OID_RT_PRO_RESET_ACK_COUNTER					0xFF818784
#define 	OID_RT_PRO_GET_ACK_COUNTER						0xFF818785
//

// 8187MP for AZ's request. 2005.09.09, by rcnjko.
#define 	OID_RT_PRO_SET_TX_POWER_BASE_OFFSET			0xFF818786
#define 	OID_RT_PRO_GET_TX_POWER_BASE_OFFSET			0xFF818787
//3 --------------------------------------------------------------------
//3 		0xFF8190xx: 8190 related
//3--------------------------------------------------------------------
#define 	OID_RT_11N_FORCED_AMPDU							0xFF819000
#define 	OID_RT_11N_FORCED_AMSDU							0xFF819001
#define 	OID_RT_FORCED_PROTECTION						0xFF819002
#define 	OID_RT_11N_FORCED_SHORTGI						0xFF819003
#define 	OID_RT_11N_FORCED_LDPC							0xFF819006
#define 	OID_RT_11N_MIMOPS_MODE							0xFF819007
#define 	OID_RT_GET_11N_MIMPO_RSSI						0xFF81900A
#define 	OID_RT_GET_11N_MIMPO_EVM						0xFF81900B
#define 	OID_RT_11N_SILENT_RESET							0xFF81900C
#define 	OID_RT_11N_RESET_HISTOGRAM						0xFF819010
#define 	OID_RT_11N_RX_REORDER_CONTROL					0xFF819011
#define 	OID_RT_CURRENT_CHANNEL_INFO						0xFF819019
#define 	OID_RT_11N_USB_TX_AGGR_NUM						0xFF81901A


#define 	OID_RT_11N_SYS_CPU_WAKE_SETTING					0xFF81901B


#define 	OID_RT_11N_FORCED_USB_RX_AGGR					0xFF81901C


#define 	OID_RT_11N_UI_SHOW_RX_RATE			  			0xFF81901D   //For show the smooth 11n Rx Rate to normal user
#define 	OID_RT_11N_FORCED_DISABLED_RAFUNC				0xFF81901F

#define 	OID_RT_11N_DYNAMIC_TX_POWER_CONTROL      		0xFF819020  //For Dynamic Tx power for near/far range enable/Disable  , by Jacken , 2008-03-06
#define 	OID_RT_11N_TX_LINK_SPEED						0xFF819021  //For Real 11n Tx Rate
#define 	OID_RT_11N_RX_LINK_SPEED						0xFF819022  //For Real 11n Rx Rate
#define 	OID_RT_AMPDU_BURST_MODE							0xFF819023
#define 	OID_RT_AMPDU_BURST_NUM							0xFF819024
#define 	OID_RT_AUTO_AMPDU_BURST_ENABLE					0xFF819025
#define 	OID_RT_AUTO_AMPDU_BURST_THRESHOLD				0xFF819026
// 2008-03-11 Add by hpfan for support hardware PBC
#define 	OID_RT_WPS_HWSET_PBC_PRESSED					0XFF819028
#define 	OID_RT_WPS_HWGET_PBC_PRESSED					0XFF819029
// 2008-03-20 Add by hpfan for support  WPS LED control
#define 	OID_RT_WPS_LED_CTL_START						0xFF81902A
#define 	OID_RT_11N_UI_SHOW_TX_RATE						0xFF81902B  //For show the smooth 11n Tx Rate to normal user

#define	OID_RT_11N_CHANGE_RPQN							0xFF81902C
#define	OID_RT_11N_DROP_PACKET							0xFF81902D
#define	OID_RT_11N_MACID_128							0xFF81902E


//added by vivi, 2008.04.24, for runtop led
#define 	OID_RT_WPS_CUSTOMIZED_LED						0xFF81902F

#define 	OID_RT_11N_INITIAL_TX_RATE						0xFF819033
#define 	OID_RT_11N_TX_RETRY_COUNT						0xFF819034
#define 	OID_RT_CURRENT_BANDWIDTH						0xFF819035
#define 	OID_RT_11N_TX_RATE_DISPLAY						0xFF819036
#define 	OID_RT_11N_FIRMWARE_VERSION						0xFF819037
#define 	OID_RT_11N_UI_SS_SUPPORT						0xFF819038	// Support for Usb Selective Suspend.
#define 	OID_RT_11N_UI_INIT_READY						0xFF819039

//added for 92D test.
#define 	OID_RT_11N_IQK_TRIGGER							0xFF81903A
#define 	OID_RT_11N_TXPKT_TRIGGER						0xFF81903B
#define 	OID_RT_11N_ENABLE_COALESCE						0xFF81903C
#define 	OID_RT_11N_DUMP_REGS							0xFF81903D
#define 	OID_RT_11N_SET_RX_FAST_BATCH_NUM				        0xFF81903E


#define 	OID_RT_11N_TDLS_ENABLE							0xFF81903F
#define 	OID_RT_11N_FORCED_ADDBA							0xFF819040	// For wifidirect sigma test.
#define   OID_RT_11N_DISABLE_TX_POWER_BY_RATE                                   0xFF819044
#define   OID_RT_11N_TX_POWER_TRAINING                                 0xFF819045

//For MacOSX UI
#define 	OID_RT_11N_SHORT_GI								0xFF819046
#define 	OID_RT_11N_MCS_RATE								0xFF819047
#define 	OID_RT_SUPPLICANT_FAIL_CODE						0xFF819048

// 11n Sigma Config Capability
#define	OID_RT_11N_SIGMA_CONFIG							0xFF81904A

#define 	OID_RT_TX_CHECK_TP_THRESHOLD						0xFF81904B
#define 	OID_RT_RX_CHECK_TP_THRESHOLD						0xFF81904C

// 11ac USB 2.0/3.0 switch mechanism
// This mechanism shall be operate with UI
#define	OID_RT_UMR_QUERY_SWITCH_CHECK							0xFF819050
#define	OID_RT_UMR_QUERY_SWITCH_CONFIRM							0xFF819051
#define	OID_RT_UMR_SET_BSSID_TO_CONNECT_FOR_SWITCH_CHECK		0xFF819052
#define	OID_RT_UMR_QUERY_USB_MODE_SUGGESTION					0xFF819053
#define	OID_RT_UMR_SET_FORCED_USB_MODE							0xFF819054
#define	OID_RT_UMR_QUERY_CURRENT_USB_MODE						0xFF819055
#define	OID_RT_UMR_SET_USB_HOST_CAPABILITY 						0xFF819056

#define OID_RT_11N_FORCED_STBC									0xFF819057
#define OID_RT_QUERY_IS_MP_CHIP									0xFF819058
// For ePHY parameter read & write
#define OID_RT_PCIE_GEN1_TO_GEN2								0xFF819060
#define OID_RT_PCIE_MDIO_WRITE									0xFF819061
#define OID_RT_PCIE_MDIO_READ									0xFF819062
#define OID_RT_FORCE_PCIE_RATE                                                            0xFF819063

// For 2.4G/5G channel SSID select
#define OID_RT_BAND_SELECT                                                            0xFF819064


//3--------------------------------------------------------------------
//3		0xFFEDC1xx: For Meeting House
//3--------------------------------------------------------------------

// Meeting House. added by Annie, 2005-07-20.
#define 	OID_RT_MH_VENDER_ID									0xFFEDC100
//CCX Rogue AP , 2006.07.27, by CCW
#define 	MH_OID_CCX_ROGUE_AP                   				0xFFEDC10B
#define 	MH_OID_CCX_ROGUE_AP_STATUS      					0xFFEDC101

//CCX NETWORK EAP for LEAP 2006.07.31,by CCW
#define 	MH_OID_CCX_NETWORK_EAP            					0xFFEDC102
//CCX  2006.08.01,by CCW
#define  	MH_OID_CCX_MIXED_CELL              					0xFFEDC103
#define  	MH_OID_CCX_FAST_ROAM               					0xFFEDC105
#define  	MH_OID_CCX_FAST_ROAM_RESULT           				0xFFEDC106
#define  	MH_OID_CCX_ADD_KRK                   				0xFFEDC107
#define  	MH_OID_CCX_REMOVE_KRK             					0xFFEDC108 
#define  	MH_OID_CCX_VERSION                   				0xFFEDC109
#define  	MH_OID_CCX_ENABLE                    				0xFFEDC10A
#define		MH_OID_CCX_RM_ENABLE								0xFFEDC10C
// CCX IHV service support, by Bruce, 2009-08-28.
#define		OID_RT_CCX_IHV_SUPPORT								0xFFEDC200


/*---------------------------------------------------------------------------
Dual Mac  Mode Switch Related OID
-----------------------------------------------------------------------------*/
// 2010/12/27 MH The constant definition we need ot to use compile flag to seperate,
#define   	OID_RT_8192D_CHANGE_MAC_PHY_MODE      			0xFFEDD000
#define	OID_RT_8192D_GET_DUAL_MAC_MODE		    		0xFFEDD001
#define  	OID_RT_8192D_GET_MODE_SWITCH_IN_PROGRESS  		0xFFEDD002
#define	OID_RT_8192D_GET_MAC_INDEX							0xFFEDD003

#define	OID_RT_EASY_COCURRENT_SET_CHANNEL_INFO			0xFFEDD004
#define	OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO			0xFFEDD005
#define	OID_RT_EASY_COCURRENT_SET_SSID						0xFFEDD006
#define	OID_RT_EASY_CONCURRENT_SET_STA_AP_SUPPORT		0xFFEDD007
#define 	OID_RT_EASY_CONCURRENT_SET_TO_SKIP_CHANGE_MACPHY_MODE   0XFFEDD008  //sherry added for skip change mac phy mode during start vwifi from ui 20110517

//------------------------------------------------------------------------
//3 Path Diversity
//Add by Neil Chen
#define  OID_RT_11N_PDIV_TRIGGER                                                 0xFFEDD010
#define  OID_RT_11N_PDIV_FW_TRIGGER                                           0xFFEDD011
//------------------------------------------------------------------------

/*----------------------------------------------------------------------------
	OID_RT_DEVICE_ID_INFO used data structure
----------------------------------------------------------------------------*/

#define RT_DEVICE_ID_INFO_TAG							0x10ec0211
#define RT_DEVICE_ID_PCI								0x00000000
#define RT_DEVICE_ID_USB								0x00000001
#define RT_DEVICE_ID_SDIO								0x00000002

typedef struct _RT_DEVICE_ID_HEADER{
	//
	// Identify whether this is a Realtek WLAN device.
	// RT_DEVICE_ID_INFO_TAG means Realtek WLAN NIC device, other values are not valid.
	//
	ULONG	RtWlanDevTag;

	//
	// Identify which IC.
	// Examples of (ChipID, ChipVer):
	// (0x8185, 0x1)	=> 8185 
	// (0x8187, 0x1)	=> 8187 
	// (0x8185, 0x2)	=> 8185B 
	// (0x8187, 0x2)	=> 8187B 
	//
	ULONG	ChipID;
	ULONG	ChipVer;

	//
	// BusType is used to identify BUS type of the device and corresponding data type, 
	// for example:
	// RT_PCI_DEVICE => struc _RT_PCI_ID_INFO
	// RT_USB_DEVICE => struc _RT_USB_ID_INFO
	//
	ULONG	BusType; 
}RT_DEVICE_ID_HEADER, *PRT_DEVICE_ID_HEADER;

typedef struct _RT_PCI_ID_INFO{
	RT_DEVICE_ID_HEADER	DevIDHeader;

	// 
	// Vendor ID and Device ID from PCI configuration space. 
	//
	USHORT	VID;
	USHORT	DID;

	// 
	// Sub Vendor ID and Subsystem ID from PCI configuration space. 
	//
	USHORT	SVID;
	USHORT	SMID;

	//
	// Revision ID from PCI configuration space.
	//
	UCHAR	RevID;

	//
	// Customer ID.
	//
	USHORT	CustomerID;
}RT_PCI_ID_INFO, *PRT_PCI_ID_INFO;

typedef struct _RT_USB_ID_INFO{
	RT_DEVICE_ID_HEADER	DevIDHeader;

	// 
	// Vendor ID and Product ID from USB Device Descriptor.
	//
	USHORT	VID;
	USHORT	PID;

	//
	// bcdDevice from USB Device Descriptor.
	//
	USHORT	RevID;

	//
	// Interface index.
	//
	USHORT	InterfaceIdx;
}RT_USB_ID_INFO, *PRT_USB_ID_INFO;

typedef struct _RT_SDIO_ID_INFO{
	RT_DEVICE_ID_HEADER	DevIDHeader;

	// 
	// Vendor ID and Product ID from SDIO Device.
	//
	USHORT	VID;
	USHORT	PID;
	
	USHORT	RevID;

//
	// Interface index.
//
	USHORT	InterfaceIdx;
}RT_SDIO_ID_INFO, *PRT_SDIO_ID_INFO;


//ShienChang: OID mapping to Vista DOT11 OID
//Scope: 0xFF010601 ~ 0xFF0106FF
#define 	OID_RT_POWER_MGMT_REQUEST						0xFF010601
#define 	OID_RT_OPERATIONAL_RATE_SET						0xFF010602
#define 	OID_RT_FRAGMENTATION_THRESHOLD					0xFF010603
#define 	OID_RT_RTS_THRESHOLD							0xFF010604
#define 	OID_RT_DISCONNECT_REQUEST						0xFF010605


//ShienChang: OID mapping for WinXP style 802.11 OID
//Scope: 0xFF070101 ~ 0xFF0702FF
#define 	RT_80211_START								0xFF070000
#define 	OID_RT_802_11_BSSID						( RT_80211_START | OID_802_11_BSSID )
#define 	OID_RT_802_11_SSID						( RT_80211_START | OID_802_11_SSID )
#define 	OID_RT_802_11_NETWORK_TYPES_SUPPORTED	( RT_80211_START | OID_802_11_NETWORK_TYPES_SUPPORTED )
#define 	OID_RT_802_11_NETWORK_TYPE_IN_USE		( RT_80211_START | OID_802_11_NETWORK_TYPE_IN_USE )
#define 	OID_RT_802_11_TX_POWER_LEVEL			( RT_80211_START | OID_802_11_TX_POWER_LEVEL )
#define 	OID_RT_802_11_RSSI						( RT_80211_START | OID_802_11_RSSI )
#define 	OID_RT_802_11_RSSI_TRIGGER				( RT_80211_START | OID_802_11_RSSI_TRIGGER )
#define 	OID_RT_802_11_INFRASTRUCTURE_MODE		( RT_80211_START | OID_802_11_INFRASTRUCTURE_MODE )
#define 	OID_RT_802_11_FRAGMENTATION_THRESHOLD	( RT_80211_START | OID_802_11_FRAGMENTATION_THRESHOLD )
#define 	OID_RT_802_11_RTS_THRESHOLD				( RT_80211_START | OID_802_11_RTS_THRESHOLD )
#define 	OID_RT_802_11_NUMBER_OF_ANTENNAS		( RT_80211_START | OID_802_11_NUMBER_OF_ANTENNAS )
#define 	OID_RT_802_11_RX_ANTENNA_SELECTED		( RT_80211_START | OID_802_11_RX_ANTENNA_SELECTED )
#define 	OID_RT_802_11_TX_ANTENNA_SELECTED		( RT_80211_START | OID_802_11_TX_ANTENNA_SELECTED )
#define 	OID_RT_802_11_SUPPORTED_RATES			( RT_80211_START | OID_802_11_SUPPORTED_RATES )
#define 	OID_RT_802_11_DESIRED_RATES			( RT_80211_START | OID_802_11_DESIRED_RATES )
#define 	OID_RT_802_11_CONFIGURATION			( RT_80211_START | OID_802_11_CONFIGURATION )
#define 	OID_RT_802_11_STATISTICS			( RT_80211_START | OID_802_11_STATISTICS )
#define 	OID_RT_802_11_ADD_WEP				( RT_80211_START | OID_802_11_ADD_WEP )
#define 	OID_RT_802_11_REMOVE_WEP			( RT_80211_START | OID_802_11_REMOVE_WEP )
#define 	OID_RT_802_11_DISASSOCIATE			( RT_80211_START | OID_802_11_DISASSOCIATE )
#define 	OID_RT_802_11_POWER_MODE			( RT_80211_START | OID_802_11_POWER_MODE )
#define 	OID_RT_802_11_BSSID_LIST			( RT_80211_START | OID_802_11_BSSID_LIST )
#define 	OID_RT_802_11_AUTHENTICATION_MODE	( RT_80211_START | OID_802_11_AUTHENTICATION_MODE )
#define 	OID_RT_802_11_PRIVACY_FILTER		( RT_80211_START | OID_802_11_PRIVACY_FILTER )
#define 	OID_RT_802_11_BSSID_LIST_SCAN		( RT_80211_START | OID_802_11_BSSID_LIST_SCAN )
#define 	OID_RT_802_11_WEP_STATUS			( RT_80211_START | OID_802_11_WEP_STATUS )
#define 	OID_RT_802_11_RELOAD_DEFAULTS		( RT_80211_START | OID_802_11_RELOAD_DEFAULTS )
#define 	OID_RT_802_11_TEST					( RT_80211_START | OID_802_11_TEST )
#define 	OID_RT_802_11_CAPABILITY			( RT_80211_START | OID_802_11_CAPABILITY )
#define 	OID_RT_802_11_PMKID					( RT_80211_START | OID_802_11_PMKID )
#define 	OID_RT_802_11_ASSOCIATION_INFORMATION		( RT_80211_START | OID_802_11_ASSOCIATION_INFORMATION )
#define 	OID_RT_802_11_ENCRYPTION_STATUS		( RT_80211_START | OID_802_11_ENCRYPTION_STATUS )
#define 	OID_RT_802_11_ADD_KEY				( RT_80211_START | OID_802_11_ADD_KEY )
#define 	OID_RT_802_11_REMOVE_KEY			( RT_80211_START | OID_802_11_REMOVE_KEY )


#endif //#ifndef	__CUSTOM_OID_H
