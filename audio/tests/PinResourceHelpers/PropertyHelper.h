// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// Module Name:
//
//  PropertyHelpers.h
//
// Abstract:
//
//  Header for common helpers/defines of property
//
// -------------------------------------------------------------------------------
#pragma once

#include "HalfApp.h"

#define kSystemDefaultPeriod    100000 // 10 milliseconds
#define kSystemMinPeriodForNonRTCapableEndpoints  20000 // 2 milliseconds



























// ----------------------------------------------------------------------
HRESULT GetConnectorId
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    bool*                   hasConnector,
    UINT*                   pConnectorId
);

// ----------------------------------------------------------------------
HRESULT GetEndpointFriendlyName
(
    IMMDevice*              pDevice,
    LPWSTR*                 ppwszEndpointName
);

// ----------------------------------------------------------------------
HRESULT GetAudioFilterAsDevice
(
    IMMDevice* pDevice,
    IMMDevice** ppAudioFilterAsDevice
);

// ----------------------------------------------------------------------
HRESULT GetCachedProcessingModes
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    ULONG                   *pCount,
    AUDIO_SIGNALPROCESSINGMODE **ppModes
);












// ----------------------------------------------------------------------
HRESULT GetProcessingModes
(
    IMMDevice*              pDevice,
    UINT                    pinId,
    ULONG                   *pCount,
    AUDIO_SIGNALPROCESSINGMODE **ppModes
);

// ----------------------------------------------------------------------
HRESULT GetCachedSupportedFormatRecords
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE mode,
    ULONG *pCount,
    FORMAT_RECORD **ppFormatRecords
);













// ----------------------------------------------------------------------
HRESULT GetSupportedFormatRecords
(
    IMMDevice*                  pDevice,
    UINT                        pinId,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    STACKWISE_DATAFLOW          dataFlow,
    ULONG                       *pCount,
    FORMAT_RECORD               **ppFormatRecords
);

// ----------------------------------------------------------------------
HRESULT IsFormatSupported
(
    IMMDevice*              pDevice,
    UINT                    pinId,
    WAVEFORMATEX            *pWfx,
    BOOL*                   pbSupported
);

// ----------------------------------------------------------------------
HRESULT DiscoverPeriodicityCharacteristicsForFormat
(
    IMMDevice*                  pDevice,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                *pWfx,
    STACKWISE_DATAFLOW          dataFlow,
    UINT32                      *pDefaultPeriodicityInFrames,
    UINT32                      *pFundamentalPeriodicityInFrames,
    UINT32                      *pMinPeriodicityInFrames,
    UINT32                      *pMaxPeriodicityInFrames,
    UINT32                      *pMaxPeriodicityInFramesExtended
);

// ----------------------------------------------------------------------
HRESULT CheckConnectorSupportForPeriodicity
(
    IMMDevice*                  pDevice,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                *pWfx,
    STACKWISE_DATAFLOW          dataFlow,
    HNSTIME                     RequestedPeriodicity,
    UINT32                      *pActualPeriodicityInFrames
);

// ----------------------------------------------------------------------
HRESULT GetCachedDefaultFormat
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    WAVEFORMATEX**          ppDefaultFormat
);

// ----------------------------------------------------------------------
HRESULT GetProposedFormatForProcessingMode
(
    IMMDevice*                  pDevice,
    UINT                        pinId,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX                **ppProposedFormat
);

// ----------------------------------------------------------------------
HRESULT GetAvailiablePinInstanceCount
(
    IMMDevice*          pDevice,
    UINT                pinId,
    UINT32*             pAvailablePinInstanceCount
);

// -------------------------------------------------------------------
HRESULT GetDriverPathViaService
(
    LPWSTR              ServiceName,
    LPWSTR              DriverFullPath,
    UINT                cchFullPath
);

// -------------------------------------------------------------------
HRESULT GetFullPathFromImagePath
(
    LPWSTR              ImagePath,
    LPWSTR              DriverFullPath,
    UINT                cchFullPath
);

// -------------------------------------------------------------------
HRESULT CheckImports
(
    LPWSTR              DriverPath,
    LPSTR               ModuleNameToCheck,
    LPSTR               MethodNameToCheck,
    bool*               pIsImported
);

// ----------------------------------------------------------------------
HRESULT IsPortCls
(
    IMMDevice*          pDevice,
    bool*               pIsPortCls
);

// ----------------------------------------------------------------------
HRESULT IsAVStream
(
    IMMDevice*          pDevice,
    bool*               pIsAVStream
);

// ----------------------------------------------------------------------
HRESULT IsBluetooth
(
    IMMDevice* pDevice,
    bool* pIsBluetooth
);

// ----------------------------------------------------------------------
HRESULT IsSideband
(
    IMMDevice* pDevice,
    bool* pIsSideband
);

// ----------------------------------------------------------------------
HRESULT IsMVA
(
    EndpointConnectorType eConnectorType,
    IMMDevice* pDevice,
    bool* pIsMVA
);

// ----------------------------------------------------------------------
HRESULT GetPnpDevnodeFromMMDevice
(
    IMMDevice* pDevice,
    IMMDevice** pDevnodeDevice
);
