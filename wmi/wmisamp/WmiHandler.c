/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
    EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

Module Name:

    WmiHandler.c

Abstract:

    --

Environment:

    Kernel mode

--*/

#include "WmiSamp.h"

#define MOFRESOURCENAME L"MofResourceName"

//
// Private methods.
//

EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiClass1DataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiClass2DataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiClass3DataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiClass5DataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiClass6DataQueryInstance;

EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiClass1DataSetItem;

EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiClass1DataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiClass2DataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiClass3DataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiClass5DataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiClass6DataSetInstance;

EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiClass1ExecuteMethod;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WmiSampWmiRegistration)

#pragma alloc_text (PAGE, EvtWmiClass1DataQueryInstance)
#pragma alloc_text (PAGE, EvtWmiClass1DataSetInstance)
#pragma alloc_text (PAGE, EvtWmiClass1DataSetItem)
#pragma alloc_text (PAGE, EvtWmiClass1ExecuteMethod)

#pragma alloc_text (PAGE, EvtWmiClass2DataQueryInstance)
#pragma alloc_text (PAGE, EvtWmiClass2DataSetInstance)

#pragma alloc_text (PAGE, EvtWmiClass3DataQueryInstance)
#pragma alloc_text (PAGE, EvtWmiClass3DataSetInstance)

#pragma alloc_text (PAGE, EvtWmiClass5DataQueryInstance)
#pragma alloc_text (PAGE, EvtWmiClass5DataSetInstance)

#pragma alloc_text (PAGE, EvtWmiClass6DataQueryInstance)
#pragma alloc_text (PAGE, EvtWmiClass6DataSetInstance)
#endif


//
// The SampleInstanceConfig defines an array of data set used for the creation
// of WMI instances.
//
WMI_SAMPLE_INSTANCE_CONFIG SampleInstanceConfig[] = {
    {
        WmiSampleClass1Guid,
        WmiSampleClass1_SIZE,
        EvtWmiClass1DataQueryInstance,
        EvtWmiClass1DataSetInstance,
        EvtWmiClass1DataSetItem,
        EvtWmiClass1ExecuteMethod
    },

    {
        WmiSampleClass2Guid,
        WmiSampleClass2_SIZE,
        EvtWmiClass2DataQueryInstance,
        EvtWmiClass2DataSetInstance,
        NULL,
        NULL
    },

    {
        WmiSampleClass5Guid,
        WmiSampleClass5_SIZE,
        EvtWmiClass5DataQueryInstance,
        EvtWmiClass5DataSetInstance,
        NULL,
        NULL
    },

    {
        WmiSampleClass6Guid,
        WmiSampleClass6_SIZE,
        EvtWmiClass6DataQueryInstance,
        EvtWmiClass6DataSetInstance,
        NULL,
        NULL
    },
};

//
// The DynamicInstanceConfig defines the data set used for the dynamic creation
// of WMI instances.
//

WMI_SAMPLE_INSTANCE_CONFIG DynamicInstanceConfig = {

        WmiSampleClass3Guid,
        WmiSampleClass3_SIZE,
        EvtWmiClass3DataQueryInstance,
        EvtWmiClass3DataSetInstance,
        NULL,
        NULL
};


NTSTATUS
WmiSampWmiRegistration(
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    This function creates WMI provider instances for handling the WMI irps to
    the driver.

Arguments:

    Device - The Framework device object for which the WMI provider instances
        are to be created and registered. This device object will be the parent
        object of the new WMI instance objects.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status;
    ULONG i;
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    DECLARE_CONST_UNICODE_STRING(mofResourceName, MOFRESOURCENAME);

    PAGED_CODE();

    //
    // Register the MOF resource names of any customized WMI data providers
    // that are not defined in wmicore.mof.
    //
    status = WdfDeviceAssignMofResourceName(Device, &mofResourceName);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampWmiRegistration\n", status));
        return status;
    }

    //
    // Create a WMI instance object for each instance of each data block that
    // the driver supports for a device.
    //
    for (i = 0; i < ARRAYSIZE(SampleInstanceConfig); i++) {

        //
        // Initialize the config structures for the Provider and the Instance
        // and define event callback functions that support a WMI client's
        // requests to access the driver's WMI data blocks.
        //

        WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &SampleInstanceConfig[i].Guid);
        providerConfig.MinInstanceBufferSize = SampleInstanceConfig[i].MinSize;

        //
        // The WDFWMIPROVIDER handle is needed if multiple instances for the provider
        // has to be created or if the instances have to be created sometime after
        // the provider is created. In case below, the provider handle is not needed
        // because only one instance is needed and can be created when the provider
        // is created.
        //
        WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

        //
        // Create a Provider object as part of the Instance creation call by setting
        // the Register value in the Instance Config to TRUE. This eliminates the
        // need to call WdfWmiProviderRegister.
        //
        instanceConfig.Register = TRUE;

        instanceConfig.EvtWmiInstanceQueryInstance = SampleInstanceConfig[i].EvtWmiInstanceQueryInstance;
        instanceConfig.EvtWmiInstanceSetInstance   = SampleInstanceConfig[i].EvtWmiInstanceSetInstance;
        instanceConfig.EvtWmiInstanceSetItem       = SampleInstanceConfig[i].EvtWmiInstanceSetItem;
        instanceConfig.EvtWmiInstanceExecuteMethod = SampleInstanceConfig[i].EvtWmiInstanceExecuteMethod;

        //
        // Create the WMI instance object for this data block.
        //
        status = WdfWmiInstanceCreate(Device,
                                      &instanceConfig,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      WDF_NO_HANDLE);

        if (!NT_SUCCESS(status)) {

            DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampWmiRegistration\n", status));
            return status;
        }

    }

    return status;
}


NTSTATUS
WmiSampDynamicWmiRegistration(
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    This function creates WMI provider instance if the dynamic instance does not
    already exist. If it does exist, this funciton deregisters and deletes the
    WMI instance. This function assumes that the mof has already been registered
    with the given Framework device object (by a previous call to WmiSampWmiRegistration).

Arguments:

    Device - The Framework device object for which a WMI provider instance
        has to be created if the instance does not exist or deleted if the
        instance exists.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;

    wmiDeviceData = GetWmiSampleDeviceData(Device);

    //
    // If the dynamic provider already exists, then delete the dynamic provider
    // and all of its instances.
    //
    if (wmiDeviceData->DynamicInstance) {

        DebugPrint(("Delete Dynamic instance.\n"));

        //
        // Deregister the dynamic WMI Instance so that this instance does not
        // receive any more WMI query callbacks.
        //
        WdfWmiInstanceDeregister(wmiDeviceData->DynamicInstance);

        //
        // Delete the dynamic WMI Instance object.
        //
        WdfObjectDelete(wmiDeviceData->DynamicInstance);
        wmiDeviceData->DynamicInstance = NULL;

    } else {

        DebugPrint(("Create Dynamic instance.\n"));

        //
        // Initialize the config structures for the Provider and the Instance
        // and define event callback functions that support a WMI client's
        // requests to access the driver's WMI data blocks.
        //

        WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &DynamicInstanceConfig.Guid);
        providerConfig.MinInstanceBufferSize = DynamicInstanceConfig.MinSize;

        //
        // You would want to create a WDFWMIPROVIDER handle separately if you are
        // going to dynamically create instances on the provider. Since we are
        // statically creating one instance, there is no need to create the provider
        // handle.
        //
        WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

        //
        // Create a Provider object as part of the Instance creation call by setting
        // the Register value in the Instance Config to TRUE. This eliminates the
        // need to call WdfWmiProviderRegister.
        //
        instanceConfig.Register = TRUE;

        instanceConfig.EvtWmiInstanceQueryInstance = DynamicInstanceConfig.EvtWmiInstanceQueryInstance;
        instanceConfig.EvtWmiInstanceSetInstance   = DynamicInstanceConfig.EvtWmiInstanceSetInstance;
        instanceConfig.EvtWmiInstanceSetItem       = DynamicInstanceConfig.EvtWmiInstanceSetItem;
        instanceConfig.EvtWmiInstanceExecuteMethod = DynamicInstanceConfig.EvtWmiInstanceExecuteMethod;

        //
        // Create the WMI instance object for this data block.
        //
        status = WdfWmiInstanceCreate(Device,
                                      &instanceConfig,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &(wmiDeviceData->DynamicInstance));

        if (!NT_SUCCESS(status)) {

            DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampDynamicWmiRegistration\n", status));
            return status;
        }

        //
        // Increment the Create count if the dynamic WMI Instance was created.
        //
        wmiDeviceData->CreateCount++;

    }

    return status;
}


NTSTATUS
EvtWmiClass1DataQueryInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG OutBufferSize,
    PVOID OutBuffer,
    PULONG BufferUsed
    )

/*++

Routine Description:

    This is the callback routine for the WMI Query irp on the Instance representing
    the sample class 1. This routine gets the current value for the data members
    of the sample class and copies it to the given output buffer. The sample class1
    is made up of the same parameters as the Embedded Class EC1.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    OutBufferSize - The size (in bytes) of the output buffer into which the
        instance data is to be copied.

    OutBuffer - Pointer to the output buffer.

    BufferUsed - Pointer to the location that receives the number of bytes that
        were copied into the output buffer.

Return Value:

    NT Status code.

--*/

{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain EC1.
    //
    *BufferUsed = WmiSampGetEc1(wmiDeviceData,
                                OutBuffer,
                                0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass1DataSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer
    )
{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    PAGED_CODE();

    //
    // InBufferSize is guaranteed to be at least WmiSampleClass1_SIZE
    // which in this case is EC1_SIZE.
    //
    UNREFERENCED_PARAMETER(InBufferSize);

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain EC1.
    //
    WmiSampSetEc1(wmiDeviceData,
                  InBuffer,
                  EC1_SIZE,
                  0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass1DataSetItem(
    WDFWMIINSTANCE WmiInstance,
    ULONG DataItemId,
    ULONG InBufferSize,
    PVOID InBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(WmiInstance);
    UNREFERENCED_PARAMETER(DataItemId);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(InBuffer);

    return status;
}


NTSTATUS
EvtWmiClass1ExecuteMethod(
    WDFWMIINSTANCE WmiInstance,
    ULONG MethodId,
    ULONG InBufferSize,
    ULONG OutBufferSize,
    PVOID Buffer,
    PULONG BufferUsed
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    switch (MethodId) {
    case SetEC1:
        {
            if (InBufferSize >= EC1_SIZE) {

                WmiSampSetEc1(wmiDeviceData,
                              Buffer,
                              EC1_SIZE,
                              0);
                status = STATUS_SUCCESS;

            } else {

                status = STATUS_INVALID_PARAMETER_MIX;
                DebugPrint(("[WmiSamp] Status = 0x%08x, EvtWmiClass1ExecuteMethod\n", status));
            }
        }
        break;

    case DummyMethod:
        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_WMI_ITEMID_NOT_FOUND;
        DebugPrint(("[WmiSamp] Status = 0x%08x, EvtWmiClass1ExecuteMethod\n", status));
        break;
    }

    *BufferUsed = 0;
    return status;
}


NTSTATUS
EvtWmiClass2DataQueryInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG OutBufferSize,
    PVOID OutBuffer,
    PULONG BufferUsed
    )

/*++

Routine Description:

    This is the callback routine for the WMI Query irp on the Instance representing
    the sample class 2. This routine gets the current value for the data members
    of the sample class and copies it to the given output buffer. The sample class2
    contains an embedded class.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    OutBufferSize - The size (in bytes) of the output buffer into which the
        instance data is to be copied.

    OutBuffer - Pointer to the output buffer.

    BufferUsed - Pointer to the location that receives the number of bytes that
        were copied into the output buffer.

Return Value:

    NT Status code.

--*/

{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain (Embedded) EC1.
    //
    *BufferUsed = WmiSampGetEc1(wmiDeviceData,
                                OutBuffer,
                                0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass2DataSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer
    )
{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    PAGED_CODE();

    //
    // InBufferSize is guaranteed to be at least WmiSampleClass2_SIZE
    // which in this case is EC1_SIZE.
    //
    UNREFERENCED_PARAMETER(InBufferSize);

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain (Embedded) EC1.
    //
    WmiSampSetEc1(wmiDeviceData,
                  InBuffer,
                  EC1_SIZE,
                  0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass3DataQueryInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG OutBufferSize,
    PVOID OutBuffer,
    PULONG BufferUsed
    )

/*++

Routine Description:

    This is the callback routine for the WMI Query irp on the Instance representing
    the sample class 3. This routine gets the current value for the data members
    of the sample class and copies it to the given output buffer. The sample class3
    contains a fixed array of embedded class EC1.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    OutBufferSize - The size (in bytes) of the output buffer into which the
        instance data is to be copied.

    OutBuffer - Pointer to the output buffer.

    BufferUsed - Pointer to the location that receives the number of bytes that
        were copied into the output buffer.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status;
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;
    ULONG sizeNeeded;
    ULONG sizeUsed;
    ULONG i;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Fixed array of EC1.
    //
    sizeNeeded = 0;
    for (i = 0; i < EC1_COUNT; i++) {
        sizeNeeded += ALIGN_UP(wmiDeviceData->Ec1Length[i], PVOID);
    }

    if (OutBufferSize < sizeNeeded) {

        *BufferUsed = 0;
        status = STATUS_BUFFER_TOO_SMALL;

    } else {

        *BufferUsed = 0;
        for (i = 0; i < EC1_COUNT; i++) {

            sizeUsed = WmiSampGetEc1(wmiDeviceData, OutBuffer, i);
            OutBuffer = Add2Ptr(OutBuffer, ALIGN_UP(sizeUsed, PVOID));
            *BufferUsed += sizeUsed;
        }
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
EvtWmiClass3DataSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer
    )

/*++

Routine Description:

    ---

Arguments:

    WmiInstance -
    InBufferSize -
    InBuffer -

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;
    ULONG i;
    PEC1 Ec1;

    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Fixed array of EC1.
    //
    Ec1 = (PEC1)InBuffer;
    for (i = 0; i < EC1_COUNT; i++) {

        if (InBufferSize >= EC1_SIZE) {

            WmiSampSetEc1(wmiDeviceData, Ec1, EC1_SIZE, i);
            InBufferSize -= EC1_SIZE;
            Ec1++;

        } else {

            status = STATUS_INVALID_PARAMETER_MIX;
            DebugPrint(("[WmiSamp] Status = 0x%08x, EvtWmiClass3DataSetInstance\n", status));
        }
    }
    return status;
}


NTSTATUS
EvtWmiClass5DataQueryInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG OutBufferSize,
    PVOID OutBuffer,
    PULONG BufferUsed
    )

/*++

Routine Description:

    This is the callback routine for the WMI Query irp on the Instance representing
    the sample class 5. This routine gets the current value for the data members
    of the sample class and copies it to the given output buffer. The sample class5
    contains an embedded class.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    OutBufferSize - The size (in bytes) of the output buffer into which the
        instance data is to be copied.

    OutBuffer - Pointer to the output buffer.

    BufferUsed - Pointer to the location that receives the number of bytes that
        were copied into the output buffer.

Return Value:

    NT Status code.

--*/

{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain (Embedded) EC2.
    //
    *BufferUsed = WmiSampGetEc2(wmiDeviceData,
                                OutBuffer,
                                0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass5DataSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer
    )
{
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    PAGED_CODE();

    //
    // InBufferSize is guaranteed to be at least WmiSampleClass5_SIZE
    // which in this case is EC2_SIZE.
    //
    UNREFERENCED_PARAMETER(InBufferSize);

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Plain (Embedded) EC2.
    //
    WmiSampSetEc2(wmiDeviceData,
                  InBuffer,
                  EC2_SIZE,
                  0);

    return STATUS_SUCCESS;
}


NTSTATUS
EvtWmiClass6DataQueryInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG OutBufferSize,
    PVOID OutBuffer,
    PULONG BufferUsed
    )

/*++

Routine Description:

    This is the callback routine for the WMI Query irp on the Instance representing
    the sample class 6. This routine gets the current value for the data members
    of the sample class and copies it to the given output buffer. The sample class6
    contains a fixed array of embedded class EC2.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    OutBufferSize - The size (in bytes) of the output buffer into which the
        instance data is to be copied.

    OutBuffer - Pointer to the output buffer.

    BufferUsed - Pointer to the location that receives the number of bytes that
        were copied into the output buffer.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status;
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;
    ULONG sizeNeeded;
    ULONG sizeUsed;
    ULONG i;

    UNREFERENCED_PARAMETER(OutBufferSize);
    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Fixed array of EC2.
    //
    sizeNeeded = 0;
    for (i = 0; i < EC2_COUNT; i++) {
        sizeNeeded += ALIGN_UP(wmiDeviceData->Ec2Length[i], PVOID);
    }

    if (OutBufferSize < sizeNeeded) {

        *BufferUsed = 0;
        status = STATUS_BUFFER_TOO_SMALL;

    } else {

        *BufferUsed = 0;
        for (i = 0; i < EC2_COUNT; i++) {

            sizeUsed = WmiSampGetEc2(wmiDeviceData, OutBuffer, i);
            OutBuffer = Add2Ptr(OutBuffer, ALIGN_UP(sizeUsed, PVOID));
            *BufferUsed += sizeUsed;
        }
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
EvtWmiClass6DataSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer
    )

/*++

Routine Description:

    This is the callback routine for setting the WMI data provider's instance
    data supplied by a WMI client. This routine copies the data in the input
    buffer and updates the sample class6 data which contains a fixed array of
    embedded class EC2.

Arguments:

    WmiInstance - The handle to the WMI instance object.

    InBufferSize - The size (in bytes) of the input buffer from which the
        instance data is to be copied.

    InBuffer - Pointer to the input buffer.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;
    ULONG i;
    PEC2 Ec2;

    PAGED_CODE();

    device = WdfWmiInstanceGetDevice(WmiInstance);
    wmiDeviceData = GetWmiSampleDeviceData(device);

    //
    // Fixed array of EC2.
    //
    Ec2 = (PEC2)InBuffer;
    for (i = 0; i < EC2_COUNT; i++) {

        if (InBufferSize >= EC2_SIZE) {

            WmiSampSetEc2(wmiDeviceData, Ec2, EC2_SIZE, i);
            InBufferSize -= EC2_SIZE;
            Ec2++;

        } else {

            status = STATUS_INVALID_PARAMETER_MIX;
            DebugPrint(("[WmiSamp] Status = 0x%08x, EvtWmiClass6DataSetInstance\n", status));
        }
    }
    return status;
}
