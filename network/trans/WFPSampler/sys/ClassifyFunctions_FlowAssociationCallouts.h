////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ClassifyFunctions_FlowAssociationCallouts.h
//
//   Abstract:
//      This module contains prototypes of WFP Classify functions for associating context to flows.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLASSIFY_FLOW_ASSOCIATION_H
#define CLASSIFY_FLOW_ASSOCIATION_H

#if(NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFlowAssociation(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                   _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                   _Inout_opt_ VOID* pLayerData,
                                   _In_opt_ const VOID* pClassifyContext,
                                   _In_ const FWPS_FILTER* pFilter,
                                   _In_ UINT64 flowContext,
                                   _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#else

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI ClassifyFlowAssociation(_In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                   _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                   _Inout_opt_ VOID* pLayerData,
                                   _In_ const FWPS_FILTER* pFilter,
                                   _In_ UINT64 flowContext,
                                   _Inout_ FWPS_CLASSIFY_OUT* pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

#endif /// CLASSIFY_FLOW_ASSOCIATION_H
