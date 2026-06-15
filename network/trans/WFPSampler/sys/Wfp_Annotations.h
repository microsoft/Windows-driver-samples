#pragma once
/*****************************************************************************\
*                                                                             *
* DriverSpecs.h - markers for documenting the semantics of WFP callout driver *
*                APIs                                                         *
*                                                                             *
* Version 1.0.0                                                               *
*                                                                             *
* Copyright (c) Microsoft Corporation. All rights reserved.                   *
*                                                                             *
\*****************************************************************************/

/*****************************************************************************\
* NOTE																		  *
* NOTE																		  *
* NOTE																		  *
*   The macro bodies in this file are subject to change without notice.       *
*   Attempting to use the annotations in the macro bodies directly is not     *
*   supported.																  *
* NOTE																		  *
* NOTE																		  *
* NOTE																		  *
\*****************************************************************************/

/*****************************************************************************\
* The annotations described by wfp_annotations.h are used to annotate drivers.
*
* Wfp_annotations.h contains those annotations which are appropriate to userspace
* code, or which might appear in headers that are shared between user space
* and kernel space.
*
* Many annotations are context dependent.  They only apply to certain versions
* of Windows, or only to certain classes of driver.  These rules can be written
* using something like __drv_when(NTDDI_VERSION >= NTDDI_WINXP, ...)
* which causes the rule only to apply to Windows XP and later.  Many of these
* symbols are already defined in various Windows headers.
*
* To facilitate using this sort of conditional rule, we collect here the
* various known symbols that are (or reasonably might) be used in such
* a conditional annotation.  Some are speculative in that the symbol has
* not yet been defined because there are no known uses of it yet.
// @@BEGIN_DDKSPLIT
* It is expected that the first user of that symbol will add it to the
* appropriate system header AND update this comment to reflect that.
* Adding additional similar comments is encouraged.
// @@END_DDKSPLIT
*
* Where the symbol already exists its relevant header is
* noted below (excluding the "really well known" ones).
*
* Each symbol is listed with the currently known possible values.
*
* Some symbols are marked as #define symbols -- they are used with #ifdef
* operators only.  To use them in __drv_when, use something like
* __drv_when(__drv_defined(NT), ...).
*
* WDK Version (copied for convenience from sdkddkver.h)
*     NTDDI_VERSION: NTDDI_WIN2K NTDDI_WIN2KSP1 NTDDI_WIN2KSP2 NTDDI_WIN2KSP3
*                    NTDDI_WIN2KSP4 NTDDI_WINXP NTDDI_WINXPSP1 NTDDI_WINXPSP2
*					 NTDDI_WS03 NTDDI_WS03SP1 NTDDI_VISTA
*     The WDK version is taken as the WDM version as well.
*
* OS Version: (copied for convenience from sdkddkver.h)
*     _WIN32_WINNT: _WIN32_WINNT_NT4 _WIN32_WINNT_WIN2K _WIN32_WINNT_WINXP
*                   _WIN32_WINNT_WS03 _WIN32_WINNT_LONGHORN
*     WINVER: 0x030B 0x0400 0x0500 0x0600
*     NT (#define symbol)
* (sdkddkver.h also defines symbols for IE versions should they be needed.)
*
* Compiler Version:
*	  _MSC_VER: too many to list.
*	  _MSC_FULL_VER: too many to list.
*
* KMDF Version:  (Currently defined/used only in makefiles.)
*     KMDF_VERSION: 1
*
* UMDF Version:  (Currently defined/used only in makefiles.)
*     UMDF_VERSION: 1
*
* Architecture kinds:
*     __WIN64 (#define symbols)
*     _X86_
*     _AMD64_
*     _IA64_
*
* Machine Architectures:
*     _M_IX86
*     _M_AMD64
*     _M_IA64
*
* Driver Kind (NYI: "not yet implemented")
*   Typically these will be defined in the most-common header for a
*   particular driver (or in individual source files if appropriate).
*   These are not intended to necessarily be orthogonal: more than one might
*   apply to a particular driver.
*     _DRIVER_TYPE_BUS: 1                // NYI
*     _DRIVER_TYPE_FUNCTIONAL: 1         // NYI
*     _DRIVER_TYPE_MINIPORT: 1           // NYI
*     _DRIVER_TYPE_STORAGE: 1            // NYI
*     _DRIVER_TYPE_DISPLAY: 1            // NYI
*     _DRIVER_TYPE_FILESYSTEM: 1
*     _DRIVER_TYPE_FILESYSTEM_FILTER: 1
*
* NDIS driver version: (see ndis.h for much more detail.)
*   These can be used to both identify an NDIS driver and to check the version.
*     NDIS40 NDIS50 NDIS51 NDIS60 (#defined symbols)
*     NDIS_PROTOCOL_MAJOR_VERSION.NDIS_PROTOCOL_MINOR_VERSION: 4.0 5.0 5.1 6.0
*     And many others in ndis.h (including MINIPORT)
*
\*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


#define _Wfp_Annotation_ __declspec("_Wfp_Annotation_")
#define _Wfp_stream_injection_classify_ __declspec("_Wfp_stream_injection_classify_")
#define _Wfp_stream_inspection_classify_ __declspec("_Wfp_stream_inspection_classify_")
#define _Wfp_stream_inspection_notify_ __declspec("_Wfp_stream_inspection_notify_")
#define _Wfp_flow_inspection_notify_ __declspec("_Wfp_flow_inspection_notify_")
#define _Wfp_flow_inspection_classify_ __declspec("_Wfp_flow_inspection_classify_")
#define _Wfp_flow_injection_classify_ __declspec("_Wfp_flow_injection_classify_")
#define _Wfp_Transport_inspection_classify_ __declspec("_Wfp_Transport_inspection_classify_")
#define _Wfp_transport_injection_classify_ __declspec("_Wfp_transport_injection_classify_")
#define _Wfp_transport_injection_classify_inline_ __declspec("_Wfp_transport_injection_classify_inline_")
#define _Wfp_ale_inspection_notify_ __declspec("_Wfp_ale_inspection_notify_")
#define _Wfp_ale_inspection_classify_ __declspec("_Wfp_ale_inspection_classify_")
#define _Wfp_connect_redirect_inline_classify_ __declspec("_Wfp_connect_redirect_inline_classify_")
#define _Wfp_connect_redirect_classify_ __declspec("_Wfp_connect_redirect_classify_")
//
//
//
//#define __drv_declspec(x) __declspec(x)
//#define __$drv_group(annotes)												\
//	  __drv_declspec("SAL_begin") annotes __drv_declspec("SAL_end")
//#define __drv_nop(x) x
//
//    /****************************************************************************\
//            Callout Classify Annotions
//    /****************************************************************************/
//
//    //_Wfp_Annotation_
//    __ANNOTATION(SAL_wfpAnnotation(void);)
//#define _Wfp_Annotation_                                   \
//    _SAL2_Source_(_Wfp_Annotation_, (), _Post_ _SA_annotes0(SAL_wfpAnnotation))
//
//// Stream Injection
//    __ANNOTATION(SAL_wfpStreamInjection(void);)
//#define _Wfp_stream_injection_classify_                                   \
//    _SAL2_Source_(_Wfp_stream_injection_classify_, (), _Post_ _SA_annotes0(SAL_wfpStreamInjection))
//
//// Stream Inspection
//    __ANNOTATION(SAL_wfpStreamInspection(void);)
//#define _Wfp_stream_inspection_classify_                                 \
//    _SAL2_Source_(_Wfp_stream_inspection_classify_, (), _Post_ _SA_annotes0(SAL_wfpStreamInspection))
//
//__ANNOTATION(SAL_wfpStreamInspectionNotify(void);)
//#define _Wfp_stream_inspection_notify_                                 \
//    _SAL2_Source_(_Wfp_stream_inspection_notify_, (), _Post_ _SA_annotes0(SAL_wfpStreamInspectionNotify))
//
//// Flow Inspections
//__ANNOTATION(SAL_wfpFlowInspectionNotify(void);)
//#define _Wfp_flow_inspection_notify_                                   \
//    _SAL2_Source_(_Wfp_flow_inspection_notify_, (), _Post_ _SA_annotes0(SAL_wfpFlowInspectionNotify))
//
//__ANNOTATION(SAL_wfpFlowInspectionClassify(void);)
//#define _Wfp_flow_inspection_classify_                                   \
//    _SAL2_Source_(_Wfp_flow_inspection_classify_, (), _Post_ _SA_annotes0(SAL_wfpFlowInspectionClassify))
//
//__ANNOTATION(SAL_wfpFlowInjectionClassify(void);)
//#define _Wfp_flow_injection_classify_                                   \
//    _SAL2_Source_(_Wfp_flow_injection_classify_, (), _Post_ _SA_annotes0(SAL_wfpFlowInjectionClassify))
//
//// Transport Injection/Inspection
//
//__ANNOTATION(SAL_wfpTransportInspection(void);)
//#define _Wfp_Transport_inspection_classify_                               \
//    _SAL2_Source_(_Wfp_Transport_inspection_classify_, (), _Post_ _SA_annotes0(SAL_wfpTransportInspection))   \
//
//__ANNOTATION(SAL_wfpTransportInjection(void);)
//#define _Wfp_transport_injection_classify_                               \
//    _SAL2_Source_(_Wfp_transport_injection_classify_, (), _Post_ _SA_annotes0(SAL_wfpTransportInjection))   \
//
//__ANNOTATION(SAL_wfpTransportInjectionInline(void);)
//#define _Wfp_transport_injection_classify_inline_                               \
//    _SAL2_Source_(_Wfp_transport_injection_classify_inline_, (), _Post_ _SA_annotes0(SAL_wfpTransportInjectionInline))   \
//
//
//
//// ALE Inspection Notify and Classify
//__ANNOTATION(SAL_wfpAleInspectionNotify(void);)
//#define _Wfp_ale_inspection_notify_                               \
//    _SAL2_Source_(_Wfp_ale_inspection_notify_, (), _Post_ _SA_annotes0(SAL_wfpAleInspectionNotify))   \
//
//__ANNOTATION(SAL_wfpAleInspectionClassify(void);)
//#define _Wfp_ale_inspection_classify_                               \
//    _SAL2_Source_(_Wfp_ale_inspection_classify_, (), _Post_ _SA_annotes0(SAL_wfpAleInspectionClassify))   \
//
//// Connection Redirection Classify
//__ANNOTATION(SAL_wfpConnectRedirectInlineClassify(void);)
//#define _Wfp_connect_redirect_inline_classify_                               \
//    _SAL2_Source_(_Wfp_connect_redirect_inline_classify_, (), _Post_ _SA_annotes0(SAL_wfpConnectRedirectInlineClassify))   \
//
//__ANNOTATION(SAL_wfpConnectRedirectClassify(void);)
//#define _Wfp_connect_redirect_classify_                               \
//    _SAL2_Source_(_Wfp_connect_redirect_classify_, (), _Post_ _SA_annotes0(SAL_wfpConnectRedirectClassify))   \

#ifdef __cplusplus
}
#endif

