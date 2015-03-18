/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Abstract:

    Declares a socket listener class

Author:

    Travis Martin (TravM) 06-24-2010

Environment:

    User-mode only.
    
--*/
#pragma once

struct ACCEPT_BUFFER
{
    GUID MagicPacket;
    
    SOCKADDR_STORAGE DestAddress;
    SOCKADDR_STORAGE SourceAddress;
};

interface IValidateAccept
{
    virtual void ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket) = 0;
};

class CSocketListener
{
public:
    CSocketListener() :
        _pValidator(NULL),
        _ThreadpoolIo(NULL),
        _ListenSocket(INVALID_SOCKET),
        _ClientSocket(INVALID_SOCKET)
    {
        ZeroMemory(&_Overlapped, sizeof(_Overlapped));
    }

    ~CSocketListener()
    {
        StopAccepting();
    }

public:
    HRESULT Bind();
    HRESULT EnableAccepting(_In_ IValidateAccept* pValidator);
    void StopAccepting();

private:

    HRESULT BeginAccept();
    void AcceptThreadProc(_In_ HRESULT hr, _In_ ULONG_PTR cbReceived);
    static void CALLBACK s_AcceptThreadProc(
        _Inout_      PTP_CALLBACK_INSTANCE /*Instance*/,
        _Inout_      PVOID                 Context,
        _Inout_opt_  PVOID                 /*Overlapped*/,
        _In_         ULONG                 IoResult,
        _In_         ULONG_PTR             NumberOfBytesTransferred,
        _Inout_      PTP_IO                /*Io*/)
    {
        CSocketListener* pSocketListener = (CSocketListener*)Context;
        pSocketListener->_ThreadpoolThreadId = GetCurrentThreadId();
        pSocketListener->AcceptThreadProc(HRESULT_FROM_WIN32(IoResult), NumberOfBytesTransferred);
        pSocketListener->_ThreadpoolThreadId = 0;
    }

private:
    ACCEPT_BUFFER _AcceptBuffer;
    
    OVERLAPPED        _Overlapped;
    volatile PTP_IO   _ThreadpoolIo;
    DWORD             _ThreadpoolThreadId;
    
    IValidateAccept* _pValidator;
    
    SOCKET _ListenSocket;
    SOCKET _ClientSocket; 
};
