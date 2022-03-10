/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:

    STRING.C

Abstract:

    This module contains the functions used to parse the PNP COM ID
    and save it in the appropriate UNICODE STRINGS.  The main function
    that is called is Serenum_ParseData.  All other functions are called
    by this main function.


Environment:

    kernel mode only

Notes:



--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Serenum_ParseData)

// Called by ParseData:
#pragma alloc_text (PAGE, Serenum_GetDevDesc)
#pragma alloc_text (PAGE, Serenum_GetDevCompId)
#pragma alloc_text (PAGE, Serenum_GetDevClass)
#pragma alloc_text (PAGE, Serenum_GetDevSerialNo)
#pragma alloc_text (PAGE, Serenum_GetDevName)
#pragma alloc_text (PAGE, Serenum_GetDevPnPRev)
#pragma alloc_text (PAGE, Serenum_GetDevOtherID)

#pragma alloc_text (PAGE, Serenum_InitMultiString)
#pragma alloc_text (PAGE, Serenum_SzCopy)
#pragma alloc_text (PAGE, Serenum_StrLen)

// Called by the above functions:
#pragma alloc_text (PAGE, Serenum_FixptToAscii)
#pragma alloc_text (PAGE, Serenum_HToI)

#endif

NTSTATUS
Serenum_ParseData(        PFDO_DEVICE_DATA FdoData, 
   _In_reads_(BufferLen) PCHAR ReadBuffer, ULONG BufferLen,
                          PUNICODE_STRING hardwareIDs, PUNICODE_STRING compIDs,
                          PUNICODE_STRING deviceIDs, PUNICODE_STRING PDeviceDesc,
                          PUNICODE_STRING serialNo, PUNICODE_STRING pnpRev
                          )
/*++

Routine Description:
    Parses the PNP COM ID out of the buffer which is passed as the
    first parameter and then saves the appropriate IDs as
    UNICODE_STRINGS in the other passed parameters.

Return value:
    NTSTATUS

--*/

{
   LPSTR pOtherId = NULL;
   LPSTR pPnpRev = NULL;
   LPSTR pDevNodeName = NULL;
   LPSTR pSerNo = NULL;
   LPSTR pClass = NULL;
   LPSTR pCompIdStar = NULL;
   LPSTR pDesc = NULL;
   PCHAR pStrBuffer = NULL;

   NTSTATUS status = STATUS_SUCCESS;

   LPSTR pDevName = NULL;
   LPSTR pCompId = NULL;

   int start;

   BOOLEAN isMouse = FALSE;
   PCHAR pMouseID = NULL;
   ULONG mouseIDLen;

   UNREFERENCED_PARAMETER(BufferLen);
   PAGED_CODE();
   
   //
   // Allocate the string buffers
   //

   pStrBuffer = ExAllocatePoolZero(PagedPool, MAX_DEVNODE_NAME * 7 + 1,SERENUM_POOL_TAG);

   if (pStrBuffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto DoneParsingErr;
   } else {
      PCHAR pCurBuffer = pStrBuffer;

      pOtherId = pCurBuffer;
      *pOtherId = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;

      pPnpRev = pCurBuffer;
      *pPnpRev = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;

      pDevNodeName = pCurBuffer;
      *pDevNodeName = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;

      pSerNo = pCurBuffer;
      *pSerNo = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;

      pClass = pCurBuffer;
      *pClass = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;

      pCompIdStar = pCurBuffer;
      pCompId = pCompIdStar + 1;
      *pCompIdStar = '\0';
      pCurBuffer += MAX_DEVNODE_NAME + 1;

      pDesc = pCurBuffer;
      *pDesc = '\0';
      pCurBuffer += MAX_DEVNODE_NAME;
   }

   start = Serenum_SzCopy ( NAME_SERENUM, pDevNodeName, MAX_DEVNODE_NAME);
   pDevName = pDevNodeName + start;

   start = 0;

   RtlInitUnicodeString(hardwareIDs, NULL);
   RtlInitUnicodeString(compIDs, NULL);
   RtlInitUnicodeString(deviceIDs, NULL);
   RtlInitUnicodeString(pnpRev, NULL);
   RtlInitUnicodeString(serialNo, NULL);


   //
   // OtherID
   //

   if(ReadBuffer!=NULL&&BufferLen>=MAX_DEVNODE_NAME){
       start = Serenum_GetDevOtherID(ReadBuffer, BufferLen, pOtherId);
   }

   if (start > 16||start==0) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                      ("Other ID string too long\n"));
      status = STATUS_UNSUCCESSFUL;
      goto DoneParsingErr;
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Other ID: %s\n", pOtherId));

   //
   // See if this is a mouse
   //

   SerenumScanOtherIdForMouse(ReadBuffer, BufferLen, &pMouseID, &mouseIDLen );

   if (pMouseID != NULL && (*pMouseID == 'M' || *pMouseID == 'B')) {
      isMouse = TRUE;
   }

   //
   // PNP revision number
   //

   status = Serenum_GetDevPnPRev(FdoData, ReadBuffer, BufferLen, pPnpRev, &start);

   if (!NT_SUCCESS(status)) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("PNP ID string bad\n"));
      goto DoneParsingErr;
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("PNP Revision: %s\n", pPnpRev));

   //
   // PNP device node name
   // EISA ID followed by Product ID
   //

   Serenum_GetDevName(ReadBuffer, BufferLen, pDevName, &start);
   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Device Node name: %s\n",
                                               pDevNodeName));

   //
   // Device serial number
   //
   if(ReadBuffer!=NULL){
       Serenum_GetDevSerialNo(ReadBuffer, BufferLen, pSerNo, &start);
   }

   if (Serenum_StrLen(pSerNo)) {
      //
      // This field exists - Make sure it is correct length.
      //

      if (Serenum_StrLen(pSerNo) != 8) {
         Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Serial number wrong"
                                                     " length\n"));
         *pSerNo = '\0';
         status = STATUS_UNSUCCESSFUL;
         goto DoneParsingErr;
      }
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Serial Number: %s\n", pSerNo));

   //
   // PNP class identifier
   //
   if(ReadBuffer!=NULL){
       Serenum_GetDevClass(ReadBuffer, BufferLen, pClass, &start);
   }

   if (Serenum_StrLen(pClass) > 32) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Class ID string too long\n"
                                                 ));
      status = STATUS_UNSUCCESSFUL;
      goto DoneParsingErr;
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Class: %s\n", pClass));

   if (_stricmp(pClass, "MOUSE") == 0) {
       RtlStringCchCopyA(pClass, MAX_DEVNODE_NAME, "SERIAL_MOUSE");
   }

   //
   // Compatible device ID
   //
   *pCompIdStar = '*';
   if(ReadBuffer!=NULL&&pCompId!=NULL){
       Serenum_GetDevCompId(ReadBuffer, BufferLen, pCompId, &start);
   }

   if (Serenum_StrLen(pCompId) > 40) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Compatible driver ID"
                                                  " string too long\n"));
      status = STATUS_UNSUCCESSFUL;
      goto DoneParsingErr;
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Compatible driver ID: %s\n",
                                               pCompId));

   //
   // End-user legible Product Description
   //
   if(ReadBuffer!=NULL){
       Serenum_GetDevDesc (ReadBuffer, BufferLen, pDesc, &start);
   }

   if (Serenum_StrLen(pDesc) > 40) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Device Description too"
                                                  " long\n"));
      status = STATUS_UNSUCCESSFUL;
      goto DoneParsingErr;
   }
   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Device Description: %s\n",
                                               pDesc));

   DoneParsingErr:
   if (pStrBuffer != NULL) {
      //
      // send back the good bits so that routine knows what driver to load
      //

      Serenum_InitMultiString (FdoData, hardwareIDs, pDevNodeName, pDevName,
                               NULL);

      if (Serenum_StrLen(pCompId) > 0) {
         if (!isMouse) {
            Serenum_InitMultiString(FdoData, compIDs, pCompIdStar, pClass,
                                    NULL);
         } else {
            Serenum_InitMultiString(FdoData, compIDs, pCompIdStar, pClass,
                                    "SERIAL_MOUSE", NULL);
         }
      } else {
         if (isMouse) {
            Serenum_InitMultiString(FdoData, compIDs, "SERIAL_MOUSE", NULL);
         }
      }

      Serenum_InitMultiString(FdoData, deviceIDs, pDevNodeName, NULL);

      Serenum_InitMultiString(FdoData, PDeviceDesc, pDesc, NULL);
      
      if (Serenum_StrLen(pSerNo)) {
         Serenum_InitMultiString(FdoData, serialNo, pSerNo, NULL);
      }
      if (Serenum_StrLen(pPnpRev)) {
         Serenum_InitMultiString(FdoData, pnpRev, pPnpRev, NULL);
      }

      ExFreePoolWithTag(pStrBuffer,SERENUM_POOL_TAG);
   }

   return status;
}

NTSTATUS
Serenum_InitMultiString(PFDO_DEVICE_DATA FdoData, PUNICODE_STRING MultiString,
                        ...)
/*++

    This routine will take a null terminated list of ascii strings and combine
    them together to generate a unicode multi-string block

Arguments:

    MultiString - a unicode structure in which a multi-string will be built
    ...         - a null terminated list of narrow strings which will be
             combined together. This list must contain at least a
        trailing NULL

Return Value:

    NTSTATUS

--*/
{
   ANSI_STRING ansiString;
   NTSTATUS status;
   PCSTR rawString;
   ULONG multiLength = 0;
   UNICODE_STRING unicodeString;
   va_list ap;
   ULONG i;

   PAGED_CODE();

#if !DBG
   UNREFERENCED_PARAMETER(FdoData);
#endif


   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Entering Serenum_InitMultiString\n"));

   va_start(ap,MultiString);

   //
   // Make sure that we won't leak memory
   //

   ASSERT(MultiString->Buffer == NULL);

   rawString = va_arg(ap, PCSTR);

   while (rawString != NULL) {
      RtlInitAnsiString(&ansiString, rawString);
      multiLength += RtlAnsiStringToUnicodeSize(&(ansiString));
      rawString = va_arg(ap, PCSTR);
   }

   va_end( ap );

   if (multiLength == 0) {
      //
      // Done
      //
      RtlInitUnicodeString(MultiString, NULL);
      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Leaving Serenum_InitMultiString (1)\n"));

      return STATUS_SUCCESS;
   }

   //
   // We need an extra null
   //
   multiLength += sizeof(WCHAR);

   MultiString->MaximumLength = (USHORT)multiLength;
   MultiString->Buffer = ExAllocatePoolZero(PagedPool, multiLength,SERENUM_POOL_TAG);
   MultiString->Length = 0;

   if (MultiString->Buffer == NULL) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Leaving Serenum_InitMultiString (2)\n"));

      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Allocated %lu bytes for buffer\n", multiLength));

#if DBG
   RtlFillMemory(MultiString->Buffer, multiLength, 0xff);
#endif

   unicodeString.Buffer = MultiString->Buffer;
   unicodeString.MaximumLength = (USHORT) multiLength;
   unicodeString.Length = 0;

   va_start(ap, MultiString);
   rawString = va_arg(ap, PCSTR);

   while (rawString != NULL) {

      RtlInitAnsiString(&ansiString,rawString);
      status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);
      if (!NT_SUCCESS(status)) {
        Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Serenum_InitMultiString: Call to RtlAnsiStringToUnicodeString Failed.\n"));
        return status;        
      }

      //
      // We don't allocate memory, so if something goes wrong here,
      // its the function that's at fault
      //
      ASSERT(NT_SUCCESS(status));

      //
      // Check for any commas and replace them with NULLs
      //

      ASSERT(unicodeString.Length % sizeof(WCHAR) == 0);

      for (i = 0; i < (unicodeString.Length / sizeof(WCHAR)); i++) {
         if (unicodeString.Buffer[i] == L'\x2C' ||
             unicodeString.Buffer[i] == L'\x0C' ) {
            unicodeString.Buffer[i] = L'\0';
         }
      }

      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("unicode buffer: %ws\n",
                                                  unicodeString.Buffer));

      //
      // Move the buffers along
      //
      unicodeString.Buffer += ((unicodeString.Length / sizeof(WCHAR)) + 1);
      unicodeString.MaximumLength -= (unicodeString.Length + sizeof(WCHAR));
      unicodeString.Length = 0;

      //
      // Next
      //

      rawString = va_arg(ap, PCSTR);
   } // while

   va_end(ap);

   ASSERT(unicodeString.MaximumLength == sizeof(WCHAR));

   //
   // Stick the final null there
   //

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("unicode buffer last addr: "
                                               "%p\n", unicodeString.Buffer));

   unicodeString.Buffer[0] = L'\0';

   //
   // Include the nulls in the length of the string
   //

   MultiString->Length = (USHORT)multiLength;
   MultiString->MaximumLength = MultiString->Length;

   Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                   ("Leaving Serenum_InitMultiString (3)\n"));

   return STATUS_SUCCESS;
}

int
Serenum_StrLen (
    _In_ LPSTR string)
// Measures the length of a string
{
    int i;
    PAGED_CODE();
    
    if (string == NULL) {
        return 0;
    }
    for (i=0; string[i] != '\0'; i++) {
    }
    return i;
}

int
Serenum_SzCopy (
    _In_              LPSTR source,
    _Out_writes_(len) PCHAR dest,
                      int len
    )
// Copies a string
// Assumes the buffer is already allocated to be copied into
{
    int i;

    PAGED_CODE();
    
    ASSERT (source);
    ASSERT (dest);

    for (i=0; source[i] != '\0' && i < len; i++) {
        *dest++ = source[i];
    }
    return i;
}

//
// String extraction functions:
//
int
Serenum_GetDevOtherID(
    _In_reads_(len)               PCHAR input,
                                  int   len,
     _Out_writes_(MAX_DEVNODE_NAME)LPSTR output
    )
{
    int tail;
    CHAR c;

    PAGED_CODE();
    
    tail = 0;
    c = input[tail++];
    while((tail < 17 ) && (c != '(') && (c != '(' - 0x20) && tail < len ) {
            *output++ = c;
#pragma warning(suppress: 6385)
            c = input[tail++];
    }
    *output = '\0';
    return(tail-1);
}


/****************************************************************************
 *
 *
 ***************************************************************************/
int
Serenum_HToI(char c)
{
    PAGED_CODE();
    
    if('0' <= c  &&  c <= '9')
    return(c - '0');

    if('A' <= c  &&  c <= 'F')
    return(c - 'A' + 10);

    if('a' <= c  &&  c <= 'f')
    return(c - 'a' + 10);

    return(-1);
}

void
Serenum_FixptToAscii(
    int n, 
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output)
/****************************************************************************
 *
 *
 ***************************************************************************/
{
    int tmp;

    PAGED_CODE();
    
    tmp = n / 100;

    if(tmp >= 10)
        *output++ = (CHAR)('0' + (tmp / 10));

    *output++ = (CHAR)('0' + (tmp % 10));
    *output++ = '.';

    tmp = n % 100;

    *output++ = (CHAR)('0' + (tmp / 10));
    *output++ = (CHAR)('0' + (tmp % 10));

    *output = '\0';
}

/****************************************************************************
 *
 *
 ***************************************************************************/
NTSTATUS
Serenum_GetDevPnPRev(              
                                   PFDO_DEVICE_DATA FdoData, 
    _In_reads_(len)               PCHAR input,  
                                   int len,
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output,
    _Inout_                        int   *start
    )
{
   int tail;
   int i;
   char delta;
   char c, begin_PnP, end_PnP_pos;
   int sum, chk_sum, msd, lsd;

   UNREFERENCED_PARAMETER(FdoData);
   PAGED_CODE();

   if (output == NULL  ||  input == NULL) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                      ("GetDevPnPRev Failed, NULL pointer!\n"));
      return STATUS_UNSUCCESSFUL;
   }
   __analysis_assume(len>=MAX_DEVNODE_NAME);
   *output = '\0';

   tail = *start;

   if ( tail >= len ) { 
       Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                       ("GetDevPnPRev Failed, insufficient input buffer\n"));
       return STATUS_UNSUCCESSFUL;
   }

   if (input[tail] == 0) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                      ("GetDevPnPRev Failed, input buffer empty!\n"));
      return STATUS_UNSUCCESSFUL;
   }
   
   c = input[tail++];

   while ((tail < 256) && (c != '(') && (c != '(' - 0x20) && tail < len) {
      c = input[tail++];
   }

   if (c != '('  &&  c != '(' - 0x20) {
      Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                      ("GetDevPnPRev Failed, no Begin PnP char!\n"));
      return STATUS_UNSUCCESSFUL;
   }

   begin_PnP = c;
   delta = '(' - begin_PnP;

   if ( tail+9 >= len ) {
       Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                       ("GetDevPnPRev Failed, insufficient input buffer\n"));
       return STATUS_UNSUCCESSFUL;
   } 

#pragma warning(suppress: 6385)
   if ( input[tail + 9] != ')' - delta) {

      //
      // compute checksum
      //

      sum = c;
      i = tail;
      while ( (i < 256)  &&  (c !=  ( ')' - delta)) && i < len ) {
         c = input[i++];
         sum += c;
      }

      if ( i < 3 || i-2 >= len) {
          Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                          ("GetDevPnPRev Failed, insufficient input buffer\n"));
          return STATUS_UNSUCCESSFUL;
      } 
      
      msd = input[i-3];
      lsd = input[i-2];
      
      sum -= msd;
      sum -= lsd;

      msd += delta;
      lsd += delta;

      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("checksum from device  (chars) = %c%c\n", (char)msd,
                       (char)lsd));

      msd = Serenum_HToI((char)msd);
      if (msd < 0) {
         Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                         ("Bad msd checksum digit\n"));
         return STATUS_UNSUCCESSFUL;
      }

      lsd = Serenum_HToI((char)lsd);
      if (lsd < 0) {
         Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                         ("Bad lsd checksum digit\n"));
         return STATUS_UNSUCCESSFUL;
      }

      chk_sum = (msd << 4) + lsd;

      sum &= 0xff;

      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("checksum read from device = %0x\n", chk_sum));
      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE,
                      ("Computed checksum = %0x\n", sum));

      end_PnP_pos = (char)i;

      if ( c - begin_PnP  !=  ')' - '(' ) {
         Serenum_KdPrint(FdoData,  SER_DBG_SS_ERROR,
                         ("GetDevPnPRev Failed,BeginPnP didn't match "
                          "EndPnP\n"));
         Serenum_KdPrint(FdoData,  SER_DBG_SS_ERROR,
                         ("begin_PnP = %02x   end_PnP = %02x\n", begin_PnP, c));
         return STATUS_UNSUCCESSFUL;
      }

      //
      // check the checksum
      //

      if (chk_sum != sum) {
         Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                         ("checksum Failed! Continuing...\n"));
         //          return STATUS_UNSUCCESSFUL;     // Commented out in Memphis
      }

      i = end_PnP_pos;

      if ( i < 3 || i-2 >= len) {
          Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                          ("GetDevPnPRev Failed, insufficient input buffer\n"));
          return STATUS_UNSUCCESSFUL;
      } 
      input[i-3] = ')' - delta;       // trash the checksum
      input[i-2] = '\0';              // since we are done with it
   }

   if ( tail+1 >= len ) {
       Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR,
                       ("GetDevPnPRev Failed, insufficient input buffer\n"));
       return STATUS_UNSUCCESSFUL;
   } 
   
   if (input[tail] > 0x3f ||
       input[tail+1] > 0x3f) {

      Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Bad PnP Rev digits\n"));
      return STATUS_UNSUCCESSFUL;
   }

   i = (input[tail++] & 0x3f) << 6;
   i |= (input[tail++]) & 0x3f;

   Serenum_FixptToAscii(i, output);

   i = tail;

   //
   // get ride of Mouse'output 0x20 bias in the string
   //
   while ( (i < 256) && delta && i < len ) {
      input[i] += delta;
      c = input[i++];
      if ( c == ')' ) {
         delta = 0;    // indicate we are done
      }
   }

   *start = tail;

   return STATUS_SUCCESS;
}

/****************************************************************************
 *
 *
 ***************************************************************************/
void Serenum_GetDevName(
    _In_reads_(len)          PCHAR input,
                              int   len,
    _Out_writes_(MAX_DEVNAME) PCHAR output,
    _Inout_                   int  *start
    )
{

    int tail;
    char c;

    PAGED_CODE();
    
    if(output == NULL  ||  input == NULL)
        return;

    tail = *start;

    // EISA ID
    if ( tail >= len ) { *start = tail; goto Exit; }
    *output++ = input[tail++];
    if ( tail >= len ) { *start = tail; goto Exit; }
    *output++ = input[tail++];
    if ( tail >= len ) { *start = tail; goto Exit; }
    *output++ = input[tail++];

    // Product ID

    if ( tail >= len ) { *start = tail; goto Exit; }
    c = input[tail++];
    if(Serenum_HToI(c) >= 0)
        *output++ = c;

    if ( tail >= len ) { *start = tail; goto Exit; }
    c = input[tail++];
    if(Serenum_HToI(c) >= 0)
        *output++ = c;

    if ( tail >= len ) { *start = tail; goto Exit; }
    c = input[tail++];
    if(Serenum_HToI(c) >= 0)
        *output++ = c;

    if ( tail >= len ) { *start = tail; goto Exit; }
    c = input[tail++];
    if(Serenum_HToI(c) >= 0)
        *output++ = c;

    Exit:
    *output = '\0';

    *start = tail;

    return;
}

/****************************************************************************
 *
 *
 ***************************************************************************/
void Serenum_GetDevSerialNo(
    _In_reads_(len)               PCHAR input,
                                   int   len,
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output,
    _Inout_                        int *start
    )
{

    int tail, cnt;
    char c;

    PAGED_CODE();

	*output = '\0';

    tail = *start;

    if ( tail >= len ) { return; }
    
    if( input[tail++] != '\\')
        return;

    if ( tail >= len ) { *start = tail-1; return; }
    c = input[tail++];

    cnt = 0;

    while(cnt < 8 && tail < 256 && ( c != '\\') && ( c != ')') && tail < len ) {
        cnt++;
        if(Serenum_HToI(c) < 0)
            break;

        *output++ = c;
        c = input[tail++];
    }

    *output = '\0';

    *start = tail - 1;

    return;
}

/****************************************************************************
 *
 *
 ***************************************************************************/
void Serenum_GetDevClass(
    _In_reads_(len)               PCHAR input,
                                   int   len,
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output,
    _Inout_                        int   *start
    )
{

    int tail, cnt;
    char c;

    PAGED_CODE();
    
    *output = '\0';

    tail = *start;

    if ( tail >= len ) { return; }
    
    if( input[tail++] != '\\')
        return;

    if ( tail >= len ) { *start = tail-1; return; }
    c = input[tail++];

    cnt = 0;

    while(tail < 256 && ( c != '\\') && ( c != ')') && tail < len && cnt < MAX_DEVNODE_NAME - 1) {
        cnt++;
        *output++ = c;
        c = input[tail++];
    }
    *output = '\0';

    *start = tail - 1;

    return;
}


void Serenum_GetDevCompId(
    _In_reads_(len)               PCHAR input,
                                   int   len,
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output,
    _Inout_                        int   *start
    )
/****************************************************************************
 *
 *
 ***************************************************************************/
{

    int tail, cnt;
    char c;

    PAGED_CODE();

    *output = '\0';

    tail = *start;

    if ( tail >= len ) { return; }

    if( input[tail++] != '\\')
        return;

    if ( tail >= len ) { *start = tail-1; return; }
    c = input[tail++];

    cnt = 0;

    while(tail < 256 && ( c != '\\') && ( c != ')') && tail < len && cnt < MAX_DEVNODE_NAME - 2 ) {
        cnt++;
        *output++ = c;
        //
        // Put a * after every comma
        //
        if ('\x0C' == c || '\x2C' == c) {
            cnt++;
            *output++ = '*';
        }
        c = input[tail++];
    }

    *output = '\0';

    *start = tail - 1;
}

void
Serenum_GetDevDesc(
    _In_reads_(len)               PCHAR input,
                                   int   len,
    _Out_writes_(MAX_DEVNODE_NAME) LPSTR output,
    _Inout_                        int   *start
    )
/****************************************************************************
 *
 *
 ***************************************************************************/
{

    int tail, cnt;
    char c;

    PAGED_CODE();

    *output = '\0';

    tail = *start;

    if ( tail >= len ) { return; }
    
    if( input[tail++] != '\\')
        return;

    if ( tail >= len ) { *start = tail-1; return; }
    c = input[tail++];

    cnt = 0;

    while(tail < 256 && ( c != '\\') && ( c != ')')  && tail < len && cnt < MAX_DEVNODE_NAME - 1) {
        cnt++;
        *output++ = c;    
        c = input[tail++];
    }

    *output = '\0';

    *start = tail - 1;
}


