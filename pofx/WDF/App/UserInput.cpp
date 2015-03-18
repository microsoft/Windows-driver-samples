#include "include.h"


// 
// The supported settings with their default values.
//
CONFIGURATION g_UserSettings[] = {
    {COMPONENT           ,0},
    {MAX_OUTSTANDING_IO  ,1},
    {DELAY               ,0},
    {CANCEL              ,0}
};

DWORD
ProcessUserInput(
    _In_ int argc,
    _In_reads_(argc) PWSTR argv[]
    )
{
    DWORD err = ERROR_SUCCESS;

    for (int i=1; i < argc; i++)
    {
        PWSTR arg = argv[i];
        
        //
        // Setting must begin with / or -
        //
        if (arg[0] != L'-' && arg[0] != L'/')
        {
            printf("\n Invalid command-line argument %ws. \n", arg);
            err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if (L'?' == arg[1])
        {
            err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }
        
        //
        // Setting must contain ':' as the seperator.
        //  
        PWSTR settingName = arg + 1;
        PWSTR settingValue = NULL;
        PWSTR seperator = wcschr(settingName, L':');
        if (seperator == NULL)
        {
            printf("\n Invalid command-line argument %ws. \n", arg);            
            err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        settingValue = seperator + 1;            
        *seperator = L'\0';

        //
        // Store the value of the setting in the appropriate location.
        //
        err = ProcessSwitch(settingName, settingValue);
        if (err != ERROR_SUCCESS)
        {
            goto clean0;
        }     
    }
    
clean0:

    if (ERROR_SUCCESS != err)
    {
        PrintUsage(argv);
    }
    return err;
}


PCONFIGURATION
LookupSwitch(
    _In_ PWSTR Param)
{
    for (UINT i=0; i < COUNT_OF(g_UserSettings); i++)
    {
        if (_wcsicmp(Param, g_UserSettings[i].Option) == 0)
        {
            return &g_UserSettings[i];
        }
    }

    return NULL;
}

ULONG
GetSetting(
    _In_ PWSTR Switch)
{
    return LookupSwitch(Switch)->Value;
}

DWORD
ProcessSwitch(
    _In_ PWSTR Param,
    _In_ PWSTR Value)
{
    PCONFIGURATION config;

    config = LookupSwitch(Param);
    if (NULL == config)
    {
        printf("\n '%ws' is not a valid switch", Param);
        return ERROR_INVALID_PARAMETER;
    }
    
    if (0 == _wcsicmp(Param, COMPONENT))
    {
        if (0 == _wcsicmp(Value, L"*"))
        {
            config->Value = RANDOM_COMPONENT;
        }
        else
        {            
            config->Value = _wtoi(Value);
            if (config->Value >= COMPONENT_COUNT)
            {
                printf("Invalid component count '%ws' specified. "
                    "Component count must be less than %d. \n", Value, COMPONENT_COUNT);
                return ERROR_INVALID_PARAMETER;
            }            
        }

    }
    else if (0 == _wcsicmp(Param, CANCEL))
    {
        if (0 == _wcsicmp(Value, L"yes"))
        {
            config->Value = TRUE;
        }
        else if (0 == _wcsicmp(Value, L"no"))
        {
            config->Value = FALSE;
        }
        else
        {
            printf("'%ws' is not a valid option for '%ws'. Must be 'yes' or 'no'. \n", 
                Value, Param);
            return ERROR_INVALID_PARAMETER;            
        }
    }
    else
    {
        //
        // For MaxOutstandingIo or Delay this is sufficient.
        //
        config->Value = _wtoi(Value);        
        if (0 == config->Value)
        {
            printf("'%ws' is not a valid option for '%ws'", Value, Param);
            return ERROR_INVALID_PARAMETER;
        }
    }

    return ERROR_SUCCESS;
}


void PrintUsage(
    _In_ PWSTR argv[]
    )
{
    printf("\n This application can be used to send IO requests to a specified component");
    printf("\n of the WDF Power Fx sample driver. The requests will be sent indefinitely ");
    printf("\n until a request fails or the user terminates the execution with ^C");
    printf("\n More details on the driver and application are in the associated help file.");
    printf("\n ");    
    printf("\n Usage:");
    printf("\n %ws [/<SettingName>:<SettingValue> ...]", argv[0]);
    printf("\n ");
    printf("\n Available settings are:");
    printf("\n ");    
    printf("\n /Component:<Number>");
    printf("\n Where <Number> can be a specific component number, or if you are running with");
    printf("\n the multi-component sample driver you can specify '*'(without quotes) and ");
    printf("\n each request will be sent to a random component between 0 and ");
    printf("\n (COMPONENT_COUNT-1). Default value is component 0.");    
    printf("\n ");
    printf("\n /Delay:<DelayInMilliSeconds>");
    printf("\n There is an approximate delay between 0 and <DelayInMilliSeconds> ms");
    printf("\n between each request. Default value is 0 (no delay).");
    printf("\n ");
    printf("\n /MaxOutstandingIO:<NumberOfRequests>");
    printf("\n The maximum number of I/O requests that can be outstanding at any point.");
    printf("\n Default value is 1 (equivalent to synchronously sending requests).");
    printf("\n ");
    printf("\n /Cancel:<YesNo>");
    printf("\n Can be 'yes' or 'no' (without quotes).");
    printf("\n When set to 'yes' the application will attempt to cancel the requests once ");
    printf("\n sent. Default value is 'no'.");
    printf("\n ");
    return;
}

