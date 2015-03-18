//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       N O T I F Y . H
//
//  Contents:   Header file for sample notify object.
//
//  Notes:
//
//  Author:     Alok Sinha
//
//----------------------------------------------------------------------------

#ifndef NOTIFY_H_INCLUDE

#define NOTIFY_H_INCLUDE

#include <windows.h>

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include <devguid.h>
#include <setupapi.h>

#include <notifyn.h>
#include "list.h"
#include "adapter.h"
#include "resource.h"
#include "common.h"


//
// CMuxNotify Object - Base class for the entire notify object
//


class CMuxNotify :

               //
               // Must inherit from CComObjectRoot(Ex) for reference count
               // management and default threading model.
               //
 
               public CComObjectRoot,

               //
               // Define the default class factory and aggregation model.
               //

               public CComCoClass<CMuxNotify, &CLSID_CMuxNotify>,

               //
               // Notify Object's interfaces.
               //

               public INetCfgComponentControl,
               public INetCfgComponentSetup,
               public INetCfgComponentPropertyUi,
               public INetCfgComponentNotifyBinding,
               public INetCfgComponentNotifyGlobal
{

   //
   // Public members.
   //

   public:

      //
      // Constructor
      //

      CMuxNotify(VOID);

      //
      // Destructors.
      //

      ~CMuxNotify(VOID);

      //
      // Notify Object's interfaces.
      //

      BEGIN_COM_MAP(CMuxNotify)
         COM_INTERFACE_ENTRY(INetCfgComponentControl)
         COM_INTERFACE_ENTRY(INetCfgComponentSetup)
         COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
         COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
         COM_INTERFACE_ENTRY(INetCfgComponentNotifyGlobal)
      END_COM_MAP()

      //
      // Uncomment the the line below if you don't want your object to
      // support aggregation. The default is to support it
      //
      // DECLARE_NOT_AGGREGATABLE(CMuxNotify)
      //

      DECLARE_REGISTRY_RESOURCEID(IDR_REG_SAMPLE_NOTIFY)

      //
      // INetCfgComponentControl
      //

      STDMETHOD (Initialize) (
                   IN INetCfgComponent  *pIComp,
                   IN INetCfg           *pINetCfg,
                   IN BOOL              fInstalling);

      STDMETHOD (CancelChanges) ();

      STDMETHOD (ApplyRegistryChanges) ();

      STDMETHOD (ApplyPnpChanges) (
                   IN INetCfgPnpReconfigCallback* pICallback);

      //
      // INetCfgComponentSetup
      //

      STDMETHOD (Install) (
                   IN DWORD dwSetupFlags);

      STDMETHOD (Upgrade) (
                   IN DWORD dwSetupFlags,
                   IN DWORD dwUpgradeFromBuildNo);

      STDMETHOD (ReadAnswerFile) (
                   IN PCWSTR szAnswerFile,
                   IN PCWSTR szAnswerSections);

      STDMETHOD (Removing) ();

      //
      // INetCfgComponentPropertyUi
      //

        STDMETHOD (QueryPropertyUi) (
                   IN IUnknown* pUnk);

        STDMETHOD (SetContext) (
                   IN IUnknown* pUnk);

        STDMETHOD (MergePropPages) (
                   IN OUT DWORD* pdwDefPages,
                   OUT LPBYTE* pahpspPrivate,
                   OUT UINT* pcPrivate,
                   IN HWND hwndParent,
                   OUT PCWSTR* pszStartPage);

        STDMETHOD (ValidateProperties) (
                   HWND hwndSheet);

        STDMETHOD (CancelProperties) ();

        STDMETHOD (ApplyProperties) ();

      //
      // INetCfgNotifyBinding
      //

      STDMETHOD (QueryBindingPath) (
                   IN DWORD dwChangeFlag,
                   IN INetCfgBindingPath* pncbp);

      STDMETHOD (NotifyBindingPath) (
                   IN DWORD dwChangeFlag,
                   IN INetCfgBindingPath* pncbp);

      //
      // INetCfgNotifyGlobal
      //

      STDMETHOD (GetSupportedNotifications) (
                   OUT DWORD* pdwNotificationFlag );

      STDMETHOD (SysQueryBindingPath) (
                   IN DWORD dwChangeFlag,
                   IN INetCfgBindingPath* pncbp);

      STDMETHOD (SysNotifyBindingPath) (
                   IN DWORD dwChangeFlag,
                   IN INetCfgBindingPath* pncbp);
            
      STDMETHOD (SysNotifyComponent) (
                   IN DWORD dwChangeFlag,
                   IN INetCfgComponent* pncc);

  //
  // Private members.
  //

  private:

     //
     // Private member variables.
     //

     INetCfgComponent  *m_pncc;  // Protocol's Net Config component
     INetCfg           *m_pnc;
     ConfigAction      m_eApplyAction;
     IUnknown*         m_pUnkContext;

     //
     // List of physical adapters currently installed.
     //

     List<CMuxPhysicalAdapter *, GUID> m_AdaptersList;

     //
     // List of physical adapters to be removed.
     //

     List<CMuxPhysicalAdapter *, GUID> m_AdaptersToRemove;

     //
     // List of physical adapters to be added.
     //

     List<CMuxPhysicalAdapter *, GUID> m_AdaptersToAdd;

     //
     // Private member functions.
     //

     HRESULT HrLoadAdapterConfiguration (VOID);

     HRESULT HrGetUpperAndLower (INetCfgBindingPath* pncbp,
                                 INetCfgComponent **ppnccUpper,
                                 INetCfgComponent **ppnccLower);

     HRESULT HrAddAdapter (INetCfgComponent *pnccAdapter);

     HRESULT HrRemoveAdapter (INetCfgComponent *pnccAdapter);

     HRESULT HrAddMiniport (CMuxPhysicalAdapter *pAdapter,
                            GUID *guidAdapter);

     HRESULT HrRemoveMiniport (CMuxPhysicalAdapter *pAdapter,
                            GUID *guidAdapter);

#ifdef DISABLE_PROTOCOLS_TO_PHYSICAL

    VOID EnableBindings (INetCfgComponent *pnccAdapter,
                         BOOL bEnable);

    BOOL IfExistMux (INetCfgBindingPath *pncbp);

    HRESULT HrGetBindingPathEnum (INetCfgComponent *pnccAdapter,
                                  DWORD dwBindingType,
                                  IEnumNetCfgBindingPath **ppencbp);

    HRESULT HrGetBindingPath (IEnumNetCfgBindingPath *pencbp,
                              INetCfgBindingPath **ppncbp);

    HRESULT HrGetBindingInterfaceEnum (INetCfgBindingPath *pncbp,
                                       IEnumNetCfgBindingInterface **ppencbi);

    HRESULT HrGetBindingInterface (IEnumNetCfgBindingInterface *pencbi,
                                   INetCfgBindingInterface **ppncbi);
#endif

  public:

     LRESULT OnInitDialog (IN HWND hWnd);
     LRESULT OnOk (IN HWND hWnd);
     LRESULT OnCancel (IN HWND hWnd);
};


INT_PTR CALLBACK NotifyDialogProc (HWND hWnd,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam);
UINT CALLBACK NotifyPropSheetPageProc (HWND hWnd,
                                       UINT uMsg,
                                       LPPROPSHEETPAGE ppsp);
#endif // NOTIFY_H_INCLUDE
