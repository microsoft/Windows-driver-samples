/*++

Copyright (c) Microsoft Corporation. All Rights Reserved

Abstract:

    WPP Macro definitions.

Author:

    Travis Martin (TravM)

--*/

//
// Helpful macros
//

#ifndef WIDEN2
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#endif


//
// WPP definitions. Listed below is a set of WPP Trace macros. The comments
// between "//begin_wpp config" and "//end_wpp" are used by the WPP pre-processor
// to create the *.tmh files
//


#define PROXIMITY_COMMON_TRACE L"Microsoft\\Windows\\ProximityCommon"

#ifndef TRACE_CONTROL_GUID
#define TRACE_CONTROL_GUID (93bfc19b, a967, 4339, a3e6, 3a4cc30681d1)
#endif

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(PROXIMITY, TRACE_CONTROL_GUID, \
        WPP_DEFINE_BIT(EntryExit) \
        WPP_DEFINE_BIT(AllocFree) \
        WPP_DEFINE_BIT(Info) \
        WPP_DEFINE_BIT(Warning) \
        WPP_DEFINE_BIT(Error) \
        \
        WPP_DEFINE_BIT(NoisyEntryExit) \
        WPP_DEFINE_BIT(NoisyAllocFree) \
        WPP_DEFINE_BIT(NoisyInfo) \
        WPP_DEFINE_BIT(NoisyWarning))


//
// Used for trace messages indentation
//
const char __indentSpacer[] =
"                                                                                "
"                                                                                "
"                                                                                "
"                                                                                "
"                                                                                "
"                                                                                ";

#define INDENT_STR(indent) \
    (__indentSpacer + ((sizeof(__indentSpacer) >= (indent)*5) ? (sizeof(__indentSpacer)-2-(indent)*5) : 0))


//---------------------------------------------------------------------------
// Stores a pointer to current tracing context
//---------------------------------------------------------------------------
extern DWORD __g_tracingTlsSlot;

//---------------------------------------------------------------------------
// This macro declares the global variable that will store TLS index for the
// tracing context pointer.
//---------------------------------------------------------------------------
#define DECLARE_TRACING_TLS    DWORD __g_tracingTlsSlot = TLS_OUT_OF_INDEXES

//---------------------------------------------------------------------------
// To be used only within non-WDTF EXE init routines or by the Tracer.
//---------------------------------------------------------------------------
inline bool TracingTlsInitialize()
{
    if (__g_tracingTlsSlot == TLS_OUT_OF_INDEXES)
    {
        __g_tracingTlsSlot = TlsAlloc();
        if (__g_tracingTlsSlot == TLS_OUT_OF_INDEXES)
        {
            // Return error: cannot allocate TLS slot
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
// To be used only within non-WDTF EXE exit routines or by the Tracer.
//---------------------------------------------------------------------------
inline void TracingTlsFree()
{
    if (__g_tracingTlsSlot != TLS_OUT_OF_INDEXES)
    {
        TlsFree(__g_tracingTlsSlot);
        __g_tracingTlsSlot = TLS_OUT_OF_INDEXES;
    }
}


namespace TracingInternal
{

//---------------------------------------------------------------------------
// Used for storing current tracing context within a TLS slot
//---------------------------------------------------------------------------
struct TracingContext
{
    ULONG CallDepth;  // Current depth of internal calls
    DWORD Context;    // A context value (used to correlate scenarios that cross-threads)
};

//---------------------------------------------------------------------------
// Auto-incrementing and decrementing variable
//---------------------------------------------------------------------------
class AutoStackDepth
{
public:
    __forceinline AutoStackDepth(ULONG *pCallDepth)
        : _pCallDepth(pCallDepth)
    {
        WUDF_SAMPLE_DRIVER_ASSERT(_pCallDepth);
        if (_pCallDepth)
        {
            ++*_pCallDepth;
        }
    }

    __forceinline ~AutoStackDepth()
    {
        if (_pCallDepth)
        {
            --*_pCallDepth;
        }
    }

private:
    AutoStackDepth(AutoStackDepth& rh);
    const AutoStackDepth& operator =(AutoStackDepth& rh);

private:
    ULONG* _pCallDepth;
};

//---------------------------------------------------------------------------
// Sets new value to a variable but saves old value and restores it on
// destrutcion
//---------------------------------------------------------------------------
template <class T>
class AutoRestoredValue
{
public:
    __forceinline AutoRestoredValue(T* pVar, T newVal)
        : _pVar(pVar)
        , _oldVal()
    {
        WUDF_SAMPLE_DRIVER_ASSERT(_pVar);
        if (_pVar)
        {
            _oldVal = *_pVar;
            *_pVar = newVal;
        }
    }

    __forceinline ~AutoRestoredValue()
    {
        if (_pVar)
        {
            *_pVar = _oldVal;
        }
    }

private:
    AutoRestoredValue(AutoRestoredValue& rh);
    const AutoRestoredValue& operator =(AutoRestoredValue& rh);

private:
    
    T* _pVar;
    T  _oldVal;
};

//---------------------------------------------------------------------------
// Auto-pointer stored in TLS
//---------------------------------------------------------------------------
template <class Pointee>
class AutoTlsPtr
{
public:
    __forceinline AutoTlsPtr()
        : _dwSlotIndex(TLS_OUT_OF_INDEXES)
    {
    }

    __forceinline ~AutoTlsPtr()
    {
        if (_dwSlotIndex != TLS_OUT_OF_INDEXES)
        {
            LPVOID pCtx = TlsGetValue(_dwSlotIndex);
            if (pCtx)
            {
                TlsSetValue(_dwSlotIndex, NULL);
            }
        }
    }

    __forceinline Attach(Pointee* pCtx, DWORD dwSlotIndex)
    {
        _dwSlotIndex = dwSlotIndex;
        TlsSetValue(_dwSlotIndex, pCtx);
    }

private:
    
    DWORD _dwSlotIndex;
};

}


//---------------------------------------------------------------------------
// This macro should be used at entry point of all functions with tracing.
// It reads from a Tracing context structure stored in the TLS.
// If the slot contains a NULL a new TracingContext is used. An object 
// is created that increments CallDepth and auto-decrements it on function exit.
//---------------------------------------------------------------------------
#define USE_DEFAULT_TRACING_CONTEXT \
    TracingInternal::TracingContext* __pCtx = (TracingInternal::TracingContext*)TlsGetValue(__g_tracingTlsSlot); \
    TracingInternal::AutoTlsPtr<TracingInternal::TracingContext> __autoTlsPtr; \
    TracingInternal::TracingContext __ctx; \
    if (__pCtx == NULL) \
    { \
        __pCtx = &__ctx; \
        __pCtx->CallDepth = 0; \
        __autoTlsPtr.Attach(__pCtx, __g_tracingTlsSlot); \
    } \
    TracingInternal::AutoStackDepth __autoStackDepth(&__pCtx->CallDepth);


//MACRO: MethodEntry
//
//begin_wpp config
//USEPREFIX (MethodEntry, "%!STDPREFIX!%s-->this(%p):%!FUNC!(", INDENT_STR(__pCtx->CallDepth), this);
//FUNC MethodEntry{ENTRYLEVEL=EntryExit}(MSG, ...);
//USESUFFIX (MethodEntry, ")");
//end_wpp
#define WPP_ENTRYLEVEL_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_ENTRYLEVEL_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_ENTRYLEVEL_PRE(LEVEL) USE_DEFAULT_TRACING_CONTEXT;


//MACRO: MethodReturn
//
//begin_wpp config
//USEPREFIX (MethodReturn, "%!STDPREFIX!%s<--this(%p):%!FUNC!(): ", INDENT_STR(__pCtx->CallDepth), this);
//FUNC MethodReturn{RETURNLEVEL=EntryExit}(RET, MSG, ...);
//end_wpp
#define WPP_RETURNLEVEL_RET_ENABLED(LEVEL, Ret) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_RETURNLEVEL_RET_LOGGER(LEVEL, Ret) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RETURNLEVEL_RET_POST(LEVEL, Ret) ;return Ret;


//MACRO: MethodReturnHR
//
//begin_wpp config
//USEPREFIX (MethodReturnHR, "%!STDPREFIX!%s<--this(%p):%!FUNC!(): %!HRESULT!", INDENT_STR(__pCtx->CallDepth), this, __hr);
//FUNC MethodReturnHR{RETURNHRLEVEL=EntryExit}(HR);
//end_wpp
#define WPP_RETURNHRLEVEL_HR_ENABLED(LEVEL, hr) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_RETURNHRLEVEL_HR_LOGGER(LEVEL, hr) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RETURNHRLEVEL_HR_PRE(LEVEL, hr) { \
                                                HRESULT __hr = hr;
#define WPP_RETURNHRLEVEL_HR_POST(LEVEL, hr)    /*TraceMessage()*/; \
                                                return __hr; \
                                            }
//MACRO: MethodReturnVoid
//
//begin_wpp config
//USEPREFIX (MethodReturnVoid, "%!STDPREFIX!%s<--this(%p):%!FUNC!()", INDENT_STR(__pCtx->CallDepth), this);
//FUNC MethodReturnVoid{RETURNVOIDLEVEL=EntryExit}(...);
//end_wpp
#define WPP_RETURNVOIDLEVEL_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_RETURNVOIDLEVEL_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RETURNVOIDLEVEL_POST(LEVEL) ;return;


//MACRO: MethodReturnBool
//
//begin_wpp config
//USEPREFIX (MethodReturnBool, "%!STDPREFIX!%s<--this(%p):%!FUNC!(): %!bool!", INDENT_STR(__pCtx->CallDepth), this, __bRet);
//FUNC MethodReturnBool{RETURNBOOLLEVEL=EntryExit}(BOOLRETVAL);
//end_wpp
#define WPP_RETURNBOOLLEVEL_BOOLRETVAL_ENABLED(LEVEL, bRet) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_RETURNBOOLLEVEL_BOOLRETVAL_LOGGER(LEVEL, bRet) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RETURNBOOLLEVEL_BOOLRETVAL_PRE(LEVEL, bRet)    { \
                                                               bool __bRet = (bRet ? true : false);
#define WPP_RETURNBOOLLEVEL_BOOLRETVAL_POST(LEVEL, bRet)       /*TraceMessage()*/; \
                                                               return __bRet; \
                                                           }
//MACRO: MethodReturnPtr
//
//begin_wpp config
//USEPREFIX (MethodReturnPtr, "%!STDPREFIX!%s<--this(%p):%!FUNC!(): %p", INDENT_STR(__pCtx->CallDepth), this, __ptrRetVal);
//FUNC MethodReturnPtr{RETURNPTRLEVEL=EntryExit}(TYPE, PRET);
//end_wpp
#define WPP_RETURNPTRLEVEL_TYPE_PRET_ENABLED(LEVEL, Type, pRet) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_RETURNPTRLEVEL_TYPE_PRET_LOGGER(LEVEL, Type, pRet) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_RETURNPTRLEVEL_TYPE_PRET_PRE(LEVEL, Type, pRet)      { \
                                                                     Type __pRet = pRet;
#define WPP_RETURNPTRLEVEL_TYPE_PRET_POST(LEVEL, Type, pRet)    /*TraceMessage()*/; \
                                                                     return __pRet; \
                                                                 }
//MACRO: MethodReturnIfNull
//
//begin_wpp config
//USEPREFIX (MethodReturnIfNull, "%!STDPREFIX!%s<-this(%p):%!FUNC!(): E_POINTER %s=NULL, bailing out!", INDENT_STR(__pCtx->CallDepth), this, #PTR);
//FUNC MethodReturnIfNull{METHOD_POINTER_LEVEL=EntryExit}(PTR);
//end_wpp
#define WPP_METHOD_POINTER_LEVEL_PTR_ENABLED(LEVEL, PTR) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_METHOD_POINTER_LEVEL_PTR_LOGGER(LEVEL, PTR) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_METHOD_POINTER_LEVEL_PTR_PRE(LEVEL, PTR)  if ((PTR) == NULL) \
                                                      {
#define WPP_METHOD_POINTER_LEVEL_PTR_POST(LEVEL, PTR)     /*TraceMessage()*/; \
                                                          return E_POINTER; \
                                                      }
//MACRO: FunctionEntry
//
//begin_wpp config
//USEPREFIX (FunctionEntry, "%!STDPREFIX!%s-->%!FUNC!(", INDENT_STR(__pCtx->CallDepth));
//FUNC FunctionEntry{FUNCENTRYLEVEL=EntryExit}(MSG, ...);
//USESUFFIX (FunctionEntry, ")");
//end_wpp
#define WPP_FUNCENTRYLEVEL_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCENTRYLEVEL_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCENTRYLEVEL_PRE(LEVEL) USE_DEFAULT_TRACING_CONTEXT;


//MACRO: FunctionReturn
//
//begin_wpp config
//USEPREFIX (FunctionReturn, "%!STDPREFIX!%s<--%!FUNC!(): ", INDENT_STR(__pCtx->CallDepth));
//FUNC FunctionReturn{FUNCRETURNLEVEL=EntryExit}(RET, MSG, ...);
//end_wpp
#define WPP_FUNCRETURNLEVEL_RET_ENABLED(LEVEL, Ret) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCRETURNLEVEL_RET_LOGGER(LEVEL, Ret) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCRETURNLEVEL_RET_POST(LEVEL, Ret) ;return Ret;


//MACRO: FunctionReturnHR
//
//begin_wpp config
//USEPREFIX (FunctionReturnHR, "%!STDPREFIX!%s<--%!FUNC!(): %!HRESULT!", INDENT_STR(__pCtx->CallDepth), __hr);
//FUNC FunctionReturnHR{FUNCRETURNHRLEVEL=EntryExit}(HR);
//end_wpp
#define WPP_FUNCRETURNHRLEVEL_HR_ENABLED(LEVEL, hr) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCRETURNHRLEVEL_HR_LOGGER(LEVEL, hr) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCRETURNHRLEVEL_HR_PRE(LEVEL, hr) { \
                                                    HRESULT __hr = hr;
#define WPP_FUNCRETURNHRLEVEL_HR_POST(LEVEL, hr)    /*TraceMessage()*/; \
                                                    return __hr; \
                                                }
//MACRO: FunctionReturnVoid
//
//begin_wpp config
//USEPREFIX (FunctionReturnVoid, "%!STDPREFIX!%s<--%!FUNC!()", INDENT_STR(__pCtx->CallDepth));
//FUNC FunctionReturnVoid{FUNCRETURNLEVELVOID=EntryExit}(...);
//end_wpp
#define WPP_FUNCRETURNLEVELVOID_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCRETURNLEVELVOID_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCRETURNLEVELVOID_POST(LEVEL) ;return;


//MACRO: FunctionReturnBool
//
//begin_wpp config
//USEPREFIX (FunctionReturnBool, "%!STDPREFIX!%s<--%!FUNC!(): %!bool!", INDENT_STR(__pCtx->CallDepth), __bRet);
//FUNC FunctionReturnBool{FUNCRETURNLEVELBOOL=EntryExit}(BOOLRETVAL);
//end_wpp
#define WPP_FUNCRETURNLEVELBOOL_BOOLRETVAL_ENABLED(LEVEL, bRet) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCRETURNLEVELBOOL_BOOLRETVAL_LOGGER(LEVEL, bRet) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCRETURNLEVELBOOL_BOOLRETVAL_PRE(LEVEL, bRet)    { \
                                                                   bool __bRet = (bRet ? true : false);
#define WPP_FUNCRETURNLEVELBOOL_BOOLRETVAL_POST(LEVEL, bRet)       /*TraceMessage()*/; \
                                                                   return __bRet; \
                                                               }
//MACRO: FunctionReturnPtr
//
//begin_wpp config
//USEPREFIX (FunctionReturnPtr, "%!STDPREFIX!%s<--%!FUNC!(): %p", INDENT_STR(__pCtx->CallDepth), __pRet);
//FUNC FunctionReturnPtr{FUNCRETURNLEVELPTR=EntryExit}(TYPE, PRET);
//end_wpp
#define WPP_FUNCRETURNLEVELPTR_TYPE_PRET_ENABLED(LEVEL, Type, pRet) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_FUNCRETURNLEVELPTR_TYPE_PRET_LOGGER(LEVEL, Type, pRet) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_FUNCRETURNLEVELPTR_TYPE_PRET_PRE(LEVEL, Type, pRet)  { \
                                                                     Type __pRet = pRet;
#define WPP_FUNCRETURNLEVELPTR_TYPE_PRET_POST(LEVEL, Type, pRet)     /*TraceMessage()*/; \
                                                                     return __pRet; \
                                                                 }



// Define Non-empty debug break for checked builds only
#ifndef NDEBUG
    #define DEBUG_BREAK() __debugbreak()
#else
    #define DEBUG_BREAK() do {} while (false)
#endif


//MACRO: TraceASSERT
// 
//       ASSERT with tracing
//
//begin_wpp config
//USEPREFIX (TraceASSERT, "%!STDPREFIX!%sWARN: ASSERTION FAILED - expression \"%s\" is false.", INDENT_STR(__pCtx->CallDepth+1), #EXPR);
//FUNC TraceASSERT{ASSERTLEVEL=Warning}(EXPR);
//end_wpp
#define WPP_ASSERTLEVEL_EXPR_ENABLED(LEVEL, EXPR) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_ASSERTLEVEL_EXPR_LOGGER(LEVEL, EXPR) WPP_LEVEL_LOGGER(LEVEL)
#define WPP_ASSERTLEVEL_EXPR_PRE(LEVEL, EXPR)    if (!(EXPR)) \
                                                 {    
#define WPP_ASSERTLEVEL_EXPR_POST(LEVEL, EXPR)       /*TraceMessage()*/; \
                                                     WUDF_SAMPLE_DRIVER_ASSERT(FALSE); \
                                                 }

//MACRO: TraceErrorHR
// 
//       ERROR trace
//
//begin_wpp config
//USEPREFIX (TraceErrorHR, "%!STDPREFIX!%sERROR: ", INDENT_STR(__pCtx->CallDepth+1));
//FUNC TraceErrorHR{ERRORLEVEL=Error}(HR, MSG, ...);
//USESUFFIX (TraceErrorHR, ": %!HRESULT!", HR);
//end_wpp
#define WPP_ERRORLEVEL_HR_ENABLED(LEVEL, HR) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_ERRORLEVEL_HR_LOGGER(LEVEL, HR) WPP_LEVEL_LOGGER(LEVEL)

//MACRO: TraceInfo
//
//begin_wpp config
//USEPREFIX (TraceInfo, "%!STDPREFIX!%s", INDENT_STR(__pCtx->CallDepth+1));
//FUNC TraceInfo{INFOLEVEL=Info}(MSG, ...);
//end_wpp
#define WPP_INFOLEVEL_ENABLED(LEVEL) WPP_LEVEL_ENABLED(LEVEL)
#define WPP_INFOLEVEL_LOGGER(LEVEL) WPP_LEVEL_LOGGER(LEVEL)