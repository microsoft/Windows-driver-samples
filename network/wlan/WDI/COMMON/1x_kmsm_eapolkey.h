/*++

Module Name:
	1x_kmsm_eapolkey.h
	
Abstract:
	1. EAPOL-Key related data structures and helper macros.	
	2. Declare helper functinos defined in 1x_kmsm_perf.c, 1x_kmsm_hmac.c. 

--*/

#ifndef LIB1X_KMSM_EAPOLKEY_H
#define LIB1X_KMSM_EAPOLKEY_H



//#include <string.h>



/*#ifdef _RTL_WPA_WINDOWS
typedef unsigned short u2Byte;
typedef unsigned char u1Byte;
typedef unsigned long u4Byte;
} OCTET_STRING, *POCTET_STRING, EAPOL_KEY;
#else
#include <sys/types.h>
#include "1x_kmsm_keydef.h"
#include "1x_types.h"
#endif
*/

//original in 1x_ether.h and 1x_eapol.h
#define ETHER_HDRLEN		14
#define LIB1X_EAPOL_HDRLEN	4
#define ETHER_ADDRLEN		6

//size of the field in information element
#define PTK_LEN_TKIP		64
//------------------------------------------------------------------------------
// From 1x_type.h, 2005.06.30, by rcnjko.
//------------------------------------------------------------------------------
typedef union _OCTET8_INTEGER {  
	u8Byte QuadPart;

	struct {    
		u4Byte LowPart;    
		u4Byte HighPart;  
	};  
} OCTET8_INTEGER, *POCTET8_INTEGER;


typedef union _OCTET16_INTEGER {
        u1Byte  charData[16];

        struct{
                OCTET8_INTEGER   HighPart;
                OCTET8_INTEGER   LowPart;
        }field;
} OCTET16_INTEGER;

typedef union  _OCTET32_INTEGER {
        u1Byte charData[32];
        struct{
                OCTET16_INTEGER HighPart;
                OCTET16_INTEGER LowPart;
        }field;
}OCTET32_INTEGER;

typedef enum    { key_desc_ver1 = 1, key_desc_ver2 = 2 } KeyDescVer;

#ifdef WPA2
typedef enum    { desc_type_WPA2 = 2, desc_type_RSN = 254 } DescTypeRSN;
#else
typedef enum    { desc_type_RSN = 254 } DescTypeRSN;
#endif

typedef enum     { type_Group = 0, type_Pairwise = 1 } KeyType;

typedef enum     { type_4way2nd = 0, type_4way4th = 1, type_2way2nd = 2, type_unknow=3 } MsgType;		// Added by Annie, 2005-07-11.

/*-----------------------------------------------------------------------------
 Network and machine byte oder conversion 
	Macro definition
-------------------------------------------------------------------------------*/	
// <RJ_TODO> The translations below are not endian-free. 
#define long2net(l,c)    (*((c) )=(unsigned char)(((l)>>24)&0xff), \
                         *((c)+1)=(unsigned char)(((l)>>16)&0xff), \
						 *((c)+2)=(unsigned char)(((l)>> 8)&0xff), \
						 *((c)+3)=(unsigned char)(((l)    )&0xff))

#define net2long(c,l)    (l =((ULONG)(*((c)  )))<<24, \
                         l|=((ULONG)(*((c)+1)))<<16, \
						 l|=((ULONG)(*((c)+2)))<< 8, \
		                 l|=((ULONG)(*((c)+3))))

#define short2net(s,c)        (*((c))=(unsigned char)(((s)>> 8)&0xff), \
								 *((c)+1)=(unsigned char)(((s)    )&0xff))

#define net2short(c,s)        (s =((unsigned short)(*((c))))<< 8, \
								s|=((unsigned short)(*((c)+1))))

#define lint2net(l,c)	(long2net(l.HighPart, c) , long2net(l.LowPart, c+4))
#define net2lint(c,l)	(net2long(c, l.HighPart) , net2long(c+4, l.LowPart))
	
/*-----------------------------------------------------------------------------
 LargeInteger
	Inline Function definition
	Macro definition
-------------------------------------------------------------------------------*/
#define LargeIntegerOverflow(x) (x.HighPart == 0xffffffff) && \
								(x.LowPart == 0xffffffff)
//#define LargeIntegerZero(x) memset(&x.charData, 0, 8);
#define LargeIntegerZero(x) x.QuadPart = 0;


/*-----------------------------------------------------------------------------
 Octet16Integer
	Inline Function definition
	Macro definition
-------------------------------------------------------------------------------*/
#define Octet16IntegerOverflow(x) LargeIntegerOverflow(x.field.HighPart) && \
								  LargeIntegerOverflow(x.field.LowPart)
//#define Octet16IntegerZero(x) memset(&x.charData, 0, 16);
#define Octet16IntegerZero(x) memset(&x, 0, 16);

/*-----------------------------------------------------------------------------
 EAPOLKey field process
	Inline Function definition
	Macro definition
-------------------------------------------------------------------------------*/
static
//inline
OCTET_STRING	SubStr(OCTET_STRING	f,	u2Byte	s,u2Byte	l);


#define	SetSubStr(f,a,l)	PlatformMoveMemory(f.Octet+l,a.Octet,a.Length)
#define	GetKeyInfo0(f, mask)  ( (f.Octet[KeyInfoPos + 1] & mask) ? 1 :0)
#define	SetKeyInfo0(f,mask,b)	( f.Octet[KeyInfoPos + 1] = (f.Octet[KeyInfoPos + 1] & ~mask) | ( b?mask:0x0) )
#define	GetKeyInfo1(f, mask)  ( (f.Octet[KeyInfoPos] & mask) ? 1 :0)
#define	SetKeyInfo1(f,mask,b)	( f.Octet[KeyInfoPos] = (f.Octet[KeyInfoPos] & ~mask) | ( b?mask:0x0) )


// EAPOLKey
#define Message_DescType(f)		(f.Octet[DescTypePos])
#define Message_setDescType(f, type)	(f.Octet[DescTypePos] = type)
// Key Information Filed
#define Message_KeyDescVer(f)		(f.Octet[KeyInfoPos+1] & 0x07)//(f.Octet[KeyInfoPos+1] & 0x01) | (f.Octet[KeyInfoPos+1] & 0x02) <<1 | (f.Octet[KeyInfoPos+1] & 0x04) <<2
#define Message_setKeyDescVer(f, v)	(f.Octet[KeyInfoPos+1] &= 0xf8) , f.Octet[KeyInfoPos+1] |= (v & 0x07)//(f.Octet[KeyInfoPos+1] |= ((v&0x01)<<7 | (v&0x02)<<6 | (v&0x04)<<5) )
#define	Message_KeyType(f)		GetKeyInfo0(f,0x08)
#define	Message_setKeyType(f, b)	SetKeyInfo0(f,0x08,b)
#define Message_KeyIndex(f)		((f.Octet[KeyInfoPos+1] & 0x30) >> 4) & 0x03//(f.Octet[KeyInfoPos+1] & 0x20) | (f.Octet[KeyInfoPos+1] & 0x10) <<1
#define Message_setKeyIndex(f, v)	(f.Octet[KeyInfoPos+1] &= 0xcf), f.Octet[KeyInfoPos+1] |= ((v<<4) & 0x30 )//(f.Octet[KeyInfoPos+1] |= ( (v&0x01)<<5 | (v&0x02)<<4)  )
#define	Message_Install(f)		GetKeyInfo0(f,0x40)
#define	Message_setInstall(f, b)	SetKeyInfo0(f,0x40,b)
#define	Message_KeyAck(f)		GetKeyInfo0(f,0x80)
#define	Message_setKeyAck(f, b)		SetKeyInfo0(f,0x80,b)

#define	Message_KeyMIC(f)		GetKeyInfo1(f,0x01)
#define	Message_setKeyMIC(f, b)		SetKeyInfo1(f,0x01,b)
#define	Message_Secure(f)		GetKeyInfo1(f,0x02)
#define	Message_setSecure(f, b)		SetKeyInfo1(f,0x02,b)
#define	Message_Error(f)		GetKeyInfo1(f,0x04)
#define	Message_setError(f, b)		SetKeyInfo1(f,0x04,b)
#define	Message_Request(f)		GetKeyInfo1(f,0x08)
#define	Message_setRequest(f, b)	SetKeyInfo1(f,0x08,b)
#define	Message_Reserved(f)		(f.Octet[KeyInfoPos] & 0xf0)
#define	Message_setReserved(f, v)	(f.Octet[KeyInfoPos] |= (v<<4&0xff))
#define	Message_EncryptedKeyData(f)		GetKeyInfo1(f,0x10)
#define	Message_setEncryptedKeyData(f, b)		SetKeyInfo1(f,0x10,b)


#define Message_KeyLength(f)		((u2Byte)(f.Octet[KeyLenPos] <<8) + (u2Byte)(f.Octet[KeyLenPos+1]))
#define Message_setKeyLength(f, v)	(f.Octet[KeyLenPos] = (v&0xff00) >>8 ,  f.Octet[KeyLenPos+1] = (v&0x00ff))


/* Replay Counter process function */
#define DEFAULT_KEY_REPLAY_COUNTER_LONG		0xffffffff
#define Message_DefaultReplayCounter(li)	((li.HighPart == DEFAULT_KEY_REPLAY_COUNTER_LONG) && (li.LowPart == DEFAULT_KEY_REPLAY_COUNTER_LONG) ) ?1:0
#define Message_ReplayCounter(f)			SubStr(f, ReplayCounterPos, KEY_RC_LEN)
#define Message_CopyReplayCounter(f1, f2)	PlatformMoveMemory(f1.Octet + ReplayCounterPos, f2.Octet + ReplayCounterPos, KEY_RC_LEN)

static __inline        void Message_ReplayCounter_OC2LI(OCTET_STRING f, OCTET8_INTEGER * li);
static __inline        void ReplayCounter_OC2LI(OCTET_STRING f, OCTET8_INTEGER * li);
static __inline int Message_EqualReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f);
static __inline int Message_SmallerEqualReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f);
static __inline int Message_LargerReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f);
static __inline void Message_setReplayCounter(OCTET_STRING f, u4Byte h, u4Byte l);


//#define SetNonce(x,y) PlatformMoveMemory(x.Octet, y.charData, 32);
void SetNonce(OCTET_STRING osDst, OCTET32_INTEGER oc32Counter);
#define	Message_KeyNonce(f)					SubStr(f,KeyNoncePos,KEY_NONCE_LEN)
#define Message_setKeyNonce(f, v)			SetSubStr(f, v, KeyNoncePos)
#define Message_EqualKeyNonce(f1, f2)		memcmp(f1.Octet + KeyNoncePos, f2.Octet, KEY_NONCE_LEN)? 0:1
#define Message_KeyIV(f)					SubStr(f, KeyIVPos, KEY_IV_LEN)
#define Message_setKeyIV(f, v)				SetSubStr(f, v, KeyIVPos)
#define Message_KeyRSC(f)					SubStr(f, KeyRSCPos, KEY_RSC_LEN)
#define Message_setKeyRSC(f, v)				SetSubStr(f, v, KeyRSCPos)
#define Message_KeyID(f)					SubStr(f, KeyIDPos, KEY_ID_LEN)
#define Message_setKeyID(f, v)				SetSubStr(f, v, KeyIDPos)
#define Message_MIC(f)						SubStr(f, KeyMICPos, KEY_MIC_LEN)
#define Message_setMIC(f, v)				SetSubStr(f, v, KeyMICPos)
#define Message_clearMIC(f)					memset(f.Octet+KeyMICPos, 0, KEY_MIC_LEN)
#define Message_KeyDataLength(f)			((u2Byte)(f.Octet[KeyDataLenPos] <<8) + (u2Byte)(f.Octet[KeyDataLenPos+1]))
#define Message_setKeyDataLength(f, v)		(f.Octet[KeyDataLenPos] = (v&0xff00) >>8 ,  f.Octet[KeyDataLenPos+1] = (v&0x00ff))
#define Message_KeyData(f, l)				SubStr(f, KeyDataPos, l)
#define Message_setKeyData(f, v)			SetSubStr(f, v, KeyDataPos);
#define Message_EqualRSNIE(f1 , f2, l)		memcmp(f1.Octet, f2.Octet, l) ? 0:1
#define Message_ReturnKeyDataLength(f)		f.Length - (ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN + EAPOLMSG_HDRLEN)

/*
typedef	union _KeyInfo
{
	u2Byte	shortData;
	u1Byte	charData[2];
	struct
	{
		u2Byte	KeyDescVersion:3;
		u2Byte	KeyType:1;
		u2Byte	KeyIndex:2;
		u2Byte	Install:1;
		u2Byte	KeyAck:1;
		u2Byte	KeyMIC:1;
		u2Byte	Secure:1;
		u2Byte	Error:1;
		u2Byte	Request:1;
		u2Byte	Reserved:4;
	}field;
}KeyInfo;

#define KeyInfo_KeyDescVersion(f)	( ((KeyInfo	*)((f).Octet))	->field.KeyDescVersion)
#define KeyInfo_KeyType(f)			( ((KeyInfo	*)((f).Octet))	->field.KeyType)
#define KeyInfo_KeyIndex(f)			( ((KeyInfo	*)((f).Octet))	->field.KeyIndex)
#define KeyInfo_Install(f)			( ((KeyInfo	*)((f).Octet))	->field.Install)
#define KeyInfo_KeyAck(f)			( ((KeyInfo	*)((f).Octet))	->field.KeyAck)
#define KeyInfo_KeyMic(f)			( ((KeyInfo	*)((f).Octet))	->field.KeyMic)
#define KeyInfo_Secure(f)			( ((KeyInfo	*)((f).Octet))	->field.Secure)
#define KeyInfo_Error(f)			( ((KeyInfo	*)((f).Octet))	->field.Error)
#define KeyInfo_Request(f)			( ((KeyInfo	*)((f).Octet))	->field.Request)
#define KeyInfo_Reserved(f)			( ((KeyInfo	*)((f).Octet))	->field.Reserved)
*/

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct _EAPOL_KEY_STRUCT
{
	u1Byte			key_desc_type;
	//KeyInfo		key_info;
	u1Byte			key_info[2];
	u1Byte			key_len[sizeof(u2Byte)];
	u1Byte			key_replay_counter[KEY_RC_LEN];
	u1Byte			key_nounce[KEY_NONCE_LEN];
	u1Byte			key_iv[KEY_IV_LEN];
	u1Byte			key_rsc[KEY_RSC_LEN];
	u1Byte			key_id[KEY_ID_LEN];
	u1Byte			key_mic[KEY_MIC_LEN];
	u1Byte			key_data_len[KEY_MATERIAL_LEN];
	u1Byte			*key_data;
}EAPOL_KEY_STRUCT, *PEAPOL_KEY_STRUCT;


typedef struct _KDE_IE_STRUCT
{
	u1Byte  IEType;	// ID = 0xDD
	u1Byte  IELen;
	u1Byte  OUI[3];      // 0x00-0x0F-0xAC 
	u1Byte  Datatype; // 0x01
	u1Byte  KID;
	u1Byte  Reserved;
	u1Byte  GTK[1];
	
}KDE_STRUCT,*PKDE_STRUCT;

#ifdef REMOVE_PACK
#pragma pack()
#endif


//---------------------------------------------------------------------
// Definition for 1x_kmsm_eapolkey.c
//---------------------------------------------------------------------
/*-----------------------------------------------------------------------------
 LargeInteger
	Inline Function definition
-------------------------------------------------------------------------------*/
static
__inline  void INCLargeInteger(OCTET8_INTEGER * x){

	if( x -> LowPart == 0xffffffff){
		if( x ->HighPart == 0xffffffff)
		{
			x ->HighPart = 0;
			x ->LowPart = 0;
		}else
		{
			x ->HighPart++;
			x ->LowPart = 0;
		}
	}else
		x ->LowPart++;
}

static
__inline  void ReplayCounter_LI2OC(
	OCTET_STRING f,
	OCTET8_INTEGER * li);

static
__inline  void INCOctet16_INTEGER(OCTET16_INTEGER * x){

	if( LargeIntegerOverflow(x ->field. LowPart)){
		if( LargeIntegerOverflow(x ->field. HighPart))
		{
			LargeIntegerZero( x ->field. HighPart);
			LargeIntegerZero( x ->field. LowPart);
		}else
		{
			INCLargeInteger(&x ->field. HighPart);
			LargeIntegerZero( x ->field. LowPart);
		}
	}else
		INCLargeInteger(&x ->field. LowPart);

}

/*-----------------------------------------------------------------------------
 OCTET32_INTEGER
	Inline Function definition
-------------------------------------------------------------------------------*/
static
__inline OCTET32_INTEGER * INCOctet32_INTEGER(OCTET32_INTEGER * x)
{

	if( Octet16IntegerOverflow(x ->field.LowPart)){
		if( Octet16IntegerOverflow(x ->field.HighPart))
		{
			Octet16IntegerZero( x ->field.HighPart);
			Octet16IntegerZero( x ->field.LowPart);
		}else
		{
			INCOctet16_INTEGER(&x ->field.HighPart);
			Octet16IntegerZero( x ->field.LowPart);
		}
	}else
		INCOctet16_INTEGER(&x ->field.LowPart);
	return x;
}

/*-----------------------------------------------------------------------------
 EAPOLKey field process
	Inline Function definition
	Macro definition
-------------------------------------------------------------------------------*/
static
__inline
OCTET_STRING	SubStr(OCTET_STRING	f,	u2Byte	s,u2Byte	l)	{			\
			OCTET_STRING		res;	\
			res.Length = l;			\
			res.Octet = f.Octet+s;	\
			return	res;			\
		}


__inline	void Message_ReplayCounter_OC2LI(OCTET_STRING f, OCTET8_INTEGER * li){

	li -> HighPart = ((u4Byte)(*(f.Octet + ReplayCounterPos + 3)))
					     + ((u4Byte)(*(f.Octet + ReplayCounterPos+ 2)) <<8 )
						 + ((u4Byte)(*(f.Octet + ReplayCounterPos + 1)) <<  16)
						 + ((u4Byte)(*(f.Octet + ReplayCounterPos + 0)) <<24);
	li -> LowPart =  ((u4Byte)(*(f.Octet + ReplayCounterPos + 7)))
						 + ((u4Byte)(*(f.Octet + ReplayCounterPos + 6)) <<8 )
					  	 + ((u4Byte)(*(f.Octet + ReplayCounterPos + 5)) <<  16)
						 + ((u4Byte)(*(f.Octet + ReplayCounterPos + 4)) <<24);
}

__inline	void ReplayCounter_OC2LI(OCTET_STRING f, OCTET8_INTEGER * li){

	li -> HighPart = ((u4Byte)(*(f.Octet + 3)))
					     + ((u4Byte)(*(f.Octet + 2)) <<8 )
						 + ((u4Byte)(*(f.Octet + 1)) << 16)
						 + ((u4Byte)(*(f.Octet + 0)) <<24);
	li -> LowPart =  ((u4Byte)(*(f.Octet + 7)))
						 + ((u4Byte)(*(f.Octet + 6)) <<8 )
					  	 + ((u4Byte)(*(f.Octet + 5)) << 16)
						 + ((u4Byte)(*(f.Octet + 4)) <<24);
}

static
__inline  void ReplayCounter_LI2OC(OCTET_STRING f, OCTET8_INTEGER * li){

	*(f.Octet + 0) = (u1Byte)((li -> HighPart >> 24) & 0xff);
	*(f.Octet + 1) = (u1Byte)((li -> HighPart >> 16) & 0xff);
	*(f.Octet + 2) = (u1Byte)((li -> HighPart >>  8) & 0xff);
	*(f.Octet + 3) = (u1Byte)((li -> HighPart >>  0) & 0xff);

	*(f.Octet + 4) = (u1Byte)((li -> LowPart >> 24) & 0xff);
	*(f.Octet + 5) = (u1Byte)((li -> LowPart >> 16) & 0xff);
	*(f.Octet + 6) = (u1Byte)((li -> LowPart >>  8) & 0xff);
	*(f.Octet + 7) = (u1Byte)((li -> LowPart >>  0) & 0xff);
}

/*-----------------------------------------------------------------------------------------------
	f is EAPOL-KEY message
------------------------------------------------------------------------------------------------*/
static
__inline int Message_EqualReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f)
{
	OCTET8_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);
	if(li1.HighPart == li2.HighPart && li1.LowPart == 
li2.LowPart)
		return 1;
	else
		return 0;
}
/*-------------------------------------------------------------------------------------------
	li1 is recorded replay counter on STA
	f is the replay counter from EAPOL-KEY message
---------------------------------------------------------------------------------------------*/
static
__inline int Message_SmallerEqualReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f) 
//f<li1
{
	OCTET8_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);
	if(li2.HighPart > li1.HighPart)
		return 0;
	else if(li2.HighPart < li1.HighPart)
		return 1;
	else if(li2.LowPart > li1.LowPart)
		return 0;
	else if(li2.LowPart <= li1.LowPart)
		return 1;
	else
		return 0;
}

/*---------------------------------------------------------------------------------------------
	li1 is recorded replay counter on STA
	f is the replay counter from EAPOL-KEY message
-----------------------------------------------------------------------------------------------*/
static
__inline int Message_LargerReplayCounter(OCTET8_INTEGER li1, OCTET_STRING f)
{
	OCTET8_INTEGER li2;
	Message_ReplayCounter_OC2LI(f, &li2);

	//lib1x_message(MESS_DBG_KEY_MANAGE, "Authenticator : HighPart = %d, LowPart = %d\n", li1.field.HighPart, li1.field.LowPart);
	//lib1x_message(MESS_DBG_KEY_MANAGE, "Supplicant : HighPart = %d, LowPart = %d\n", li2.field.HighPart, li2.field.LowPart);

	if(li2.HighPart > li1.HighPart)
		return 1;
	else if(li2.LowPart > li1.LowPart)
		return 1;
	else
		return 0;

}

typedef union	LINT_CH8{
	OCTET8_INTEGER	uli;
	char				ch[8];
}LINT_CH8;

static
__inline  void Message_setReplayCounter(OCTET_STRING f, u4Byte h, u4Byte l){

	//OCTET8_INTEGER *li = (OCTET8_INTEGER *)(f.Octet + ReplayCounterPos);
	LINT_CH8 *li = (LINT_CH8 *)(f.Octet + ReplayCounterPos);
	li -> ch[0] = (u1Byte)(h >> 24) & 0xff;
	li -> ch[1] = (u1Byte)(h >> 16) & 0xff;
	li -> ch[2] = (u1Byte)(h >>  8) & 0xff;
	li -> ch[3] = (u1Byte)(h >>  0) & 0xff;
	li -> ch[4] = (u1Byte)(l >> 24) & 0xff;
	li -> ch[5] = (u1Byte)(l >> 16) & 0xff;
	li -> ch[6] = (u1Byte)(l >>  8) & 0xff;
	li -> ch[7] = (u1Byte)(l >>  0) & 0xff;

}

//------------------------------------------------------------------------
// Definition file for 1x_kmsm_eapolkey.c
//------------------------------------------------------------------------
void CalcPTK(u1Byte *addr1, u1Byte *addr2, u1Byte *nonce1, u1Byte *nonce2,
			 u1Byte * keyin, int keyinlen,
			 u1Byte * keyout, int keyoutlen);

void CalcEapolMIC(
	OCTET_STRING EAPOLMsgSend,
	int algo,
	u1Byte *key,
	int keylen);

BOOLEAN	CheckEapolMIC(
	PADAPTER		Adapter,
	OCTET_STRING 	EAPOLMsgRecvd,
	u1Byte 			*key,
	int 			keylen);

void EncEapolKeyData(
	PADAPTER		Adapter,
	OCTET_STRING	EapolKeyMsg,
	pu1Byte			pKEK, 
	int				nKEKLen);



void 
CalcCCKMRequestIEMIC( 
	OCTET_STRING MICdata , 
	u1Byte *KRK , 
	u1Byte *outdata );

void
CalcCCKMRequestIEMICV2( 
	OCTET_STRING MICdata , 
	u1Byte *KRK , 
	u1Byte *outdata );

/*

void CalcGTK(
	u1Byte *addr,
	u1Byte *nonce,
	u1Byte * keyin,
	int keyinlen,
	u1Byte * keyout,
	int keyoutlen);
*/

int DecGTK(
	PADAPTER		Adapter,
	OCTET_STRING 	EAPOLMsgRecvd, 
	u1Byte 			*kek, 
	int	 			keklen, 
	int 			keylen, 
	u1Byte 			*kout);	


//------------------------------------------------------------------------
// Definition file for 1x_kmsm_hmac.c
//------------------------------------------------------------------------
void hmac_sha(
	unsigned char*    k,     /* secret key */
	int      lk,    /* length of the key in bytes */
	unsigned char*    d,     /* data */
	int      ld,    /* length of data in bytes */
	unsigned char*    out,   /* output buffer, at least "t" bytes */
	int      t
	);
void hmac_sha1(
	unsigned char *text,
	int text_len,
	unsigned char *key,
	int key_len,
	unsigned char *digest);

void
hmac_md5(
	unsigned char *text,
	int text_len,
	unsigned char *key,
	int key_len,
	void * digest);

//------------------------------------------------------------------------
// Definition file for 1x_kmsm_prf.c
//------------------------------------------------------------------------
void i_PRF(
	unsigned char*	secret,
	int				secret_len,
	unsigned char*	prefix,
	int				prefix_len,
	unsigned char*	random,
	int				random_len,
	unsigned char*  digest,             // caller digest to be filled in
	int				digest_len			// in byte
	);

int PasswordHash (
	const char *password,
	int passwordlength,
	unsigned char *ssid,
	int ssidlength,
	unsigned char *output
	);

////------------------------------------------------------------------------
//// Definition file for 1x_kmsm_aes.c
////------------------------------------------------------------------------
//void AES_WRAP(
//	u08b * plain,
//	int plain_len,
//	u08b * iv,
//	int iv_len,
//	u08b * kek,
//	int kek_len,
//	u08b *cipher,
//	u16b *cipher_len);
//
//void AES_UnWRAP(
//	u08b * cipher,
//	int cipher_len,
//	u08b * kek,
//	int kek_len,
//	u08b * plain);
//

#endif
