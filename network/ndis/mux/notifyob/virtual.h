//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2001.
//
//  File:       V I R T U A L . H
//
//  Contents:   Header file for virtual miniport class.
//
//  Notes:
//
//  Author:     Alok Sinha
//
//----------------------------------------------------------------------------

#ifndef VIRTUAL_H_INCLUDED

#define VIRTUAL_H_INCLUDE

#include <windows.h>
#include <stdio.h>
#include "netcfgn.h"

#include "common.h"

//
// Class to represent a virtual miniport created by IM driver.
//

class CMuxVirtualMiniport
{
    //
    // Private member variables.
    //

    INetCfg   *m_pnc;
    GUID      m_guidAdapter;
    GUID      m_guidMiniport;

    //
    // Public members.
    //

    public:

    CMuxVirtualMiniport(INetCfg *m_pnc,
                        GUID    *pguidMiniport,
                        GUID    *guidAdapter);

    virtual ~CMuxVirtualMiniport(VOID);
                                 
    HRESULT LoadConfiguration(VOID);

    VOID    GetAdapterGUID (GUID *);

    VOID    GetMiniportGUID (GUID *);

    HRESULT Install (VOID);

    HRESULT DeInstall (VOID);

    HRESULT ApplyRegistryChanges (ConfigAction eApplyAction);

    HRESULT ApplyPnpChanges (INetCfgPnpReconfigCallback *pfCallback,
                             ConfigAction eApplyAction);
};

#endif // VIRTUAL_H_INCLUDED