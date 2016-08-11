
#ifndef _STREAM_TRACE_H
#define _STREAM_TRACE_H

// WPP Software Tracing Definitions 


/* 524F4849-5420-5241-494E-41205A9EEF01 */

// You can have up to 32 defines. If you want more than that,
// you have to provide another trace control GUID

// Don't use 0x suffix in hex numbers in the guid

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid, \
    (524F4849, 5420, 5241, 494E, 41205A9EEF01),\
    WPP_DEFINE_BIT(CO_ENTER_EXIT)       /* bit  0 = 0x00000001 */ \
    WPP_DEFINE_BIT(CO_GENERAL)          /* bit  1 = 0x00000002 */ \
    WPP_DEFINE_BIT(CO_REFCOUNT)         /* bit  2 = 0x00000004 */ \
    )

// For DoTraceLevelMessage
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_FLAG_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)
#define WPP_LEVEL_FLAGS_LOGGER(lvl, flags) WPP_LEVEL_LOGGER(flags)


#pragma warning(disable:4204) // C4204 nonstandard extension used : non-constant aggregate initializer

//
// Define the 'xstr' structure for logging buffer and length pairs
// and the 'log_xstr' function which returns it to create one in-place.
// this enables logging of complex data types.
//
typedef struct xstr { char * _buf; short  _len; } xstr_t;
__inline xstr_t log_xstr(void * p, short l) { xstr_t xs = { (char*)p, l }; return xs; }

#pragma warning(default:4204)

//
// Define the macro required for a hexdump use as:
//
// DebugTraceEx((LEVEL, FLAG,"%!HEXDUMP!\n", log_xstr(buffersize,(char *)buffer) ));
//
//
#define WPP_LOGHEXDUMP(x) WPP_LOGPAIR(2, &((x)._len)) WPP_LOGPAIR((x)._len, (x)._buf)

// begin_wpp config
// CUSTOM_TYPE(InlineState, ItemListLong(Idle, Skipping, Modifying, Scanning));
// CUSTOM_TYPE(OobState,    ItemListLong(Idle, Processing, Busy, Error));
// CUSTOM_TYPE(FWP_DIRECTION, ItemListLong(Outbound, Inbound, Outbound+Inbound));
//
// CUSTOM_TYPE(FWPS_NOTIFY, ItemEnum(FWPS_CALLOUT_NOTIFY_TYPE));
// CUSTOM_TYPE(SACTION, ItemEnum(FWPS_STREAM_ACTION_TYPE));
//
// DEFINE_CPLX_TYPE(HEXDUMP, WPP_LOGHEXDUMP, xstr_t, ItemHEXDump, "s", _HEX_, 0, 2);
//
// end_wpp


#endif // _STREAM_TRACE_H
