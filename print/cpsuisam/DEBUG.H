/*++

Copyright (c) 1990-2003 Microsoft Corporation
All Rights Reserved


Module Name:

    debug.h


Abstract:

    This module contains all debugger definitions


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


--*/


#if DBG

VOID
cdecl
CPSUIDbgPrint
(
    LPCSTR   pszFormat,
    ...
);

VOID
CPSUIDbgType
(
    INT    Type
);

extern BOOL DoCPSUIWarn;

#define DBGP(x)                 (CPSUIDbgPrint x)

#define DEFINE_DBGVAR(x)        DWORD DBG_CPSUIFILENAME=(x)

#define CPSUIDBG(x,y)           if((x)&DBG_CPSUIFILENAME){CPSUIDbgType(0);DBGP(y);}

#define CPSUIDBGBLK(x)          x;
#define CPSUIWARN(x)            if(DoCPSUIWarn) { CPSUIDbgType(1);DBGP(x); }
#define CPSUIERR(x)             CPSUIDbgType(-1);DBGP(x)
#define CPSUIRIP(x)             CPSUIERR(x); DebugBreak()
#define CPSUIASSERT(b,x,e,i)     \
            if (!(e)) { _CPSUIAssert(x,#e,__FILE__,(UINT)__LINE__,(DWORD)i,b); }

#else   // DBG

#define CPSUIDBGBLK(x)
#define DEFINE_DBGVAR(x)
#define CPSUIDBG(x,y)
#define CPSUIWARN(x)
#define CPSUIERR(x)
#define CPSUIRIP(x)
#define CPSUIASSERT(b,x,e,i)

#endif  // DBG
