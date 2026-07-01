#include "pch.hpp"
#include "netvadapter.h"

#include "trace.h"
#include "configuration.tmh"

typedef struct _NETVADAPTER_ADVANCED_PROPERTY
{
    UNICODE_STRING RegName;  // variable name text
    UINT32 FieldOffset;     // offset to NetvAdapter field
    UINT32 FieldSize;       // size (in bytes) of the field
    UINT32 Default;         // default value to use
    UINT32 Min;             // minimum value allowed
    UINT32 Max;             // maximum value allowed
} NETVADAPTER_ADVANCED_PROPERTY;

#define NETV_OFFSET(field)   ((UINT32)FIELD_OFFSET(NetvAdapter,field))
#define NETV_SIZE(field)     RTL_FIELD_SIZE(NetvAdapter,field)

#define CONSTANT_UNICODE_STRING(s) {sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }

NETVADAPTER_ADVANCED_PROPERTY NetvSupportedProperties[] =
{
    // reg value name - Offset in NetvAdapter - Field size - Default Value - Min - Max

    // Standard Keywords
    { CONSTANT_UNICODE_STRING(L"MACLastByte"), NETV_OFFSET(MACLastByte), NETV_SIZE(MACLastByte), 1, 1, 254 },
    { CONSTANT_UNICODE_STRING(L"LinkProcIndex"), NETV_OFFSET(LinkProcIndex), NETV_SIZE(LinkProcIndex), 1000, 0, 1023 },
    { CONSTANT_UNICODE_STRING(L"S0Idle"), NETV_OFFSET(S0Idle), NETV_SIZE(S0Idle), 0, 0, 1 },
    { CONSTANT_UNICODE_STRING(L"EnableUsoUro"), NETV_OFFSET(EnableUsoUro), NETV_SIZE(EnableUsoUro), 0, 0, 1 },
};

NTSTATUS
NetvAdapterReadConfiguration(
    NetvAdapter *Adapter,
    WDFDEVICE Device
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    NETCONFIGURATION configuration;
    RETURN_IF_NOT_STATUS_SUCCESS(
        NetDeviceOpenConfiguration(Device, WDF_NO_OBJECT_ATTRIBUTES, &configuration));

    // A missing or out-of-range registry value must not fail initialization;
    // fall back to the property default instead.  Under NT the parameters are
    // all read in as DWORDs.
    for (auto &property : NetvSupportedProperties)
    {
        ULONG value = 0;
        status = NetConfigurationQueryUlong(
            configuration,
            NET_CONFIGURATION_QUERY_ULONG_NO_FLAGS,
            &property.RegName,
            &value);

        if (! NT_SUCCESS(status) ||
            value < property.Min ||
            value > property.Max)
        {
            value = property.Default;
            status = STATUS_SUCCESS;
        }

        auto pointer = (PUCHAR)Adapter + property.FieldOffset;
        switch (property.FieldSize)
        {
            case 1:  *((PUCHAR)pointer)  = (UCHAR)value;  break;
            case 2:  *((PUSHORT)pointer) = (USHORT)value; break;
            case 4:  *((PULONG)pointer)  = (ULONG)value;  break;
            default: break;
        }
    }

    NetConfigurationClose(configuration);

    RETURN_STATUS_SUCCESS();
}
