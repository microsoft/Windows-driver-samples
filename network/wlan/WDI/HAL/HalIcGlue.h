#if (RTL8822B_SUPPORT == 0)
#define	ex_halbtc8822b1ant_power_on_setting(btcoexist)
#define	ex_halbtc8822b1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8822b1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8822b1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8822b1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8822b1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8822b1ant_halt_notify(btcoexist)
#define	ex_halbtc8822b1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8822b1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8822b1ant_periodical(btcoexist)
#define	ex_halbtc8822b1ant_display_coex_info(btcoexist)
#define	ex_halbtc8822b1ant_antenna_detection(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8822b1ant_antenna_isolation(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8822b1ant_psd_scan(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8822b1ant_display_ant_detection(btcoexist)
#define	ex_halbtc8822b1ant_dbg_control(btcoexist, op_code, op_len, pdata)

#define SDIO_8822B_HWDESC_HEADER_LEN	0
#define SDIO_8822B_DUMMY_OFFSET			1
#define SDIO_8822B_ALL_DUMMY_LENGTH		0

#define REG_SYS_FUNC_EN_8822B	0
#define REG_RF_CTRL_8822B		0

#define TxDescriptorChecksum_8822B(_txDesc)

#define SwLedOn_8822B(_Adapter, _pLed)		0
#define SwLedOff_8822B(_Adapter, _pLed)		0

#define TxUsbAggregation8822BU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8822B(_Adapter)
#define HalSetFWWoWlanMode8822B(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8822B(_pAdapter, _bFuncEn)
#define H2CCmdAction8822B(_Adapter, _pBuf, _bufLen)			0

#define SET_TX_DESC_USB_TXAGG_NUM_8822B(__pTxDesc, __Value)

#define TxDescriptorChecksum_8822B(_txDesc)

#define Hal_InitEfuseVars_8822B(_pEfuseHal)
#define Hal_ReadTxPowerInfo8822B(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8822B(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8822B(_pAdapter, _bWrite, _PwrState)
#define Hal_EfusePowerSwitch8822B_TestChip(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8822B(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8822B(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8822B(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8822B(_Adapter, _Enable)
#define SetFwInactivePSCmd_8822B(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8822B(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8822B(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8822B(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8822B(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8822B(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8822B(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define PHY_BB8822B_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8822B(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8822B(_Adapter, _PowerIndex, _RFPath, _Rate)

#define PHY_Combine4ByteTxPowerIndex_8822B(_A, _RF, _R, _B, _C)	0
#define HAL_DownloadRSVD_PreCfg_8822B(_A)
#define HAL_DownloadRSVD_PostCfg_8822B(_A)	RT_STATUS_FAILURE



#endif	//#if (RTL8822B_SUPPORT == 0)

#if (RTL8723D_SUPPORT == 0)

#define	NAN_Allocate(_pAdapter)	RT_STATUS_SUCCESS
#define	NAN_Free(_pAdapter)
#define	NAN_FreeAllWorkItem(Adapter)
#define	NAN_InitTimer(Adapter)
#define	NAN_CancelTimer(Adapter)
#define	NAN_ReleaseTimer(Adapter)
#define	NAN_ENABLED(_pAdapter) FALSE
#define	NAN_OnBeaconReceived(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	NAN_OnSDFReceived(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	NAN_UpdateDefaultSetting(_pAdapter, _ID, _pInputBuffer, _InputBufferLen)	RT_STATUS_SUCCESS
#define	NAN_GetAvailabilityInfo(_pAdapter, _pInputBuffer, _InputBufferLen)	RT_STATUS_SUCCESS

#define MAX_RX_DMA_BUFFER_SIZE_8723D		0x3800	// RX 14K

#define	ex_halbtc8723d1ant_power_on_setting(btcoexist)
#define	ex_halbtc8723d1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8723d1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8723d1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8723d1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8723d1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8723d1ant_halt_notify(btcoexist)
#define	ex_halbtc8723d1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8723d1ant_ScoreBoardStatusNotify(btcoexist, tmp_buf, length)
#define	ex_halbtc8723d1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8723d1ant_periodical(btcoexist)
#define	ex_halbtc8723d1ant_display_coex_info(btcoexist)
#define	ex_halbtc8723d1ant_antenna_detection(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8723d1ant_antenna_isolation(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8723d1ant_psd_scan(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8723d1ant_display_ant_detection(btcoexist)

#define	ex_halbtc8723d2ant_power_on_setting(btcoexist)
#define	ex_halbtc8723d2ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8723d2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8723d2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8723d2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8723d2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8723d2ant_halt_notify(btcoexist)
#define	ex_halbtc8723d2ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8723d2ant_periodical(btcoexist)
#define	ex_halbtc8723d2ant_display_coex_info(btcoexist)

#define PrepareRxDesc8723DE(_Adapter, _LengthArray)		RT_STATUS_FAILURE
#define FreeRxDesc8723DE(_Adapter)
#define ResetRxDesc8723DE(_Adapter, _LengthArray)		RT_STATUS_FAILURE
#define ReturnRxDescBuffer8723DE(_Adapter, _QueueID, _index, _buffer)
#define	IsRxDescFilledWithPacket8723DE(_Adapter, _QueueID, _index, _uBufferAddress)		FALSE

#define	HalDownloadRSVDPage8723DE(_Adapter)
#define	Hal_DBIRead4Byte_8723DE(_Adapter, _Addr)		0
#define	Hal_DBIWrite4Byte_8723DE(_Adapter, _Addr, _Data)

#define SwLedOn_8723D(_Adapter, _pLed)		0
#define SwLedOff_8723D(_Adapter, _pLed)		0

#define TxUsbAggregation8723DU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8723D(_Adapter)
#define HalSetFWWoWlanMode8723D(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8723D(_pAdapter, _bFuncEn)
#define H2CCmdAction8723D(_Adapter, _pBuf, _bufLen)			0

#define Hal_InitEfuseVars_8723D(_pEfuseHal)
#define Hal_ReadTxPowerInfo8723D(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8723D(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8723D(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8723D(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8723D(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8723D(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8723D(_Adapter, _Enable)
#define SetFwInactivePSCmd_8723D(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8723D(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8723D(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define HAL_FwLPSDeepSleepInit8723D(_pAdapter)
#define HAL_FwLPSDeepSleepDeInit8723D(_pAdapter)
#define HAL_CheckChangeTxBoundaryInProgress8723D(_pAdapter)		FALSE
#define HAL_FwLPSDeepSleepGetStatusForEnterLPS8723D(_pAdapter)	FALSE

#define PHY_QueryBBReg8723D(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8723D(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8723D(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8723D(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8723D_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8723D(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8723D(_Adapter, _PowerIndex, _RFPath, _Rate)

#endif	// (RTL8723D_SUPPORT == 0)


#if (RTL8703B_SUPPORT == 0)

#define MAX_RX_DMA_BUFFER_SIZE_8703B		0x2800	// RX 10K

#define SET_TX_DESC_USB_TXAGG_NUM_8723D(__pTxDesc, __Value)

#define TxDescriptorChecksum_8723D(_txDesc)

#define SwLedOn_8703B(_Adapter, _pLed)		0
#define SwLedOff_8703B(_Adapter, _pLed)		0

#define TxUsbAggregation8703BU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8703B(_Adapter)
#define HalSetFWWoWlanMode8703B(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8703B(_pAdapter, _bFuncEn)
#define H2CCmdAction8703B(_Adapter, _pBuf, _bufLen)			0

#define SET_TX_DESC_USB_TXAGG_NUM_8703B(__pTxDesc, __Value)

#define TxDescriptorChecksum_8703B(_txDesc)

#define Hal_InitEfuseVars_8703B(_pEfuseHal)
#define Hal_ReadTxPowerInfo8703B(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8703B(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8703B(_pAdapter, _bWrite, _PwrState)
#define Hal_EfusePowerSwitch8703B_TestChip(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8703B(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8703B(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8703B(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8703B(_Adapter, _Enable)
#define SetFwInactivePSCmd_8703B(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8703B(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8703B(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8703B(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8703B(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8703B(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8703B(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8703B_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8703B(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8703B(_Adapter, _PowerIndex, _RFPath, _Rate)

#define	ex_halbtc8703b1ant_power_on_setting(btcoexist)
#define	ex_halbtc8703b1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8703b1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8703b1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8703b1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8703b1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8703b1ant_halt_notify(btcoexist)
#define	ex_halbtc8703b1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8703b1ant_ScoreBoardStatusNotify(btcoexist, tmp_buf, length)
#define	ex_halbtc8703b1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8703b1ant_periodical(btcoexist)
#define	ex_halbtc8703b1ant_display_coex_info(btcoexist)
#define	ex_halbtc8703b1ant_antenna_detection(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8703b1ant_antenna_isolation(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8703b1ant_psd_scan(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8703b1ant_display_ant_detection(btcoexist)


#endif	// (RTL8703B_SUPPORT == 0)

#if (RTL8188F_SUPPORT == 0)

#define MAX_RX_DMA_BUFFER_SIZE_8188F		0x4000	// RX 10K
#define AVG_THERMAL_NUM_8188F	4
#define RF_T_METER_8188F		0x42
#define IQK_DELAY_TIME_8188F	25

#define SwLedOn_8188F(_Adapter, _pLed)		0
#define SwLedOff_8188F(_Adapter, _pLed)		0

#define TxUsbAggregation8188FU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8188F(_Adapter)
#define HalSetFWWoWlanMode8188F(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8188F(_pAdapter, _bFuncEn)
#define H2CCmdAction8188F(_Adapter, _pBuf, _bufLen)			0

#define TxDescriptorChecksum_8188F(_txDesc)

#define Hal_InitEfuseVars_8188F(_pEfuseHal)
#define Hal_ReadTxPowerInfo8188F(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8188F(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8188F(_pAdapter, _bWrite, _PwrState)
#define Hal_EfusePowerSwitch8188F_TestChip(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8188F(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8188F(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8188F(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8188F(_Adapter, _Enable)
#define SetFwInactivePSCmd_8188F(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8188F(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8188F(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8188F(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8188F(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8188F(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8188F(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8188F_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8188F(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8188F(_Adapter, _PowerIndex, _RFPath, _Rate)

#define SET_TX_DESC_USB_TXAGG_NUM_8188F(__pTxDesc, __Value)

void PHY_IQCalibrate_8188F(	
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	IN PDM_ODM_T		pDM_Odm,
#else
	IN PADAPTER	Adapter,
#endif
	IN	BOOLEAN 	bReCovery,
	IN	BOOLEAN 	bRestore
	);

#endif	// (RTL8188F_SUPPORT == 0)
#if (RTL8821B_SUPPORT == 0)

#define MAX_RX_DMA_BUFFER_SIZE_8821B		0x6000

#define TxDescriptorChecksum_8821B(_txDesc)

#define SwLedOn_8821B(_Adapter, _pLed)		0
#define SwLedOff_8821B(_Adapter, _pLed)		0

#define TxUsbAggregation8821BU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8821B(_Adapter)
#define HalSetFWWoWlanMode8821B(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8821B(_pAdapter, _bFuncEn)
#define H2CCmdAction8821B(_Adapter, _pBuf, _bufLen)			0

#define SET_TX_DESC_USB_TXAGG_NUM_8821B(__pTxDesc, __Value)

#define TxDescriptorChecksum_8821B(_txDesc)

#define Hal_InitEfuseVars_8821B(_pEfuseHal)
#define Hal_ReadTxPowerInfo8821B(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8821B(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8821B(_pAdapter, _bWrite, _PwrState)
#define Hal_EfusePowerSwitch8821B_TestChip(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8821B(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8821B(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8821B(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8821B(_Adapter, _Enable)
#define SetFwInactivePSCmd_8821B(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8821B(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8821B(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8821B(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8821B(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8821B(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8821B(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8821B_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8821B(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8821B(_Adapter, _PowerIndex, _RFPath, _Rate)

#endif	// (RTL8821B_SUPPORT == 0)

#if (RTL8814A_SUPPORT == 0)

#define REG_HSIMR_8814A		0
#define REG_IQ_DUMP_8814A	0
#define REG_PKTBUF_DBG_CTRL_8814A	0
#define RX_STATUS_DESC_SIZE	0
#define REG_SYS_FUNC_EN_8814A	0


#define REG_RF_CTRL0_8814A				0x001F	// 1 Byte
#define REG_RF_CTRL1_8814A				0x0020	// 1 Byte
#define REG_RF_CTRL3_8814A				0x0076	// 1 Byte

#define H2C_8814A_RSSI_REPORT 0x42
#define H2C_8814A_RA_PARA_ADJUST 0x47
#define H2C_8814A_DYNAMIC_TX_PATH 0x48
#define H2C_8814A_FW_TRACE_EN 0x49

#define REG_NDPA_OPT_CTRL_8814A		0x045F
#define REG_NDPA_RATE_8814A				0x045D

#define TxDescriptorChecksum_8814A(_txDesc)

#define SwLedOn_8814A(_Adapter, _pLed)		0
#define SwLedOff_8814A(_Adapter, _pLed)		0

#define TxUsbAggregation8814AU(_Adapter, _queueId)

#define SetWoWLANCAMEntry8814A(_Adapter)
#define HalSetFWWoWlanMode8814A(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8814A(_pAdapter, _bFuncEn)
#define H2CCmdAction8814A(_Adapter, _pBuf, _bufLen)			0

#define SET_TX_DESC_USB_TXAGG_NUM_8814A(__pTxDesc, __Value)

#define TxDescriptorChecksum_8814A(_txDesc)

#define Hal_InitEfuseVars_8814A(_pEfuseHal)
#define Hal_ReadTxPowerInfo8814A(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8814A(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8814A(_pAdapter, _bWrite, _PwrState)
#define Hal_EfusePowerSwitch8814A_TestChip(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8814A(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8814A(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8814A(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8814A(_Adapter, _Enable)
#define SetFwInactivePSCmd_8814A(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8814A(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8814A(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8814A(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8814A(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8814A(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8814A(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8814A_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8814A(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8814A(_Adapter, _PowerIndex, _RFPath, _Rate)

#endif	// (RTL8821B_SUPPORT == 0)


#if (RTL8192E_SUPPORT == 0)


#define RX_DESC_SIZE_92EE			24
#define REG_DARFRC_8192E					0x0430
#define REG_NDPA_OPT_CTRL_8192E				0x045F

#define	REG_RXQ_TXBD_IDX_8192E			0x03B4
#define		rA_RSSIDump_92E 			0xcb0
#define		rB_RSSIDump_92E 			0xcb1
#define		rS1_RXevmDump_92E			0xcb2 
#define		rS2_RXevmDump_92E			0xcb3
#define		rA_RXsnrDump_92E			0xcb4
#define		rB_RXsnrDump_92E			0xcb5
#define		rA_CfoShortDump_92E		0xcb6 
#define		rB_CfoShortDump_92E		0xcb8
#define   	rA_CfoLongDump_92E			0xcba
#define		rB_CfoLongDump_92E			0xcbc

#define HalDownloadRSVDPage8192EE(_Adapter)
#define Hal_DBIRead4Byte_8192EE(_Adapter, _Addr)	0
#define Hal_DBIWrite4Byte_8192EE(_Adapter, _Addr, _Data)

#define SwLedOn_8192E(_Adapter, _pLed)		0
#define SwLedOff_8192E(_Adapter, _pLed)		0

#define SetWoWLANCAMEntry8192E(_Adapter)
#define HalSetFWWoWlanMode8192E(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8192E(_pAdapter, _bFuncEn)
#define H2CCmdAction8192E(_Adapter, _pBuf, _bufLen)			0

#define SET_TX_DESC_USB_TXAGG_NUM_92E(__pTxDesc, __Value)

#define TxDescriptorChecksum_8192E(_txDesc)
#define TxUsbAggregation8192E(_Adapter, _queueId)

#define Hal_InitEfuseVars_8192E(_pEfuseHal)
#define Hal_ReadTxPowerInfo8192E(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfuseSwitchToBank8192E(_pAdapter, _bank, _bPseudoTest)		TRUE
#define Hal_EfusePowerSwitch8192E(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8192E(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8192E(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8192E(_Adapter, _Type, _ScanOffloadEnable, _NLOEnable)
#define SetFwRemoteWakeCtrlCmd_8192E(_Adapter, _Enable)
#define SetFwInactivePSCmd_8192E(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8192E(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8192E(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define PHY_QueryBBReg8192E(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8192E(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8192E(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8192E(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8192E_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8192E(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8192E(_Adapter, _PowerIndex, _RFPath, _Rate)

#define	ex_halbtc8192e1ant_power_on_setting(btcoexist)
#define	ex_halbtc8192e1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8192e1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8192e1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8192e1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8192e1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8192e1ant_halt_notify(btcoexist)
#define	ex_halbtc8192e1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8192e1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8192e1ant_periodical(btcoexist)
#define	ex_halbtc8192e1ant_display_coex_info(btcoexist)
#define	ex_halbtc8192e1ant_dbg_control(btcoexist, op_code, op_len, pdata)

#define	ex_halbtc8192e2ant_power_on_setting(btcoexist)
#define	ex_halbtc8192e2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8192e2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8192e2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8192e2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8192e2ant_halt_notify(btcoexist)
#define	ex_halbtc8192e2ant_periodical(btcoexist)
#define	ex_halbtc8192e2ant_display_coex_info(btcoexist)

#endif	// (RTL8192E_SUPPORT == 0)


#if (RTL8188E_SUPPORT == 0)

#define H2C_88E_RSSI_REPORT  0x42

#define PCIE_8188E_HWDESC_HEADER_LEN	32//sizeof(TX_DESC_8192SE)

#define PrepareRxDesc8188EE(_Adapter, _LengthArray)		RT_STATUS_FAILURE
#define FreeRxDesc8188EE(_Adapter)
#define ResetRxDesc8188EE(_Adapter, _LengthArray)		RT_STATUS_FAILURE
#define ReturnRxDescBuffer8188EE(_Adapter, _QueueID, _index, _buffer)
#define	IsRxDescFilledWithPacket8188EE(_Adapter, _QueueID, _index, _uBufferAddress)		FALSE

#define SwLedOn_8188E(_Adapter, _pLed)		0
#define SwLedOff_8188E(_Adapter, _pLed)		0

#define TxUsbAggregation8188E(_Adapter, _queueId)

#define ForceLeaveHwClock32K8188E(_Adapter)

#define SetWoWLANCAMEntry8188E(_Adapter)
#define HalSetFWWoWlanMode8188E(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8188E(_pAdapter, _bFuncEn)
#define H2CCmdAction8188E(_Adapter, _pBuf, _bufLen)			0
#define SetBcnCtrlReg_8188E(_Adapter, _SetBits, _ClearBits)

#define SET_TX_DESC_USB_TXAGG_NUM_88E(__pTxDesc, __Value)
#define TxDescriptorChecksum_8188E(_txDesc)

#define Hal_InitEfuseVars_8188E(_pEfuseHal)
#define Hal_ReadTxPowerInfo88E(_Adapter, _PROMContent, _AutoLoadFail)

#define SetFwGlobalInfoCmd_8188E(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8188E(_Adapter, _bEnabled)
#define SetFwScanOffloadCtrlCmd_8188E(_Adapter, _Type, _ScanOffloadEnable)
#define SetFwRemoteWakeCtrlCmd_8188E(_Adapter, _Enable)
#define SetFwInactivePSCmd_8188E(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd88E(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload88E(_Adapter, _bUsedWoWLANFw)	RT_STATUS_SUCCESS

#define PHY_QueryBBReg8188E(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8188E(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8188E(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8188E(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8188E_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8188E(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8188E(_Adapter, _PowerIndex, _RFPath, _Rate)

#define SDIO_8188E_DUMMY_OFFSET			1 // Reseved for Early Mode information.
#define SDIO_92C_DUMMY_UNIT			8
#define SDIO_8188E_ALL_DUMMY_LENGTH	(SDIO_8188E_DUMMY_OFFSET * SDIO_92C_DUMMY_UNIT)
#define SDIO_8188E_HWDESC_HEADER_LEN	(TX_DESC_SIZE + SDIO_8188E_ALL_DUMMY_LENGTH)


#define MAX_RX_DMA_BUFFER_SIZE_88E(__Adapter)		((!IS_VENDOR_8188E_SMIC_SERIES(__Adapter))?((GET_HAL_DATA(__Adapter)->bIsMPChip) ? 0x2400 : 0x1C00):0x3A00)


#endif	// (RTL8188E_SUPPORT == 0)

#if (RTL8812A_SUPPORT == 0)

#define REG_RXDMA_AGG_PG_TH_8812A	0x280
#define REG_APS_FSMCO_8812A			0x0004
#define REG_MACID_8812A				0x0610
#define REG_TRXDMA_CTRL_8812A		0x010C
#define REG_TXDMA_OFFSET_CHK_8812A	0x020C
#define REG_DUAL_TSF_RST_8812A		0x0553
#define REG_SECONDARY_CCA_CTRL_8812A	0x577

#define REG_MULTI_FUNC_CTRL_8812A		0x0068

#define REG_CR_8812A						0x0100
#define REG_FWHW_TXQ_CTRL_8812A		0x0420
#define REG_TDECTRL_8812A				0x0208
#define REG_MACID1_8812A				0x0700
#define REG_BSSID1_8812A				0x0708
#define REG_BFMEE_SEL_8812A				0x0714
#define REG_SND_PTCL_CTRL_8812A		0x0718
#define REG_TXBF_CTRL_8812A				0x042C
#define REG_BFMER0_INFO_8812A			0x06E4
#define REG_BFMER1_INFO_8812A			0x06EC
#define REG_CSI_RPT_PARAM_BW20_8812A	0x06F4
#define REG_CSI_RPT_PARAM_BW40_8812A	0x06F8
#define REG_CSI_RPT_PARAM_BW80_8812A	0x06FC
#define REG_TXPKT_EMPTY_8812A			0x041A
#define	REG_PCIE_CTRL_REG_8812A		0x0300
#define REG_TXPAUSE_8812A				0x0522
#define REG_SCH_TXCMD_8812A			0x05F8
#define REG_RXDMA_CONTROL_8812A		0x0286 // Control the RX DMA.
#define REG_SYS_CLKR_8812A				0x0008	// 2 Byte
#define REG_AFE_PLL_CTRL_8812A			0x0028	// 4 Byte

#define REG_NDPA_OPT_CTRL_8812A		0x045F

#define		EFUSE_MAX_SECTION_JAGUAR			64
#define		HWSET_MAX_SIZE_JAGUAR				512

#define		EFUSE_PROTECT_BYTES_BANK_JAGUAR		16
#define		EFUSE_MAX_SECTION_JAGUAR			64
#define		EFUSE_REAL_CONTENT_LEN_JAGUAR		512
#define		EFUSE_MAP_LEN_JAGUAR				512



#define REG_RF_CTRL_8812A				0x001F	// 1 Byte
#define REG_OPT_CTRL_8812A				0x0074

#define		rDMA_trigger_Jaguar2		0x95C	// ADC sample mode
#define		rDMA_trigger_Jaguar2		0x95C	// ADC sample mode


#define		rA_SIRead_Jaguar			0xd08 // RF readback with SI
#define		rB_SIRead_Jaguar			0xd48 // RF readback with SI
#define		rA_PIRead_Jaguar			0xd04 // RF readback with PI
#define		rB_PIRead_Jaguar			0xd44 // RF readback with PI

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

#define		rHSSIRead_Jaguar			0x8b0  // RF read addr

#define HalDownloadRSVDPage8812AE(_Adapter)
#define Hal_DBIRead4Byte_8812AE(_Adapter, _Addr)	0
#define Hal_DBIWrite4Byte_8812AE(_Adapter, _Addr, _Data)
#define Hal_DBIRead4Byte_8812AE(_Adapter, _Addr)	0
#define	Hal_DBIWrite4Byte_8812AE(_Adapter, _Addr, _Data)

#define SwLedOn_8812A(_Adapter, _pLed)		0
#define SwLedOff_8812A(_Adapter, _pLed)		0

#define SetWoWLANCAMEntry8812A(_Adapter)
#define HalSetFWWoWlanMode8812A(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8812A(_pAdapter, _bFuncEn)
#define H2CCmdAction8812A(_Adapter, _pBuf, _bufLen)			0

#define Hal_InitEfuseVars_8812A(_pEfuseHal)
#define Hal_ReadTxPowerInfo_8812A(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfusePgPacketExceptionHandle_8812A(_pAdapter, _ErrOffset)	0
#define Hal_EfusePowerSwitch8812A(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8812A(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8812A(_Adapter, _bEnabled)
#define SetFwRemoteWakeCtrlCmd_8812A(_Adapter, _Enable)
#define SetFwInactivePSCmd_8812A(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8812A(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8812A(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define TxDescriptorChecksum_8812(_txDesc)

#define PHY_QueryBBReg8812(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8812(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8812(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8812(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8812A_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8812A(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8812A(_Adapter, _PowerIndex, _RFPath, _Rate)
#define PHY_GetTxBBSwing_8812A(_Adapter, _Band, _RFPath)			0

#define	ex_halbtc8812a1ant_power_on_setting(btcoexist)
#define	ex_halbtc8812a1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8812a1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8812a1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8812a1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8812a1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8812a1ant_halt_notify(btcoexist)
#define	ex_halbtc8812a1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8812a1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8812a1ant_periodical(btcoexist)
#define	ex_halbtc8812a1ant_dbg_control(btcoexist, op_code, op_len, pdata)
#define	ex_halbtc8812a1ant_display_coex_info(btcoexist)

#define	ex_halbtc8812a2ant_power_on_setting(btcoexist)
#define	ex_halbtc8812a2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8812a2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8812a2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8812a2ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8812a2ant_halt_notify(btcoexist)
#define	ex_halbtc8812a2ant_periodical(btcoexist)
#define	ex_halbtc8812a2ant_display_coex_info(btcoexist)
#define	ex_halbtc8812a2ant_dbg_control(btcoexist, op_code, op_len, pdata)



#endif	// (RTL8812A_SUPPORT == 0)

#if (RTL8821A_SUPPORT == 0)

#define 	EFUSE_MAX_BANK_8821A					3
#define SET_TX_DESC_USB_TXAGG_NUM_8812(__pTxDesc, __Value)
#define DRIVER_EARLY_INT_TIME_8821			0x05

#define FillH2CCommand8821A(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)	0


#define Hal_EfuseSwitchToBank8821A(_pAdapter, _bank, _bPseudoTest)		TRUE

#define HalDownloadRSVDPage8821AE(_Adapter)
#define Hal_DBIRead4Byte_8821AE(_Adapter, _Addr)	0
#define Hal_DBIWrite4Byte_8821AE(_Adapter, _Addr, _Data)
#define Hal_DBIRead4Byte_8821AE(_Adapter, _Addr)	0
#define	Hal_DBIWrite4Byte_8821AE(_Adapter, _Addr, _Data)

#define SwLedOn_8821A(_Adapter, _pLed)		0
#define SwLedOff_8821A(_Adapter, _pLed)		0

#define SetWoWLANCAMEntry8821A(_Adapter)
#define HalSetFWWoWlanMode8821A(_pAdapter, _bFuncEn)		RT_STATUS_SUCCESS
#define HalSetFwKeepAliveCmd8821A(_pAdapter, _bFuncEn)
#define H2CCmdAction8821A(_Adapter, _pBuf, _bufLen)			0

#define Hal_InitEfuseVars_8821A(_pEfuseHal)
#define Hal_ReadTxPowerInfo_8821A(_Adapter, _PROMContent, _AutoLoadFail)
#define Hal_EfusePgPacketExceptionHandle_8821A(_pAdapter, _ErrOffset)	0
#define Hal_EfusePowerSwitch8821A(_pAdapter, _bWrite, _PwrState)

#define SetFwGlobalInfoCmd_8821A(_Adapter)
#define SetFwDisconnectDecisionCtrlCmd_8821A(_Adapter, _bEnabled)
#define SetFwRemoteWakeCtrlCmd_8821A(_Adapter, _Enable)
#define SetFwInactivePSCmd_8821A(_Adapter, _Enable, _bActiveWakeup, _bForceClkOff)
#define FillH2CCmd8821A(_Adapter, _ElementID, _CmdLen, _pCmdBuffer)
#define FirmwareDownload8821A(_Adapter, _bUsedWoWLANFw)		RT_STATUS_SUCCESS

#define TxDescriptorChecksum_8821(_txDesc)

#define PHY_QueryBBReg8821A(_Adapter, _RegAddr, _BitMask)			0
#define PHY_SetBBReg8821A(_Adapter, _RegAddr, _BitMask, _Data)
#define PHY_QueryRFReg8821A(_Adapter, _eRFPath, _RegAddr, _BitMask)	0
#define PHY_SetRFReg8821A(_Adapter, _eRFPath, _RegAddr, _BitMask, _Data)
#define phy_BB8821A_Config_ParaFile(_Adapter)		RT_STATUS_SUCCESS
#define PHY_GetTxPowerIndex_8821A(_pAdapter, _RFPath, _Rate, _BandWidth, _Channel)	0
#define PHY_SetTxPowerIndex_8821A(_Adapter, _PowerIndex, _RFPath, _Rate)
#define PHY_GetTxBBSwing_8821A(_Adapter, _Band, _RFPath)			0

#define	ex_halbtc8821a1ant_power_on_setting(btcoexist)
#define	ex_halbtc8821a1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8821a1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8821a1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8821a1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8821a1ant_halt_notify(btcoexist)
#define	ex_halbtc8821a1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8821a1ant_periodical(btcoexist)
#define	ex_halbtc8821a1ant_display_coex_info(btcoexist)

#define	ex_halbtc8821a2ant_power_on_setting(btcoexist)
#define	ex_halbtc8821a2ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8821a2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8821a2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8821a2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8821a2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8821a2ant_halt_notify(btcoexist)
#define	ex_halbtc8821a2ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8821a2ant_periodical(btcoexist)
#define	ex_halbtc8821a2ant_display_coex_info(btcoexist)

#define	ex_halbtc8821aCsr2ant_power_on_setting(btcoexist)
#define	ex_halbtc8821aCsr2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8821aCsr2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8821aCsr2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8821aCsr2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8821aCsr2ant_halt_notify(btcoexist)
#define	ex_halbtc8821aCsr2ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8821aCsr2ant_periodical(btcoexist)
#define	ex_halbtc8821aCsr2ant_display_coex_info(btcoexist)

#define SDIO_8821_DUMMY_UNIT				8
#define SDIO_8821_DUMMY_OFFSET			1
#define SDIO_8821_ALL_DUMMY_LENGTH		SDIO_8821_DUMMY_OFFSET * SDIO_8821_DUMMY_UNIT
#define HWDESC_HEADER_LEN_SDIO_8821		40
#define SDIO_8821_HWDESC_HEADER_LEN		(HWDESC_HEADER_LEN_SDIO_8821 + SDIO_8821_ALL_DUMMY_LENGTH)	// 8byte dummy


#endif	// (RTL8821A_SUPPORT == 0)

#if (RTL8723B_SUPPORT == 0)

#define		EFUSE_REAL_CONTENT_LEN_8723B	512
#define 	AVAILABLE_EFUSE_ADDR_8723B(addr) 	(addr < EFUSE_REAL_CONTENT_LEN_8723B)


BOOLEAN
Hal_EfuseSwitchToBank8723B(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		bank,
	IN		BOOLEAN		bPseudoTest
	);

VOID
Hal_EfusePowerSwitch8723B(
	IN	PADAPTER	pAdapter,
	IN	UINT8		bWrite,
	IN	UINT8		PwrState
	);

VOID
Hal_EfusePowerSwitch8723B_TestChip(
	IN	PADAPTER	pAdapter,
	IN	UINT8		bWrite,
	IN	UINT8		PwrState
	);

VOID
Hal_InitEfuseVars_8723B(
	IN PEFUSE_HAL		pEfuseHal
	);

VOID
HalSetFcsAdjustTsf_8723B(
	IN	PADAPTER			Adapter
	);

VOID
SetWoWLANCAMEntry8723B(
	PADAPTER			Adapter
	);

VOID
SetFwGlobalInfoCmd_8723B(
	IN PADAPTER Adapter
	);

RT_STATUS
HalSetFWWoWlanMode8723B(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN 	bFuncEn
	);

VOID
HalSetFwKeepAliveCmd8723B(	
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN bFuncEn
	);

VOID
SetFwScanOffloadCtrlCmd_8723B(
	IN PADAPTER		Adapter,
	IN u1Byte		Type,
	IN u1Byte		ScanOffloadEnable,
	IN u1Byte		NLOEnable
	);

VOID
SetFwDisconnectDecisionCtrlCmd_8723B(
	IN PADAPTER Adapter,
	IN BOOLEAN	bEnabled
	);

VOID
SetFwRemoteWakeCtrlCmd_8723B(
	IN PADAPTER Adapter,
	IN u1Byte		Enable
	);

VOID
SetFwInactivePSCmd_8723B(
	IN PADAPTER 	Adapter,
	IN u1Byte		Enable,
	IN BOOLEAN		bActiveWakeup,
	IN BOOLEAN		bForceClkOff
	);

VOID
SetFwWiFiCalibrationCmd_8723B(
	IN PADAPTER	Adapter,
	IN u1Byte	u1Enable
	);

VOID
FillH2CCmd8723B(	
	IN	PADAPTER		Adapter,
	IN	u1Byte	ElementID,
	IN	u4Byte	CmdLen,
	IN	pu1Byte pCmdBuffer
	);

u4Byte
FillH2CCommand8723B(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
	);

u4Byte
FW8723B_FillH2cCommand(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
	);

RT_STATUS
FirmwareDownload8723B(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN			bUsedWoWLANFw
	);

VOID
HalDownloadRSVDPage8723BE(
	IN	PADAPTER			Adapter
	);

u1Byte
hal_DBIRead_8723BE(	
	IN	PADAPTER 	Adapter,
	IN 	u2Byte		Addr
	);

VOID
hal_DBIWrite_8723BE(	
	IN	PADAPTER 	Adapter,
	IN 	u2Byte		Addr,
	IN	u1Byte		Data
	);

VOID
Hal_ReadTxPowerInfo8723B(
	IN	PADAPTER 		Adapter,
	IN	pu1Byte			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_Set8723bAntPath(
	IN	PADAPTER				pAdapter,
	IN	BOOLEAN					bAntennaAux,	//For 1-Ant--> 1: Antenna at S0, 0: Antenna at S1. Set 0 for 2-Ant
	IN	BOOLEAN					bExtSwitch,		// 1: Ext Switch (SPDT) exist on module, 0: no Ext Switch (SPDT) exist on module
	IN	BOOLEAN					bTwoAntenna,	// 1: 2-Antenna, 0:1-Antenna
	IN	u1Byte					antennaPos,		//Set Antenna Pos, For 1-Ant: BTC_ANT_PATH_WIFI, BTC_ANT_PATH_BT, BTC_ANT_PATH_PTA, For 2-Ant:BTC_ANT_WIFI_AT_MAIN, BTC_ANT_WIFI_AT_Aux
	IN	u1Byte					wifiState		//BTC_WIFI_STAT_INIT, BTC_WIFI_STAT_IQK, BTC_WIFI_STAT_NORMAL_OFF, BTC_WIFI_STAT_MP_OFF, BTC_WIFI_STAT_NORMAL, BTC_WIFI_STAT_ANT_DIV
	);

#define	ex_halbtc8723b1ant_power_on_setting(btcoexist)
#define	ex_halbtc8723b1ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8723b1ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8723b1ant_init_coex_dm(btcoexist)
#define	ex_halbtc8723b1ant_ips_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_lps_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_scan_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_connect_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8723b1ant_rf_status_notify(btcoexist, type)
#define	ex_halbtc8723b1ant_halt_notify(btcoexist)
#define	ex_halbtc8723b1ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8723b1ant_coex_dm_reset(btcoexist)
#define	ex_halbtc8723b1ant_periodical(btcoexist)
#define	ex_halbtc8723b1ant_display_coex_info(btcoexist)
#define	ex_halbtc8723b1ant_antenna_detection(btcoexist, cent_freq, offset, span, seconds)
#define	ex_halbtc8723b1ant_display_ant_detection(btcoexist)

#define	ex_halbtc8723b2ant_power_on_setting(btcoexist)
#define	ex_halbtc8723b2ant_pre_load_firmware(btcoexist)
#define	ex_halbtc8723b2ant_init_hw_config(btcoexist, wifi_only)
#define	ex_halbtc8723b2ant_init_coex_dm(btcoexist)
#define	ex_halbtc8723b2ant_ips_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_lps_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_scan_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_connect_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_media_status_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_specific_packet_notify(btcoexist, type)
#define	ex_halbtc8723b2ant_bt_info_notify(btcoexist, tmp_buf, length)
#define	ex_halbtc8723b2ant_halt_notify(btcoexist)
#define	ex_halbtc8723b2ant_pnp_notify(btcoexist, pnp_state)
#define	ex_halbtc8723b2ant_periodical(btcoexist)
#define	ex_halbtc8723b2ant_display_coex_info(btcoexist)

u4Byte
PHY_QueryBBReg8723B(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	);

VOID
PHY_SetBBReg8723B(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	);

u4Byte
PHY_QueryRFReg8723B(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask
	);

VOID
PHY_SetRFReg8723B(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask,
	IN	u4Byte				Data
	);

RT_STATUS
PHY_RF6052_Config_8723B(
	IN	PADAPTER		Adapter
	);

VOID
PHY_RF6052SetBandwidth8723B(
	IN	PADAPTER				Adapter,
	IN	CHANNEL_WIDTH		Bandwidth
	);

RT_STATUS
phy_BB8723B_Config_ParaFile(
	IN	PADAPTER	Adapter
	);

u1Byte
PHY_GetTxPowerIndex_8723B(
	IN	PADAPTER			Adapter,
	IN	u1Byte					RFPath,
	IN	u1Byte				Rate,
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u1Byte				Channel
	);

VOID
PHY_SetTxPowerIndex_8723B(
	IN	PADAPTER			Adapter,
	IN	u4Byte				PowerIndex,
	IN	u1Byte				RFPath, 
	IN	u1Byte				Rate
	);

u4Byte 
PHY_GetTxBBSwing_8723B(
	IN	PADAPTER	Adapter,
	IN	BAND_TYPE 	Band,
	IN  u1Byte 		RFPath
	);

u4Byte
H2CCmdAction8723B(
	IN	PADAPTER		Adapter,
	IN	PVOID		pBuf,
	IN	u2Byte		bufLen
	);

VOID
TxDescriptorChecksum_8723B(
	IN	pu1Byte txDesc
	);


#define MAX_RX_DMA_BUFFER_SIZE_8723B		0x2800	// RX 10K
#define SET_TX_DESC_USB_TXAGG_NUM_8723B(__pTxDesc, __Value) SET_BITS_TO_LE_4BYTE(__pTxDesc+28, 24, 8, __Value)

VOID
SwLedOn_8723B(
	IN	PADAPTER			Adapter, 
	IN	PLED_SDIO			pLed
	);

VOID
SwLedOff_8723B(
	IN	PADAPTER			Adapter, 
	IN	PLED_SDIO			pLed
	);

#endif	// (RTL8723B_SUPPORT == 0)



