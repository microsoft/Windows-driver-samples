#ifndef	__INC_RC4_H
#define	__INC_RC4_H

#define CRC32_POLY 0x04c11db7

void 
EncodeWEP(
	pu1Byte key,
	s4Byte 	keysize,
	pu1Byte plaintext,
	s4Byte 	len,
	pu1Byte out);
BOOLEAN 
DecodeWEP(
	pu1Byte key,
	s4Byte 	keysize,
	pu1Byte ciphertext,
	s4Byte 	len,
	pu1Byte out);

u1Byte 
ReverseBit( 
	u1Byte data 
	);

// Isaiah note: previous declaration in libkern.h
//              uint32_t	crc32(uint32_t crc, const void *bufp, size_t len);

u1Byte 
ReverseBit( 
	u1Byte data 
	);


ULONG 
crc32(
	pu1Byte buf, 
	u4Byte len);


void init_crc32(void);
typedef struct
{
	u4Byte x;
	u4Byte y;
	u1Byte state[256];
}ArcfourContext;

#endif //#ifndef __INC_RC4_H
