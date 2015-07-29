/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

--*/

#include "Internal.h"
#include "Connection.tmh"

/* 9C7D2C68-5AD8-4A14-BE20-F8741D60D100 */
const GUID MAGIC_PACKET_P2P = 
  {0x9C7D2C68, 0x5AD8, 0x4A14, {0xBE, 0x20, 0xF8, 0x74, 0x1D, 0x60, 0xD1, 0x00}};

/* 9C7D2C68-5AD8-4A14-BE20-F8741D60D101 */
const GUID MAGIC_PACKET_TAG = 
  {0x9C7D2C68, 0x5AD8, 0x4A14, {0xBE, 0x20, 0xF8, 0x74, 0x1D, 0x60, 0xD1, 0x01}};

/* 9C7D2C68-5AD8-4A14-BE20-F8741D60D102 */
const GUID MAGIC_PACKET_HCE =
  {0x9C7D2C68, 0x5AD8, 0x4A14, {0xBE, 0x20, 0xF8, 0x74, 0x1D, 0x60, 0xD1, 0x02}};

HRESULT SetSocketIpv6Only(_In_ SOCKET socket, _In_ BOOL Ipv6Only);

HRESULT SynchronousReadSocket(_In_ SOCKET Socket, _In_reads_bytes_(cbBuffer) PVOID pBuffer, _In_ DWORD cbBuffer)
{
    HRESULT hr = S_OK;
    DWORD cbBytesRead = 0;
    OVERLAPPED Overlapped = {};

    if (!ReadFile((HANDLE)Socket, pBuffer, cbBuffer, &cbBytesRead, &Overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            if (!GetOverlappedResult((HANDLE)Socket, &Overlapped, &cbBytesRead, TRUE)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

BOOL CConnection::Create(_In_ IConnectionCallback* pCallback, _Outptr_result_maybenull_ CConnection** ppConnection)
{
    NT_ASSERT(ppConnection != nullptr);

    *ppConnection = new CConnection(pCallback);

    if (*ppConnection != nullptr) {
        return TRUE;
    }

    return FALSE;
}

void CConnection::Terminate()
{
    MethodEntry("void");

    STATE PriorState = (STATE)(InterlockedExchange((long*)&_State, (long)TERMINATED));

    if (PriorState != TERMINATED) {
        shutdown(_Socket, SD_SEND);
        
        if (_ThreadpoolThreadId != GetCurrentThreadId()) {
            WaitForThreadpoolWorkCallbacks(_ThreadpoolWork, false);
        }
        
        SOCKET Socket = (SOCKET)InterlockedExchangePointer((PVOID*)&_Socket, (PVOID)INVALID_SOCKET);

        if (Socket != INVALID_SOCKET) {
            closesocket(Socket);
        }
    }

    MethodReturnVoid();
}

HRESULT CConnection::InitializeAsClient(_In_ BEGIN_PROXIMITY_ARGS* pArgs)
{
    MethodEntry("pArgs->szName = '%S'", pArgs->szName);

    HRESULT hr = S_OK;

    pArgs->szName[MAX_PATH-1] = L'\0';

    _Socket = socket(AF_INET6, SOCK_STREAM, 0);

    if (_Socket == INVALID_SOCKET) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
    }
    
    if (SUCCEEDED(hr)) {
        hr = SetSocketIpv6Only(_Socket, FALSE);
    }
    
    if (SUCCEEDED(hr)) {
        SOCKADDR_STORAGE LocalAddress = {};
        SOCKADDR_STORAGE RemoteAddress = {};
        DWORD cbLocalAddress = sizeof(LocalAddress);
        DWORD cbRemoteAddress = sizeof(RemoteAddress);
        timeval Timeout = {8, 0};

        if (!WSAConnectByName(
                _Socket,
                pArgs->szName,
                L"9299", 
                &cbLocalAddress,
                (SOCKADDR*)&LocalAddress, 
                &cbRemoteAddress,
                (SOCKADDR*)&RemoteAddress,
                &Timeout,
                nullptr)) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
            TraceErrorHR(hr, L"WSAConnectByName returned failure");
        }
    }

    if (SUCCEEDED(hr)) {
        if (setsockopt(_Socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) == SOCKET_ERROR) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
    }

    if (SUCCEEDED(hr)) {
        if (send(_Socket, (char*)&MAGIC_PACKET_P2P, sizeof(MAGIC_PACKET_P2P), 0) == SOCKET_ERROR) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
        
        if (SUCCEEDED(hr)) {
            GUID MagicPacket = {};

            hr = SynchronousReadSocket(_Socket, &MagicPacket, sizeof(MagicPacket));

            if (SUCCEEDED(hr)) {
                if (memcmp(&MagicPacket, &MAGIC_PACKET_P2P, sizeof(MAGIC_PACKET_P2P)) != 0) {
                    hr = E_FAIL;
                }
            }
        }
        
        if (SUCCEEDED(hr)) {
            hr = FinalizeEstablish(INVALID_SOCKET);
        }
    }
    
    if (FAILED(hr)) {
        if (_Socket != INVALID_SOCKET) {
            closesocket(_Socket);
            _Socket = INVALID_SOCKET;
        }
    }

    MethodReturnHR(hr);
}

void CConnection::ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket)
{
    MethodEntry("...");
    
    TraceASSERT(Socket != INVALID_SOCKET);
    
    HRESULT hr = S_OK;

    if (memcmp(pMagicPacket, &MAGIC_PACKET_P2P, sizeof(MAGIC_PACKET_P2P)) == 0) {
        _ConnectionType = CONNECTION_TYPE_P2P;
    }
    else if (memcmp(pMagicPacket, &MAGIC_PACKET_TAG, sizeof(MAGIC_PACKET_TAG)) == 0) {
        _ConnectionType = CONNECTION_TYPE_TAG;
    }
    else if (memcmp(pMagicPacket, &MAGIC_PACKET_HCE, sizeof(MAGIC_PACKET_HCE)) == 0) {
        _ConnectionType = CONNECTION_TYPE_HCE;
    }
    else {
        hr = E_FAIL;
    }
    
    if (SUCCEEDED(hr)) {
        if (send(Socket, (char*)pMagicPacket, sizeof(GUID), 0) == SOCKET_ERROR) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
        }
    }
    
    if (SUCCEEDED(hr)) {
        hr = FinalizeEstablish(Socket);
    }
    
    if (FAILED(hr)) {
        closesocket(Socket);
    }
    
    MethodReturnVoid();
}

HRESULT CConnection::FinalizeEstablish(_In_ SOCKET Socket)
{
    MethodEntry("...");

    HRESULT hr = S_OK;

    STATE PriorState = (STATE)(InterlockedCompareExchange((long*)&_State, (long)ESTABLISHED, (long)INITIAL));

    if (PriorState != INITIAL) {
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    
    if (SUCCEEDED(hr)) {
        _ThreadpoolWork = CreateThreadpoolWork(s_ReceiveThreadProc, this, nullptr);

        if (_ThreadpoolWork == nullptr) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else {
            if (Socket != INVALID_SOCKET) {
                _Socket = Socket;
            }

            SubmitThreadpoolWork(_ThreadpoolWork);
            _pCallback->ConnectionEstablished(this);
        }
    }
    
    MethodReturnHR(hr);
}

BOOL CConnection::ReceiveThreadProc()
{
    MethodEntry("void");

    MESSAGE* pMessage = new MESSAGE();

    if (pMessage == nullptr) {
        Terminate();
    }
    else
    {
        while (_Socket != INVALID_SOCKET) {
            if (recv(_Socket, (char*)pMessage, sizeof(*pMessage), MSG_WAITALL) == sizeof(*pMessage)) {
                _pCallback->HandleReceivedMessage(_ConnectionType, pMessage);
            }
            else {
                Terminate();
                break;
            }
        }
        delete pMessage;
    }

    BOOL fConnectionDeleted = _pCallback->ConnectionTerminated(this);

    MethodReturnBool(fConnectionDeleted);
}

HRESULT CConnection::TransmitMessage(_In_ MESSAGE* pMessage)
{
    MethodEntry("...");

    HRESULT hr = S_OK;

    if (_Socket == INVALID_SOCKET) {
        hr = HRESULT_FROM_WIN32(WSAENOTSOCK);
    }
    else {
        if (send(_Socket, (char*)pMessage, sizeof(*pMessage), 0) == SOCKET_ERROR) {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
            Terminate();
        }
    }
    
    MethodReturnHR(hr);
}
