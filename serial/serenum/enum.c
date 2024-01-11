/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    ENUM.C

Abstract:

    This module contains the enumeration code needed to figure out
    whether or not a device is attached to the serial port.  If there
    is one, it will obtain the PNP COM ID (if the device is PNP) and
    parse out the relevant fields.


Environment:

    kernel mode only

Notes:


--*/


#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESENM, SerenumValidateID)
#pragma alloc_text(PAGESENM, SerenumDoEnumProtocol)
#pragma alloc_text(PAGESENM, SerenumCheckForLegacyDevice)
#pragma alloc_text(PAGESENM, SerenumScanOtherIdForMouse)
#pragma alloc_text(PAGESENM, Serenum_ReenumerateDevices)
#pragma alloc_text(PAGESENM, Serenum_IoSyncReq)
#pragma alloc_text(PAGESENM, Serenum_IoSyncReqWithIrp)
#pragma alloc_text(PAGESENM, Serenum_IoSyncIoctlEx)
#pragma alloc_text(PAGESENM, Serenum_ReadSerialPort)
#pragma alloc_text(PAGESENM, Serenum_Wait)
#pragma alloc_text(PAGESENM, SerenumReleaseThreadReference)

//#pragma alloc_text (PAGE, Serenum_GetRegistryKeyValue)
#endif

#if !defined(__isascii)
#define __isascii(_c)   ( (unsigned)(_c) < 0x80 )
#endif // !defined(__isascii)

// disable warnings

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4127) // conditional expression is constant

void
SerenumScanOtherIdForMouse( 
    _In_reads_(BufLen)                  PCHAR PBuffer, 
    _In_ ULONG                           BufLen,
    _Outptr_result_buffer_maybenull_(*PmouseIdLen) PCHAR *PpMouseId,
    _Out_                                ULONG *PmouseIdLen                                            
    )
/*++

Routine Description:

    This routines a PnP packet for a mouse ID up to the first PnP delimiter
    (i.e, '(').

Arguments:

   PBuffer - Pointer to the buffer to scan

   BufLen - Length of the buffer in bytes

   PpMouseId - Pointer to the pointer to the mouse ID (this will be set
               to point to the location in the buffer where the mouse ID
               was found)

Return value:

    void

--*/
{
   PAGED_CODE();

   *PpMouseId = PBuffer;

   while (BufLen--) {
      if (**PpMouseId == 'M' || **PpMouseId == 'B') {
         *PmouseIdLen = BufLen+1;
         return;
      } else if (**PpMouseId == '(' || **PpMouseId == ('(' - 0x20)) {
         *PmouseIdLen = 0;
         *PpMouseId = NULL;
         return;
      }
      (*PpMouseId)++;
   }
   *PmouseIdLen = 0;
   *PpMouseId = NULL;
}

#if DBG
VOID SerenumHexDump(PUCHAR PBuf, ULONG NBytes)
/*++

Routine Description:

   Hex dump a buffer with NPerRow chars per row output

Arguments:

   PBuf - Pointer to the buffer to dump

   NBytes - Length of the buffer in bytes

Return value:

   VOID

--*/
{
   const ULONG NPerRow = 20;

   ULONG dmpi;
   ULONG col;
   UCHAR c;
   ULONG LoopCount = 1;
   ULONG dividend = NBytes / NPerRow;
   ULONG remainder = NBytes % NPerRow;
   ULONG nHexChars = NPerRow;
   ULONG nSpaces = 1;

   DbgPrint("SERENUM: Raw Data Packet on probe\n");

   if (remainder) {
      LoopCount++;
   }

   for (dmpi = 0; dmpi < (dividend + 1); dmpi++) {
      DbgPrint("-------: ");

      for (col = 0; col < nHexChars; col++) {
         DbgPrint("%02x ", (unsigned char)PBuf[dmpi * NPerRow + col]);
      }

      for (col = 0; col < nSpaces; col++) {
         DbgPrint(" ");
      }

      for (col = 0; col < nHexChars; col++){
         c = PBuf[dmpi * NPerRow + col];

         if (__isascii(c) && (c > ' ')){
            DbgPrint("%c", c);
            }else{
               DbgPrint(".");
            }
      }

      DbgPrint("\n");

      //
      // If this is the last one, then we have less that NPerRow to dump
      //

      if (dmpi == dividend) {
         if (remainder == 0) {
            //
            // This was an even multiple -- we're done
            //

            break; // for (dmpi)
         } else {
            nHexChars = remainder;
            nSpaces = NPerRow - nHexChars;
         }
      }
   }
}
#endif // DBG

NTSTATUS
SerenumDoEnumProtocol(
                                  PFDO_DEVICE_DATA PFdoData, 
    _Outptr_result_buffer_(*PNBytes)  PUCHAR *PpBuf, 
                                  PUSHORT PNBytes,
                                  PBOOLEAN PDSRMissing)
{
   IO_STATUS_BLOCK ioStatusBlock;
   ULONG i;
   ULONG bitMask;
   KEVENT event;
   KTIMER timer;
   NTSTATUS status;
   PUCHAR pReadBuf;
   USHORT nRead;
   PDEVICE_OBJECT pDevStack = PFdoData->TopOfStack;
   SERIAL_BAUD_RATE baudRate;
   SERIAL_LINE_CONTROL lineControl;

#if DBG
#define PERFCNT 1
#endif

#if defined(PERFCNT)
   LARGE_INTEGER perfFreq;
   LARGE_INTEGER stPerfCnt, endPerfCnt;
   LONG diff;
#endif

   LARGE_INTEGER DefaultWait;

   PAGED_CODE();

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   KeInitializeTimer(&timer);

#if defined(PERFCNT)
   perfFreq.QuadPart = (LONGLONG) 0;
#endif
   DefaultWait.QuadPart = (LONGLONG) -(SERENUM_DEFAULT_WAIT);
   *PpBuf = NULL;
   pReadBuf = NULL;
   nRead = 0;
   *PDSRMissing = FALSE;

   LOGENTRY(LOG_ENUM, 'SDEP', PFdoData,  PpBuf, PDSRMissing);


   pReadBuf = ExAllocatePoolZero(NonPagedPoolNx, MAX_DEVNODE_NAME + sizeof(CHAR) ,SERENUM_POOL_TAG);
   
   if (pReadBuf == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      LOGENTRY(LOG_ENUM, 'SDE1', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   *(pReadBuf + MAX_DEVNODE_NAME) = 0; 

   //
   // Set DTR
   //

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Setting DTR...\n"));

   status = Serenum_IoSyncIoctl(IOCTL_SERIAL_SET_DTR, FALSE, pDevStack, &event);

   if (!NT_SUCCESS(status)) {
      LOGENTRY(LOG_ENUM, 'SDE2', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   //
   // Clear RTS
   //
   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Clearing RTS...\n"));

   status = Serenum_IoSyncIoctl(IOCTL_SERIAL_CLR_RTS, FALSE, pDevStack, &event);

   if (!NT_SUCCESS(status)) {
      LOGENTRY(LOG_ENUM, 'SDE3', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   //
   // Wait for the default timeout period
   //

#if defined(PERFCNT)
   stPerfCnt = KeQueryPerformanceCounter(&perfFreq);
#endif

   status = Serenum_Wait(&timer, DefaultWait);

#if defined(PERFCNT)
   endPerfCnt = KeQueryPerformanceCounter(NULL);
   diff = (LONG)(endPerfCnt.QuadPart - stPerfCnt.QuadPart);
   diff *= 1000;
   diff /= (LONG)perfFreq.QuadPart;

   LOGENTRY(LOG_ENUM, 'SDT0', PFdoData, diff, 0);
#endif

   if (!NT_SUCCESS(status)) {
      Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                      ("Timer failed with status %x\n", status ));
      LOGENTRY(LOG_ENUM, 'SDE4', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Checking DSR...\n"));

   status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_GET_MODEMSTATUS, FALSE,
                                  pDevStack, &event, NULL, 0, &bitMask,
                                  sizeof(ULONG));

   if (!NT_SUCCESS(status)) {
      LOGENTRY(LOG_ENUM, 'SDE5', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   //
   // If DSR is not set, then a legacy device (like a mouse) may be attached --
   // they are not required to assert DSR when they are present and ready.
   //

   if ((SERIAL_DSR_STATE & bitMask) == 0) {
      Serenum_KdPrint (PFdoData, SER_DBG_SS_TRACE,
                       ("No PNP device available - DSR not set.\n"));
      *PDSRMissing = TRUE;
      LOGENTRY(LOG_ENUM, 'SDND', PFdoData,  0, 0);
   }

   //
   // Setup the serial port for 1200 bits/s, 7 data bits,
   // no parity, one stop bit
   //
   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Setting baud rate to 1200..."
                                               "\n"));
   baudRate.BaudRate = 1200;
   status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_SET_BAUD_RATE, FALSE, pDevStack,
                                  &event, &baudRate, sizeof(SERIAL_BAUD_RATE),
                                  NULL, 0);
   if (!NT_SUCCESS(status)) {
      LOGENTRY(LOG_ENUM, 'SDE6', PFdoData,  status, 0);
      goto ProtocolDone;
   }

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                   ("Setting the line control...\n"));

   lineControl.StopBits = STOP_BIT_1;
   lineControl.Parity = NO_PARITY;
   lineControl.WordLength = 7;

   status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_SET_LINE_CONTROL, FALSE,
                                  pDevStack, &event, &lineControl,
                                  sizeof(SERIAL_LINE_CONTROL), NULL, 0);

   if (!NT_SUCCESS(status)) {
      LOGENTRY(LOG_ENUM, 'SDE7', PFdoData,  status, 0);
      goto ProtocolDone;
   }


   //
   // loop twice
   // The first iteration is for reading the PNP ID string from modems
   // and mice.
   // The second iteration is for other devices.
   //
   for (i = 0; i < 2; i++) {
      //
      // Purge the buffers before reading
      //

      LOGENTRY(LOG_ENUM, 'SDEI', PFdoData,  i, 0);

      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Purging all buffers...\n"));

      bitMask = SERIAL_PURGE_RXCLEAR;

      status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_PURGE, FALSE, pDevStack,
                                     &event, &bitMask, sizeof(ULONG), NULL, 0);

      if (!NT_SUCCESS(status)) {
         LOGENTRY(LOG_ENUM, 'SDE8', PFdoData,  status, 0);
         break;
      }

      //
      // Clear DTR
      //
      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Clearing DTR...\n"));

      status = Serenum_IoSyncIoctl(IOCTL_SERIAL_CLR_DTR, FALSE, pDevStack,
                                   &event);

      if (!NT_SUCCESS(status)) {
         LOGENTRY(LOG_ENUM, 'SDE9', PFdoData,  status, 0);
         break;
      }

      //
      // Clear RTS
      //
      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Clearing RTS...\n"));

      status = Serenum_IoSyncIoctl(IOCTL_SERIAL_CLR_RTS, FALSE, pDevStack,
                                   &event);

      if (!NT_SUCCESS(status)) {
         LOGENTRY(LOG_ENUM, 'SDEA', PFdoData,  status, 0);
         break;
      }

      //
      // Set a timer for 200 ms
      //

      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Waiting...\n"));

#if defined(PERFCNT)
      stPerfCnt = KeQueryPerformanceCounter(&perfFreq);
#endif

      status = Serenum_Wait(&timer, DefaultWait);

      if (!NT_SUCCESS(status)) {
         Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                         ("Timer failed with status %x\n", status ));
         LOGENTRY(LOG_ENUM, 'SDEB', PFdoData,  status, 0);
         break;
      }

      //
      // set DTR
      //

      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Setting DTR...\n"));

      status = Serenum_IoSyncIoctl(IOCTL_SERIAL_SET_DTR, FALSE, pDevStack,
                                   &event);

      if (!NT_SUCCESS(status)) {
         LOGENTRY(LOG_ENUM, 'SDEC', PFdoData,  status, 0);
         break;
      }


#if defined(PERFCNT)
   endPerfCnt = KeQueryPerformanceCounter(NULL);
   diff = (LONG)(endPerfCnt.QuadPart - stPerfCnt.QuadPart);
   diff *= 1000;
   diff /= (LONG)perfFreq.QuadPart;

   LOGENTRY(LOG_ENUM, 'SDT1', PFdoData, diff, 0);
#endif

      //
      // First iteration is for modems
      // Therefore wait for 200 ms as per protocol for getting PNP string out
      //

      if (!i) {
         status = Serenum_Wait(&timer, DefaultWait);
         if (!NT_SUCCESS(status)) {
            Serenum_KdPrint (PFdoData, SER_DBG_SS_ERROR,
                             ("Timer failed with status %x\n", status ));
            LOGENTRY(LOG_ENUM, 'SDED', PFdoData,  status, 0);
            break;
         }
      }

      //
      // set RTS
      //

      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Setting RTS...\n"));

      status = Serenum_IoSyncIoctl(IOCTL_SERIAL_SET_RTS, FALSE, pDevStack,
                                   &event);

      if (!NT_SUCCESS(status)) {
         LOGENTRY(LOG_ENUM, 'SDEF', PFdoData,  status, 0);
         break;
      }

      //
      // Read from the serial port
      //
      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                      ("Reading the serial port...\n"));

      Serenum_KdPrint(PFdoData, SER_DBG_SS_INFO, ("Address: %p\n", pReadBuf));

      nRead = 0;

#if DBG
      RtlFillMemory(pReadBuf, MAX_DEVNODE_NAME, 0xff);
#endif

      //
      // Flush the input buffer
      //

      status = Serenum_ReadSerialPort((PCHAR)pReadBuf, MAX_DEVNODE_NAME,
                                      SERENUM_SERIAL_READ_TIME, &nRead,
                                      &ioStatusBlock, PFdoData);

      switch (status) {
      case STATUS_TIMEOUT:
         if (nRead == 0) {
            Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                      ("Timeout with no bytes read; continuing\n"));
            LOGENTRY(LOG_ENUM, 'SDEG', PFdoData,  status, 0);
            continue;
         }

         //
         // We timed out with data, so we use what we have
         //

         status = STATUS_SUCCESS;

         LOGENTRY(LOG_ENUM, 'SDEH', PFdoData,  status, 0);
         break;

      case STATUS_SUCCESS:
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Read succeeded\n"));
         LOGENTRY(LOG_ENUM, 'SDEJ', PFdoData,  status, 0);
         goto ProtocolDone;
         break;

      default:
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Read failed with 0x%x\n",
                                                       status));
         LOGENTRY(LOG_ENUM, 'SDEK', PFdoData,  status, 0);
         goto ProtocolDone;
         break;
      }

      //
      // If anything was read from the serial port, we're done!
      //

      if (nRead) {
         break;
      }
   }

ProtocolDone:;

   if (!NT_SUCCESS(status)) {
      if (pReadBuf != NULL) {
         ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
         pReadBuf = NULL;
         return status;
      }
   }

   *PNBytes = nRead;
   *PpBuf = pReadBuf;
   LOGENTRY(LOG_ENUM, 'SDE0', PFdoData,  status, nRead);
#pragma warning(suppress: 26045)
   return status;
}

BOOLEAN
SerenumValidateID(IN PUNICODE_STRING PId)
/*++

Routine Description:

    This validates all the characters in a MULTI_SZ for a Pnp ID.

    Invalid characters are:
        c <  0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    PId - Pointer to a multi_sz containing the IDs

Return value:

    BOOLEAN -- TRUE if valid ID, FALSE otherwise

--*/
{
   WCHAR *cp;

   PAGED_CODE();

   //
   // Walk each string in the multisz and check for bad characters
   //

   cp = PId->Buffer;

   if (cp == NULL) {
      return TRUE;
   }

   do {
      while (*cp) {
         if ((*cp < L' ') || (*cp > L'\x7f') || (*cp == L',') ) {
            return FALSE;
         }

         cp++;
      }

      cp++;
   } while (*cp);

   return TRUE;
}

BOOLEAN
SerenumCheckForLegacyDevice(IN PFDO_DEVICE_DATA PFdoData, 
     _In_reads_(BufferLen) IN PCHAR PIdBuf,
                            IN ULONG BufferLen,
                            IN OUT PUNICODE_STRING PHardwareIDs,
                            IN OUT PUNICODE_STRING PCompIDs,
                            IN OUT PUNICODE_STRING PDeviceIDs)
/*++

Routine Description:

   This routine implements legacy mouse detection

Arguments:

    PFdoData      - pointer to the FDO's device-specific data
    PIdBuf        - Buffer of data returned from device
    BufferLen     - length of PIdBuf in bytes
    PHardwareIDs  - MULTI_SZ to return hardware ID's in
    PCompIDs      - MULTI_SZ to return compatible ID's in
    PDeviceIDs    - MULTI_SZ to return device ID's in

Return value:

    BOOLEAN -- TRUE if mouse detected, FALSE otherwise

--*/
{
   PCHAR mouseId = PIdBuf;
   ULONG mouseIdLen;
   BOOLEAN rval = FALSE;
   
   PAGED_CODE();

   SerenumScanOtherIdForMouse(PIdBuf, BufferLen, &mouseId, &mouseIdLen);

   if (mouseId != NULL) {
      //
      // A legacy device is attached to the serial port, since DSR was
      // not set when RTS was set.
      // If we find a mouse from the PIdBuf, copy the appropriate
      // strings into the hardwareIDs and compIDs manually.
      //
      if (*mouseId == 'M') {
         if ((mouseId - PIdBuf) > 1
             && mouseIdLen > 1 
             && mouseId[1] == '3' ) {
            Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("*PNP0F08 mouse\n"));
            Serenum_InitMultiString(PFdoData, PHardwareIDs, "*PNP0F08", NULL);
            Serenum_InitMultiString(PFdoData, PCompIDs, "SERIAL_MOUSE", NULL);
            //
            // CIMEXCIMEX 04/28/1999 -
            //     Device ID's should be unique, at least as unique as the
            // hardware ID's. This ID should really be Serenum\\PNP0F08
            //
            Serenum_InitMultiString(PFdoData, PDeviceIDs, "Serenum\\Mouse",
                                    NULL);
            rval = TRUE;

         } else {
            Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("*PNP0F01 mouse\n"));
            Serenum_InitMultiString(PFdoData, PHardwareIDs, "*PNP0F01", NULL);
            Serenum_InitMultiString(PFdoData, PCompIDs, "SERIAL_MOUSE", NULL);
            //
            // CIMEXCIMEX 04/28/1999 -
            //     Device ID's should be unique, at least as unique as the
            // hardware ID's. This ID should really be Serenum\\PNP0F01
            //
            Serenum_InitMultiString(PFdoData, PDeviceIDs, "Serenum\\Mouse",
                                    NULL);
            rval = TRUE;
         }
      } else if (*mouseId == 'B') {
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("*PNP0F09 mouse\n"));
         Serenum_InitMultiString(PFdoData, PHardwareIDs, "*PNP0F09", NULL);
         Serenum_InitMultiString(PFdoData, PCompIDs, "*PNP0F0F", "SERIAL_MOUSE",
                                 NULL);
         //
         // CIMEXCIMEX 04/28/1999 -
         //     Device ID's should be unique, at least as unique as the
         // hardware ID's. This ID should really be Serenum\\PNP0F09
         //
         Serenum_InitMultiString(PFdoData, PDeviceIDs, "Serenum\\BallPoint",
                                 NULL);
         rval = TRUE;
      }

#if DBG
      if (rval) {
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                         ("Buffers at 0x%p 0x%p 0x%p\n",
                          PHardwareIDs->Buffer, PCompIDs->Buffer,
                          PDeviceIDs->Buffer));
      }
#endif // DBG

   }

   return rval;
}

NTSTATUS
Serenum_ReenumerateDevices(IN PIRP Irp, IN PFDO_DEVICE_DATA PFdoData,
                           PBOOLEAN PSameDevice)
/*++

Routine Description:

    This enumerates the serenum bus which is represented by Fdo (a pointer
    to the device object representing the serial bus). It creates new PDOs
    for any new devices which have been discovered since the last enumeration

Arguments:

    PFdoData - Pointer to the fdo's device extension
                for the serial bus which needs to be enumerated
    Irp - Pointer to the Irp which was sent to reenumerate.

Return value:

    NTSTATUS

--*/
{
   NTSTATUS status;
   KEVENT event;
   KTIMER timer;

   UNICODE_STRING pdoUniName;
   PDEVICE_OBJECT pdo = PFdoData->NewPDO;
   PDEVICE_OBJECT pDevStack = PFdoData->TopOfStack;
   PPDO_DEVICE_DATA pdoData;
   UNICODE_STRING hardwareIDs;
   UNICODE_STRING compIDs;
   UNICODE_STRING deviceIDs;
   UNICODE_STRING devDesc;
   UNICODE_STRING serNo;
   UNICODE_STRING pnpRev;


   BOOLEAN DSRMissing = FALSE;
   BOOLEAN legacyDeviceFound = FALSE;

   USHORT nActual = 0;

   PCHAR pReadBuf = NULL;
   WCHAR pdoName[] = SERENUM_PDO_NAME_BASE;

   SERIAL_BASIC_SETTINGS basicSettings;
   BOOLEAN basicSettingsDone = FALSE;
   SERIAL_TIMEOUTS timeouts, newTimeouts;

   ULONG curTry = 0;
   BOOLEAN sameDevice = FALSE;

   PAGED_CODE();

   //
   // While enumeration is taking place, we can't allow a Create to come down
   // from an upper driver.  We use this semaphore to protect ourselves.
   //

   status = KeWaitForSingleObject(&PFdoData->CreateSemaphore, Executive,
                                  KernelMode, FALSE, NULL);

   if (!NT_SUCCESS(status)) {
      return status;
   }


   //
   // Initialization
   //

   RtlInitUnicodeString(&pdoUniName, pdoName);
   pdoName[((sizeof(pdoName)/sizeof(WCHAR)) - 2)] = L'0' + PFdoData->PdoIndex++;

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   KeInitializeTimer(&timer);

   RtlInitUnicodeString(&hardwareIDs, NULL);
   RtlInitUnicodeString(&compIDs, NULL);
   RtlInitUnicodeString(&deviceIDs, NULL);
   RtlInitUnicodeString(&devDesc, NULL);
   RtlInitUnicodeString(&serNo, NULL);
   RtlInitUnicodeString(&pnpRev, NULL);

   //
   // If the current PDO should be marked missing, do so.
   //
   if (PFdoData->PDOForcedRemove && pdo != NULL) {
       Serenum_PDO_EnumMarkMissing(PFdoData, pdo->DeviceExtension);
       pdo = NULL;
   }

   //
   // Open the Serial port before sending Irps down
   // Use the Irp passed to us, and grab it on the way up.
   //

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                   ("Opening the serial port...\n"));

   status = Serenum_IoSyncReqWithIrp(Irp, IRP_MJ_CREATE, &event, pDevStack);

   LOGENTRY(LOG_ENUM, 'SRRO', PFdoData, status, 0);

   //
   // If we cannot open the stack, odd's are we have a live and started PDO on
   // it. Since enumeration might interfere with running devices, we do not
   // adjust our list of children if we cannot open the stack.
   //
   if (!NT_SUCCESS(status)) {
      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                      ("Failed to open the serial port...\n"));
      KeReleaseSemaphore(&PFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);
      LOGENTRY(LOG_ENUM, 'SRR1', PFdoData, status, 0);
      return status;
   }

   //
   // Set up the COM port
   //

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Setting up port\n"));

   status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS, TRUE,
                                  pDevStack, &event, NULL, 0,
                                  &basicSettings, sizeof(basicSettings));

   if (NT_SUCCESS(status)) {
      basicSettingsDone = TRUE;
   } else {
      //
      // This "serial" driver doesn't support BASIC_SETTINGS so instead
      // we just set what we really need the old fashioned way
      //

      status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_GET_TIMEOUTS, FALSE, 
                                     pDevStack, &event,
                                     NULL, 0, &timeouts, sizeof(timeouts));
      
      if (!NT_SUCCESS(status)) {
         //
         // This should not happen because we are sending an Ioctl to Serial
         // but for robustness of the code we check the return status.
         //
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                         ("Failed to get the serial timeouts...\n"));
         KeReleaseSemaphore(&PFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);
         return status;
      }

      RtlZeroMemory(&newTimeouts, sizeof(newTimeouts));

      Serenum_IoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS, FALSE, pDevStack, &event,
                             &newTimeouts, sizeof(newTimeouts), NULL, 0);
   }


   //
   // Run the serial PnP device detection protocol; give it up to 3 tries
   //

   while (curTry <= 2) {
      if (pReadBuf) {
         ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
         pReadBuf = NULL;
      }

      status = SerenumDoEnumProtocol(PFdoData, (PUCHAR*)&pReadBuf, &nActual,
                                     &DSRMissing);

      if (status == STATUS_SUCCESS) {
         break;
      }

      curTry++;
   }

   //
   // If DSR wasn't set any existing pdos will be eliminated
   //


   if (basicSettingsDone) {
      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                      ("Restoring basic settings\n"));

      Serenum_IoSyncIoctlEx(IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS, TRUE,
                            pDevStack, &event, &basicSettings,
                            sizeof(basicSettings), NULL, 0);
   } else {
      Serenum_IoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS, FALSE, pDevStack, &event,
                             &timeouts, sizeof(timeouts), NULL, 0);
   }

   //
   // Cleanup and then Close
   //

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                   ("Cleanup on the serial port...\n"));

   //
   // We ignore the status -- we have to finish closing
   //

   (void)Serenum_IoSyncReqWithIrp(Irp, IRP_MJ_CLEANUP, &event, pDevStack);

#if DBG
   if (!NT_SUCCESS(status)) {
      Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                      ("Failed to cleanup the serial port...\n"));
      // don't return because we want to attempt to close!
   }
#endif

   //
   // Close the Serial port after everything is done
   //

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                   ("Closing the serial port...\n"));

   //
   // We ignore the status -- we have to close!!
   //

   Serenum_IoSyncReqWithIrp(Irp, IRP_MJ_CLOSE, &event, pDevStack);

   LOGENTRY(LOG_ENUM, 'SRRC', PFdoData, 0, 0);

   //
   // Our status is that of the enumeration
   //

   if (!NT_SUCCESS(status)) {
      Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                      ("Failed to enumerate the serial port...\n"));
      KeReleaseSemaphore(&PFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);
      if (pReadBuf != NULL) {
         ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
      }
      LOGENTRY(LOG_ENUM, 'SRR2', PFdoData, status, 0);
      return status;
   }

   //
   // Check if anything was read, and if not, we're done
   //

   if (nActual == 0) {
      if (pReadBuf != NULL) {
         ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
         pReadBuf = NULL;
      }

      if (pdo != NULL) {
         //
         // Something was there.  The device must have been unplugged.
         // Remove the PDO.
         //

         Serenum_PDO_EnumMarkMissing(PFdoData, pdo->DeviceExtension);
         pdo = NULL;
      }

      goto ExitReenumerate;
   }

   Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                   ("Something was read from the serial port...\n"));



#if 0
   if (PFdoData->DebugLevel & SER_DBG_PNP_DUMP_PACKET) {
      SerenumHexDump(pReadBuf, nActual);
   }
#endif

   //
   // Determine from the result whether the current pdo (if we have one),
   // should be deleted.  If it's the same device, then keep it.  If it's a
   // different device or if the device is a legacy device, then create a
   // new pdo.
   //
   if (DSRMissing) {
      legacyDeviceFound
         = SerenumCheckForLegacyDevice(PFdoData, pReadBuf, nActual,
                                       &hardwareIDs, &compIDs, &deviceIDs);
   }

   if (!legacyDeviceFound) {
      //
      // No legacy device was found, so parse the data we got back
      // from the device.
      //

      status = Serenum_ParseData(PFdoData, pReadBuf, nActual, &hardwareIDs,
                                 &compIDs, &deviceIDs, &devDesc, &serNo, &pnpRev);

      //
      // Last chance:
      //
      // 1) DSR is present
      // 2) Not a PnP device
      //
      // There are some devices that are legacy but also assert DSR (e.g., the
      // gyropoint mouse).  Give it one last shot.
      //

      if (!DSRMissing && !NT_SUCCESS(status)) {


         //
         // CIMEXCIMEX Serenum_ParseData() isn't very tidy, so we
         // must clean up after them
         //

         SerenumFreeUnicodeString(&hardwareIDs);
         SerenumFreeUnicodeString(&compIDs);
         SerenumFreeUnicodeString(&deviceIDs);
         SerenumFreeUnicodeString(&devDesc);
         SerenumFreeUnicodeString(&serNo);
         SerenumFreeUnicodeString(&pnpRev);

         if (SerenumCheckForLegacyDevice(PFdoData, pReadBuf, nActual,
                                         &hardwareIDs, &compIDs, &deviceIDs)) {
            status = STATUS_SUCCESS;
         }
      }

      //
      // If the data can't be parsed and this isn't a legacy device, then
      // it is something we don't understand.  We bail out at this point
      //


      if (!NT_SUCCESS(status)) {
         Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                         ("Failed to parse the data for the new device\n"));

         //
         // If there is a current PDO, remove it since we can't ID the
         // attached device.
         //

         if (pdo) {
            Serenum_PDO_EnumMarkMissing(PFdoData, pdo->DeviceExtension);
            pdo = NULL;
         }

         SerenumFreeUnicodeString(&hardwareIDs);
         SerenumFreeUnicodeString(&compIDs);
         SerenumFreeUnicodeString(&deviceIDs);
         SerenumFreeUnicodeString(&devDesc);
         SerenumFreeUnicodeString(&serNo);
         SerenumFreeUnicodeString(&pnpRev);


         ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
         pReadBuf = NULL;

         goto ExitReenumerate;
      }
   }

   //
   // We're now finally able to free this read buffer.
   //

   if (pReadBuf != NULL) {
      ExFreePoolWithTag(pReadBuf,SERENUM_POOL_TAG);
   }

   //
   // Validate all the ID's -- if any are illegal,
   // then we fail the enumeration
   //

   if (!SerenumValidateID(&hardwareIDs) || !SerenumValidateID(&compIDs)
       || !SerenumValidateID(&deviceIDs)) {

      //
      // If a PDO already exists, mark it missing and get rid
      // of it since we don't know what is out there any longer
      //

      if (pdo) {
         Serenum_PDO_EnumMarkMissing(PFdoData, pdo->DeviceExtension);
         pdo = NULL;
      }

      SerenumFreeUnicodeString(&hardwareIDs);
      SerenumFreeUnicodeString(&compIDs);
      SerenumFreeUnicodeString(&deviceIDs);
      SerenumFreeUnicodeString(&devDesc);
      SerenumFreeUnicodeString(&serNo);
      SerenumFreeUnicodeString(&pnpRev);


      goto ExitReenumerate;
   }

   //
   // Check if the current device is the same as the one that we're
   // enumerating.  If so, we'll just keep the current pdo.
   //
   if (pdo) {
      pdoData = pdo->DeviceExtension;

      //
      // CIMEXCIMEX 04/28/1999 -
      //     We should be comparing device ID's here, but the above mentioned
      // bug must be fixed first. Note that even this code is broken as it
      // doesn't take into account that hardware/compID's are multiSz.
      //

      if (!(RtlEqualUnicodeString(&pdoData->HardwareIDs, &hardwareIDs, FALSE)
            && RtlEqualUnicodeString(&pdoData->CompIDs, &compIDs, FALSE))) {
         //
         // The ids are not the same, so get rid of this pdo and create a
         // new one so that the PNP system will query the ids and find a
         // new driver
         //
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE, ("Different device."
                                                     " Removing PDO %p\n",
                                                     pdo));
         Serenum_PDO_EnumMarkMissing(PFdoData, pdoData);
         pdo = NULL;
      } else {
         Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                         ("Same device. Keeping current Pdo %p\n", pdo));
         sameDevice = TRUE;
      }
   }

   //
   // If there isn't a pdo, then create one!
   //
   if (!pdo) {
      //
      // Allocate a pdo
      //
      status = IoCreateDevice(PFdoData->Self->DriverObject,
                              sizeof(PDO_DEVICE_DATA), &pdoUniName,
                              FILE_DEVICE_UNKNOWN,
                              FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &pdo);

      if (!NT_SUCCESS(status)) {

         Serenum_KdPrint(PFdoData, SER_DBG_SS_ERROR,
                         ("Create device failed\n"));
         KeReleaseSemaphore(&PFdoData->CreateSemaphore, IO_NO_INCREMENT, 1,
                            FALSE);
         return status;
      }

      Serenum_KdPrint(PFdoData, SER_DBG_SS_TRACE,
                      ("Created PDO on top of filter: %p\n",pdo));


      //
      // Initialize the rest of the device object
      //


      pdoData = pdo->DeviceExtension;

      //
      // Copy our temp buffers over to the DevExt
      //

      pdoData->HardwareIDs = hardwareIDs;
      pdoData->CompIDs = compIDs;
      pdoData->DeviceIDs = deviceIDs;
      pdoData->DevDesc = devDesc;
      pdoData->SerialNo = serNo;
      pdoData->PnPRev = pnpRev;

      Serenum_InitPDO(pdo, PFdoData);

   }

ExitReenumerate:;

   KeReleaseSemaphore(&PFdoData->CreateSemaphore, IO_NO_INCREMENT, 1, FALSE);

   *PSameDevice = sameDevice;

   return STATUS_SUCCESS;
}

void
Serenum_PDO_EnumMarkMissing(PFDO_DEVICE_DATA FdoData, PPDO_DEVICE_DATA PdoData)
/*++

Routine Description:
    Removes the attached pdo from the fdo's list of children.

    NOTE: THIS FUNCTION CAN ONLY BE CALLED DURING AN ENUMERATION. If called
          outside of enumeration, Serenum might delete it's PDO before PnP has
          been told the PDO is gone.

Arguments:
    FdoData - Pointer to the fdo's device extension
    PdoData - Pointer to the pdo's device extension

Return value:
    none

--*/
{
   KIRQL oldIrql;

   Serenum_KdPrint (FdoData, SER_DBG_SS_TRACE, ("Removing Pdo %p\n",
                                                 PdoData->Self));

   ASSERT(PdoData->Attached);

   KeAcquireSpinLock(&FdoData->EnumerationLock, &oldIrql);

   PdoData->Attached = FALSE;
   FdoData->NewPDO = NULL;
   FdoData->NewPdoData = NULL;
   FdoData->NewNumPDOs = 0;
   FdoData->NewPDOForcedRemove = FALSE;

   FdoData->EnumFlags |= SERENUM_ENUMFLAG_DIRTY;

   KeReleaseSpinLock(&FdoData->EnumerationLock, oldIrql);
}

NTSTATUS
Serenum_IoSyncReqWithIrp(PIRP PIrp, UCHAR MajorFunction, PKEVENT PEvent,
                         PDEVICE_OBJECT PDevObj )
/*++

Routine Description:
    Performs a synchronous IO request by waiting on the event object
    passed to it.  The IRP isn't deallocated after this call.

Arguments:
    PIrp - The IRP to be used for this request

    MajorFunction - The major function

    PEvent - An event used to wait for the IRP

    PDevObj - The object that we're performing the IO request upon

Return value:
    NTSTATUS

--*/
{
    PIO_STACK_LOCATION stack;
    NTSTATUS status;

    PAGED_CODE();
    
    stack = IoGetNextIrpStackLocation(PIrp);

    stack->MajorFunction = MajorFunction;

    KeClearEvent(PEvent);

    IoSetCompletionRoutine(PIrp, Serenum_EnumComplete, PEvent, TRUE,
                           TRUE, TRUE);

    status = Serenum_IoSyncReq(PDevObj, PIrp, PEvent);

    if (status == STATUS_SUCCESS) {
       status = PIrp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
Serenum_IoSyncIoctlEx(ULONG Ioctl, BOOLEAN Internal, PDEVICE_OBJECT PDevObj,
                      PKEVENT PEvent, PVOID PInBuffer, ULONG InBufferLen,
                      PVOID POutBuffer, ULONG OutBufferLen)
/*++

Routine Description:
    Performs a synchronous IO control request by waiting on the event object
    passed to it.  The IRP is deallocated by the IO system when finished.

Return value:
    NTSTATUS

--*/
{
    PIRP pIrp;
    NTSTATUS status;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();
    
    KeClearEvent(PEvent);

    // Allocate an IRP - No need to release
    // When the next-lower driver completes this IRP, the IO Mgr releases it.

    pIrp = IoBuildDeviceIoControlRequest(Ioctl, PDevObj, PInBuffer, InBufferLen,
                                         POutBuffer, OutBufferLen, Internal,
                                         PEvent, &IoStatusBlock);

    if (pIrp == NULL) {
        Serenum_KdPrint_Def (SER_DBG_SS_ERROR, ("Failed to allocate IRP\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = Serenum_IoSyncReq(PDevObj, pIrp, PEvent);


    if (status == STATUS_SUCCESS) {
       status = IoStatusBlock.Status;
    }

    return status;
}


NTSTATUS
Serenum_IoSyncReq(PDEVICE_OBJECT PDevObj, IN PIRP PIrp, PKEVENT PEvent)
/*++

Routine Description:
    Performs a synchronous IO request by waiting on the event object
    passed to it.  The IRP is deallocated by the IO system when finished.

Return value:
    NTSTATUS

--*/
{
   NTSTATUS status;
   PLARGE_INTEGER Timeout=NULL;

   PAGED_CODE();
    
   status = IoCallDriver(PDevObj, PIrp);

   if (status == STATUS_PENDING) {
      // wait for it...
      __analysis_assume(Timeout!=NULL);
      status = KeWaitForSingleObject(PEvent, Executive, KernelMode, FALSE,
                                     Timeout);
   }

    return status;
}

NTSTATUS
Serenum_Wait(IN PKTIMER Timer, IN LARGE_INTEGER DueTime)
/*++

Routine Description:
    Performs a wait for the specified time.
    NB: Negative time is relative to the current time.  Positive time
    represents an absolute time to wait until.

Return value:
    NTSTATUS

--*/
{
   PAGED_CODE();
    
   if (KeSetTimer(Timer, DueTime, NULL)) {
      Serenum_KdPrint_Def(SER_DBG_SS_INFO, ("Timer already set: %p\n", Timer));
   }

   return KeWaitForSingleObject(Timer, Executive, KernelMode, FALSE, NULL);
}


NTSTATUS
Serenum_CompletionRoutine(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp,
    PVOID            Context
    )
/*++

Routine Description:

    The completion routine for plug & play irps that needs to be
    processed first by the lower drivers.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - pointer to an event object.

Return Value:

      NT status code

--*/
{
    PCOMMON_DEVICE_DATA commonData=(PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;
    UNREFERENCED_PARAMETER(Context);    
    
    // the driver must mark the irp as pending if PendingReturned is set.
    if (Irp->PendingReturned == TRUE){
        IoMarkIrpPending(Irp);
    }
    // release the removelock
    IoReleaseRemoveLock(&commonData->RemoveLock, Irp); 
    return STATUS_CONTINUE_COMPLETION;
    
}




NTSTATUS
Serenum_EnumComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.  It sets the event for the synchronous
    calls done.

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

#pragma warning(suppress: 6387)
    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}


NTSTATUS
Serenum_ReadSerialPort(
  _Out_writes_(Buflen) OUT PCHAR PReadBuffer, IN USHORT Buflen,
                       IN ULONG Timeout, OUT PUSHORT nActual,
                       OUT PIO_STATUS_BLOCK PIoStatusBlock,
                       IN const PFDO_DEVICE_DATA FdoData)
{
    NTSTATUS status;
    PIRP pIrp;
    LARGE_INTEGER startingOffset;
    KEVENT event;
    SERIAL_TIMEOUTS timeouts;

    PAGED_CODE();
    
    startingOffset.QuadPart = (LONGLONG) 0;
    //
    // Set the proper timeouts for the read
    //

    timeouts.ReadIntervalTimeout = MAXULONG;
    timeouts.ReadTotalTimeoutMultiplier = MAXULONG;
    timeouts.ReadTotalTimeoutConstant = Timeout;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    status = Serenum_IoSyncIoctlEx(IOCTL_SERIAL_SET_TIMEOUTS, FALSE,
                                   FdoData->TopOfStack, &event, &timeouts,
                                   sizeof(timeouts), NULL, 0);

    if (!NT_SUCCESS(status)) {
       return status;
    }

    Serenum_KdPrint(FdoData, SER_DBG_SS_TRACE, ("Read pending...\n"));

    *nActual = 0;

    while (*nActual < Buflen) {
        KeClearEvent(&event);

        pIrp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, FdoData->TopOfStack,
                                            PReadBuffer, 1, &startingOffset,
                                            &event, PIoStatusBlock);

        if (pIrp == NULL) {
            Serenum_KdPrint(FdoData, SER_DBG_SS_ERROR, ("Failed to allocate IRP"
                                                        "\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(FdoData->TopOfStack, pIrp);

        if (status == STATUS_PENDING) {

            //
            // Wait for the IRP
            //

            status = KeWaitForSingleObject(&event, Executive, KernelMode,
                                           FALSE, NULL);

            if (status == STATUS_SUCCESS) {
               status = PIoStatusBlock->Status;
            }
        }

        if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT) {
           Serenum_KdPrint (FdoData, SER_DBG_SS_ERROR,
                            ("IO Call failed with status %x\n", status));
           return status;
        }

        *nActual += (USHORT)PIoStatusBlock->Information;
        PReadBuffer += (USHORT)PIoStatusBlock->Information;
    }

    return status;
}


NTSTATUS
Serenum_GetRegistryKeyValue(
    _In_ HANDLE Handle,
    _In_reads_bytes_(KeyNameStringLength) PWCHAR KeyNameString,
    _In_ ULONG KeyNameStringLength,
    _Out_writes_bytes_to_(DataLength, *ActualLength) PVOID Data,
    _In_ ULONG DataLength,
    _Out_ PULONG ActualLength
    )
/*++

Routine Description:

    Reads a registry key value from an already opened registry key.

Arguments:

    Handle              Handle to the opened registry key

    KeyNameString       ANSI string to the desired key

    KeyNameStringLength Length of the KeyNameString

    Data                Buffer to place the key value in

    DataLength          Length of the data buffer

Return Value:

    STATUS_SUCCESS if all works, otherwise status of system call that
    went wrong.

--*/
{
    UNICODE_STRING              keyName;
    ULONG                       length;
    PKEY_VALUE_FULL_INFORMATION fullInfo = NULL;

    NTSTATUS                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

#pragma warning(suppress: 26035)
#pragma warning(suppress: 26018)
	RtlInitUnicodeString (&keyName, KeyNameString);

    length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength
      + DataLength;
    fullInfo = ExAllocatePoolZero(PagedPool, length,SERENUM_POOL_TAG);

    if (ActualLength != NULL) {
       *ActualLength = 0;
    }

    if (fullInfo) {
#pragma warning(suppress: 26018)
        ntStatus = ZwQueryValueKey(Handle, &keyName, KeyValueFullInformation,
                                   fullInfo, length, &length);

        if (NT_SUCCESS(ntStatus)) {
            //
            // If there is enough room in the data buffer, copy the output
            //

            if (DataLength >= fullInfo->DataLength) {
                RtlCopyMemory(Data, ((PUCHAR)fullInfo) + fullInfo->DataOffset,
                              fullInfo->DataLength);
                if (ActualLength != NULL) {
                   *ActualLength = fullInfo->DataLength;
                }
            }
        }

        ExFreePoolWithTag(fullInfo,SERENUM_POOL_TAG);
        fullInfo = NULL;
    }

    if (!NT_SUCCESS(ntStatus) && !NT_ERROR(ntStatus)) {
       if (ntStatus == STATUS_BUFFER_OVERFLOW) {
          ntStatus = STATUS_BUFFER_TOO_SMALL;
       } else {
          ntStatus = STATUS_UNSUCCESSFUL;
       }
    }
#pragma warning(suppress: 26045)
	return ntStatus;
}


VOID
SerenumWaitForEnumThreadTerminate(IN PFDO_DEVICE_DATA PFdoData)
{
    KIRQL oldIrql;
    PVOID pThreadObj;

    //
    // Take a reference under the lock so the thread can't disappear on us.
    //

    KeAcquireSpinLock(&PFdoData->EnumerationLock, &oldIrql);

    //
    // If the work item beat us, then the thread is done and we can
    // delete/stop/unload.  Otherwise, we have to wait.  We can use
    // the reference we stole to hold the object around.
    //

    if (PFdoData->ThreadObj != NULL) {
        pThreadObj = PFdoData->ThreadObj;
        PFdoData->ThreadObj = NULL;
        PFdoData->EnumFlags &= ~SERENUM_ENUMFLAG_PENDING;
    } else {
        pThreadObj = NULL;
    }

    KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);

    if (pThreadObj != NULL) {
        KeWaitForSingleObject(pThreadObj, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(pThreadObj);
    }
}


IO_WORKITEM_ROUTINE
SerenumEnumThreadWorkItem;

VOID
SerenumEnumThreadWorkItem(IN PDEVICE_OBJECT PDevObj, IN PVOID PFdoData)
{
    PFDO_DEVICE_DATA pFdoData = (PFDO_DEVICE_DATA)PFdoData;
    KIRQL oldIrql;
    PVOID pThreadObj;
    PIO_WORKITEM pWorkItem;

    _Analysis_assume_(pFdoData != NULL); // // Not NULL when passed to IoQueueWorkItem()
    UNREFERENCED_PARAMETER(PDevObj);

    //
    // See if the delete/stop code beat us to the thread obj.
    // If not, we can derefence the thread.
    //

    KeAcquireSpinLock(&pFdoData->EnumerationLock, &oldIrql);

    if (pFdoData->ThreadObj != NULL) {
        pThreadObj = pFdoData->ThreadObj;
        pFdoData->ThreadObj = NULL;
        pFdoData->EnumFlags &= ~SERENUM_ENUMFLAG_PENDING;
    } else {
        pThreadObj = NULL;
    }

    pWorkItem = pFdoData->EnumWorkItem;
    pFdoData->EnumWorkItem = NULL;

    KeReleaseSpinLock(&pFdoData->EnumerationLock, oldIrql);

    if (pThreadObj != NULL) {
        ObDereferenceObject(pThreadObj);
    }

    IoFreeWorkItem(pWorkItem);
}


KSTART_ROUTINE
SerenumEnumThread;

VOID
SerenumEnumThread(IN PVOID PFdoData)
{
   PFDO_DEVICE_DATA pFdoData = (PFDO_DEVICE_DATA)PFdoData;
   PIRP pIrp = NULL;
   NTSTATUS status;
   KIRQL oldIrql;
   PKTHREAD pThread;
   BOOLEAN sameDevice = TRUE;

   pThread = KeGetCurrentThread();

   KeSetPriorityThread(pThread, HIGH_PRIORITY);

   pIrp = IoAllocateIrp(pFdoData->TopOfStack->StackSize + 1, FALSE);

   if (pIrp == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerenumEnumThreadErrOut;
   }
   IoSetNextIrpStackLocation(pIrp);
   status = Serenum_ReenumerateDevices(pIrp, pFdoData, &sameDevice);

SerenumEnumThreadErrOut:

   if (pIrp != NULL) {
      IoFreeIrp(pIrp);
   }

   KeAcquireSpinLock(&pFdoData->EnumerationLock, &oldIrql);

   if ((status == STATUS_SUCCESS) && !sameDevice) {
      pFdoData->EnumFlags |= SERENUM_ENUMFLAG_DIRTY;
   }

   KeReleaseSpinLock(&pFdoData->EnumerationLock, oldIrql);

   if ((status == STATUS_SUCCESS) && !sameDevice) {
      IoInvalidateDeviceRelations(pFdoData->UnderlyingPDO, BusRelations);
   }

   //
   // Queue a work item to release the last reference if remove/stop
   // hasn't already.
   //

   IoQueueWorkItem(pFdoData->EnumWorkItem, SerenumEnumThreadWorkItem,
                   DelayedWorkQueue, pFdoData);

   PsTerminateSystemThread(STATUS_SUCCESS);
}


NTSTATUS
SerenumStartProtocolThread(IN PFDO_DEVICE_DATA PFdoData)
{
   NTSTATUS status;
   OBJECT_ATTRIBUTES objAttrib;
   HANDLE handle;
   PVOID tmpObj;
   KIRQL oldIrql;
   PIO_WORKITEM pWorkItem;

   InitializeObjectAttributes(&objAttrib, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

   pWorkItem = IoAllocateWorkItem(PFdoData->Self);

   if (pWorkItem == NULL) {
       return STATUS_INSUFFICIENT_RESOURCES;
   }

   PFdoData->EnumWorkItem = pWorkItem;

   status = PsCreateSystemThread(&handle, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                 SerenumEnumThread, PFdoData);

   if (NT_SUCCESS(status)) {

      ASSERT(PFdoData->ThreadObj == NULL);

      //
      // We do this merely to get an object pointer that the remove
      // code can wait on.
      //

      status = ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL,
                                         KernelMode, &tmpObj, NULL);


      KeAcquireSpinLock(&PFdoData->EnumerationLock, &oldIrql);

      if (NT_SUCCESS(status)) {
         PFdoData->ThreadObj = tmpObj;
         KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);
      } else {
         //
         // The thread may be done by now, so no one would need to
         // synchronize with it.
         //

         PFdoData->ThreadObj = NULL;
         PFdoData->EnumWorkItem = NULL;
         KeReleaseSpinLock(&PFdoData->EnumerationLock, oldIrql);

      }

      //
      // Close the handle so the only references possible are the ones
      // for the thread itself and the one either the work item or
      // remove will take care of
      //

      ZwClose(handle);
   } else {
       PFdoData->EnumWorkItem = NULL;
       IoFreeWorkItem(pWorkItem);
   }

   return status;
}


#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#pragma warning(default:4127) // conditional expression is constant




