/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    socketlistener.cpp

Abstract:

    Implements a socket listener class

Environment:

    User-mode only.
    
--*/

#include "Internal.h"
#include "SocketListener.tmh"

HRESULT SetSocketIpv6Only(_In_ SOCKET socket, _In_ BOOL Ipv6Only)  
{
    FunctionEntry("...");

    HRESULT hr = S_OK;

    if (setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&Ipv6Only, sizeof(Ipv6Only)) == SOCKET_ERROR) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    }

    FunctionReturnHR(hr);
}

HRESULT CSocketListener::EnableAccepting()
{
    MethodEntry("...");

    HRESULT hr = S_OK;

    if (_ThreadpoolIo == nullptr) {
        _ThreadpoolIo = CreateThreadpoolIo((HANDLE)_ListenSocket, s_AcceptThreadProc, this, nullptr);

        if (_ThreadpoolIo == nullptr) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (SUCCEEDED(hr)) {
            if (listen(_ListenSocket, 2) == SOCKET_ERROR) {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
            }
        }

        if (SUCCEEDED(hr)) {
            hr = BeginAccept();

            if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
                hr = S_OK;
            }
        }
    }

    MethodReturnHR(hr);
}

void CSocketListener::StopAccepting()
{
    MethodEntry("void");

    SOCKET listenSocket = InterlockedExchange(&_ListenSocket, INVALID_SOCKET);

    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }

    PTP_IO threadpoolIo = (PTP_IO)InterlockedExchangePointer((PVOID*)&_ThreadpoolIo, nullptr);

    if (threadpoolIo != nullptr) {
        // Don't wait for threadpool callbacks when this thread is actually the threadpool callback
        if (_ThreadpoolThreadId != GetCurrentThreadId())   {
            WaitForThreadpoolIoCallbacks(threadpoolIo, false);  
        }
        CloseThreadpoolIo(threadpoolIo);
    }

    if (_ClientSocket != INVALID_SOCKET) {
        closesocket(_ClientSocket);
        _ClientSocket = INVALID_SOCKET;
    }

    MethodReturnVoid();
}

HRESULT CSocketListener::BeginAccept()
{  
    MethodEntry("void");
  
    HRESULT hr = S_OK;
    ULONG_PTR cbReceived = 0;

    _ClientSocket = socket(AF_INET6, SOCK_STREAM, 0);

    if (_ClientSocket == INVALID_SOCKET) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    }

    if (SUCCEEDED(hr)) {
        hr = SetSocketIpv6Only(_ClientSocket, FALSE);
    }

    if (SUCCEEDED(hr)) {
        PTP_IO threadpoolIo = _ThreadpoolIo;

        if (threadpoolIo != nullptr) {  
            StartThreadpoolIo(threadpoolIo);
            ZeroMemory(&_Overlapped, sizeof(_Overlapped));

            if (!AcceptEx(
                    _ListenSocket,
                    _ClientSocket,
                    &_AcceptBuffer,
                    sizeof(_AcceptBuffer.MagicPacket),
                    sizeof(_AcceptBuffer.DestAddress),
                    sizeof(_AcceptBuffer.SourceAddress),
                    (LPDWORD)&cbReceived,
                    &_Overlapped)) {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());

                if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
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
    MethodEntry("hr = %!HRESULT!, cbReceived = %d", hr, (ULONG)cbReceived);

    if (SUCCEEDED(hr)) {
        if (cbReceived == sizeof(_AcceptBuffer.MagicPacket)) {
            _pValidator->ValidateAccept(_ClientSocket, &_AcceptBuffer.MagicPacket);
        }
        else {
            closesocket(_ClientSocket);
        }

        _ClientSocket = INVALID_SOCKET;
    }

    BeginAccept();

    MethodReturnVoid();
}
  
HRESULT CSocketListener::Bind()
{  
    MethodEntry("void");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr)) {
        _ListenSocket = socket(AF_INET6, SOCK_STREAM, 0);

        if (_ListenSocket == INVALID_SOCKET) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }

        if (SUCCEEDED(hr)) {  
            hr = SetSocketIpv6Only(_ListenSocket, FALSE);
        }

        if (SUCCEEDED(hr)) {
            if (bind(_ListenSocket, (const sockaddr*)&_ListenAddress, (int)sizeof(_ListenAddress)) == SOCKET_ERROR) {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
            }
        }
    }

    MethodReturnHR(hr);
}