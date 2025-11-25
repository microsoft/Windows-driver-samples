//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once
#ifndef TRACE_H
#define TRACE_H

#define WPP_USE_TRACE_LEVELS

// 21BA7B61-05F8-41F1-9048-C09493DCFE38
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WdiLibraryCtlGuid, (21BA7B61, 05F8, 41F1, 9048, C09493DCFE38), WPP_DEFINE_BIT(DUMMY))

//#define WPP_LEVEL_EXP_ENABLED(LEVEL, EXP) WPP_LEVEL_ENABLED(LEVEL)
//#define WPP_LEVEL_EXP_LOGGER(LEVEL, EXP) WPP_LEVEL_LOGGER(LEVEL)


#define WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags) WPP_CONTROL(WPP_BIT_##flags).AutoLogContext, lvl, WPP_BIT_##flags
#define WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags) \
    (WPP_LEVEL_ENABLED(lvl) || lvl < TRACE_LEVEL_VERBOSE || WPP_CONTROL(WPP_BIT_##flags).AutoLogVerboseEnabled)

#define WPP_RECORDER_LEVEL_ARGS(LEVEL) WPP_RECORDER_LEVEL_FLAGS_ARGS(LEVEL, DUMMY)
#define WPP_RECORDER_LEVEL_FILTER(LEVEL) WPP_RECORDER_LEVEL_FLAGS_FILTER(LEVEL, DUMMY)

#define WPP_RECORDER_LEVEL_EXP_FILTER(LEVEL, EXP) WPP_RECORDER_LEVEL_FILTER(LEVEL)
#define WPP_RECORDER_LEVEL_EXP_ARGS(LEVEL, EXP) WPP_RECORDER_LEVEL_ARGS(LEVEL)

// Suppress warnings about constants in logical expressions because the
// level is often a constant
#define WPP_LEVEL_PRE(LEVEL) __pragma(warning(suppress : 25039 25040))
#define WPP_LEVEL_EXP_PRE(LEVEL, EXP) __pragma(warning(suppress : 25039 25040))

#define TraceEntry(...)
#define TraceExit(Status)
#define WFCTrace(Format, ...)
#define WFCError(Format, ...)
#define WFCInfo(Format, ...)

// begin_wpp config
// USEPREFIX (TraceEntry, "%!STDPREFIX!");
// FUNC TraceEntry{LEVEL=TRACE_LEVEL_VERBOSE}(...);
// USESUFFIX (TraceEntry, "--> %!FUNC!");
// end_wpp

// begin_wpp config
// USEPREFIX (TraceExit, "%!STDPREFIX!");
// FUNC TraceExit{LEVEL=TRACE_LEVEL_VERBOSE}(EXP);
// USESUFFIX (TraceExit, "<-- %!FUNC!: 0x%x", EXP);
// end_wpp

//
// Flat-C trace commands
//
// begin_wpp config
//
// USEPREFIX (WFCError, "%!STDPREFIX! %!FUNC!: [ERROR]");
// FUNC WFCError{LEVEL=TRACE_LEVEL_ERROR}(MSG, ...);
//
// USEPREFIX (WFCInfo, "%!STDPREFIX! %!FUNC!: [INFO]");
// FUNC WFCInfo{LEVEL=TRACE_LEVEL_INFORMATION}(MSG, ...);
//
// USEPREFIX (WFCTrace, "%!STDPREFIX! %!FUNC!: [TRACE]");
// FUNC WFCTrace{LEVEL=TRACE_LEVEL_VERBOSE}(MSG, ...);
//
// end_wpp
//

#define MACRO_START \
    do \
    {
#define MACRO_END \
    } \
    while (0)

//
// WPP Macros: WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG
//
// begin_wpp config
// FUNC WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG{COMPNAME=DUMMY,LEVEL=TRACE_LEVEL_ERROR}(NTEXPR,MSG,...);
// USEPREFIX (WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG, "%!STDPREFIX! !! WifiIhv - %!FUNC!: ");
// USESUFFIX (WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG, " [status=%!STATUS!]", nt__wpp);
// end_wpp

#define WPP_COMPNAME_LEVEL_NTEXPR_PRE(comp, level, ntexpr) \
    MACRO_START NTSTATUS nt__wpp = (ntexpr); \
    if (!NT_SUCCESS(nt__wpp)) \
    {
#define WPP_COMPNAME_LEVEL_NTEXPR_POST(comp, level, ntexpr) \
    ; \
    return nt__wpp; \
    } \
    MACRO_END
#define WPP_RECORDER_COMPNAME_LEVEL_NTEXPR_FILTER(comp, level, ntexpr) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, comp)
#define WPP_RECORDER_COMPNAME_LEVEL_NTEXPR_ARGS(comp, level, ntexpr) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, comp)

//
// WPP Macros: WX_RETURN_INSUFFICIENT_RESOURCES_IF_NULL_MSG
//
// begin_wpp config
// FUNC WX_RETURN_INSUFFICIENT_RESOURCES_IF_NULL_MSG{COMPNAME=DUMMY,LEVEL=TRACE_LEVEL_ERROR}(PTR2,MSG,...);
// USEPREFIX (WX_RETURN_INSUFFICIENT_RESOURCES_IF_NULL_MSG, "%!STDPREFIX! !! WifiIhv - %!FUNC!: ");
// USESUFFIX (WX_RETURN_INSUFFICIENT_RESOURCES_IF_NULL_MSG, "%!s! is null", #PTR2);
// end_wpp

#define WPP_COMPNAME_LEVEL_PTR2_PRE(comp, level, ptr) \
    MACRO_START if ((ptr == nullptr)) \
    {
#define WPP_COMPNAME_LEVEL_PTR2_POST(comp, level, ptr) \
    ; \
    return STATUS_INSUFFICIENT_RESOURCES; \
    } \
    MACRO_END
#define WPP_RECORDER_COMPNAME_LEVEL_PTR2_FILTER(comp, level, ptr) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, comp)
#define WPP_RECORDER_COMPNAME_LEVEL_PTR2_ARGS(comp, level, ptr) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, comp)

//
// WPP Macros: WX_RETURN_IF_NULL_MSG
//
// begin_wpp config
// FUNC WX_RETURN_IF_NULL_MSG{COMPNAME=DUMMY,LEVEL=TRACE_LEVEL_ERROR}(PTR3,MSG,...);
// USEPREFIX (WX_RETURN_IF_NULL_MSG, "%!STDPREFIX! !! WifiIhv - %!FUNC!: ");
// USESUFFIX (WX_RETURN_IF_NULL_MSG, "%!s! is null", #PTR3);
// end_wpp

#define WPP_COMPNAME_LEVEL_PTR3_PRE(comp, level, ptr) \
    MACRO_START if ((ptr == nullptr)) \
    {
#define WPP_COMPNAME_LEVEL_PTR3_POST(comp, level, ptr) \
    ; \
    return; \
    } \
    MACRO_END
#define WPP_RECORDER_COMPNAME_LEVEL_PTR3_FILTER(comp, level, ptr) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, comp)
#define WPP_RECORDER_COMPNAME_LEVEL_PTR3_ARGS(comp, level, ptr) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, comp)

typedef struct _ByteArray
{
    USHORT usLength;
    const void* pvBuffer;
} ByteArray;

__inline ByteArray log_lenstr(ULONG len, const void* buf)
{
    ByteArray xs{};
    xs.usLength = (USHORT)len;
    xs.pvBuffer = buf;
    return xs;
}

#define WPP_LOGHEXDUMP(x) \
    WPP_LOGPAIR(2, &((x).usLength)) \
    WPP_LOGPAIR((x).usLength, (x).pvBuffer)

#define WPP_LOGANSISTRING(x) \
    WPP_LOGPAIR(2, &((x).usLength)) \
    WPP_LOGPAIR((x).usLength, (x).pvBuffer)

#define WPP_LOGMACADDR(x) WPP_LOGPAIR(6, x)

#define WPP_LOGDOT11SSID(x) \
    WPP_LOGPAIR(2, &((*(x)).uSSIDLength)) \
    WPP_LOGPAIR((*(x)).uSSIDLength, ((const char*)(*(x)).ucSSID))

//
// Custom types
//
// begin_wpp config
//
// DEFINE_CPLX_TYPE(HEXDUMP, WPP_LOGHEXDUMP, ByteArray, ItemHEXDump,"s", _HEX_, 0,2);
// WPP_FLAGS(-DLOG_HEXDUMP(len,str)=log_lenstr(len,str));
//
// DEFINE_CPLX_TYPE(ANSISTRING, WPP_LOGANSISTRING, ByteArray, ItemPString,"s", _SSID_, 0,2);
// WPP_FLAGS(-DLOG_ANSISTRING(len,str)=log_lenstr(len,str));
//
// DEFINE_CPLX_TYPE(DOT11SSID, WPP_LOGDOT11SSID, DOT11_SSID*, ItemPString,"s", _SSID_, 0,2);
//
// DEFINE_CPLX_TYPE(MACADDR, WPP_LOGMACADDR, DOT11_MAC_ADDRESS, ItemMACAddr,"s", _MAC_, 0);
//
// CUSTOM_TYPE(MESSAGE_ID, ItemEnum(WDI_TLV::ENUMS::MESSAGE_ID));
// end_wpp
//

#endif
