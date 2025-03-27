//
// File Name:
//
//    WppTrace.h
//
// Abstract:
//
//    WPP tracing definitions.
//

#pragma once



#define WPP_CONTROL_GUIDS WPP_DEFINE_CONTROL_GUID(                                       \
                                XpsRenderFilter,                              \
                                (d091e354,5abc,4f62,b52c,c6fecd5ab6cd),                  \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_ERROR)                 \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_WARNING)               \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_INFO)                  \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_VERBOSE)               \
                                )


