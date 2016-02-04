#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "GeneralFunc.tmh"
#endif

BOOLEAN 
eqNByte(
	pu1Byte	str1,
	pu1Byte	str2,
	u4Byte	num
	)
{
	if(num==0)
		return FALSE;
	while(num>0)
	{
		num--;
		if(str1[num]!=str2[num])
			return FALSE;
	}
	return TRUE;
}


//
//	Description:
//		Return TRUE if chTmp is represent for hex digit and 
//		FALSE otherwise.
//
//
BOOLEAN
IsHexDigit(
	IN		s1Byte		chTmp
)
{
	if( (chTmp >= '0' && chTmp <= '9') ||  
		(chTmp >= 'a' && chTmp <= 'f') ||
		(chTmp >= 'A' && chTmp <= 'F') )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//
//	Description:
//		Translate a character to hex digit.
//
u4Byte
MapCharToHexDigit(
	IN		s1Byte		chTmp
)
{
	if(chTmp >= '0' && chTmp <= '9')
		return (chTmp - '0');
	else if(chTmp >= 'a' && chTmp <= 'f')
		return (10 + (chTmp - 'a'));
	else if(chTmp >= 'A' && chTmp <= 'F') 
		return (10 + (chTmp - 'A'));
	else
		return 0;	
}



//
//	Description:
//		Parse hex number from the string pucStr.
//
BOOLEAN 
GetHexValueFromString(
	IN		ps1Byte			szStr,
	IN OUT	pu4Byte			pu4bVal,
	IN OUT	pu4Byte			pu4bMove
)
{
	ps1Byte		szScan = szStr;

	// Check input parameter.
	if(szStr == NULL || pu4bVal == NULL || pu4bMove == NULL)
	{
		RT_TRACE(COMP_DBG, DBG_WARNING, 
			("GetHexValueFromString(): Invalid inpur argumetns! szStr: %p, pu4bVal: %p, pu4bMove: %p\n", szStr, pu4bVal, pu4bMove));
		return FALSE;
	}

	// Initialize output.
	*pu4bMove = 0;
	*pu4bVal = 0;

	// Skip leading space.
	while(	*szScan != '\0' && 
			(*szScan == ' ' || *szScan == '\t') )
	{
		szScan++;
		(*pu4bMove)++;
	}

	// Skip leading '0x' or '0X'.
	if(*szScan == '0' && (*(szScan+1) == 'x' || *(szScan+1) == 'X'))
	{
		szScan += 2;
		(*pu4bMove) += 2;
	}	

	// Check if szScan is now pointer to a character for hex digit, 
	// if not, it means this is not a valid hex number.
	if(!IsHexDigit(*szScan))
	{
		return FALSE;
	}

	// Parse each digit.
	do
	{
		(*pu4bVal) <<= 4;
		*pu4bVal += MapCharToHexDigit(*szScan);

		szScan++;
		(*pu4bMove)++;
	} while(IsHexDigit(*szScan));

	return TRUE;
}

BOOLEAN 
GetFractionValueFromString(
	IN		ps1Byte			szStr,
	IN OUT	pu1Byte			pInteger,
	IN OUT  pu1Byte			pFraction,
	IN OUT	pu4Byte			pu4bMove
)
{
	ps1Byte		szScan = szStr;

	// Initialize output.
	*pu4bMove = 0;
	*pInteger = 0;
	*pFraction = 0;

	// Skip leading space.
	while (	*szScan != '\0' && 	(*szScan == ' ' || *szScan == '\t') ) {
		++szScan;
		++(*pu4bMove);
	}

	// Parse each digit.
	do {
		(*pInteger) *= 10;
		*pInteger += ( *szScan - '0' );

		++szScan;
		++(*pu4bMove);

		if ( *szScan == '.' ) 
		{
			++szScan;
			++(*pu4bMove);
			
			if ( *szScan < '0' || *szScan > '9' )
				return FALSE;
			else {
				*pFraction = *szScan - '0';
				++szScan;
				++(*pu4bMove);
				return TRUE;
			}
		}
	} while(*szScan >= '0' && *szScan <= '9');

	return TRUE;
}


//
//	Description:
//		Return TRUE if szStr is comment out with leading "//".
//
BOOLEAN
IsCommentString(
	IN		ps1Byte			szStr
)
{
	if(*szStr == '/' && *(szStr+1) == '/')
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// <20121004, Kordan> For example, 
// ParseQualifiedString(inString, 0, outString, '[', ']') gets "Kordan" from a string "Hello [Kordan]".
// If RightQualifier does not exist, it will hang on in the while loop
BOOLEAN 
ParseQualifiedString(
    IN      ps1Byte 		In, 
    IN OUT  pu4Byte 		Start, 
    OUT     ps1Byte 		Out,
    OUT     const u4Byte 	MaxOutLen,    
    IN      s1Byte  		LeftQualifier, 
    IN      s1Byte  		RightQualifier
    )
{
    u4Byte  i = 0, j = 0;
	u4Byte  origin = *Start;
    s1Byte  c = In[(*Start)++];
	BOOLEAN status = TRUE;

    if (c != LeftQualifier)
        status =  FALSE;

    i = (*Start);

	while ((c = In[(*Start)++]) != RightQualifier) 
		; // Keep going until the RightQualifier is found.

	j = (*Start) - 2;

	if (j - i + 1 >= MaxOutLen)
	{
		status = FALSE;
	}
	else
	{
		//strncpy((char *)Out, (const char*)(In+i), j-i+1);
		//Prefast warning C28719: Banned API Usage: strncpy is a Banned API as listed in dontuse.h for security purposes
		strncpy_s((char *)Out, j - i + 1, (const char*)(In + i), j - i + 1);
	}

	if (status == FALSE)
		*Start = origin; // Reset the position
	
    return status;
}

BOOLEAN
GetU1ByteIntegerFromStringInDecimal(
	IN		ps1Byte Str,
	IN OUT	pu1Byte pInt
	)
{
	u2Byte i = 0;
	*pInt = 0;

	while ( Str[i] != '\0' )
	{
		if ( Str[i] >= '0' && Str[i] <= '9' )
		{
			*pInt *= 10;
			*pInt += ( Str[i] - '0' );
		}
		else
		{
			return FALSE;
		}
		++i;
	}

	return TRUE;
}


BOOLEAN
GetS1ByteIntegerFromStringInDecimal(
	IN		ps1Byte Str,
	IN OUT	ps1Byte pInt
	)
{
	u2Byte i = 0;
	s1Byte Sign = 1; // Positive
	
	*pInt = 0;
	
	if( Str[i] == '-' )
	{
		Sign = -1; // Negative
		i++;
	}

	while ( Str[i] != '\0' )
	{
		if ( Str[i] >= '0' && Str[i] <= '9' )
		{
			*pInt *= 10;
			*pInt += ( Str[i] - '0' );
		}
		else
		{
			return FALSE;
		}
		++i;
	}
	*pInt *= Sign; // Apply +/- sign

	return TRUE;
}

BOOLEAN
isAllSpaceOrTab(
	pu1Byte data,
	u1Byte	size
	)
{
	u1Byte		cnt = 0, NumOfSpaceAndTab = 0;
	while( size > cnt )
	{
		if ( data[cnt] == ' ' || data[cnt] == '\t' || data[cnt] == '\0' )
			++NumOfSpaceAndTab;

		++cnt;
	}

	return size == NumOfSpaceAndTab;
}

//
// Description:
//	Generate a unique tag for debug
//
// For example:
//
//	If pFunName is "abcde", we will generate tag by function length, first character and last character.
//
//	genTag will return "50ea"
//
//	Windbg will show "ae05"
//
u4Byte
GenTag(
	IN	char	*pFunName
	)
{
	u2Byte	units, tens = 0;
	u4Byte	tag;
	
	units = (u2Byte)strlen(pFunName);
	tag = pFunName[0] | (pFunName[units-1] << 8);
	
	while(units >= 10)
	{
		units -= 10;
		tens ++;
	}
	tag |= ((tens+0x30) << 16) | ((units+0x30) << 24);

	return tag;
}

