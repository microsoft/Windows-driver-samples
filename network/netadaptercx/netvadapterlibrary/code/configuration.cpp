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
    { CONSTANT_UNICODE_STRING(L"MACLastByte"), NETV_OFFSET(MACLastByte), NETV_SIZE(MACLastByte), 0, 0, 254 },
    { CONSTANT_UNICODE_STRING(L"LinkProcIndex"), NETV_OFFSET(LinkProcIndex), NETV_SIZE(LinkProcIndex), 1000, 0, 1023 },
    { CONSTANT_UNICODE_STRING(L"S0Idle"), NETV_OFFSET(S0Idle), NETV_SIZE(S0Idle), 0, 0, 1 },
    { CONSTANT_UNICODE_STRING(L"EnableUsoUro"), NETV_OFFSET(EnableUsoUro), NETV_SIZE(EnableUsoUro), 0, 0, 1 },
#if ((NETADAPTER_VERSION_MAJOR == 2) && (NETADAPTER_VERSION_MINOR >= 6))
    { CONSTANT_UNICODE_STRING(L"PreallocatedRxBuffers"), NETV_OFFSET(PreallocatedRxBuffers), NETV_SIZE(PreallocatedRxBuffers), 0, 0, 1 },
#endif //NETCX 2.6 only
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

    // read all the registry values
    for (auto &property : NetvSupportedProperties)
    {
        // Driver should NOT fail the initialization only because it can not
        // read the registry
        auto pointer = (PUCHAR)Adapter + property.FieldOffset;

        // Get the configuration value for a specific parameter.  Under NT the
        // parameters are all read in as DWORDs.
        ULONG value = 0;
        
        status = NetConfigurationQueryUlong(
            configuration,
            NET_CONFIGURATION_QUERY_ULONG_NO_FLAGS,
            &property.RegName,
            &value);

        // Store the value in the adapter structure.
        switch (property.FieldSize)
        {
            case 1:
                *((PUCHAR)pointer) = (UCHAR)value;
                break;

            case 2:
                *((PUSHORT)pointer) = (USHORT)value;
                break;

            case 4:
                *((PULONG)pointer) = (ULONG)value;
                break;

            default:
                break;
        }

        // If the parameter was present, then check its value for validity.
        if (NT_SUCCESS(status))
        {
            // Check that param value is not too small or too large

            if (value < property.Min ||
                value > property.Max)
            {
                value = property.Default;
            }
        }
        else
        {
            value = property.Default;
            status = STATUS_SUCCESS;
        }
    }

    NetConfigurationClose(configuration);

    RETURN_STATUS_SUCCESS();
}
