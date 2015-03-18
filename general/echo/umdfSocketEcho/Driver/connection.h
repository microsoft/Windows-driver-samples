/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Connection.h

Abstract:

   Header file for the socketecho connection class 
   
Environment:

    User mode only


--*/
#pragma once

class CConnection 
{
public:
    CConnection();
    HRESULT Connect(IN  IWDFDevice *pDevice);
    VOID Close();
    HANDLE GetSocketHandle( );

private:
    SOCKET m_socket;
};

