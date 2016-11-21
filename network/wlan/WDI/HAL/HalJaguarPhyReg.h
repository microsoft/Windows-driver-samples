#ifndef __INC_HALJAGUARPHYREG_H
#define __INC_HALJAGUARPHYREG_H


/*--------------------------Define Parameters-------------------------------*/

//
// BB-PHY register PMAC 0x100 PHY 0x800 - 0xEFF
// 1. PMAC duplicate register due to connection: RF_Mode, TRxRN, NumOf L-STF
// 2. 0x800/0x900/0xA00/0xC00/0xD00/0xE00
// 3. RF register 0x00-2E
// 4. Bit Mask for BB/RF register
// 5. Other defintion for BB/RF R/W
//


// BB Register Definition
#define		rCCAonSec_Jaguar			0x838
#define		rPwed_TH_Jaguar			0x830
#define		rL1_Weight_Jaguar			0x840

// BW and sideband setting
#define		rBWIndication_Jaguar		0x834
#define		rL1PeakTH_Jaguar			0x848
#define		rRFMOD_Jaguar				0x8ac	//RF mode 

#define		rADC_Buf_160_Clk_Jaguar		0x8c4
#define		rADC_Buf_40_Clk_Jaguar2		0x8c8

#define		bRFMOD_Jaguar				0xc3
#define		rCCK_System_Jaguar			0xa00   // for cck sideband
#define		bCCK_System_Jaguar			0x10

// Block & Path enable
#define		rOFDMCCKEN_Jaguar 			0x808 // OFDM/CCK block enable
#define		bOFDMEN_Jaguar			0x20000000
#define		bCCKEN_Jaguar				0x10000000
#define		rRxPath_Jaguar				0x808	// Rx antenna
#define		bRxPath_Jaguar				0xff
#define		rTxPath_Jaguar				0x80c	// Tx antenna
#define		bTxPath_Jaguar				0x0fffffff
#define		rCCK_RX_Jaguar				0xa04	// for cck rx path selection
#define		bCCK_RX_Jaguar				0x0c000000 
#define		rVhtlen_Use_Lsig_Jaguar		0x8c3	// Use LSIG for VHT length

#define		rRxPath_Jaguar2				0xa04	// Rx antenna
#define		rTxAnt_1Nsts_Jaguar2		0x93c	// Tx antenna for 1Nsts
#define		rTxAnt_23Nsts_Jaguar2		0x940	// Tx antenna for 2Nsts and 3Nsts


// RF read/write-related
#define		rHSSIRead_Jaguar			0x8b0  // RF read addr
#define		bHSSIRead_addr_Jaguar		0xff
#define		bHSSIRead_trigger_Jaguar	0x100
#define		rA_PIRead_Jaguar			0xd04 // RF readback with PI
#define		rB_PIRead_Jaguar			0xd44 // RF readback with PI
#define		rA_SIRead_Jaguar			0xd08 // RF readback with SI
#define		rB_SIRead_Jaguar			0xd48 // RF readback with SI
#define		rRead_data_Jaguar			0xfffff
#define		rA_LSSIWrite_Jaguar			0xc90 // RF write addr
#define		rB_LSSIWrite_Jaguar			0xe90 // RF write addr
#define		bLSSIWrite_data_Jaguar		0x000fffff
#define		bLSSIWrite_addr_Jaguar		0x0ff00000

#define		rC_PIRead_Jaguar2			0xd84 // RF readback with PI
#define		rD_PIRead_Jaguar2			0xdC4 // RF readback with PI
#define		rC_SIRead_Jaguar2			0xd88 // RF readback with SI
#define		rD_SIRead_Jaguar2			0xdC8 // RF readback with SI
#define		rC_LSSIWrite_Jaguar2		0x1890 // RF write addr
#define		rD_LSSIWrite_Jaguar2		0x1A90 // RF write addr


// NBI & CSI Mask setting
#define		rCSI_Mask_Setting1_Jaguar	0x874
#define		rCSI_Fix_Mask0_Jaguar		0x880
#define		rCSI_Fix_Mask1_Jaguar		0x884
#define		rCSI_Fix_Mask2_Jaguar		0x888
#define		rCSI_Fix_Mask3_Jaguar		0x88c
#define		rCSI_Fix_Mask4_Jaguar		0x890
#define		rCSI_Fix_Mask5_Jaguar		0x894
#define		rCSI_Fix_Mask6_Jaguar		0x898
#define		rCSI_Fix_Mask7_Jaguar		0x89c
#define		rNBI_Setting_Jaguar			0x87c


// YN: mask the following register definition temporarily 
//#define		rFPGA0_XA_RFInterfaceOE		0x860	// RF Channel switch
#define		rFPGA0_XB_RFInterfaceOE		0x864

//#define	rFPGA0_XAB_RFInterfaceSW		0x870	// RF Interface Software Control
//#define	rFPGA0_XCD_RFInterfaceSW		0x874

//#define	rFPGA0_XAB_RFParameter		0x878	// RF Parameter
//#define	rFPGA0_XCD_RFParameter		0x87c

//#define	rFPGA0_AnalogParameter1		0x880	// Crystal cap setting RF-R/W protection for parameter4??
//#define	rFPGA0_AnalogParameter2		0x884
//#define	rFPGA0_AnalogParameter3		0x888
//#define	rFPGA0_AdDaClockEn			0x888	// enable ad/da clock1 for dual-phy
//#define	rFPGA0_AnalogParameter4		0x88c


// CCK TX scaling
#define		rCCK_TxFilter1_Jaguar		0xa20
#define		bCCK_TxFilter1_C0_Jaguar	0x00ff0000
#define		bCCK_TxFilter1_C1_Jaguar	0xff000000
#define		rCCK_TxFilter2_Jaguar		0xa24
#define		bCCK_TxFilter2_C2_Jaguar	0x000000ff
#define		bCCK_TxFilter2_C3_Jaguar	0x0000ff00
#define		bCCK_TxFilter2_C4_Jaguar	0x00ff0000
#define		bCCK_TxFilter2_C5_Jaguar	0xff000000
#define		rCCK_TxFilter3_Jaguar		0xa28
#define		bCCK_TxFilter3_C6_Jaguar	0x000000ff
#define		bCCK_TxFilter3_C7_Jaguar	0x0000ff00


// YN: mask the following register definition temporarily
//#define		rPdp_AntA      					0xb00  
//#define		rPdp_AntA_4    				0xb04
//#define		rConfig_Pmpd_AntA 			0xb28
//#define		rConfig_AntA 					0xb68
//#define		rConfig_AntB 					0xb6c
//#define		rPdp_AntB 					0xb70
//#define		rPdp_AntB_4 					0xb74
//#define		rConfig_Pmpd_AntB			0xb98
//#define		rAPK							0xbd8

// RXIQC
#define		rA_RxIQC_AB_Jaguar    	0xc10  //RxIQ imblance matrix coeff. A & B
#define		rA_RxIQC_CD_Jaguar    	0xc14  //RxIQ imblance matrix coeff. C & D
#define	 	rA_TxScale_Jaguar 		0xc1c  // Pah_A TX scaling factor
#define		rB_TxScale_Jaguar 		0xe1c  // Path_B TX scaling factor
#define		rB_RxIQC_AB_Jaguar    	0xe10  //RxIQ imblance matrix coeff. A & B
#define		rB_RxIQC_CD_Jaguar    	0xe14  //RxIQ imblance matrix coeff. C & D
#define		b_RxIQC_AC_Jaguar		0x02ff  // bit mask for IQC matrix element A & C
#define		b_RxIQC_BD_Jaguar		0x02ff0000 // bit mask for IQC matrix element A & C

#define	 	rC_TxScale_Jaguar2 		0x181c  // Pah_C TX scaling factor
#define		rD_TxScale_Jaguar2 		0x1A1c  // Path_D TX scaling factor
#define		rRF_TxGainOffset		0x55

// DIG-related
#define		rA_IGI_Jaguar				0xc50	// Initial Gain for path-A
#define		rB_IGI_Jaguar				0xe50	// Initial Gain for path-B
#define		rC_IGI_Jaguar2				0x1850	// Initial Gain for path-C
#define		rD_IGI_Jaguar2				0x1A50	// Initial Gain for path-D

#define		rOFDM_FalseAlarm1_Jaguar	0xf48  // counter for break
#define		rOFDM_FalseAlarm2_Jaguar	0xf4c  // counter for spoofing
#define		rCCK_FalseAlarm_Jaguar		0xa5c // counter for cck false alarm
#define		b_FalseAlarm_Jaguar			0xffff
#define		rCCK_CCA_Jaguar				0xa08	// cca threshold

#define		bCCK_CCA_Jaguar				0x00ff0000

// Tx Power Ttraining-related
#define		rA_TxPwrTraing_Jaguar		0xc54
#define		rB_TxPwrTraing_Jaguar		0xe54

// Report-related
#define		rOFDM_ShortCFOAB_Jaguar		0xf60  
#define		rOFDM_LongCFOAB_Jaguar		0xf64
#define		rOFDM_EndCFOAB_Jaguar		0xf70
#define		rOFDM_AGCReport_Jaguar		0xf84
#define		rOFDM_RxSNR_Jaguar			0xf88
#define		rOFDM_RxEVMCSI_Jaguar		0xf8c
#define		rOFDM_SIGReport_Jaguar		0xf90

// Misc functions
#define		rEDCCA_Jaguar				0x8a4 // EDCCA
#define		bEDCCA_Jaguar				0xffff
#define		rAGC_table_Jaguar			0x82c   // AGC tabel select
#define		bAGC_table_Jaguar			0x3
#define		b_sel5g_Jaguar    			0x1000 // sel5g
#define		b_LNA_sw_Jaguar			0x8000 // HW/WS control for LNA
#define		rFc_area_Jaguar				0x860   // fc_area 
#define		bFc_area_Jaguar				0x1ffe000
#define		rSingleTone_ContTx_Jaguar 	0x914

#define		rAGC_table_Jaguar2			0x958	// AGC tabel select
#define		rDMA_trigger_Jaguar2		0x95C	// ADC sample mode


// RFE
#define		rA_RFE_Pinmux_Jaguar	0xcb0	// Path_A RFE cotrol pinmux
#define		rB_RFE_Pinmux_Jaguar	0xeb0	// Path_B RFE control pinmux
#define		rA_RFE_Inv_Jaguar		0xcb4	// Path_A RFE cotrol   
#define		rB_RFE_Inv_Jaguar		0xeb4	// Path_B RFE control
#define		rA_RFE_Jaguar			0xcb8 	// Path_A RFE cotrol   
#define		rB_RFE_Jaguar			0xeb8	// Path_B RFE control
#define		r_ANTSEL_SW_Jaguar		0x900	// ANTSEL SW Control
#define		bMask_RFEInv_Jaguar	0x3ff00000
#define		bMask_AntselPathFollow_Jaguar 0x00030000   

#define		rC_RFE_Pinmux_Jaguar	0x18B4	// Path_C RFE cotrol pinmux
#define		rD_RFE_Pinmux_Jaguar	0x1AB4	// Path_D RFE cotrol pinmux
#define		rA_RFE_Sel_Jaguar2		0x1990



// TX AGC 
#define		rTxAGC_A_CCK11_CCK1_JAguar	0xc20
#define		rTxAGC_A_Ofdm18_Ofdm6_JAguar	0xc24
#define		rTxAGC_A_Ofdm54_Ofdm24_JAguar	0xc28
#define		rTxAGC_A_MCS3_MCS0_JAguar	0xc2c
#define		rTxAGC_A_MCS7_MCS4_JAguar	0xc30
#define		rTxAGC_A_MCS11_MCS8_JAguar	0xc34
#define		rTxAGC_A_MCS15_MCS12_JAguar	0xc38
#define		rTxAGC_A_Nss1Index3_Nss1Index0_JAguar	0xc3c
#define		rTxAGC_A_Nss1Index7_Nss1Index4_JAguar	0xc40
#define		rTxAGC_A_Nss2Index1_Nss1Index8_JAguar	0xc44
#define		rTxAGC_A_Nss2Index5_Nss2Index2_JAguar	0xc48
#define		rTxAGC_A_Nss2Index9_Nss2Index6_JAguar	0xc4c
#define		rTxAGC_B_CCK11_CCK1_JAguar	0xe20
#define		rTxAGC_B_Ofdm18_Ofdm6_JAguar	0xe24
#define		rTxAGC_B_Ofdm54_Ofdm24_JAguar	0xe28
#define		rTxAGC_B_MCS3_MCS0_JAguar	0xe2c
#define		rTxAGC_B_MCS7_MCS4_JAguar	0xe30
#define		rTxAGC_B_MCS11_MCS8_JAguar	0xe34
#define		rTxAGC_B_MCS15_MCS12_JAguar	0xe38
#define		rTxAGC_B_Nss1Index3_Nss1Index0_JAguar	0xe3c
#define		rTxAGC_B_Nss1Index7_Nss1Index4_JAguar	0xe40
#define		rTxAGC_B_Nss2Index1_Nss1Index8_JAguar	0xe44
#define		rTxAGC_B_Nss2Index5_Nss2Index2_JAguar	0xe48
#define		rTxAGC_B_Nss2Index9_Nss2Index6_JAguar	0xe4c
#define		bTxAGC_byte0_Jaguar	0xff
#define		bTxAGC_byte1_Jaguar	0xff00
#define		bTxAGC_byte2_Jaguar	0xff0000
#define		bTxAGC_byte3_Jaguar	0xff000000


// TX AGC 
#define		rTxAGC_A_CCK11_CCK1_Jaguar2	0xc20
#define		rTxAGC_A_Ofdm18_Ofdm6_Jaguar2	0xc24
#define		rTxAGC_A_Ofdm54_Ofdm24_Jaguar2	0xc28
#define		rTxAGC_A_MCS3_MCS0_Jaguar2	0xc2c
#define		rTxAGC_A_MCS7_MCS4_Jaguar2	0xc30
#define		rTxAGC_A_MCS11_MCS8_Jaguar2	0xc34
#define		rTxAGC_A_MCS15_MCS12_Jaguar2	0xc38
#define		rTxAGC_A_MCS19_MCS16_Jaguar2	0xcd8
#define		rTxAGC_A_MCS23_MCS20_Jaguar2	0xcdc
#define		rTxAGC_A_Nss1Index3_Nss1Index0_Jaguar2	0xc3c
#define		rTxAGC_A_Nss1Index7_Nss1Index4_Jaguar2	0xc40
#define		rTxAGC_A_Nss2Index1_Nss1Index8_Jaguar2	0xc44
#define		rTxAGC_A_Nss2Index5_Nss2Index2_Jaguar2	0xc48
#define		rTxAGC_A_Nss2Index9_Nss2Index6_Jaguar2	0xc4c
#define		rTxAGC_A_Nss3Index3_Nss3Index0_Jaguar2	0xce0
#define		rTxAGC_A_Nss3Index7_Nss3Index4_Jaguar2	0xce4
#define		rTxAGC_A_Nss3Index9_Nss3Index8_Jaguar2	0xce8
#define		rTxAGC_B_CCK11_CCK1_Jaguar2	0xe20
#define		rTxAGC_B_Ofdm18_Ofdm6_Jaguar2	0xe24
#define		rTxAGC_B_Ofdm54_Ofdm24_Jaguar2	0xe28
#define		rTxAGC_B_MCS3_MCS0_Jaguar2	0xe2c
#define		rTxAGC_B_MCS7_MCS4_Jaguar2	0xe30
#define		rTxAGC_B_MCS11_MCS8_Jaguar2	0xe34
#define		rTxAGC_B_MCS15_MCS12_Jaguar2	0xe38
#define		rTxAGC_B_MCS19_MCS16_Jaguar2	0xed8
#define		rTxAGC_B_MCS23_MCS20_Jaguar2	0xedc
#define		rTxAGC_B_Nss1Index3_Nss1Index0_Jaguar2	0xe3c
#define		rTxAGC_B_Nss1Index7_Nss1Index4_Jaguar2	0xe40
#define		rTxAGC_B_Nss2Index1_Nss1Index8_Jaguar2	0xe44
#define		rTxAGC_B_Nss2Index5_Nss2Index2_Jaguar2	0xe48
#define		rTxAGC_B_Nss2Index9_Nss2Index6_Jaguar2	0xe4c
#define		rTxAGC_B_Nss3Index3_Nss3Index0_Jaguar2	0xee0
#define		rTxAGC_B_Nss3Index7_Nss3Index4_Jaguar2	0xee4
#define		rTxAGC_B_Nss3Index9_Nss3Index8_Jaguar2	0xee8
#define		rTxAGC_C_CCK11_CCK1_Jaguar2	0x1820
#define		rTxAGC_C_Ofdm18_Ofdm6_Jaguar2	0x1824
#define		rTxAGC_C_Ofdm54_Ofdm24_Jaguar2	0x1828
#define		rTxAGC_C_MCS3_MCS0_Jaguar2	0x182c
#define		rTxAGC_C_MCS7_MCS4_Jaguar2	0x1830
#define		rTxAGC_C_MCS11_MCS8_Jaguar2	0x1834
#define		rTxAGC_C_MCS15_MCS12_Jaguar2	0x1838
#define		rTxAGC_C_MCS19_MCS16_Jaguar2	0x18d8
#define		rTxAGC_C_MCS23_MCS20_Jaguar2	0x18dc
#define		rTxAGC_C_Nss1Index3_Nss1Index0_Jaguar2	0x183c
#define		rTxAGC_C_Nss1Index7_Nss1Index4_Jaguar2	0x1840
#define		rTxAGC_C_Nss2Index1_Nss1Index8_Jaguar2	0x1844
#define		rTxAGC_C_Nss2Index5_Nss2Index2_Jaguar2	0x1848
#define		rTxAGC_C_Nss2Index9_Nss2Index6_Jaguar2	0x184c
#define		rTxAGC_C_Nss3Index3_Nss3Index0_Jaguar2	0x18e0
#define		rTxAGC_C_Nss3Index7_Nss3Index4_Jaguar2	0x18e4
#define		rTxAGC_C_Nss3Index9_Nss3Index8_Jaguar2	0x18e8
#define		rTxAGC_D_CCK11_CCK1_Jaguar2	0x1a20
#define		rTxAGC_D_Ofdm18_Ofdm6_Jaguar2	0x1a24
#define		rTxAGC_D_Ofdm54_Ofdm24_Jaguar2	0x1a28
#define		rTxAGC_D_MCS3_MCS0_Jaguar2	0x1a2c
#define		rTxAGC_D_MCS7_MCS4_Jaguar2	0x1a30
#define		rTxAGC_D_MCS11_MCS8_Jaguar2	0x1a34
#define		rTxAGC_D_MCS15_MCS12_Jaguar2	0x1a38
#define		rTxAGC_D_MCS19_MCS16_Jaguar2	0x1ad8
#define		rTxAGC_D_MCS23_MCS20_Jaguar2	0x1adc
#define		rTxAGC_D_Nss1Index3_Nss1Index0_Jaguar2	0x1a3c
#define		rTxAGC_D_Nss1Index7_Nss1Index4_Jaguar2	0x1a40
#define		rTxAGC_D_Nss2Index1_Nss1Index8_Jaguar2	0x1a44
#define		rTxAGC_D_Nss2Index5_Nss2Index2_Jaguar2	0x1a48
#define		rTxAGC_D_Nss2Index9_Nss2Index6_Jaguar2	0x1a4c
#define		rTxAGC_D_Nss3Index3_Nss3Index0_Jaguar2	0x1ae0
#define		rTxAGC_D_Nss3Index7_Nss3Index4_Jaguar2	0x1ae4
#define		rTxAGC_D_Nss3Index9_Nss3Index8_Jaguar2	0x1ae8
// IQK YN: temporaily mask this part
//#define		rFPGA0_IQK					0xe28
//#define		rTx_IQK_Tone_A				0xe30
//#define		rRx_IQK_Tone_A				0xe34
//#define		rTx_IQK_PI_A					0xe38
//#define		rRx_IQK_PI_A					0xe3c

//#define		rTx_IQK 						0xe40
//#define		rRx_IQK						0xe44
//#define		rIQK_AGC_Pts					0xe48
//#define		rIQK_AGC_Rsp					0xe4c
//#define		rTx_IQK_Tone_B				0xe50
//#define		rRx_IQK_Tone_B				0xe54
//#define		rTx_IQK_PI_B					0xe58
//#define		rRx_IQK_PI_B					0xe5c
//#define		rIQK_AGC_Cont				0xe60


// AFE-related
#define		rA_AFEPwr1_Jaguar					0xc60 // dynamic AFE power control
#define		rA_AFEPwr2_Jaguar					0xc64 // dynamic AFE power control
#define		rA_Rx_WaitCCA_Tx_CCKRFON_Jaguar	0xc68
#define		rA_Tx_CCKBBON_OFDMRFON_Jaguar	0xc6c
#define		rA_Tx_OFDMBBON_Tx2Rx_Jaguar		0xc70
#define		rA_Tx2Tx_RXCCK_Jaguar				0xc74
#define		rA_Rx_OFDM_WaitRIFS_Jaguar		0xc78
#define		rA_Rx2Rx_BT_Jaguar					0xc7c
#define		rA_sleep_nav_Jaguar					0xc80
#define		rA_pmpd_Jaguar 					0xc84
#define		rB_AFEPwr1_Jaguar					0xe60 // dynamic AFE power control
#define		rB_AFEPwr2_Jaguar					0xe64 // dynamic AFE power control
#define		rB_Rx_WaitCCA_Tx_CCKRFON_Jaguar	0xe68
#define		rB_Tx_CCKBBON_OFDMRFON_Jaguar	0xe6c
#define		rB_Tx_OFDMBBON_Tx2Rx_Jaguar		0xe70
#define		rB_Tx2Tx_RXCCK_Jaguar				0xe74
#define		rB_Rx_OFDM_WaitRIFS_Jaguar		0xe78
#define		rB_Rx2Rx_BT_Jaguar					0xe7c
#define		rB_sleep_nav_Jaguar 			0xe80
#define		rB_pmpd_Jaguar					0xe84


// YN: mask these registers temporaily
//#define		rTx_Power_Before_IQK_A		0xe94
//#define		rTx_Power_After_IQK_A			0xe9c

//#define		rRx_Power_Before_IQK_A		0xea0
//#define		rRx_Power_Before_IQK_A_2		0xea4
//#define		rRx_Power_After_IQK_A			0xea8
//#define		rRx_Power_After_IQK_A_2		0xeac

//#define		rTx_Power_Before_IQK_B		0xeb4
//#define		rTx_Power_After_IQK_B			0xebc

//#define		rRx_Power_Before_IQK_B		0xec0
//#define		rRx_Power_Before_IQK_B_2		0xec4
//#define		rRx_Power_After_IQK_B			0xec8
//#define		rRx_Power_After_IQK_B_2		0xecc


// RSSI Dump
#define		rA_RSSIDump_Jaguar		0xBF0
#define		rB_RSSIDump_Jaguar		0xBF1
#define		rS1_RXevmDump_Jaguar	0xBF4 
#define		rS2_RXevmDump_Jaguar	0xBF5
#define		rA_RXsnrDump_Jaguar		0xBF6
#define		rB_RXsnrDump_Jaguar		0xBF7
#define		rA_CfoShortDump_Jaguar	0xBF8 
#define		rB_CfoShortDump_Jaguar	0xBFA
#define		rA_CfoLongDump_Jaguar	0xBEC
#define		rB_CfoLongDump_Jaguar	0xBEE
 

// RF Register
//
#define		RF_AC_Jaguar				0x00	// 
#define		RF_RF_Top_Jaguar			0x07	// 
#define		RF_TXLOK_Jaguar				0x08	// 
#define		RF_TXAPK_Jaguar				0x0B
#define		RF_CHNLBW_Jaguar			0x18	// RF channel and BW switch
#define		RF_RCK1_Jaguar				0x1c	// 
#define		RF_RCK2_Jaguar				0x1d
#define		RF_RCK3_Jaguar				0x1e
#define		RF_ModeTableAddr			0x30
#define		RF_ModeTableData0			0x31
#define		RF_ModeTableData1			0x32
#define		RF_TxLCTank_Jaguar			0x54
#define		RF_APK_Jaguar				0x63
#define		RF_LCK						0xB4
#define		RF_WeLut_Jaguar				0xEF

#define		bRF_CHNLBW_MOD_AG_Jaguar	0x70300
#define		bRF_CHNLBW_BW				0xc00


/*--------------------------Define Parameters-------------------------------*/


#endif	//__INC_HALJAGUARPHYREG_H

