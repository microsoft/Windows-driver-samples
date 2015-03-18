#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include <initguid.h>
#include <devguid.h>

#ifdef SubclassWindow

#undef SubclassWindow

#endif
#pragma prefast(push)
#pragma prefast(disable:28196 28197, "supress warnings caused by atl headers")
#include <atlwin.h>

#ifdef _ATL_STATIC_REGISTRY

#include <statreg.h>

#endif


#pragma prefast(pop)


//EXTERN_C const IID IID_INetLanConnectionUiInfo = {0xC08956A6,0x1CD3,0x11D1,{0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

