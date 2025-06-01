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
                                v4PrintDriverRenderFilter,                              \
                                (bb85bce3,b458,457a,ba34,631f619fe7e2),                  \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_ERROR)                 \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_WARNING)               \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_INFO)                  \
                                WPP_DEFINE_BIT(RENDERFILTER_TRACE_VERBOSE)               \
                                )


