//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       A D A P T E R . H
//
//  Contents:   Header file for physical adapter class.
//
//  Notes:
//
//  Author:     Alok Sinha 31-Oct-2000
//
//----------------------------------------------------------------------------

#ifndef ADAPTER_H_INCLUDED

#define ADAPTER_H_INCLUDED

#include <windows.h>
#include <netcfgn.h>

#include "common.h"
#include "virtual.h"
#include "list.h"

//
// Class to represent a physical adapter.
//

class CMuxPhysicalAdapter
{
    //
    // Private member variables.
    //

    GUID    m_guidAdapter;

    INetCfg *m_pnc;

    //
    // List of virtual miniports associated with the adapter.
    //

    List<CMuxVirtualMiniport *, GUID> m_MiniportList;

    //
    // List of virtual miniports to be added.
    //

    List<CMuxVirtualMiniport *, GUID> m_MiniportsToAdd;

    //
    // List of virtual miniports to be removed.
    //

    List<CMuxVirtualMiniport *, GUID> m_MiniportsToRemove;

    //
    // Private member functions.
    //

  public:

    //
    // Public member functions
    //

    CMuxPhysicalAdapter (INetCfg *pnc,
                         GUID *guidAdapter);

    virtual ~CMuxPhysicalAdapter (VOID);

    HRESULT LoadConfiguration (VOID);

    VOID    GetAdapterGUID (GUID *guidAdapter);

    HRESULT AddMiniport (CMuxVirtualMiniport *pNewMiniport);

    HRESULT RemoveMiniport (GUID *pguidMiniport);

    HRESULT Remove (VOID);

    HRESULT ApplyRegistryChanges (ConfigAction eApplyAction);

    HRESULT ApplyPnpChanges (INetCfgPnpReconfigCallback *pfCallback,
                             ConfigAction eApplyAction);

    HRESULT CancelChanges (VOID);

    DWORD MiniportCount (VOID) { return m_MiniportList.ListCount(); }

    DWORD MiniportAddCount (VOID) { return m_MiniportsToAdd.ListCount(); }

    BOOL  AllMiniportsRemoved (VOID);
};


#endif // ADAPTER_H_INCLUDED
