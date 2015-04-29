/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrCB.c

Abstract:

        callback handler for PSCR.xxx driver

Author:

        Andreas Straub

Environment:

        Win 95          Sys... calls are resolved by Pscr95Wrap.asm functions and
                                Pscr95Wrap.h macros, resp.

        NT      4.0             Sys... functions resolved by PscrNTWrap.c functions and
                                PscrNTWrap.h macros, resp.

Notes:

        The file pscrcb.c was reviewed by LCA in June 2011 and per license is
        acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:

        Andreas Straub                  8/18/1997       1.00    Initial Version
        Andreas Straub                  9/24/1997       1.02    Flush Interface if card tracking
                                                                                                requested

--*/

#include <PscrNT.h>
#include <PscrRdWr.h>
#include <PscrCmd.h>
#include <PscrCB.h>


NTSTATUS
CBCardPower(
           PSMARTCARD_EXTENSION SmartcardExtension
           )
/*++

CBCardPower:
        callback handler for SMCLIB RDF_CARD_POWER

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_BUFFER_TOO_SMALL

--*/
{
    NTSTATUS                        NTStatus = STATUS_SUCCESS;
    UCHAR                           ATRBuffer[ ATR_SIZE ], TLVList[16];
    ULONG                           Command,
    ATRLength = 0;
    PREADER_EXTENSION       ReaderExtension;
    BYTE                CardState;
    KIRQL               irql;
#if DBG || DEBUG
    static PCHAR request[] = { "PowerDown",  "ColdReset", "WarmReset"};
#endif

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBCardPower: Enter, Request = %s\n",
                    request[SmartcardExtension->MinorIoControlCode])
                  );

    ReaderExtension = SmartcardExtension->ReaderExtension;

        //
        //      update actual power state
        //
    Command = SmartcardExtension->MinorIoControlCode;

    switch ( Command ) {
    case SCARD_WARM_RESET:

        //      if the card was not powerd, fall thru to cold reset
        KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                          &irql);

        if ( SmartcardExtension->ReaderCapabilities.CurrentState >
             SCARD_SWALLOWED ) {
            KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                              irql);

                                //      reset the card
            ATRLength = ATR_SIZE;
            NTStatus = CmdReset(
                               ReaderExtension,
                               ReaderExtension->Device,
                               TRUE,                           // warm reset
                               ATRBuffer,
                               &ATRLength
                               );

            break;
        } else {
            KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                              irql);

        }

                        //      warm reset not possible because card was not powerd
    case SCARD_COLD_RESET:

                        //      reset the card
        ATRLength = ATR_SIZE;
        NTStatus = CmdReset(
                           ReaderExtension,
                           ReaderExtension->Device,
                           FALSE,                          // cold reset
                           ATRBuffer,
                           &ATRLength
                           );
        break;

    case SCARD_POWER_DOWN:
        ATRLength = 0;
        NTStatus = CmdDeactivate(
                                ReaderExtension,
                                ReaderExtension->Device
                                );

                        //      discard old card status
        CardState = CBGetCardState(SmartcardExtension);
        CBUpdateCardState(SmartcardExtension, CardState, FALSE);
        break;
    }

    if (NT_SUCCESS(NTStatus)) {

        //
        // Set the 'restart of work waiting time' counter for T=0
        // This will send a WTX request for n NULL bytes received
        //
        TLVList[0] = TAG_SET_NULL_BYTES;
        TLVList[1] = 1;
        TLVList[2] = 0x05;

        NTStatus = CmdSetInterfaceParameter(
                                           ReaderExtension,
                                           DEVICE_READER,
                                           TLVList,
                                           3
                                           );
    }

    // Removing this assert as this causes unnecessary confusion.  The driver
    // passes ifdtest even with an error here.
    // NT_ASSERT(NT_SUCCESS(NTStatus));

        //      finish the request
    if ( NT_SUCCESS( NTStatus )) {
                //      update all neccessary data if an ATR was received
        if ( ATRLength > 2 ) {
                        //
                        //      the lib expects only the ATR, so we skip the
                        //      900x from the reader
                        //
            ATRLength -= 2;

                        //      copy ATR to user buffer buffer
            if ( ATRLength <= SmartcardExtension->IoRequest.ReplyBufferLength ) {
                SysCopyMemory(
                             SmartcardExtension->IoRequest.ReplyBuffer,
                             ATRBuffer,
                             ATRLength
                             );
                *SmartcardExtension->IoRequest.Information = ATRLength;
            }

                        //      copy ATR to card capability buffer
            if ( ATRLength <= MAXIMUM_ATR_LENGTH ) {
                SysCopyMemory(
                             SmartcardExtension->CardCapabilities.ATR.Buffer,
                             ATRBuffer,
                             ATRLength
                             );

                SmartcardExtension->CardCapabilities.ATR.Length =
                ( UCHAR )ATRLength;

                                //      let the lib update the card capabilities
                NTStatus = SmartcardUpdateCardCapabilities(
                                                          SmartcardExtension
                                                          );
            } else {
                NTStatus = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }

    if ( !NT_SUCCESS( NTStatus )) {
        switch ( NTStatus ) {
        case STATUS_NO_MEDIA:
        case STATUS_BUFFER_TOO_SMALL:
            break;

        case STATUS_TIMEOUT:
            NTStatus = STATUS_IO_TIMEOUT;
            break;

        default:
            NTStatus = STATUS_UNRECOGNIZED_MEDIA;
            break;
        }
        if( NTStatus != STATUS_SUCCESS )
        {

            // notify resman if card was removed
            CBUpdateCardState(
                SmartcardExtension,
                CBGetCardState( SmartcardExtension ),
                FALSE
                );

            // resman requires status ok to translate ntstatus to scard... error.
            if( SmartcardExtension->ReaderCapabilities.CurrentState <= SCARD_ABSENT )
            {
                SmartcardUpdateCardCapabilities( SmartcardExtension );
                NTStatus = STATUS_SUCCESS;
            }
        }
    }

    // start freeze routine, if an interrupt was sent, but no freeze data was read
    if( SmartcardExtension->ReaderExtension->RequestInterrupt )
        PscrFreeze( SmartcardExtension );

    // Clearing the buffers
    RtlZeroMemory (ATRBuffer, ATR_SIZE);
    RtlZeroMemory (TLVList, 16);

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBCardPower: Exit (%lx)\n", NTStatus )
                  );

    return( NTStatus );
}

NTSTATUS
CBSetProtocol(
             PSMARTCARD_EXTENSION SmartcardExtension
             )

/*++

CBSetProtocol:
        callback handler for SMCLIB RDF_SET_PROTOCOL

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_BUFFER_TOO_SMALL
        STATUS_INVALID_DEVICE_STATE
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS NTStatus = STATUS_PENDING;
    USHORT SCLibProtocol;
    UCHAR TLVList[ TLV_BUFFER_SIZE ];
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    KIRQL irql;

    KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                      &irql);

    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC) {
        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          irql);


        return STATUS_SUCCESS;
    } else {
        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          irql);

    }

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBSetProtocol: Enter\n" )
                  );

    SCLibProtocol = ( USHORT )( SmartcardExtension->MinorIoControlCode );

    if (SCLibProtocol & (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)) {
                //
                //      setup the TLV list for the Set Interface Parameter List
                //
        TLVList[ 0 ] = TAG_ICC_PROTOCOLS;
        TLVList[ 1 ] = 0x01;
        TLVList[ 2 ] =
        (SCLibProtocol & SCARD_PROTOCOL_T1 ? PSCR_PROTOCOL_T1 : PSCR_PROTOCOL_T0);

                //      do the PTS
        NTStatus = CmdSetInterfaceParameter(
                                           ReaderExtension,
                                           ReaderExtension->Device,
                                           TLVList,
                                           3                       // size of list
                                           );

    } else {

                //      we don't support other modi
        NTStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

        //      if protocol selection failed, prevent from calling invalid protocols
    if ( NT_SUCCESS( NTStatus )) {

        KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                          &irql);
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
        SCLibProtocol = (SCLibProtocol & SCARD_PROTOCOL_T1 &
                         SmartcardExtension->CardCapabilities.Protocol.Supported) ?
                        SCARD_PROTOCOL_T1 :
                        SCARD_PROTOCOL_T0;

        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          irql);
    } else {
        SCLibProtocol = SCARD_PROTOCOL_UNDEFINED;
    }

        //      Return the selected protocol to the caller.
    SmartcardExtension->CardCapabilities.Protocol.Selected = SCLibProtocol;
    *( PULONG )( SmartcardExtension->IoRequest.ReplyBuffer ) = SCLibProtocol;
    *( SmartcardExtension->IoRequest.Information ) = sizeof( ULONG );

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBSetProtocol: Exit (%lx)\n", NTStatus )
                  );

    return( NTStatus );
}

NTSTATUS
CBTransmit(
          PSMARTCARD_EXTENSION SmartcardExtension
          )
/*++

CBTransmit:
        callback handler for SMCLIB RDF_TRANSMIT

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS  NTStatus = STATUS_SUCCESS;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBTransmit: Enter\n" )
                  );

        //      dispatch on the selected protocol
    switch ( SmartcardExtension->CardCapabilities.Protocol.Selected ) {
    case SCARD_PROTOCOL_T0:
        NTStatus = CBT0Transmit( SmartcardExtension );
        break;

    case SCARD_PROTOCOL_T1:
        NTStatus = CBT1Transmit( SmartcardExtension );
        break;

    case SCARD_PROTOCOL_RAW:
        NTStatus = CBRawTransmit( SmartcardExtension );
        break;

    default:
        NTStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBTransmit: Exit (%lx)\n", NTStatus )
                  );

    return( NTStatus );
}

NTSTATUS
CBRawTransmit(
             PSMARTCARD_EXTENSION SmartcardExtension
             )
/*++

CBRawTransmit:
        finishes the callback RDF_TRANSMIT for the RAW protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS                    NTStatus = STATUS_SUCCESS;
    UCHAR                           TLVList[ TLV_BUFFER_SIZE ],
    Val,
    Len;
    ULONG                           TLVListLen;
    PREADER_EXTENSION       ReaderExtension;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBRawTransmit: Enter\n" )
                  );

    ReaderExtension = SmartcardExtension->ReaderExtension;
        //
        //      read the status file of ICC1 from the reader
        //
    TLVListLen = TLV_BUFFER_SIZE;
    NTStatus = CmdReadStatusFile(
                                ReaderExtension,
                                ReaderExtension->Device,
                                TLVList,
                                &TLVListLen
                                );

        //
        //      check the active protocol of the reader
        //
    if ( NT_SUCCESS( NTStatus )) {
        Len = sizeof(Val);
        NTStatus = CmdGetTagValue(
                                 TAG_ICC_PROTOCOLS,
                                 TLVList,
                                 TLVListLen,
                                 &Len,
                                 ( PVOID ) &Val
                                 );

                //      execute the active protocol
        if ( NT_SUCCESS( NTStatus )) {

                        //      translate the actual protocol to a value the lib can understand
            switch ( Val ) {
            case PSCR_PROTOCOL_T0:
                NTStatus = CBT0Transmit( SmartcardExtension );
                break;
            case PSCR_PROTOCOL_T1:
                NTStatus = CBT1Transmit( SmartcardExtension );
                break;
            default:
                NTStatus = STATUS_UNSUCCESSFUL;
                break;
            }
        }
    }
    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBRawTransmit: Exit (%lx)\n", NTStatus )
                  );
    return( NTStatus );
}

NTSTATUS
CBT1Transmit(
            PSMARTCARD_EXTENSION SmartcardExtension
            )
/*++

CBT1Transmit:
        finishes the callback RDF_TRANSMIT for the T1 protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS    NTStatus = STATUS_SUCCESS;
    ULONG           IOBytes;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBT1Transmit: Enter\n" )
                  );
        //
        //      use the lib support to construct the T=1 packets
        //
    do {
                //
                //      no header for the T=1 protocol
                //
        SmartcardExtension->SmartcardRequest.BufferLength = 0;
                //
                //      SCM-TM: Siemens 4440 accepts only NAD=0!!!
                //
        SmartcardExtension->T1.NAD = 0;
                //
                //      let the lib setup the T=1 APDU & check for errors
                //
        NTStatus = SmartcardT1Request( SmartcardExtension );
        if ( NT_SUCCESS( NTStatus )) {

                        //      send command (don't calculate LRC because CRC may be used!)
            IOBytes = 0;
            NTStatus = PscrWriteDirect(
                                      SmartcardExtension->ReaderExtension,
                                      SmartcardExtension->SmartcardRequest.Buffer,
                                      SmartcardExtension->SmartcardRequest.BufferLength,
                                      &IOBytes
                                      );
            if ( !NT_SUCCESS(NTStatus)) {
                SmartcardDebug(
                    DEBUG_TRACE,
                    ( "PSCR!CBT1Transmit: PscrWriteDirect error (%lx)\n", NTStatus )
                    );
            }
                        //
                        //      extend the timeout if a Wtx request was sent by the card. if the
                        //      card responds before the waiting time extension expires, the data are
                        //      buffered in the reader. A delay without polling the reader status
                        //      slows down the performance of the driver, but wtx is an exeption,
                        //      not the rule.
                        //
            if (SmartcardExtension->T1.Wtx) {
                SysDelay(
                        (( SmartcardExtension->T1.Wtx *
                           SmartcardExtension->CardCapabilities.T1.BWT + 999L )/
                         1000L)
                        );

            }

                        //      get response
            SmartcardExtension->SmartcardReply.BufferLength = 0;
            NTStatus = PscrRead(
                               SmartcardExtension->ReaderExtension,
                               SmartcardExtension->SmartcardReply.Buffer,
                               MAX_T1_BLOCK_SIZE,
                               &SmartcardExtension->SmartcardReply.BufferLength
                               );

                        //      if PscrRead detects an LRC error, ignore it (maybe CRC used)
            if ( NTStatus == STATUS_CRC_ERROR ) {
                SmartcardDebug(
                    DEBUG_TRACE,
                    ( "PSCR!CBT1Transmit: PscrRead CRC error (%lx)\n", NTStatus )
                    );
            }

            //
            // We even continue if the prev. read failed.
            // We let the smart card library continue, because it might
            // send a resynch. request in case of a timeout
            //
            NTStatus = SmartcardT1Reply( SmartcardExtension );
        }

        //      continue if the lib wants to send the next packet
    } while ( NTStatus == STATUS_MORE_PROCESSING_REQUIRED );

    // start freeze routine, if an interrupt was sent, but no freeze data was read
    if( SmartcardExtension->ReaderExtension->RequestInterrupt )
        PscrFreeze( SmartcardExtension );

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBT1Transmit: Exit (%lx)\n", NTStatus )
                  );

    return( NTStatus );
}

NTSTATUS
CBT0Transmit(
            PSMARTCARD_EXTENSION SmartcardExtension
            )
/*++

CBT0Transmit:
        finishes the callback RDF_TRANSMIT for the T0 protocol

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_TIMEOUT
        STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS NTStatus = STATUS_SUCCESS;
    PUCHAR pRequest,pReply;
    ULONG IOBytes, APDULength, RequestLength;
    UCHAR IOData[ MAX_T1_BLOCK_SIZE ];
    UCHAR WtxReply[16];

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBT0Transmit: Enter\n" )
                  );

    SysFillMemory( SmartcardExtension->SmartcardReply.Buffer, 0xCC, MAX_T1_BLOCK_SIZE );

    pRequest        = SmartcardExtension->SmartcardRequest.Buffer;
    pReply          = SmartcardExtension->SmartcardReply.Buffer;

        //      setup the command header
    pRequest[ PSCR_NAD ] =
    ( SmartcardExtension->ReaderExtension->Device == DEVICE_ICC1 ) ?
    NAD_TO_ICC1 : NAD_TO_ICC1;

    pRequest[ PSCR_PCB ] = PCB_DEFAULT;
        //
        //      get the length of the user data packet & set the appropriate LEN
        //      information the complete user packet consists of a SCARD_IO_REQUEST
        //      structure followed by the APDU. the length of SCARD_IO_REQUEST is
        //      transferred in the member cbPciLength of the structure
        //
    APDULength = SmartcardExtension->IoRequest.RequestBufferLength;
    if ((((PSCARD_IO_REQUEST) SmartcardExtension->IoRequest.RequestBuffer)->cbPciLength) >= APDULength) {
        // Prevent integer underflow
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    APDULength -= ((PSCARD_IO_REQUEST) SmartcardExtension->
                   IoRequest.RequestBuffer)->cbPciLength;
        //
        //      a 4 byte APDU will be patched to a 5 byte TPDU by the lib; see
        //      annex of the ISO
        //
    if ( APDULength == 4 ) APDULength++;
        //
        //      if the total length of the T1 (reader) packet is larger than 0xFF
        //      the extended length notation will be used
        //
    if ( APDULength >= 0xFF ) {
        pRequest[ PSCR_LEN ]    = 0xFF;
        pRequest[ PSCR_LEN+1 ]  = HIBYTE( APDULength );
        pRequest[ PSCR_LEN+2 ]  = LOBYTE( APDULength );
        SmartcardExtension->SmartcardRequest.BufferLength =
        PSCR_EXT_PROLOGUE_LENGTH;
    } else {
        pRequest[ PSCR_LEN ] = ( UCHAR ) APDULength;
        SmartcardExtension->SmartcardRequest.BufferLength =
        PSCR_PROLOGUE_LENGTH;
    }

        //      let the lib setup the T=1 APDU & check for errors
    NTStatus = SmartcardT0Request( SmartcardExtension );
    RequestLength = SmartcardExtension->SmartcardRequest.BufferLength;

    while ( NT_SUCCESS( NTStatus )) {

        //  check to see if device is busy.
        {
            PPSCR_REGISTERS IOBase;
            UCHAR           Status;

            IOBase = SmartcardExtension->ReaderExtension->IOBase;

            Status = READ_PORT_UCHAR( &IOBase->CmdStatusReg );

            if( 0x03 == Status )
            {
                NTStatus = STATUS_DEVICE_BUSY;
                break;
            }
        }

                //      send command
        IOBytes = 0;
        NTStatus = PscrWrite(
                            SmartcardExtension->ReaderExtension,
                            pRequest,
                            RequestLength,
                            &IOBytes
                            );

                //      get response
        if ( NT_SUCCESS( NTStatus )) {
            IOBytes = 0;
            NTStatus = PscrRead(
                               SmartcardExtension->ReaderExtension,
                               IOData,
                               MAX_T1_BLOCK_SIZE,
                               &IOBytes
                               );

                        //      extract APDU from T=1 transport packet
            if ( NT_SUCCESS( NTStatus )) {

                if (IOBytes < 4) {

                    NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
                    break;

                }
                if (IOData[ PSCR_PCB ] == WTX_REQUEST) {

                    WtxReply[PSCR_NAD] = NAD_TO_PSCR;
                    WtxReply[PSCR_PCB] = WTX_REPLY;
                    WtxReply[PSCR_LEN] = 1;
                    WtxReply[PSCR_INF] = IOData[PSCR_INF];

                    RequestLength = 4;
                    pRequest = WtxReply;
                    continue;
                }

                if ( IOData[ PSCR_LEN ] == 0xFF ) {
                                        //
                                        //      extended length byte used
                                        //
                    APDULength  = IOData[ PSCR_LEN + 1 ] << 8;
                    APDULength += IOData[ PSCR_LEN + 2 ];

                    SmartcardExtension->SmartcardReply.BufferLength = APDULength ;
                    if (APDULength >= (sizeof(IOData) - (PSCR_APDU + 2))) {
                        NTStatus = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }
                    SysCopyMemory( pReply, &IOData[ PSCR_APDU + 2 ], APDULength );
                } else {

                    if ((IOData[PSCR_LEN] > SmartcardExtension->SmartcardReply.BufferSize) ||
                        (IOData[PSCR_LEN] > MAX_T1_BLOCK_SIZE + PSCR_APDU)) {
                        NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
                        break;
                    }
                    SmartcardExtension->SmartcardReply.BufferLength =
                    IOData[ PSCR_LEN ];

                    SysCopyMemory(
                                 pReply,
                                 &IOData[ PSCR_APDU ],
                                 IOData[ PSCR_LEN ]
                                 );
                }

                if( SmartcardExtension->ReaderCapabilities.CurrentState <=
                    SCARD_ABSENT )
                {
                    NTStatus = STATUS_NO_MEDIA;
                }
                else
                {
                    // let the lib evaluate the result & tansfer the data
                    NTStatus = SmartcardT0Reply( SmartcardExtension );
                }
                break;
            }
        }
    }

    // start freeze routine, if an interrupt was sent, but no freeze data was read
    if( SmartcardExtension->ReaderExtension->RequestInterrupt )
        PscrFreeze( SmartcardExtension );

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBT0Transmit: Exit (%lx)\n", NTStatus )
                  );

    return( NTStatus );
}

NTSTATUS
CBCardTracking(
              PSMARTCARD_EXTENSION SmartcardExtension
              )
/*++

CBCardTracking:
        callback handler for SMCLIB RDF_CARD_TRACKING. the requested event was
        validated by the smclib (i.e. a card removal request will only be passed
        if a card is present).
        for a win95 build STATUS_PENDING will be returned without any other action.
        for NT the cancel routine for the irp will be set to the drivers cancel
        routine.

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_PENDING

--*/
{
    WDFREQUEST request;
    PDEVICE_EXTENSION       DeviceExtension;
    NTSTATUS status;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBCardTracking: Enter\n" )
                  );

    request = GET_WDFREQUEST_FROM_IRP(SmartcardExtension->OsData->NotificationIrp);

    DeviceExtension = GetDeviceExtension(WdfIoQueueGetDevice(WdfRequestGetIoQueue(request)));

    IoMarkIrpPending(SmartcardExtension->OsData->NotificationIrp);

    //
    // Move the stack pointer back to where it was when the IRP
    // was delivered to the driver. This compensates for the 
    // IoSetNextIrpStackLocation that we did when we presented the IRP 
    // to the smartcard libarary. 
    //
    IoSkipCurrentIrpStackLocation(SmartcardExtension->OsData->NotificationIrp);

    status = WdfRequestForwardToIoQueue(request,
                               DeviceExtension->NotificationQueue);
    if (!NT_SUCCESS(status)) {
        NT_ASSERT(NT_SUCCESS(status));
        InterlockedExchangePointer(
                                 &(SmartcardExtension->OsData->NotificationIrp),
                                 NULL
                                 );
        WdfRequestComplete(request, status);
    }

    status = STATUS_PENDING;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBCardTracking: Exit \n" )
                  );

    return status;
}

VOID
CBUpdateCardState(
                 PSMARTCARD_EXTENSION SmartcardExtension,
                 UCHAR IccState,
                 BOOLEAN SystemWakeUp
                 )
{
    ULONG oldState;
    NTSTATUS status;
    KIRQL irql;
    WDFREQUEST request;
    PDEVICE_EXTENSION       DeviceExtension;


    KeAcquireSpinLock(
                     &SmartcardExtension->OsData->SpinLock,
                     &irql
                     );

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBUpdateCardState: Enter \n" )
                  );

    oldState =
    (SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ?
     SCARD_PRESENT : SCARD_ABSENT);

    SmartcardExtension->ReaderCapabilities.CurrentState =
    (IccState == PSCR_ICC_PRESENT ? SCARD_PRESENT : SCARD_ABSENT);

    SmartcardDebug(
                  DEBUG_DRIVER,
                  ( "PSCR!CBUpdateCardState: Smart card %s\n",
                    IccState == PSCR_ICC_PRESENT ? "inserted" : "removed")
                  );

    if ( SmartcardExtension->OsData->NotificationIrp != NULL &&
         (SystemWakeUp &&
          (oldState == SCARD_PRESENT ||
           SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_PRESENT) ||
          SmartcardExtension->ReaderCapabilities.CurrentState != oldState)) {
        PIRP notificationIrp ;

        notificationIrp = InterlockedExchangePointer(
                                 &(SmartcardExtension->OsData->NotificationIrp),
                                 NULL
                                 );

        DeviceExtension = GetDeviceExtension(WdfWdmDeviceGetWdfDeviceHandle(SmartcardExtension->OsData->DeviceObject));

        KeReleaseSpinLock(
             &SmartcardExtension->OsData->SpinLock,
             irql
             );

        status = WdfIoQueueRetrieveNextRequest(
                            DeviceExtension->NotificationQueue,
                            &request);
        if (NT_SUCCESS(status)) {

            SmartcardDebug(
                          DEBUG_DRIVER,
                          ( "PSCR!CBUpdateCardState: Completing Irp %p\n",
                            notificationIrp)
                          );

            WdfRequestCompleteWithInformation(request, status, 0);
        }
        else {
            NT_ASSERTMSG("WdfIoQueueRetrieveNextRequest failed",
                      status == STATUS_NO_MORE_ENTRIES);
        }
    } else {
        KeReleaseSpinLock(
                          &SmartcardExtension->OsData->SpinLock,
                          irql
                          );
    }

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!CBUpdateCardState: Exit \n" )
                  );
}

UCHAR
CBGetCardState(
              PSMARTCARD_EXTENSION SmartcardExtension
              )
/*++

CBUpdateCardState:
        updates the variable CurrentState in SmartcardExtension

Arguments:
        SmartcardExtension      context of call

Return Value:
        STATUS_SUCCESS

--*/
{
    UCHAR TLVList[ TLV_BUFFER_SIZE ],       Val, Len;
    ULONG TLVListLen;
    PREADER_EXTENSION       ReaderExtension = SmartcardExtension->ReaderExtension;

        //      read the status file of ICC1 from the reader
    TLVListLen = TLV_BUFFER_SIZE;

    if ( NT_SUCCESS( CmdReadStatusFile(
                                      ReaderExtension,
                                      ReaderExtension->Device,
                                      TLVList,
                                      &TLVListLen
                                      ))) {

                //      get reader status value
        Len = sizeof(Val);
        CmdGetTagValue(
                      TAG_READER_STATUS,
                      TLVList,
                      TLVListLen,
                      &Len,
                      ( PVOID ) &Val
                      );
    } else {
                //      IO-error is interpreted as card absent
        Val = PSCR_ICC_ABSENT;
    }

    return Val;
}

//      -------------------------------- END OF FILE ------------------------------



