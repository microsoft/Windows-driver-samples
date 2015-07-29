#include <windows.h>
#include <comutil.h>
#include <wbemcli.h>
#include <stdio.h>
#include <tchar.h>

#define ARRAY_SIZE(_X_)     (sizeof((_X_))/sizeof((_X_)[0]))


HRESULT
SetInterfaceSecurity(
    _In_     IUnknown* InterfaceObj,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    );

HRESULT
ExecuteMethodsInClass(
    _In_     IWbemServices* WbemServices,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    );

HRESULT
GetAndSetValuesInClass(
    _In_     IWbemServices* WbemServices,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    );
