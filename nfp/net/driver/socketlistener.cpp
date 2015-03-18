/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Abstract:

    Implements a socket listener class

Author:

    Travis Martin (TravM) 06-24-2010

Environment:

    User-mode only.
    
--*/
#include "internal.h"

#include "SocketListener.tmh"

HRESULT SetSocketIpv6Only(_In_ SOCKET socket, _In_ BOOL Ipv6Only)
{
    HRESULT hr = S_OK;
    if (setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&Ipv6Only, sizeof(Ipv6Only)) == SOCKET_ERROR)
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
        USE_DEFAULT_TRACING_CONTEXT;
        TraceErrorHR(hr, "setsockopt IPV6_V6ONLY");
    }
    return hr;
}

HRESULT CSocketListener::EnableAccepting(_In_ IValidateAccept* pValidator)
{
    HRESULT hr = S_OK;
    if (_pValidator == NULL)
    {
        USE_DEFAULT_TRACING_CONTEXT;
        
        _ThreadpoolIo = CreateThreadpoolIo((HANDLE)_ListenSocket, s_AcceptThreadProc, this, NULL);
        if (_ThreadpoolIo == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (SUCCEEDED(hr))
        {
            int backlog = 2;
            if (listen(_ListenSocket, backlog) == SOCKET_ERROR)
            {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
            }
        }
        
        if (SUCCEEDED(hr))
        {
            // We're now accepting
            _pValidator = pValidator;
            
            hr = BeginAccept();
            if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            {
                hr = S_OK;
            }
        }

        TraceInfo("EnableAccepting(): %!HRESULT!", hr);
    }
    
    return hr;
}

void CSocketListener::StopAccepting()
{
    MethodEntry("void");
    
    SOCKET listenSocket = InterlockedExchange(&_ListenSocket, INVALID_SOCKET);
    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
    }
    
    PTP_IO threadpoolIo = (PTP_IO)InterlockedExchangePointer((PVOID*)&_ThreadpoolIo, NULL);
    if (threadpoolIo != NULL)
    {
        // Don't wait for threadpool callbacks when this thread is actually the threadpool callback
        if (_ThreadpoolThreadId != GetCurrentThreadId())
        {
            WaitForThreadpoolIoCallbacks(threadpoolIo, false);
        }
        CloseThreadpoolIo(threadpoolIo);
    }
    
    _pValidator = NULL;
    
    if (_ClientSocket != INVALID_SOCKET)
    {
        closesocket(_ClientSocket);
        _ClientSocket = INVALID_SOCKET;
    }
    
    MethodReturnVoid();
}

HRESULT CSocketListener::BeginAccept()
{
    MethodEntry("void");

    HRESULT hr = S_OK;
    _ClientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (_ClientSocket == INVALID_SOCKET)
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    } 

    if (SUCCEEDED(hr))
    {
        hr = SetSocketIpv6Only(_ClientSocket, FALSE);
    }

    if (SUCCEEDED(hr))
    {
        PTP_IO threadpoolIo = _ThreadpoolIo;
        if (threadpoolIo != NULL)
        {
            StartThreadpoolIo(threadpoolIo);
            
            ULONG_PTR cbReceived = 0;
            ZeroMemory(&_Overlapped, sizeof(_Overlapped));
            if (!AcceptEx(_ListenSocket, _ClientSocket, &_AcceptBuffer, 
                          sizeof(_AcceptBuffer.MagicPacket), 
                          sizeof(_AcceptBuffer.DestAddress), 
                          sizeof(_AcceptBuffer.SourceAddress), 
                          (LPDWORD)&cbReceived, &_Overlapped))
            {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
                if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
                {
                    // Failed to accept, so cleanup
                    CancelThreadpoolIo(threadpoolIo);
                    StopAccepting();
                }
            }
        }
    }
    
    MethodReturnHR(hr);
}

void CSocketListener::AcceptThreadProc(_In_ HRESULT hr, _In_ ULONG_PTR cbReceived)
{
    MethodEntry("hr = %!HRESULT!, cbReceived = %d",
                 hr,              (ULONG)cbReceived);

    if (SUCCEEDED(hr))
    {
        if (cbReceived == sizeof(_AcceptBuffer.MagicPacket))
        {
            // Transfer ownership of _ClientSocket
            _pValidator->ValidateAccept(_ClientSocket, &_AcceptBuffer.MagicPacket);
        }
        else
        {
            // Wrong header size, close immediately
            closesocket(_ClientSocket);
        }
        _ClientSocket = INVALID_SOCKET;
    }
    
    // Start up another accept request
    BeginAccept();
    
    MethodReturnVoid();
}

HRESULT CSocketListener::Bind()
{
    MethodEntry("void");
    
    // Create a SOCKET for connecting to this server
    HRESULT hr = S_OK;
    addrinfoW* pResult = NULL;
    addrinfoW  Hints = {};
    Hints.ai_family = AF_INET6;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE;
    
    // Resolve the server address and port
    if (GetAddrInfoW(NULL, L"9299", &Hints, &pResult) != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    }

    if (SUCCEEDED(hr))
    {
        // Create a SOCKET for connecting to server
        _ListenSocket = socket(AF_INET6, SOCK_STREAM, 0);
        if (_ListenSocket == INVALID_SOCKET)
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }

        if (SUCCEEDED(hr))
        {
            hr = SetSocketIpv6Only(_ListenSocket, FALSE);
        }

        if (SUCCEEDED(hr))
        {
            // Setup the TCP listening socket
            if (bind(_ListenSocket, pResult->ai_addr, (int)pResult->ai_addrlen) == SOCKET_ERROR)
            {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
            }
        } 
        FreeAddrInfoW(pResult);
    }
    
    MethodReturnHR(hr);
}

