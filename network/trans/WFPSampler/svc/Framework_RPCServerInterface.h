////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_RPCServerInterface.h
//
//   Abstract:
//      This module contains prototypes for functions which implement the RPC server interface.
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

#ifndef FRAMEWORK_RPC_SERVER_INTERFACE_H
#define FRAMEWORK_RPC_SERVER_INTERFACE_H

_Success_(return == RPC_S_OK)
RPC_STATUS RPCServerInterfaceTerminate();

_Success_(return == RPC_S_OK)
RPC_STATUS RPCServerInterfaceInitialize();

#endif /// FRAMEWORK_RPC_SERVER_INTERFACE_H