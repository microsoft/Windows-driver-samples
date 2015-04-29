/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrCmd.c

Abstract:

   Basic command functions for SCM PSCR smartcard reader

Author:

   Andreas Straub

Environment:


   Win 95      Sys... calls are resolved by Pscr95Wrap.asm functions and
            Pscr95Wrap.h macros, resp.

   NT 4.0      Sys... functions resolved by PscrNTWrap.c functions and
            PscrNTWrap.h macros, resp.

Notes:

   The file pscrcmd.c was reviewed by LCA in June 2011 and per license is
   acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:

   Andreas Straub       8/18/1997   1.00  Initial Version
   Andreas Straub       9/24/1997   1.02  delay for read/write removed

--*/

#include <PscrNT.h>
#include <PscrRdWr.h>
#include <PscrCmd.h>

NTSTATUS
CmdResetInterface(
                 PREADER_EXTENSION ReaderExtension
                 )
/*++
CmdResetInterface:

   Performs a reset of the reader interface (NOT of the PCMCIA controller)
   - flush available data
   - set RESET bit
   - perform a buffer size exchange between reader & host
   - enables interrupts for freeze events
   - disables default PTS

Arguments:
   ReaderExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_IO_DEVICE_ERROR

--*/
{

    NTSTATUS    NTStatus = STATUS_SUCCESS;
    UCHAR       Len,
    Tag,
    Cnt,
    InData[ TLV_BUFFER_SIZE ] = {0};
    PPSCR_REGISTERS   IOBase;
    UCHAR       EnableInterrupts[]   = { 0x28, 0x01, 0x01};


    IOBase = ReaderExtension->IOBase;

   // discard any data
    PscrFlushInterface( ReaderExtension );

   // reset reader
    WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_RESET_BIT );
    SysDelay( DELAY_WRITE_PSCR_REG );
    WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );

    NTStatus = PscrWait( ReaderExtension, PSCR_DATA_AVAIL_BIT );

   // read & check vendor string
    if ( NT_SUCCESS( NTStatus )) {

        WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );
        SysDelay( DELAY_WRITE_PSCR_REG );
      //
      // get actual len from TLV list
      //
        READ_PORT_UCHAR( &IOBase->SizeMSReg );
        READ_PORT_UCHAR( &IOBase->SizeLSReg );

        Tag = READ_PORT_UCHAR( &IOBase->DataReg );
        Len = READ_PORT_UCHAR( &IOBase->DataReg );
      //
      // avoid overwrite of buffer
      //
        if ( Len > TLV_BUFFER_SIZE ) {
            Len = TLV_BUFFER_SIZE;
        }
        for ( Cnt = 0; Cnt < Len; Cnt++ ) {
            InData[ Cnt ] = READ_PORT_UCHAR( &IOBase->DataReg );
        }
        WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
      //
      // check vendor string
      //
        if ( SysCompareMemory(
                             InData,
                             PSCR_ID_STRING,
                             sizeof( PSCR_ID_STRING )
                             )) {
            NTStatus = STATUS_IO_DEVICE_ERROR;
        } else {
         //
         // vendor string was correct, check buffer size
         //
            WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_SIZE_READ_BIT );
            NTStatus = PscrWait( ReaderExtension, PSCR_DATA_AVAIL_BIT );
         //
         // reader ready to transfer interface buffer size
         //
            if ( NT_SUCCESS( NTStatus )) {
            //
            // set size read & host control
            //
                WRITE_PORT_UCHAR(
                                &IOBase->CmdStatusReg,
                                PSCR_SIZE_READ_BIT | PSCR_HOST_CONTROL_BIT
                                );

                SysDelay( DELAY_WRITE_PSCR_REG );
            //
            // read buffer length
            //
                Len = READ_PORT_UCHAR( &IOBase->SizeMSReg );
                Len = READ_PORT_UCHAR( &IOBase->SizeLSReg );
            //
            // avoid overwrite of buffer
            //
                if ( Len > TLV_BUFFER_SIZE ) {
                    Len = TLV_BUFFER_SIZE;
                }
                for ( Cnt = 0; Cnt < Len; Cnt++ ) {
                    InData[ Cnt ] = READ_PORT_UCHAR( &IOBase->DataReg );
                }
            //
            // transfer of interface buffer size okay
            //
                WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
                SysDelay( DELAY_WRITE_PSCR_REG );
            //
            // notify the reader about the supported buffer size
            //
                WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );
                SysDelay( DELAY_WRITE_PSCR_REG );

                WRITE_PORT_UCHAR( &IOBase->SizeMSReg, 0 );
                WRITE_PORT_UCHAR( &IOBase->SizeLSReg, 2 );
            //
            // Write the same data buffer size as the one we just got.
            //
		if (Len >= 1) {
                    WRITE_PORT_UCHAR( &IOBase->DataReg, InData[ 0 ] );
                    WRITE_PORT_UCHAR( &IOBase->DataReg, InData[ 1 ] );
		}
            //
            // store the size to report to the lib
            //  The maximum buffer size of the reader is to betrieved with
                //  ((ULONG)InData[ 1 ] << 8) | InData[ 0 ]
                //
                ReaderExtension->MaxIFSD = 254;

            //
            // let the reader process the size write command
            //
                WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_SIZE_WRITE_BIT );
                NTStatus = PscrWait( ReaderExtension, PSCR_FREE_BIT );
            }
        }
    }
   //
   // clean up any host control settings
   //
    WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
   //
   // enable interrupts
   //
    CmdSetInterfaceParameter(
                            ReaderExtension,
                            ReaderExtension->Device,
                            EnableInterrupts,
                            sizeof( EnableInterrupts )
                            );

    return( NTStatus );
}

NTSTATUS
CmdReset(
        PREADER_EXTENSION ReaderExtension,
        UCHAR          Device,
        BOOLEAN           WarmReset,
        PUCHAR            pATR,
        PULONG            pATRLength
        )
/*++
CmdReset:
   performs a reset of the reader / ICC

Arguments:
   ReaderExtension      context of call
   Device            device requested ( ICC_1, ICC_2, PSCR )
   WarmReset         kind of ICC reset
   pATR           ptr to ATR buffer, NULL if no ATR required
   pATRLength        size of ATR buffer / length of ATR

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_UNRECOGNIZED_MEDIA
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    UCHAR    IOData[ MAX_T1_BLOCK_SIZE ],
    P2;
    USHORT      ICCStatus;
    ULONG    IOBytes;

   // ATR from the smartcard requestet? P2 = 1
    P2 = 0;
    if (( pATR != NULL ) && ( pATRLength != NULL )) {
        if ( *pATRLength > 0 )
            P2 = 0x01;
    }

   // build the RESET command.
    IOData[ PSCR_NAD ] = NAD_TO_PSCR;
    IOData[ PSCR_PCB ] = PCB_DEFAULT;
    IOData[ PSCR_LEN ] = 0x05;

    if ( WarmReset == TRUE ) {
        IOData[ PSCR_INF+0 ] = CLA_WARM_RESET;
        IOData[ PSCR_INF+1 ] = INS_WARM_RESET;
    } else {
        IOData[ PSCR_INF+0 ] = CLA_RESET;
        IOData[ PSCR_INF+1 ] = INS_RESET;
    }
    IOData[ PSCR_INF+2 ] = Device;
    IOData[ PSCR_INF+3 ] = P2;
    IOData[ PSCR_INF+4 ] = 0x00;

   // write command
    NTStatus = PscrWrite(
                        ReaderExtension,
                        IOData,
                        8,
                        &IOBytes
                        );

    if ( NT_SUCCESS( NTStatus )) {
      // read data
        IOBytes = 0;
        NTStatus = PscrRead(
                           ReaderExtension,
                           IOData,
                           MAX_T1_BLOCK_SIZE,
                           &IOBytes
                           );

      // error detection
        if ( NT_SUCCESS( NTStatus )) {
         //
         // the location of the error code in the buffer
         // is: ( data ) - STATUS_MSB - STATUS_LSB - EPILOGUE
         //
            ICCStatus = (( USHORT )IOData[ IOBytes-PSCR_EPILOGUE_LENGTH-2 ]) << 8;
            ICCStatus |= ( USHORT )IOData[ IOBytes-PSCR_EPILOGUE_LENGTH-1 ];

            switch ( ICCStatus ) {
            case PSCR_SW_SYNC_ATR_SUCCESS:
            case PSCR_SW_ASYNC_ATR_SUCCESS:
                break;

            case PSCR_SW_NO_ICC:
                NTStatus = STATUS_NO_MEDIA;
                break;

            case PSCR_SW_NO_PROTOCOL:
            case PSCR_SW_NO_ATR:
            case PSCR_SW_NO_ATR_OR_PROTOCOL:
            case PSCR_SW_NO_ATR_OR_PROTOCOL2:
            case PSCR_SW_ICC_NOT_ACTIVE:
            case PSCR_SW_NON_SUPPORTED_PROTOCOL:
            case PSCR_SW_PROTOCOL_ERROR:
            default:
                NTStatus = STATUS_UNRECOGNIZED_MEDIA;
            }
         //
         // copy ATR if required
         //
            if ( NT_SUCCESS( NTStatus )) {
                if ( P2 == 0x01 ) {
                    IOBytes -= PSCR_PROLOGUE_LENGTH + PSCR_EPILOGUE_LENGTH;
                    if ( IOBytes > *pATRLength ) {
                        IOBytes = *pATRLength;
                    }
                    SysCopyMemory(
                                 pATR,
                                 &IOData[ PSCR_PROLOGUE_LENGTH ],
                                 IOBytes
                                 );
                    *pATRLength = IOBytes;
                }
            }
        }
    }
    return( NTStatus );
}


NTSTATUS
CmdDeactivate(
             PREADER_EXTENSION ReaderExtension,
             UCHAR          Device
             )
/*++
CmdDeactivate:
   Deactivates the requested device

Arguments:
   ReaderExtension      context of call
   Device            requested device

Return Value:
   STATUS_SUCCESS
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    UCHAR    IOData[  MAX_T1_BLOCK_SIZE ];
    ULONG    IOBytes;

   //
   // build the DEACTIVATE command.
   //
    IOData[ PSCR_NAD ] = NAD_TO_PSCR;
    IOData[ PSCR_PCB ] = PCB_DEFAULT;
    IOData[ PSCR_LEN ] = 0x05;

    IOData[ PSCR_INF+0 ] = CLA_DEACTIVATE;
    IOData[ PSCR_INF+1 ] = INS_DEACTIVATE;
    IOData[ PSCR_INF+2 ] = Device;
    IOData[ PSCR_INF+3 ] = 0x00;
    IOData[ PSCR_INF+4 ] = 0x00;
   //
   // write command
   //
    NTStatus = PscrWrite(
                        ReaderExtension,
                        IOData,
                        8,
                        &IOBytes
                        );

    if ( NT_SUCCESS( NTStatus )) {
      //
      // read data to trap communication errors
      //
        IOBytes = 0;
        NTStatus = PscrRead(
                           ReaderExtension,
                           IOData,
                           MAX_T1_BLOCK_SIZE,
                           &IOBytes
                           );
    }
    return( NTStatus );
}

NTSTATUS
CmdReadBinary(
             PREADER_EXTENSION ReaderExtension,
             USHORT            Offset,
             PUCHAR            pBuffer,
             PULONG            pBufferLength
             )
/*++
CmdReadBinary:
   read binary data from an PSCR data file

Arguments:
   ReaderExtension      context of call
   Offset            offset in file
   pBuffer           ptr to data buffer
   pBufferLength     length of buffer / number of bytes read

Return Value:

   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS   NTStatus = STATUS_SUCCESS;
    UCHAR      IOData[ MAX_T1_BLOCK_SIZE ];
    USHORT      ICCStatus;
    ULONG    IOBytes;
   //
   // check parameters
   //
    if (( pBuffer == NULL ) || ( pBufferLength == NULL)) {
        NTStatus = STATUS_INVALID_PARAMETER;
    } else {
      //
      // build the READ BINARY command
      //
        IOData[ PSCR_NAD] = NAD_TO_PSCR;
        IOData[ PSCR_PCB] = PCB_DEFAULT;
        IOData[ PSCR_LEN] = 0x05;

        IOData[ PSCR_INF+0 ] = CLA_READ_BINARY;
        IOData[ PSCR_INF+1 ] = INS_READ_BINARY;
        IOData[ PSCR_INF+2 ] = HIBYTE( Offset );
        IOData[ PSCR_INF+3 ] = LOBYTE( Offset );
        IOData[ PSCR_INF+4 ] = 0x00;
      //
      // write command
      //
        NTStatus = PscrWrite(
                            ReaderExtension,
                            IOData,
                            8,
                            &IOBytes
                            );

        if ( NT_SUCCESS( NTStatus )) {
         //
         // read data
         //
            IOBytes = 0;
            NTStatus = PscrRead(
                               ReaderExtension,
                               IOData,
                               MAX_T1_BLOCK_SIZE,
                               &IOBytes
                               );

            if ( NT_SUCCESS( NTStatus )) {
            //
            // error check
            //
                ICCStatus =
                ((USHORT)IOData[ IOBytes-2-PSCR_EPILOGUE_LENGTH ]) << 8;
                ICCStatus |=
                (USHORT)IOData[ IOBytes-1-PSCR_EPILOGUE_LENGTH ];

                switch ( ICCStatus ) {
                case PSCR_SW_FILE_NO_ACCEPPTED_AUTH:
                case PSCR_SW_FILE_NO_ACCESS:
                case PSCR_SW_FILE_BAD_OFFSET:
                case PSCR_SW_END_OF_FILE_READ:
                    NTStatus = STATUS_UNSUCCESSFUL;
                    break;
                default:
                    break;
                }
            //
            // copy data
            //
                if ( NT_SUCCESS( NTStatus )) {
                    if ( *pBufferLength <
                         IOBytes-PSCR_PROLOGUE_LENGTH-PSCR_STATUS_LENGTH ) {
                        NTStatus = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        ( *pBufferLength ) =
                        IOBytes-PSCR_PROLOGUE_LENGTH-PSCR_STATUS_LENGTH;
                        SysCopyMemory(
                                     pBuffer,
                                     &IOData[ PSCR_PROLOGUE_LENGTH ],
                                     *pBufferLength
                                     );
                    }
                }
            }
        }
    }
    return( NTStatus );
}

NTSTATUS
CmdSelectFile(
             PREADER_EXTENSION ReaderExtension,
             USHORT            FileId
             )
/*++
CmdSelectFile:
   selects a file/directoy of the reader

Arguments:
   ReaderExtension      context of call
   FileId            ID of file

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
   error values from PscrRead / PscrWrite
--*/
{
    NTSTATUS   NTStatus = STATUS_SUCCESS;
    UCHAR      IOData[ MAX_T1_BLOCK_SIZE ];
    USHORT      ICCStatus;
    ULONG    IOBytes;

   //
   // build the SELECT FILE command
   //
    IOData[ PSCR_NAD ] = NAD_TO_PSCR;
    IOData[ PSCR_PCB ] = PCB_DEFAULT;
    IOData[ PSCR_LEN ] = 0x07;

    IOData[ PSCR_INF+0 ] = CLA_SELECT_FILE;
    IOData[ PSCR_INF+1 ] = INS_SELECT_FILE;
    IOData[ PSCR_INF+2 ] = 0x00;
    IOData[ PSCR_INF+3 ] = 0x00;
    IOData[ PSCR_INF+4 ] = 0x02;
    IOData[ PSCR_INF+5 ] = HIBYTE( FileId );
    IOData[ PSCR_INF+6 ] = LOBYTE( FileId );
   //
   // write command
   //
    NTStatus = PscrWrite(
                        ReaderExtension,
                        IOData,
                        10,
                        &IOBytes
                        );

    if ( NT_SUCCESS( NTStatus )) {
      //
      // get the response of the reader
      //
        IOBytes = 0;
        NTStatus = PscrRead(
                           ReaderExtension,
                           IOData,
                           MAX_T1_BLOCK_SIZE,
                           &IOBytes
                           );

        if ( NT_SUCCESS( NTStatus )) {
         //
         // check errors
         //
            ICCStatus =
            ((USHORT)IOData[ IOBytes-2-PSCR_EPILOGUE_LENGTH ]) << 8;
            ICCStatus |=
            (USHORT)IOData[ IOBytes-1-PSCR_EPILOGUE_LENGTH ];

            if ( ICCStatus == PSCR_SW_FILE_NOT_FOUND ) {
                NTStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }
    return( NTStatus );
}

NTSTATUS
CmdSetInterfaceParameter(
                        PREADER_EXTENSION ReaderExtension,
                        UCHAR          Device,
                        PUCHAR            pTLVList,
                        UCHAR          TLVListLen
                        )
/*++
CmdSetInterfaceParameter:
   Sets the interface pareameter of the ICC interface to the values specified
   in the TLV list

Arguments:
   ReaderExtension      context of call
   Device            device
   pTLVList       ptr to list of tag-len-value's specified by caller
   TLVListLen        length of list

Return Value:
   STATUS_SUCCESS
   STATUS_INVALID_PARAMETER
   STATUS_INVALID_DEVICE_STATE
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS   NTStatus = STATUS_SUCCESS;
    UCHAR      IOData[ MAX_T1_BLOCK_SIZE ];
    USHORT      ICCStatus;
    ULONG    IOBytes;

   //
   // check parameter
   //
    if ( pTLVList == NULL ) {
        NTStatus = STATUS_INVALID_PARAMETER;
    } else {
      //
      // build the SET INTERFACE PARAMETER command
      //
        IOData[ PSCR_NAD ] = NAD_TO_PSCR;
        IOData[ PSCR_PCB ] = PCB_DEFAULT;
        IOData[ PSCR_LEN ] = 0x05 + TLVListLen;

        IOData[ PSCR_INF+0 ] = CLA_SET_INTERFACE_PARAM;
        IOData[ PSCR_INF+1 ] = INS_SET_INTERFACE_PARAM;
        IOData[ PSCR_INF+2 ] = Device;
        IOData[ PSCR_INF+3 ] = 0x00;
        IOData[ PSCR_INF+4 ] = TLVListLen;

        SysCopyMemory( &IOData[ PSCR_INF+5 ], pTLVList, TLVListLen );
      //
      // write command
      //
        NTStatus = PscrWrite(
                            ReaderExtension,
                            IOData,
                            8 + TLVListLen,
                            &IOBytes
                            );

        if ( NT_SUCCESS( NTStatus )) {

         // do an dummy read to catch errors.
            IOBytes = 0;
            NTStatus = PscrRead(
                               ReaderExtension,
                               IOData,
                               MAX_T1_BLOCK_SIZE,
                               &IOBytes
                               );

            if ( NT_SUCCESS( NTStatus )) {

            // check error
                ICCStatus =
                ((USHORT)IOData[ IOBytes - 2 - PSCR_EPILOGUE_LENGTH ]) << 8;
                ICCStatus |=
                (USHORT)IOData[ IOBytes - 1 - PSCR_EPILOGUE_LENGTH ];

                if ( ICCStatus != 0x9000 ) {
                    NTStatus = STATUS_INVALID_DEVICE_STATE;
                }
            }
        }
    }
    return( NTStatus );
}


NTSTATUS
CmdReadStatusFile (
                  PREADER_EXTENSION ReaderExtension,
                  UCHAR          Device,
                  PUCHAR            pTLVList,
                  PULONG            pTLVListLen
                  )
/*++
CmdReadStatusFile:
   read the status file of the requested device from the reader filesystem

Arguments:
   ReaderExtension      context of call
   Device            requested device
   pTLVList       ptr to list (i.e. the status file)
   pTLVListLen       length of buffer / returned list

Return Value:
   STATUS_SUCCESS
   STATUS_BUFFER_TOO_SMALL
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS       NTStatus = STATUS_UNSUCCESSFUL;
    UCHAR          IOData[ MAX_T1_BLOCK_SIZE ];
    ULONG          IOBytes;

   // select ICC status file if it's not the active file
    if ( ReaderExtension->StatusFileSelected == FALSE ) {

      // select master file on reader
        NTStatus = CmdSelectFile( ReaderExtension, FILE_MASTER );

      // select ICC directory
        if ( NT_SUCCESS( NTStatus )) {
            if ( Device != DEVICE_ICC1 ) {
                NTStatus = STATUS_UNSUCCESSFUL;
            } else {
                NTStatus = CmdSelectFile(
                                        ReaderExtension,
                                        FILE_ICC1_DIR
                                        );

            // select status file
                if ( NT_SUCCESS( NTStatus )) {
                    NTStatus = CmdSelectFile(
                                            ReaderExtension,
                                            FILE_ICC1_DIR_STATUS
                                            );
                    if ( NT_SUCCESS( NTStatus )) {
                        ReaderExtension->StatusFileSelected = TRUE;
                    }
                }
            }
        }
    }

   // read status file if successful selected
    if ( ReaderExtension->StatusFileSelected == TRUE ) {
        IOBytes = MAX_T1_BLOCK_SIZE;
        NTStatus = CmdReadBinary(
                                ReaderExtension,
                                0,
                                IOData,
                                &IOBytes
                                );

      // copy data to user buffer
        if ( NT_SUCCESS( NTStatus )) {
            if (( pTLVList != NULL ) && ( IOBytes < *pTLVListLen )) {
                *pTLVListLen = IOBytes;
                SysCopyMemory( pTLVList, IOData, IOBytes );
            } else {
                NTStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }
    return( NTStatus );
}


NTSTATUS
CmdGetFirmwareRevision (
                       PREADER_EXTENSION ReaderExtension
                       )
/*++
CmdGetFirmwareRevision:
   get the firmware revision of the reader. Ther firmware revision is found
   in the PSCR configuration file (ID 0x0020) in the master directory.
   The tag of the revision is 0x0F, and the value is coded as an ASCII string,
   p.E. "2.20"

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS       NTStatus = STATUS_SUCCESS;
    UCHAR          TLVList[ MAX_T1_BLOCK_SIZE ],
    Len;
    char           Revision[ 0x10 ],
    UpdateKey[ 0x10 ];
    ULONG          IOBytes;
   //
   // select master file on reader
   //
    NTStatus = CmdSelectFile( ReaderExtension, FILE_MASTER );
   //
   // select pscr configuration file
   //
    if ( NT_SUCCESS( NTStatus )) {
        NTStatus = CmdSelectFile( ReaderExtension, FILE_PSCR_CONFIG );
      //
      // read confiuration file
      //
        if ( NT_SUCCESS( NTStatus )) {
            IOBytes = MAX_T1_BLOCK_SIZE;
            NTStatus = CmdReadBinary(
                                    ReaderExtension,
                                    0,
                                    TLVList,
                                    &IOBytes
                                    );
         //
         // get the value of revison
         //
            if ( NT_SUCCESS( NTStatus )) {
                Len = sizeof(Revision);
                CmdGetTagValue(
                              TAG_SOFTWARE_REV,
                              TLVList,
                              IOBytes,
                              &Len,
                              Revision
                              );
            //
            // the coding is always X.YY (in ASCII), so we can get the numeric
            // values hardcoded by taking the low nibbles of the char's.
            //
                ReaderExtension->FirmwareMajor =   Revision[0] & 0x0F;
                ReaderExtension->FirmwareMinor = ( Revision[2] & 0x0F ) << 4;
                ReaderExtension->FirmwareMinor |=  Revision[3] & 0x0F;
            //
            // get value of update key
            //
                Len = sizeof(UpdateKey);
                CmdGetTagValue(
                              TAG_UPDATE_KEY,
                              TLVList,
                              IOBytes,
                              &Len,
                              UpdateKey
                              );

                ReaderExtension->UpdateKey = UpdateKey[0];
            }
        }
    }
    ReaderExtension->StatusFileSelected = FALSE;
    return( NTStatus );
}

NTSTATUS
CmdPscrCommand (
               PREADER_EXTENSION ReaderExtension,
               PUCHAR            pInData,
               ULONG          InDataLen,
               PUCHAR            pOutData,
               ULONG          OutDataLen,
               PULONG            pNBytes
               )
/*++
CmdPscrCommand:
   send a command transparent to the reader

Arguments:
   ReaderExtension      context of call
   pInData,       ptr to input buffer
   InDataLen,        len of input buffer
   pOutData,         ptr to output buffer
   OutDataLen,       len of output buffer
   pNBytes           number of bytes transferred

Return Value:
   STATUS_SUCCESS
   STATUS_INVALID_PARAMETER
   error values from PscrRead / PscrWrite

--*/
{
    NTSTATUS       NTStatus = STATUS_SUCCESS;
    UCHAR          IOData[ MAX_T1_BLOCK_SIZE ] = { 0};
    ULONG          IOBytes;
   //
   // the function is used for generic ioctl's, so carful ALL
   // parametes will be checked!
   //
    if ( ( pInData == NULL ) ||
         ( pOutData == NULL ) ||
         ( pNBytes == NULL ) ||
         (InDataLen == 0 ) ||
         (OutDataLen == 0 )
       ) {
        NTStatus = STATUS_INVALID_PARAMETER;
    } else {
        IOBytes = 0;
        NTStatus = PscrWriteDirect(
                                  ReaderExtension,
                                  pInData,
                                  InDataLen,
                                  &IOBytes
                                  );

        if ( NT_SUCCESS( NTStatus )) {
         //
         // get result. ignore all reader errors
         //
            IOBytes = 0;
            NTStatus = PscrRead(
                               ReaderExtension,
                               IOData,
                               MAX_T1_BLOCK_SIZE,
                               &IOBytes
                               );
         //
         // tranfer data
         //
            if ( IOBytes > OutDataLen ) {
                NTStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                *pNBytes = IOBytes;
                SysCopyMemory( pOutData, IOData, IOBytes );
            }
        }
    }
    return( NTStatus );
}

NTSTATUS
CmdGetTagValue (
               UCHAR Tag,
               PUCHAR   pTLVList,
               ULONG TLVListLen,
               PUCHAR   pTagLen,
               PVOID pTagVal
               )
/*++
CmdGetTagValue:
   scans a TLV list for the value of a user specified tag
   it is assumed, the caller knows the kind of the requested
   field, so only the ptr to the buffer will be checked

Arguments:
   Tag            requested Tag
   pTLVList    ptr to list
   TLVListLen     length of list
   pTagLen        ptr to length
   pTagVal        ptr to value

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL
   STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    ULONG    Idx;
    UCHAR maxLen;
   //
   // step through the given list
   //
    if (( pTLVList != NULL ) && ( pTagVal != NULL ) && ( pTagLen != NULL )) {
        maxLen = *pTagLen;
      //
      // look for requested tag
      //
        Idx = 0;
        while ( Idx < TLVListLen ) {
            if ( pTLVList[ Idx ] == Tag ) {
            //
            // ASSUMED THE CALLER KNOWS KIND OF FIELD!!!
            //

                if (pTLVList[Idx+1] > maxLen) {
                    NTStatus = STATUS_UNSUCCESSFUL;
                    break;
                }
                *pTagLen = pTLVList[ Idx + 1 ];
                SysCopyMemory(
                             pTagVal,
                             &pTLVList[ Idx+2 ],
                             pTLVList[ Idx+1 ]
                             );

                break;
            }
            Idx += pTLVList[ Idx+1 ] + 2;
        }
        if ( Idx >= TLVListLen ) {
            NTStatus = STATUS_UNSUCCESSFUL;
        }
    } else {
        NTStatus = STATUS_INVALID_PARAMETER;
    }
    return( NTStatus );
}

// ------------------------------- END OF FILE -------------------------------

