#ifndef __INC_HAL_WOWLAN_H
#define __INC_HAL_WOWLAN_H

VOID
HALWOWL_DisableMACRxDMA(
	IN	PADAPTER	Adapter
	);

VOID
HALWOWL_EnableMACRxDMA(
	IN	PADAPTER	Adapter
	);


VOID
HALWOWL_EnablePowerSave(
	IN	PADAPTER	Adapter
	);

VOID
HALWOWL_DisablePowerSave(
	IN	PADAPTER	Adapter
	);

VOID
HALWOWL_HWPowerSettings(
	IN	PADAPTER	Adapter
	);

RT_STATUS
HALWOWL_FuncEnable(
	IN	PADAPTER	Adapter
	);

VOID
HALWOWL_FuncDisable(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bSimpleInit
	);

VOID
HALWOWL_ResetVar(
	IN	PADAPTER	Adapter
	);

#endif
