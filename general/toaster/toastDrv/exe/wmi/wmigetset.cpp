#include "wmitoast.h"

#define TOASTER_DEVICE_INFO_CLASS   L"ToasterControl"
#define TOASTER_VAR1                L"ControlValue"


HRESULT
GetAndSetValuesInClass(
    _In_     IWbemServices* WbemServices,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    )

/*++

Routine Description:

    This routine enumerates the instances of the Toaster Device Information
    class and gets/sets the value of one of its Properties.

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

    const BSTR className   = SysAllocString(TOASTER_DEVICE_INFO_CLASS);

    VARIANT instProperty;
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
        status = instanceObj->Get(_bstr_t(L"__PATH"), 0, &instProperty, 0, 0);
        if (FAILED(status)) {
            goto exit;
        }

        printf("\n");
        printf("Instance Path .: %ws\n", (wchar_t*)instProperty.bstrVal);

        //
        // Get the current value of the DummyValue property.
        //
        status = instanceObj->Get(TOASTER_VAR1, 0, &instProperty, NULL, NULL);
        if (FAILED(status)) {
            goto exit;
        }

        printf("  Property ....: %ws\n", TOASTER_VAR1);
        printf("  Old Value ...: %d\n", instProperty.lVal);

        //
        // Set a new value for the DummyValue property.
        //
        instProperty.lVal++;

        status = instanceObj->Put(TOASTER_VAR1, 0, &instProperty, 0);
        if (FAILED(status)) {
            goto exit;
        }

        status = WbemServices->PutInstance(instanceObj,
                                           WBEM_FLAG_UPDATE_ONLY,
                                           NULL,
                                           NULL);
        if (FAILED(status)) {
            goto exit;
        }

        status = instanceObj->Get(_bstr_t(TOASTER_VAR1), 0, &instProperty, NULL, NULL);
        if (FAILED(status)) {
            goto exit;
        }

        printf("  New Value ...: %d\n", instProperty.lVal);

        instanceObj->Release();
        instanceObj = NULL;

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

