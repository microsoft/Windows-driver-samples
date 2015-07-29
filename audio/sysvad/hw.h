/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    hw.h

Abstract:

    Declaration of SYSVAD HW class. 
    SYSVAD HW has an array for storing mixer and volume settings
    for the topology.

--*/

#ifndef _SYSVAD_HW_H_
#define _SYSVAD_HW_H_

//=============================================================================
// Defines
//=============================================================================
// BUGBUG we should dynamically allocate this...
#define MAX_TOPOLOGY_NODES      20

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CSYSVADHW
// This class represents virtual SYSVAD HW. An array representing volume
// registers and mute registers.

class CSYSVADHW
{
public:
protected:
    BOOL                        m_MuteControls[MAX_TOPOLOGY_NODES];
    LONG                        m_VolumeControls[MAX_TOPOLOGY_NODES];
    LONG                        m_PeakMeterControls[MAX_TOPOLOGY_NODES];
    ULONG                       m_ulMux;            // Mux selection
    BOOL                        m_bDevSpecific;
    INT                         m_iDevSpecific;
    UINT                        m_uiDevSpecific;

private:

public:
    CSYSVADHW();
    
    void                        MixerReset();
    BOOL                        bGetDevSpecific();
    void                        bSetDevSpecific
    (
        _In_  BOOL                bDevSpecific
    );
    INT                         iGetDevSpecific();
    void                        iSetDevSpecific
    (
        _In_  INT                 iDevSpecific
    );
    UINT                        uiGetDevSpecific();
    void                        uiSetDevSpecific
    (
        _In_  UINT                uiDevSpecific
    );
    BOOL                        GetMixerMute
    (
        _In_  ULONG               ulNode,
        _In_  ULONG               ulChannel
    );
    void                        SetMixerMute
    (
        _In_  ULONG               ulNode,
        _In_  ULONG               ulChannel,
        _In_  BOOL                fMute
    );
    ULONG                       GetMixerMux();
    void                        SetMixerMux
    (
        _In_  ULONG               ulNode
    );
    LONG                        GetMixerVolume
    (   
        _In_  ULONG               ulNode,
        _In_  ULONG               ulChannel
    );
    void                        SetMixerVolume
    (   
        _In_  ULONG               ulNode,
        _In_  ULONG               ulChannel,
        _In_  LONG                lVolume
    );
    
    LONG                        GetMixerPeakMeter
    (   
        _In_  ULONG               ulNode,
        _In_  ULONG               ulChannel
    );

protected:
private:
};
typedef CSYSVADHW                *PCSYSVADHW;

#endif  // _SYSVAD_HW_H_

