

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for effects.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __effects_h__
#define __effects_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWMPEffects_FWD_DEFINED__
#define __IWMPEffects_FWD_DEFINED__
typedef interface IWMPEffects IWMPEffects;
#endif 	/* __IWMPEffects_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void * __RPC_USER MIDL_user_allocate(size_t size);
void __RPC_USER MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void * ); 

/* interface __MIDL_itf_effects_0000 */
/* [local] */ 

#define	EFFECT_CANGOFULLSCREEN	( 0x1 )

#define	EFFECT_HASPROPERTYPAGE	( 0x2 )

#define	EFFECT_VARIABLEFREQSTEP	( 0x4 )

#define	SA_BUFFER_SIZE	( 1024 )


enum PlayerState
    {	stop_state	= 0,
	pause_state	= 1,
	play_state	= 2
    } ;

//**********************************************************************
// Define the minimum and maximum frequency ranges returned in our
// TimedLevel frequency array (i.e. first index in TimedLevel.frequency
// is at 20Hz and last is at 22050Hz).
//**********************************************************************
const float kfltTimedLevelMaximumFrequency = 22050.0F;
const float kfltTimedLevelMinimumFrequency = 20.0F;

/*
 * FREQUENCY_INDEX() returns the index into TimedLevel.frequency[] where 
 * the specified frequency is located in the power spectrum
 */
#define FREQUENCY_INDEX(FREQ)\
  (int)(((FREQ) - kfltTimedLevelMinimumFrequency) /\
    (((kfltTimedLevelMaximumFrequency - kfltTimedLevelMinimumFrequency) / SA_BUFFER_SIZE)))

typedef struct tagTimedLevel
    {
    unsigned char frequency[ 2 ][ 1024 ];
    unsigned char waveform[ 2 ][ 1024 ];
    int state;
    hyper timeStamp;
    } 	TimedLevel;



extern RPC_IF_HANDLE __MIDL_itf_effects_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_effects_0000_v0_0_s_ifspec;

#ifndef __IWMPEffects_INTERFACE_DEFINED__
#define __IWMPEffects_INTERFACE_DEFINED__

/* interface IWMPEffects */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPEffects;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D3984C13-C3CB-48e2-8BE5-5168340B4F35")
    IWMPEffects : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Render( 
            /* [in] */ TimedLevel *pLevels,
            /* [in] */ HDC hdc,
            /* [in] */ RECT *prc) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE MediaInfo( 
            /* [in] */ LONG lChannelCount,
            /* [in] */ LONG lSampleRate,
            /* [in] */ BSTR bstrTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ DWORD *pdwCapabilities) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [out] */ BSTR *bstrTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPresetTitle( 
            /* [in] */ LONG nPreset,
            /* [out] */ BSTR *bstrPresetTitle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPresetCount( 
            /* [out] */ LONG *pnPresetCount) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCurrentPreset( 
            /* [in] */ LONG nPreset) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCurrentPreset( 
            /* [out] */ LONG *pnPreset) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisplayPropertyPage( 
            /* [in] */ HWND hwndOwner) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GoFullscreen( 
            /* [in] */ BOOL fFullScreen) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RenderFullScreen( 
            /* [in] */ TimedLevel *pLevels) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPEffectsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMPEffects * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMPEffects * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMPEffects * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Render )( 
            IWMPEffects * This,
            /* [in] */ TimedLevel *pLevels,
            /* [in] */ HDC hdc,
            /* [in] */ RECT *prc);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *MediaInfo )( 
            IWMPEffects * This,
            /* [in] */ LONG lChannelCount,
            /* [in] */ LONG lSampleRate,
            /* [in] */ BSTR bstrTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetCapabilities )( 
            IWMPEffects * This,
            /* [out] */ DWORD *pdwCapabilities);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTitle )( 
            IWMPEffects * This,
            /* [out] */ BSTR *bstrTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPresetTitle )( 
            IWMPEffects * This,
            /* [in] */ LONG nPreset,
            /* [out] */ BSTR *bstrPresetTitle);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPresetCount )( 
            IWMPEffects * This,
            /* [out] */ LONG *pnPresetCount);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetCurrentPreset )( 
            IWMPEffects * This,
            /* [in] */ LONG nPreset);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetCurrentPreset )( 
            IWMPEffects * This,
            /* [out] */ LONG *pnPreset);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DisplayPropertyPage )( 
            IWMPEffects * This,
            /* [in] */ HWND hwndOwner);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GoFullscreen )( 
            IWMPEffects * This,
            /* [in] */ BOOL fFullScreen);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RenderFullScreen )( 
            IWMPEffects * This,
            /* [in] */ TimedLevel *pLevels);
        
        END_INTERFACE
    } IWMPEffectsVtbl;

    interface IWMPEffects
    {
        CONST_VTBL struct IWMPEffectsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPEffects_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPEffects_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPEffects_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPEffects_Render(This,pLevels,hdc,prc)	\
    (This)->lpVtbl -> Render(This,pLevels,hdc,prc)

#define IWMPEffects_MediaInfo(This,lChannelCount,lSampleRate,bstrTitle)	\
    (This)->lpVtbl -> MediaInfo(This,lChannelCount,lSampleRate,bstrTitle)

#define IWMPEffects_GetCapabilities(This,pdwCapabilities)	\
    (This)->lpVtbl -> GetCapabilities(This,pdwCapabilities)

#define IWMPEffects_GetTitle(This,bstrTitle)	\
    (This)->lpVtbl -> GetTitle(This,bstrTitle)

#define IWMPEffects_GetPresetTitle(This,nPreset,bstrPresetTitle)	\
    (This)->lpVtbl -> GetPresetTitle(This,nPreset,bstrPresetTitle)

#define IWMPEffects_GetPresetCount(This,pnPresetCount)	\
    (This)->lpVtbl -> GetPresetCount(This,pnPresetCount)

#define IWMPEffects_SetCurrentPreset(This,nPreset)	\
    (This)->lpVtbl -> SetCurrentPreset(This,nPreset)

#define IWMPEffects_GetCurrentPreset(This,pnPreset)	\
    (This)->lpVtbl -> GetCurrentPreset(This,pnPreset)

#define IWMPEffects_DisplayPropertyPage(This,hwndOwner)	\
    (This)->lpVtbl -> DisplayPropertyPage(This,hwndOwner)

#define IWMPEffects_GoFullscreen(This,fFullScreen)	\
    (This)->lpVtbl -> GoFullscreen(This,fFullScreen)

#define IWMPEffects_RenderFullScreen(This,pLevels)	\
    (This)->lpVtbl -> RenderFullScreen(This,pLevels)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_Render_Proxy( 
    IWMPEffects * This,
    /* [in] */ TimedLevel *pLevels,
    /* [in] */ HDC hdc,
    /* [in] */ RECT *prc);


void __RPC_STUB IWMPEffects_Render_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_MediaInfo_Proxy( 
    IWMPEffects * This,
    /* [in] */ LONG lChannelCount,
    /* [in] */ LONG lSampleRate,
    /* [in] */ BSTR bstrTitle);


void __RPC_STUB IWMPEffects_MediaInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GetCapabilities_Proxy( 
    IWMPEffects * This,
    /* [out] */ DWORD *pdwCapabilities);


void __RPC_STUB IWMPEffects_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GetTitle_Proxy( 
    IWMPEffects * This,
    /* [out] */ BSTR *bstrTitle);


void __RPC_STUB IWMPEffects_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GetPresetTitle_Proxy( 
    IWMPEffects * This,
    /* [in] */ LONG nPreset,
    /* [out] */ BSTR *bstrPresetTitle);


void __RPC_STUB IWMPEffects_GetPresetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GetPresetCount_Proxy( 
    IWMPEffects * This,
    /* [out] */ LONG *pnPresetCount);


void __RPC_STUB IWMPEffects_GetPresetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_SetCurrentPreset_Proxy( 
    IWMPEffects * This,
    /* [in] */ LONG nPreset);


void __RPC_STUB IWMPEffects_SetCurrentPreset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GetCurrentPreset_Proxy( 
    IWMPEffects * This,
    /* [out] */ LONG *pnPreset);


void __RPC_STUB IWMPEffects_GetCurrentPreset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_DisplayPropertyPage_Proxy( 
    IWMPEffects * This,
    /* [in] */ HWND hwndOwner);


void __RPC_STUB IWMPEffects_DisplayPropertyPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_GoFullscreen_Proxy( 
    IWMPEffects * This,
    /* [in] */ BOOL fFullScreen);


void __RPC_STUB IWMPEffects_GoFullscreen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IWMPEffects_RenderFullScreen_Proxy( 
    IWMPEffects * This,
    /* [in] */ TimedLevel *pLevels);


void __RPC_STUB IWMPEffects_RenderFullScreen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPEffects_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     __RPC__in unsigned long *, __RPC__in BSTR * ); 

unsigned long             __RPC_USER  HDC_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in HDC * ); 
unsigned char * __RPC_USER  HDC_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in HDC * ); 
unsigned char * __RPC_USER  HDC_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out HDC * ); 
void                      __RPC_USER  HDC_UserFree(     __RPC__in unsigned long *, __RPC__in HDC * ); 

unsigned long             __RPC_USER  HWND_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out HWND * ); 
void                      __RPC_USER  HWND_UserFree(     __RPC__in unsigned long *, __RPC__in HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


