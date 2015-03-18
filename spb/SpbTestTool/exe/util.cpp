/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    util.cpp

Abstract:

    This module contains the internal helper function definitions
    for the SpbTestTool app.

Environment:

    user-mode

Revision History:

--*/

#include "internal.h"

_Success_(return)
bool
PopStringParameter(
    _Inout_   list<string> *Parameters,
    _Out_     string       *Value,
    _Out_opt_ bool         *Present
    )
{
    if (Parameters->empty())
    {
        if (Present != nullptr)
        {
            *Present = false;
            *Value = string("");
            return true;
        }
        else
        {
            printf("Missing required parameter\n");
            return false;
        }
    }

    if (Present != nullptr)
    {
        *Present = true;
    }
    
    *Value = Parameters->front();
    Parameters->pop_front();
    return true;
}

_Success_(return)
bool
ParseNumber(
    _In_     const string &String,
    _In_     ULONG         Radix,
    _Out_    ULONG        *Value,
    _In_opt_ bounds        Bounds
    )
{
    PSTR end;
    
#pragma prefast(suppress:__WARNING_MISSING_ZERO_TERMINATION2 ,"zero-termination is checked below")
    *Value = strtoul(String.c_str(), &end, Radix);

    //
    // Make sure the entire string parsed.
    //

    if (*end != '\0')
    {
        printf("Value %s is not a number\n", String.c_str());
        return false;
    }

    //
    // See if we should do a bounds check.
    //

    if ((Bounds.first != 0) || (Bounds.second != 0))
    {
        if ((*Value < Bounds.first) || 
            (*Value > Bounds.second))
        {
            printf("Value %s is out of bounds\n", String.c_str());
            return false;
        }
    }

    return true;
}

_Success_(return)
bool
PopNumberParameter(
    _Inout_   list<string> *Parameters,
    _In_      ULONG         Radix,
    _Out_     ULONG        *Value,
    _In_opt_  bounds        Bounds,
    _Out_opt_ bool         *Present
    )
{
    string s;

    bool found = PopStringParameter(Parameters, &s, Present);
 
    if ((Present != nullptr) && (*Present == false))
    {
        *Value = Bounds.first;
        return true;
    }
    else if (found == false)
    {
        return false;
    }

    return ParseNumber(s, Radix, Value, Bounds);
}

_Success_(return)
bool
PopBufferParameter(
    _Inout_ list<string>       *Parameters,
    _Out_   pair<ULONG, PBYTE> *Value
    )
{
    ULONG length = 0;
    PBYTE buffer = nullptr;

    //
    // Check for an explict length
    //

    if (Parameters->front() != "{")
    {
        if (PopNumberParameter(Parameters, 10, &length) == false)
        {
            printf("Length expected\n");
            return false;
        }

        //
        // Consume any leading {
        //

        if ((Parameters->empty() == false) && 
            (Parameters->front() == "{"))
        {
            Parameters->pop_front();
        }
    } 
    else 
    {
        string tmp;
        if (PopStringParameter(Parameters, &tmp) == false)
        {
            return false;
        }
        else if (tmp != "{")
        {
            printf("output buffer must start with {\n");
            return false;
        }

        //
        // Count values until the trailing } - assume one byte per value
        //

        list<string>::iterator i;

        for(i = Parameters->begin(); 
            ((i != Parameters->end()) && (*i != "}")); 
            i++)
        {
            length += 1;
        }

        if (*i != "}")
        {
            printf("output buffer must end with }\n");
            return false;
        }
    }
    
    if (length != 0)
    {
        ULONG b = 0;
        bool bufferEnd = false;

        //
        // Allocate the buffer.
        //

        buffer = new BYTE[length];

        for(b = 0; b < length; b += 1)
        {
            ULONG value = 0;

            if (bufferEnd == false) 
            {
                string nextElement;
               
                PopStringParameter(Parameters, &nextElement);

                if (nextElement == "}")
                {
                    value = 0;
                    bufferEnd = true;
                }
                else if (ParseNumber(nextElement, 16, &value, bounds(0, 0xff)) == false)
                {
                    printf("invalid byte value %s\n", nextElement.c_str());
                    delete [] buffer;
                    return false;
                }
            }

            buffer[b] = (BYTE) value;
        }

        if (bufferEnd == false)
        {
            string end;
           
            if (PopStringParameter(Parameters, &end) == false)
            {
                printf("unclosed buffer\n");
                delete [] buffer;
                return false;
            }
            else if (end != "}")
            {
                printf("buffer has too many initializers\n");
                delete [] buffer;
                return false;
            }
        }
    }

    Value->first = length;
    Value->second = buffer;

    return true;
}
