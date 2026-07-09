// Copyright (C) Microsoft Corporation. All rights reserved.
//
// OEM sample: enumerates supported device services, then sends "Hello, My Driver"
// to the WiFiCx sample driver via WlanDeviceServiceCommand and prints the
// driver's "Nice to meet you, My OEM".

#define NOMINMAX // use std::min/std::max instead of the windows.h min/max macros
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h> // StringFromGUID2
#include <algorithm> // std::min
#include <cstdio>

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib") // StringFromGUID2

// WlanGetSupportedDeviceServices, WlanDeviceServiceCommand and
// WLAN_DEVICE_SERVICE_GUID_LIST are all declared by wlanapi.h.

// This GUID/opcode pair MUST match the driver (drivercode\SharedTypes.h).
// {2d6f9a14-3a1d-4f0a-9b7e-1c2e3a4b5c6d}
static const GUID GUID_OEM_SAMPLE_DEVICE_SERVICE =
{ 0x2d6f9a14, 0x3a1d, 0x4f0a, { 0x9b, 0x7e, 0x1c, 0x2e, 0x3a, 0x4b, 0x5c, 0x6d } };

#define OEM_DEVICE_SERVICE_OPCODE_HELLO   0x00000001
#define OEM_DEVICE_SERVICE_REQUEST_STRING "Hello, My Driver"

static void PrintGuid(const GUID& g)
{
    wchar_t buf[64] = { 0 };
    StringFromGUID2(g, buf, ARRAYSIZE(buf));
    wprintf(L"%s", buf);
}

// Enumerate the device services the driver advertises (via WDI_GET_SUPPORTED_DEVICE_SERVICES).
static bool QuerySupportedServices(HANDLE hClient, const GUID& interfaceGuid)
{
    PWLAN_DEVICE_SERVICE_GUID_LIST pList = nullptr;
    DWORD result = WlanGetSupportedDeviceServices(hClient, &interfaceGuid, &pList);
    if (result != ERROR_SUCCESS || pList == nullptr)
    {
        printf("WlanGetSupportedDeviceServices failed with error %u\n", result);
        return false;
    }

    bool found = false;
    printf("Supported device services: %u\n", pList->dwNumberOfItems);
    for (DWORD i = 0; i < pList->dwNumberOfItems; i++)
    {
        printf("  [%u] ", i);
        PrintGuid(pList->DeviceService[i]);
        if (IsEqualGUID(pList->DeviceService[i], GUID_OEM_SAMPLE_DEVICE_SERVICE))
        {
            found = true;
            printf("  <-- OEM sample service");
        }
        printf("\n");
    }

    WlanFreeMemory(pList);
    return found;
}

static void SendHelloToInterface(HANDLE hClient, const GUID& interfaceGuid)
{
    char inBuffer[] = OEM_DEVICE_SERVICE_REQUEST_STRING; // includes null terminator
    DWORD inBufferSize = static_cast<DWORD>(sizeof(inBuffer));

    BYTE  outBuffer[256] = { 0 };
    DWORD outBufferSize = static_cast<DWORD>(sizeof(outBuffer));
    DWORD bytesReturned = 0;

    printf("Sending device service command: \"%s\"\n", inBuffer);

    DWORD result = WlanDeviceServiceCommand(
        hClient,
        &interfaceGuid,
        const_cast<LPGUID>(&GUID_OEM_SAMPLE_DEVICE_SERVICE),
        OEM_DEVICE_SERVICE_OPCODE_HELLO,
        inBufferSize,
        inBuffer,
        outBufferSize,
        outBuffer,
        &bytesReturned);

    if (result != ERROR_SUCCESS)
    {
        printf("WlanDeviceServiceCommand failed with error %u\n", result);
        return;
    }

    if (bytesReturned > 0)
    {
        outBuffer[std::min(bytesReturned, static_cast<DWORD>(sizeof(outBuffer) - 1))] = '\0';
        printf("Driver responded: \"%s\" (%u bytes)\n", reinterpret_cast<char*>(outBuffer), bytesReturned);
    }
    else
    {
        printf("Driver returned no data.\n");
    }
}

int __cdecl main()
{
    HANDLE hClient = nullptr;
    DWORD  negotiatedVersion = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;

    // 1) WlanOpenHandle
    DWORD result = WlanOpenHandle(WLAN_API_VERSION_2_0, nullptr, &negotiatedVersion, &hClient);
    if (result != ERROR_SUCCESS)
    {
        printf("WlanOpenHandle failed with error %u\n", result);
        return 1;
    }

    // 2) WlanEnumInterfaces
    result = WlanEnumInterfaces(hClient, nullptr, &pIfList);
    if (result != ERROR_SUCCESS)
    {
        printf("WlanEnumInterfaces failed with error %u\n", result);
        WlanCloseHandle(hClient, nullptr);
        return 1;
    }

    printf("Found %u WLAN interface(s).\n", pIfList->dwNumberOfItems);

    for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++)
    {
        const WLAN_INTERFACE_INFO& ifInfo = pIfList->InterfaceInfo[i];
        printf("\nInterface[%u]: %ws\n", i, ifInfo.strInterfaceDescription);

        // Enumerate supported device services first.
        bool supported = QuerySupportedServices(hClient, ifInfo.InterfaceGuid);

        // 3) WlanDeviceServiceCommand (only if our service is advertised)
        if (supported)
        {
            SendHelloToInterface(hClient, ifInfo.InterfaceGuid);
        }
        else
        {
            printf("OEM sample device service not advertised on this interface; skipping command.\n");
        }
    }

    // 4) WlanFreeMemory(pIfList);
    if (pIfList != nullptr)
    {
        WlanFreeMemory(pIfList);
        pIfList = nullptr;
    }

    // 5) WlanCloseHandle(hClient, nullptr);
    WlanCloseHandle(hClient, nullptr);

    return 0;
}
