#include "wmitoast.h"

#define TOASTER_METHOD_CLASS    L"ToasterControl"
#define TOASTER_METHOD_1        L"ToasterControl1"
#define TOASTER_METHOD_2        L"ToasterControl2"
#define TOASTER_METHOD_3        L"ToasterControl3"
#define IN_DATA1_VALUE          25
#define IN_DATA2_VALUE          30

//
// Private methods.
//

HRESULT
ExecuteMethod1InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData
    );

HRESULT
ExecuteMethod2InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData1,
    _In_ ULONG InData2
    );

HRESULT
ExecuteMethod3InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData1,
    _In_ ULONG InData2
    );


HRESULT
ExecuteMethodsInClass(
    _In_     IWbemServices* WbemServices,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    )

/*++

Routine Description:

    This routine enumerates the instances of the Toaster method class and
    executes the methods in the class for each instance.

Arguments:

    WbemServices - Pointer to the WBEM services interface used for accessing
        the WMI services.

    UserId - Pointer to the user id information or NULL.

    Password - Pointer to password or NULL. If the user id is not specified,
        this parameter is ignored.

    DomainName - Pointer to domain name or NULL. If the user id is not specified,
        this parameter is ignored.

Return Value:

    HRESULT Status code.

--*/

{
    HRESULT status = S_OK;

    IEnumWbemClassObject* enumerator  = NULL;
    IWbemClassObject* classObj        = NULL;
    IWbemClassObject* instanceObj     = NULL;

    const BSTR className   = SysAllocString(TOASTER_METHOD_CLASS);

    VARIANT pathVariable;
    _bstr_t instancePath;
    ULONG   nbrObjsSought = 1;
    ULONG   nbrObjsReturned;

    //
    // Create an Enumeration object to enumerate the instances of the given class.
    //
    status = WbemServices->CreateInstanceEnum(className,
                                              WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                              NULL,
                                              &enumerator);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Set authentication information for the interface.
    //
    status = SetInterfaceSecurity(enumerator, UserId, Password, DomainName);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Get the class object for the method definition.
    //
    status = WbemServices->GetObject(className, 0, NULL, &classObj, NULL);
    if (FAILED(status) || NULL == classObj) {
        goto exit;
    }

    do {

        //
        // Get the instance object for each instance of the class.
        //
        status = enumerator->Next(WBEM_INFINITE,
                                  nbrObjsSought,
                                  &instanceObj,
                                  &nbrObjsReturned);

        if (status == WBEM_S_FALSE) {
            status = S_OK;
            break;
        }

        if (FAILED(status)) {
            if (status == WBEM_E_INVALID_CLASS) {
                printf("ERROR: Toaster driver may not be active on the system.\n");
            }
            goto exit;
        }

        //
        // To obtain the object path of the object for which the method has to be
        // executed, query the "__PATH" property of the WMI instance object.
        //
        status = instanceObj->Get(_bstr_t(L"__PATH"), 0, &pathVariable, NULL, NULL);
        if (FAILED(status)) {
            goto exit;
        }

        instancePath = pathVariable.bstrVal;
        instanceObj->Release();
        instanceObj = NULL;

        //
        // Execute the methods in this instance of the class.
        //
        status = ExecuteMethod1InInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE);
        if (FAILED(status)) {
            goto exit;
        }

        status = ExecuteMethod2InInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE, IN_DATA2_VALUE);
        if (FAILED(status)) {
            goto exit;
        }

        status = ExecuteMethod3InInstance(WbemServices, classObj, instancePath, IN_DATA1_VALUE, IN_DATA2_VALUE);
        if (FAILED(status)) {
            goto exit;
        }
    } while (!FAILED(status));

exit:

    if (className != NULL) {
        SysFreeString(className);
    }

    if (classObj != NULL) {
        classObj->Release();
    }

    if (enumerator != NULL) {
        enumerator->Release();
    }

    if (instanceObj != NULL) {
        instanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethod1InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData
    )
{
    HRESULT status;

    IWbemClassObject* inputParamsObj  = NULL;
    IWbemClassObject* inputParamsInstanceObj  = NULL;
    IWbemClassObject* outputParamsInstanceObj = NULL;

    const BSTR methodName = SysAllocString(TOASTER_METHOD_1);
    VARIANT funcParam;

    //
    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Set the input variables values (i.e., inData for ToasterMethod1).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData;

    status = inputParamsInstanceObj->Put(L"InData", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);

    if (FAILED(status) || NULL == outputParamsInstanceObj) {
        goto exit;
    }

    //
    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData....: %d\n", funcParam.ulVal);

    //
    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData...: %d\n", funcParam.ulVal);

exit:

    if (methodName != NULL) {
        SysFreeString(methodName);
    }

    if (inputParamsObj != NULL) {
        inputParamsObj->Release();
    }

    if (inputParamsInstanceObj != NULL) {
        inputParamsInstanceObj->Release();
    }

    if (outputParamsInstanceObj != NULL) {
        outputParamsInstanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethod2InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData1,
    _In_ ULONG InData2
    )
{
    HRESULT status;

    IWbemClassObject* inputParamsObj  = NULL;
    IWbemClassObject* inputParamsInstanceObj  = NULL;
    IWbemClassObject* outputParamsInstanceObj = NULL;

    const BSTR methodName = SysAllocString(TOASTER_METHOD_2);
    VARIANT funcParam;

    //
    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Set the input variables values (i.e., inData1, inData2 for ToasterMethod2).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData1;

    status = inputParamsInstanceObj->Put(L"InData1", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    funcParam.vt = VT_I4;
    funcParam.ulVal = InData2;

    status = inputParamsInstanceObj->Put(L"InData2", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);

    if (FAILED(status) || NULL == outputParamsInstanceObj) {
        goto exit;
    }

    //
    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData1...: %d\n", funcParam.ulVal);

    status = inputParamsInstanceObj->Get(L"InData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData2...: %d\n", funcParam.ulVal);

    //
    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData...: %d\n", funcParam.ulVal);

exit:

    if (methodName != NULL) {
        SysFreeString(methodName);
    }

    if (inputParamsObj != NULL) {
        inputParamsObj->Release();
    }

    if (inputParamsInstanceObj != NULL) {
        inputParamsInstanceObj->Release();
    }

    if (outputParamsInstanceObj != NULL) {
        outputParamsInstanceObj->Release();
    }

    return status;
}


HRESULT
ExecuteMethod3InInstance(
    _In_ IWbemServices* WbemServices,
    _In_ IWbemClassObject* ClassObj,
    _In_ const BSTR InstancePath,
    _In_ ULONG InData1,
    _In_ ULONG InData2
    )
{
    HRESULT status;

    IWbemClassObject* inputParamsObj  = NULL;
    IWbemClassObject* inputParamsInstanceObj  = NULL;
    IWbemClassObject* outputParamsInstanceObj = NULL;

    const BSTR methodName = SysAllocString(TOASTER_METHOD_3);
    VARIANT funcParam;

    //
    // Get the input parameters class objects for the method.
    //
    status = ClassObj->GetMethod(methodName, 0, &inputParamsObj, NULL);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Spawn an instance of the input parameters class object.
    //
    status = inputParamsObj->SpawnInstance(0, &inputParamsInstanceObj);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Set the input variables values (i.e., inData1, inData2 for ToasterMethod3).
    //
    funcParam.vt = VT_I4;
    funcParam.ulVal = InData1;

    status = inputParamsInstanceObj->Put(L"InData1", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    funcParam.vt = VT_I4;
    funcParam.ulVal = InData2;

    status = inputParamsInstanceObj->Put(L"InData2", 0, &funcParam, 0);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Call the method.
    //
    printf("\n");
    printf("Instance Path .: %ws\n", (wchar_t*)InstancePath);
    printf("  Method Name..: %ws\n", (wchar_t*)methodName);
    status = WbemServices->ExecMethod(InstancePath,
                                      methodName,
                                      0,
                                      NULL,
                                      inputParamsInstanceObj,
                                      &outputParamsInstanceObj,
                                      NULL);

    if (FAILED(status) || NULL == outputParamsInstanceObj) {
        goto exit;
    }

    //
    // Get the "in" Parameter values from the input parameters object.
    //
    status = inputParamsInstanceObj->Get(L"InData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData1...: %d\n", funcParam.ulVal);

    status = inputParamsInstanceObj->Get(L"InData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     InData2...: %d\n", funcParam.ulVal);

    //
    // Get the "out" Parameter values from the output parameters object.
    //
    status = outputParamsInstanceObj->Get(L"OutData1", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData1..: %d\n", funcParam.ulVal);

    status = outputParamsInstanceObj->Get(L"OutData2", 0, &funcParam, NULL, NULL);
    if (FAILED(status)) {
        goto exit;
    }
    printf("     OutData2..: %d\n", funcParam.ulVal);

exit:

    if (methodName != NULL) {
        SysFreeString(methodName);
    }

    if (inputParamsObj != NULL) {
        inputParamsObj->Release();
    }

    if (inputParamsInstanceObj != NULL) {
        inputParamsInstanceObj->Release();
    }

    if (outputParamsInstanceObj != NULL) {
        outputParamsInstanceObj->Release();
    }

    return status;
}

