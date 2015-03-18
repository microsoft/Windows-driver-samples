
#pragma once

#ifdef __cplusplus
extern "C" {
#endif



#include <packon.h>

typedef struct EAPOL_PACKET
{
    BYTE        ProtocolVersion;
    BYTE        PacketType;
    BYTE        PacketBodyLength[2];
    BYTE        PacketBody[1];
} EAPOL_PACKET, *UNALIGNED PEAPOL_PACKET;

#include <packoff.h>


VOID
WINAPI
RC4UtilsFreeKeyMaterial
(
    PBYTE   pbDecryptedKey,
    DWORD   dwKeyLen
);


DWORD
WINAPI
RC4UtilsParseKeyPacket
(
    PEAPOL_PACKET                   pEapolPkt,
    ULONG                           uPktLen,
    PDOT11_MSONEX_RESULT_PARAMS     pOneXResultParams,
    BOOL*                           pbUCast,
    PBYTE*                          ppbDecryptedKey,
    DWORD*                          pdwKeyLen,
    DWORD*                          pdwKeyIndex
);



DWORD
WINAPI
RC4UtilsDecryptResultParams
(
    PDOT11_MSONEX_RESULT_PARAMS     pResultParamsOrig,
    PDOT11_MSONEX_RESULT_PARAMS*    ppResultParamsCopy
);


VOID
WINAPI
RC4UtilsFreeResultParams
(
    PDOT11_MSONEX_RESULT_PARAMS*    ppResultParams
);


#ifdef __cplusplus
}
#endif

