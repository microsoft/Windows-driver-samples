#include "wmitoast.h"


//
// Private methods.
//

bool
ParseCommandLine(
    _In_              ULONG Argc,
    _In_reads_(Argc) PWSTR Argv[],
    _Outptr_result_maybenull_   PWSTR* ComputerName,
    _Outptr_result_maybenull_   PWSTR* UserId,
    _Outptr_result_maybenull_   PWSTR* Password,
    _Outptr_result_maybenull_   PWSTR* DomainName
    );

void
DisplayUsage();



ULONG
_cdecl
wmain(
    _In_              ULONG Argc,
    _In_reads_(Argc) PWSTR Argv[]
    )
{
    HRESULT status = S_OK;
    BOOLEAN initialized = FALSE;

    BSTR temp           = NULL;
    BSTR wmiRoot        = NULL;
    BSTR userIdString   = NULL;
    BSTR passwordString = NULL;

    PWSTR ComputerName = NULL;
    PWSTR userId       = NULL;
    PWSTR password     = NULL;
    PWSTR domain       = NULL;

    IWbemLocator* wbemLocator   = NULL;
    IWbemServices* wbemServices = NULL;

    //
    // Parse the input command line parameters.
    //
    if (!ParseCommandLine(Argc,
                          Argv,
                          &ComputerName,
                          &userId,
                          &password,
                          &domain)) {
        //
        // Display the usage information if there was an error parsing the input
        // parameters or if usage information was requested.
        //
        status = E_INVALIDARG;
        DisplayUsage();
        goto exit;
    }

    //
    // Initialize COM environment for multi-threaded concurrency.
    //
    status = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(status)) {
        goto exit;
    }

    initialized = TRUE;

    //
    // Initialize the security layer and set the specified values as the
    // security default for the process.
    //
    status = CoInitializeSecurity(NULL,
                                  -1,
                                  NULL,
                                  NULL,
                                  RPC_C_AUTHN_LEVEL_PKT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE,
                                  NULL,
                                  EOAC_NONE,
                                  0);

    if (FAILED(status)) {
        goto exit;
    }

    //
    // Create a single uninitialized object associated with the class id
    // CLSID_WbemLocator.
    //
    status = CoCreateInstance(CLSID_WbemLocator,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IWbemLocator,
                              (PVOID*)&wbemLocator);

    if (FAILED(status)) {
        goto exit;
    }

    //
    // Construct the object path for the WMI namespace. For local access to the
    // WMI namespace, use a simple object path: "\\.\root\WMI". For access to
    // the WMI namespace on a remote computer, include the computer name in the
    // object path: "\\myserver\root\WMI".
    //
    if (ComputerName != NULL) {

        status = VarBstrCat(_bstr_t(L"\\\\"), _bstr_t(ComputerName), &temp);
        if (FAILED(status)) {
            goto exit;
        }

    } else {

        status = VarBstrCat(_bstr_t(L"\\\\"), _bstr_t(L"."), &temp);
        if (FAILED(status)) {
            goto exit;
        }
    }

    status = VarBstrCat(temp, _bstr_t(L"\\root\\WMI"), &wmiRoot);
    if (FAILED(status)) {
        goto exit;
    }

    SysFreeString(temp);
    temp = NULL;

    //
    // Construct the user id and password strings.
    //
    if (userId != NULL) {

        if (domain != NULL) {

            status = VarBstrCat(_bstr_t(domain), _bstr_t(L"\\"), &temp);
            if (FAILED(status)) {
                goto exit;
            }

            status = VarBstrCat(temp, _bstr_t(userId), &userIdString);
            if (FAILED(status)) {
                goto exit;
            }

            SysFreeString(temp);
            temp = NULL;

        } else {

            userIdString = SysAllocString(userId);
            if (userIdString == NULL) {
                status = E_OUTOFMEMORY;
                goto exit;
            }
        }

        passwordString = SysAllocString(password);
        if (passwordString == NULL) {
            status = E_OUTOFMEMORY;
            goto exit;
        }
    }

    //
    // Connect to the WMI server on this computer and, possibly, through it to another system.
    //
    status = wbemLocator->ConnectServer(wmiRoot,
                                        userIdString,
                                        passwordString,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL,
                                        &wbemServices);
    if (FAILED(status)) {
        if (status != WBEM_E_LOCAL_CREDENTIALS) {
            goto exit;
        }

        //
        // Use the identity inherited from the current process.
        //
        status = wbemLocator->ConnectServer(wmiRoot,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL,
                                            &wbemServices);
        if (FAILED(status)) {
            goto exit;
        }
    }

    //
    // Set authentication information for the interface.
    //
    status = SetInterfaceSecurity(wbemServices, userId, password, domain);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Execute the methods in each instance of the desired class.
    //
    printf("\n1. Execute Methods in class ...\n");
    status = ExecuteMethodsInClass(wbemServices,
                                   userId,
                                   password,
                                   domain);
    if (FAILED(status)) {
        goto exit;
    }

    //
    // Get and Set the property values in each instance of the desired class.
    //
    printf("\n2. Get/Set Property Values in class ...\n");
    status = GetAndSetValuesInClass(wbemServices,
                                    userId,
                                    password,
                                    domain);
    if (FAILED(status)) {
        goto exit;
    }

exit:

    if (temp != NULL) {
        SysFreeString(temp);
    }

    if (userIdString != NULL) {
        SysFreeString(userIdString);
    }

    if (passwordString != NULL) {
        SysFreeString(passwordString);
    }

    if (wmiRoot != NULL) {
        SysFreeString(wmiRoot);
    }

    if (wbemServices != NULL) {
        wbemServices->Release();
    }

    if (wbemLocator != NULL) {
        wbemLocator->Release();
    }

    if (initialized == TRUE) {
        CoUninitialize();
    }

    if (FAILED(status)) {
        printf("FAILED with Status = 0x%08x\n", status);
    }
    return status;
}


HRESULT
SetInterfaceSecurity(
    _In_     IUnknown* InterfaceObj,
    _In_opt_ PWSTR UserId,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DomainName
    )

/*++

Routine Description:

    Set the interface security to allow the server to impersonate the specified
    user.

Arguments:

    InterfaceObj - Pointer to interface for which the security settings need
        to be applied.

    UserId - Pointer to the user id information or NULL.

    Password - Pointer to password or NULL. If the user id is not specified,
        this parameter is ignored.

    DomainName - Pointer to domain name or NULL. If the user id is not specified,
        this parameter is ignored.

Return Value:

    HRESULT Status code.

--*/

{
    HRESULT    hr;

    COAUTHIDENTITY AuthIdentity;
    DWORD AuthnSvc;
    DWORD AuthzSvc;
    DWORD AuthnLevel;
    DWORD ImpLevel;
    DWORD Capabilities;
    PWSTR pServerPrinName = NULL;
    RPC_AUTH_IDENTITY_HANDLE   pAuthHndl = NULL;

    //
    // Get current authentication information for interface.
    //
    hr = CoQueryProxyBlanket(InterfaceObj,
                             &AuthnSvc,
                             &AuthzSvc,
                             &pServerPrinName,
                             &AuthnLevel,
                             &ImpLevel,
                             &pAuthHndl,
                             &Capabilities);

    if (FAILED(hr)) {
        goto exit;
    }

    if (UserId == NULL) {

        AuthIdentity.User           = NULL;
        AuthIdentity.UserLength     = 0;
        AuthIdentity.Password       = NULL;
        AuthIdentity.PasswordLength = 0;
        AuthIdentity.Domain         = NULL;
        AuthIdentity.DomainLength   = 0;

    } else {

        AuthIdentity.User           = (USHORT *) UserId;
#pragma prefast(suppress:6387, "0 length UserId is valid")        
        AuthIdentity.UserLength     = (ULONG)wcslen(UserId);
        AuthIdentity.Password       = (USHORT *) Password;
#pragma prefast(suppress:6387, "0 length Password is valid")               
        AuthIdentity.PasswordLength = (ULONG)wcslen(Password);
        AuthIdentity.Domain         = (USHORT *) DomainName;
        AuthIdentity.DomainLength   = (DomainName == NULL) ? 0 : (ULONG)wcslen(DomainName);

    }

    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    //
    // Change authentication information for interface, providing the identity
    // information and "echoing back" everything else.
    //
    hr = CoSetProxyBlanket(InterfaceObj,
                           AuthnSvc,
                           AuthzSvc,
                           pServerPrinName,
                           AuthnLevel,
                           ImpLevel,
                           &AuthIdentity,
                           Capabilities);

    if (FAILED(hr)) {
        goto exit;
    }

exit:
    return hr;
}


bool
ParseCommandLine(
    _In_              ULONG Argc,
    _In_reads_(Argc) PWSTR Argv[],
    _Outptr_result_maybenull_   PWSTR* ComputerName,
    _Outptr_result_maybenull_   PWSTR* UserId,
    _Outptr_result_maybenull_   PWSTR* Password,
    _Outptr_result_maybenull_   PWSTR* DomainName
    )

/*++

Routine Description:

    This routine parses the command line input and extracts the computer name,
    user id, password and domain name information.

Arguments:

    Argc - Number of elements in the Agrv variable.

    Argv - Array of strings received as input to the wmain function.

    ComputerName - Pointer to the location that receives the address of the
        computer name.

    UserId - Pointer to the location that receives the address of the UserId.

    Password - Pointer to the location that receives the adddress of the
        Password string for the given user id.

    DomainName - Pointer to location that recieves the address of DomainName.
        of the user.

Return Value:

    TRUE, if the command line could be parsed successfully.
    FALSE, otherwise.

--*/

{
    ULONG i;

    *ComputerName = NULL;
    *UserId       = NULL;
    *Password     = NULL;
    *DomainName   = NULL;

    //
    // Parameters are (when supplied):
    //
    //   0 -- Program name.  Always supplied (by OS).
    //   1 -- System name.  Optional.
    //   2 -- Userid.  Optional but reconized only if system name supplied.
    //   3 -- Password.  Optional but recognized only if system name and UserId supplied.
    //   4 -- DomainName.  Optional but recognized only if system name, UserId and Password supplied.
    //

    //
    // Validate the number of input parameters.
    //
    if (!(Argc == 1 || Argc==2 || Argc==4 || Argc==5)) {

        printf("\nInvalid input parameters\n");
        return FALSE;
    }

    //
    // Parse the input parameters.
    //
    for (i = 1; i < Argc; i++) {
        switch(i) {
        case 1:

            //
            // System name.
            //
            *ComputerName = Argv[i];
            break;

        case 2:

            //
            // User ID.
            //
            *UserId = Argv[i];
            break;

        case 3:

            //
            // Password.
            //
            *Password = Argv[i];
            break;

        case 4:

            //
            // Domain Name.
            //
            *DomainName = Argv[i];
            break;

        default:

            printf("\nInvalid input parameters\n");
            return FALSE;
        }
    }

    //
    // Verify that if User Id was specified, then the Password is sepcified too.
    //
    if (((*UserId != NULL) && (*Password == NULL)) ||
        ((*UserId == NULL) && (*Password != NULL))) {

        printf("\nIllegal UserId/Password combination\n\n");
        return FALSE;
    }

    //
    // Check whether the usage help was requested.
    //
    if (Argc == 2) {
        PWSTR HelpCmds[] = {L"help", L"?"};
        ULONG ArgLength = (ULONG)wcslen(Argv[1]);

        for (i = 0; i < ARRAY_SIZE(HelpCmds); i ++) {
            if (_wcsnicmp(Argv[1], HelpCmds[i], ArgLength) == 0) {

                *ComputerName = NULL;
                return FALSE;
            }
        }
    }

    return TRUE;
}


void
DisplayUsage()
{
    printf("\nParameter information --\n\n\n");
    printf("  <no parameter>\n\n");
    printf("    Run on local system with current identity.\n\n");
    printf("  <system name or IP address>\n\n");
    printf("    Run on specified system with current identity.\n\n");
    printf("  <system name or IP address> <UserId Password <DomainName>>\n\n");
    printf("    Run on specified system with specified UserId and Password and, possibly, DomainName.\n\n");
    printf("  help\n\n");
    printf("  ?\n\n");
}

