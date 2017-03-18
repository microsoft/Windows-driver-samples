/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    internal.h

Abstract:

    This module contains the internal type definitations and
    helper function declarations for the SpbTestTool app.

Environment:

    user-mode

Revision History:

--*/

#include <string>
#include <map>
#include <list>
#include <map>
#include <functional>

#include <iomanip>

#include <cstdlib>

#include <stdexcept>

#include <math.h>

#include <wchar.h>
#include <windows.h>
#include <winioctl.h>
#include <specstrings.h>

#include "spbtestioctl.h"

using namespace std;

//
// Global Variables
//

extern HANDLE g_Peripheral;

typedef pair<ULONG, PBYTE> BUFPAIR;
typedef list<BUFPAIR>      BUFLIST;

VOID
PrintBytes(
    _In_                  ULONG BufferCb,
    _In_reads_bytes_(BufferCb) BYTE  Buffer[]
    );

_Success_(return)
bool
PopStringParameter(
    _Inout_   list<string> *Parameters,
    _Out_     string       *Value,
    _Out_opt_ bool         *Present = nullptr
    );

typedef pair<ULONG,ULONG> bounds;

_Success_(return)
bool
PopNumberParameter(
    _Inout_   list<string>      *Parameters,
    _In_      ULONG              Radix,
    _Out_     ULONG             *Value,
    _In_opt_  pair<ULONG,ULONG>  Bounds = bounds(0,0),
    _Out_opt_ bool              *Present = nullptr
    );

_Success_(return)
bool
ParseNumber(
    _In_     const string &String,
    _In_     ULONG         Radix,
    _Out_    ULONG        *Value,
    _In_opt_ bounds        Bounds = bounds(0,0)
    );

_Success_(return)
bool
PopBufferParameter(
    _Inout_ list<string>       *Parameters,
    _Out_   pair<ULONG, PBYTE> *Value
    );


#define countof(x) (sizeof(x) / sizeof(x[0]))
    
#include "command.h"
