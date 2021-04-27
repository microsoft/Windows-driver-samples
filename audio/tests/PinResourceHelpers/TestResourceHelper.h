// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// Module Name:
//
//  TestResourceHelpers.h
//
// Abstract:
//
//  Header for BuildResourceList helpers.
//
// -------------------------------------------------------------------------------
#pragma once

#include <HalfApp.h>

// ----------------------------------------------------------------------
HRESULT GetProcessingModesForConnector
(
    IMMDevice*              pDevice,
    UINT                    uConnectorId,
    EndpointConnectorType   eConnectorType,
    ULONG*                  pCount,
    AUDIO_SIGNALPROCESSINGMODE** ppModes
);

// ----------------------------------------------------------------------
HRESULT GetDefaultFormatForConnector
(
    IMMDevice*              pDevice,
    EndpointConnectorType   eConnectorType,
    WAVEFORMATEX**          ppDefaultFormat
);

// ----------------------------------------------------------------------
HRESULT GetPreferredFormatForConnector
(
    IMMDevice*                  pDevice,
    UINT                        uConnectorId,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    WAVEFORMATEX**              ppPreferredFormat
);

// ----------------------------------------------------------------------
HRESULT GetPreferredFormatPeriodicityCharacteristicsForConnector
(
    IMMDevice*                  pDevice,
    EndpointConnectorType       eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE  mode,
    STACKWISE_DATAFLOW          dataFlow,
    WAVEFORMATEX*               pPreferredFormat,
    ULONG                       cFormatRecords,
    PFORMAT_RECORD              pFormatRecords,
    UINT32*                     pDefaultPeriodicityInFrames,
    UINT32*                     pFundamentalPeriodicityInFrames,
    UINT32*                     pMinPeriodicityInFrames,
    UINT32*                     pMaxPeriodicityInFrames,
    UINT32*                     pMaxPeriodicityInFramesExtended
);

// ----------------------------------------------------------------------
HRESULT GetSupportedFormatRecordsForConnector
(
    IMMDevice*              pDevice,
    UINT                    uConnectorId,
    EndpointConnectorType   eConnectorType,
    AUDIO_SIGNALPROCESSINGMODE mode,
    STACKWISE_DATAFLOW      dataFlow,
    ULONG*                  pCount,
    FORMAT_RECORD**         ppFormatRecords
);
