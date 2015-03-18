//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This source code is intended only as a supplement to Microsoft
//  Development Tools and/or on-line documentation.  See these other
//  materials for detailed information regarding Microsoft code samples.
//
//  THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
//  Abstract:
//     WDK print filter sample.
//  The filters need to derive from DllLockManager. Thus the filter constructor and
//  destructor call the DllLockManager constructor and destructor, respectively.
//
//----------------------------------------------------------------------------

#ifndef _FILTER_MAIN_HXX_
#define _FILTER_MAIN_HXX_

class DllLockManager
{
public:

    DllLockManager()
    {
        InterlockedIncrement(&m_cGlobalRef);
    }

    ~DllLockManager()
    {
        InterlockedDecrement(&m_cGlobalRef);
    }

    static
    LONG
    GetGlobalRef()
    {
        return InterlockedCompareExchange(&m_cGlobalRef, 0, 0);
    }

private:

    static LONG m_cGlobalRef;
};

#endif


