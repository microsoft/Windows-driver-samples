#pragma once

#define WPP_CHECK_FOR_NULL_STRING  //to prevent exceptions due to NULL strings

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(HidUsbFx2TraceGuid, (091182d7, 5167, 4e77, aa80, ca6c1e6201e9), \
        WPP_DEFINE_BIT(DBG_INIT) \
        WPP_DEFINE_BIT(DBG_PNP) \
        WPP_DEFINE_BIT(DBG_IOCTL) \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)
