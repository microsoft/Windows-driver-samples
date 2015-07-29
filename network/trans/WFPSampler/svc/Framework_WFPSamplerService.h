////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WFPSamplerService.h
//
//   Abstract:
//      This module contains include headers for central area of exportation
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_WFP_SAMPLER_SERVICE_H
#define FRAMEWORK_WFP_SAMPLER_SERVICE_H

#include <Windows.h>                            /// Include\UM
#include <WInternl.h>                           /// Include\UM
#include <StdLib.h>                             /// Inc\CRT
#include <FWPSU.h>                              /// Include\UM
#include <FWPMU.h>                              /// Include\UM
#include <NTDDNDIS.h>                           /// Include\Shared
#include <WinSock2.h>                           /// Include\UM
#include <WS2TCPIP.h>                           /// Include\UM
#include <MSTCPIP.h>                            /// Include\Shared
#include <StrSafe.h>                            /// Include\Shared
#include <IntSafe.h>                            /// Include\Shared
#include <netfw.h>
#include <conio.h>
#include <psapi.h>

#include "WFPSamplerRPC.h"                      /// $(OBJ_PATH)\..\idl\$(O)
#include "Framework_WFPSamplerService_Msg.h"    /// $(OBJ_PATH)\$(O)
#include "Identifiers.h"                        /// ..\inc
#include "WFPArrays.h"                          /// ..\inc
#include "ScenarioData.h"                       /// ..\inc
#include "HelperFunctions_Include.h"            /// ..\lib

extern BOOLEAN g_isCOMInitialized;

#endif /// FRAMEWORK_WFP_SAMPLER_SERVICE_H