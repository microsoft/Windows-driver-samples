////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmTransaction.h
//
//   Abstract:
//      This module contains prototypes for functions which assist in actions pertaining to BFE 
//      transactions.
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

#ifndef HELPERFUNCTIONS_FWPM_TRANSACTION_H
#define HELPERFUNCTIONS_FWPM_TRANSACTION_H

_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionAbort(_In_ HANDLE engineHandle);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionBegin(_In_ HANDLE engineHandle,
                                _In_ UINT32 flags = 0);

_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionCommit(_In_ HANDLE engineHandle);

#endif /// HELPERFUNCTIONS_FWPM_TRANSACTION_H