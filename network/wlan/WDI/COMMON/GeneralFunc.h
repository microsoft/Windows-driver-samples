#ifndef __INC_GENERALFUNC_H
#define __INC_GENERALFUNC_H

#define eqMacAddr(a,b)						( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2] && (a)[3]==(b)[3] && (a)[4]==(b)[4] && (a)[5]==(b)[5]) ? 1:0 )
#define cpMacAddr(des,src)					((des)[0]=(src)[0],(des)[1]=(src)[1],(des)[2]=(src)[2],(des)[3]=(src)[3],(des)[4]=(src)[4],(des)[5]=(src)[5])
#define cpIpAddr(des,src)					((des)[0]=(src)[0],(des)[1]=(src)[1],(des)[2]=(src)[2],(des)[3]=(src)[3])

// OUI, 3 bytes
#define eqOUI(a,b)							( ((a)[0]==(b)[0] && (a)[1]==(b)[1] && (a)[2]==(b)[2]) ? 1 : 0 )
#define cpOUI(des, src)						((des)[0]=(src)[0], (des)[1]=(src)[1], (des)[2]=(src)[2])

#define	SET_OUI_WITH_TYPE(_pStart, _OUI, _Type) {cpOUI((UNALIGNED pu1Byte)(_pStart), _OUI); WriteEF1Byte((UNALIGNED pu1Byte)(_pStart) + 3, (u1Byte)_Type);}
#define	GET_OUI_WITH_TYPE(_pStart, _pOUI, _Type) {cpOUI(_pOUI, (UNALIGNED pu1Byte)(_pStart)); (u1Byte)_Type = ReadEF1Byte((UNALIGNED pu1Byte)(_pStart) + 3);}



#define MacAddr_isBcst(addr) \
( \
	( (addr[0] == 0xff) && (addr[1] == 0xff) && \
		(addr[2] == 0xff) && (addr[3] == 0xff) && \
		(addr[4] == 0xff) && (addr[5] == 0xff) )  ? TRUE : FALSE \
)

#define MacAddr_isMulticast(addr) \
( \
	( addr[0] & 0x01 )  ? TRUE : FALSE \
)

//
// Copy String2 to String1 and end by the length or the end of character "\0".
// Only used for ASCII.
//
#define ASCII_STR_COPY(__STR1, __STR2, __Length)	\
{												\
	LONG	__i;								\
	for(__i = 0; __i < __Length; __i ++)		\
	{											\
		__STR1[__i] = __STR2[__i];				\
		if(__STR2[__i] == '\0')					\
		{										\
			break;								\
		}										\
	}											\
}

#define TEST_FLAG(__Flag,__testFlag)		(((__Flag) & (__testFlag)) != 0)
#define	SET_FLAG(__Flag, __setFlag)			((__Flag) |= __setFlag)
#define	CLEAR_FLAG(__Flag, __clearFlag)		((__Flag) &= ~(__clearFlag))
#define	CLEAR_FLAGS(__Flag)					((__Flag) = 0)
#define	TEST_FLAGS(__Flag, __testFlags)		(((__Flag) & (__testFlags)) == (__testFlags))

//
// Description:
//	RT_OBJECT_HEADER: Type + Id + Version + Length
//	Type - The type of the data or id.
//	Id - The number of the object.
//	Version - The version of content. Different versions cause different content or format of data.
//	Length - The total length in byte of Value.
// Format:
//	|   Type    |    	Id	|Version    |    Length    |     Value    
//	| ULONG   |    ULONG	|ULONG    |    ULONG    |    Veriable....
// Remark:
//	It's a general header for data commnuication between layers and applications under variable versions.
//	Each module can check this header to determine if the carring version of data is compatible with itself.
//
#pragma pack(1)
typedef struct _RT_OBJECT_HEADER
{
	u4Byte		Type;
	u4Byte		Id;
	u4Byte		Version;
	u4Byte		Length;
	u1Byte		Value[1];
}RT_OBJECT_HEADER, *PRT_OBJECT_HEADER;
#pragma pack()

#define	RT_OBJECT_HEADER_SIZE	(FIELD_OFFSET(RT_OBJECT_HEADER, Value))

#define RT_ASSIGN_OBJECT_HEADER(_pHeader, _Type, _Id, _Ver, _Len) \
{	\
		((PRT_OBJECT_HEADER)(_pHeader))->Type = (u4Byte)(_Type); \
		((PRT_OBJECT_HEADER)(_pHeader))->Id = (u4Byte)(_Id); \
		((PRT_OBJECT_HEADER)(_pHeader))->Version = (u4Byte)(_Ver); \
		((PRT_OBJECT_HEADER)(_pHeader))->Length = (u4Byte)(_Len);	\
}

#define	RT_OB_HDR_TYPE_UNKNOWN		0
#define	RT_OB_HDR_TYPE_QUERY		1
#define	RT_OB_HDR_TYPE_SET			2
#define	RT_OB_HDR_TYPE_QUERY_SET	3
#define	RT_OB_HDR_TYPE_INDIC		4
#define	RT_OB_HDR_TYPE_DATA			5

BOOLEAN 
eqNByte(
	pu1Byte	str1,
	pu1Byte	str2,
	u4Byte	num
	);

BOOLEAN 
IsHexDigit(
	IN		s1Byte		chTmp
	);

u4Byte
MapCharToHexDigit(
	IN		s1Byte		chTmp
);

BOOLEAN 
GetHexValueFromString(
	IN		ps1Byte			szStr,
	IN OUT	pu4Byte			pu4bVal,
	IN OUT	pu4Byte			pu4bMove
	);

BOOLEAN 
GetFractionValueFromString(
	IN		ps1Byte			szStr,
	IN OUT	pu1Byte			pInteger,
	IN OUT  pu1Byte			pFraction,
	IN OUT	pu4Byte			pu4bMove
	);


BOOLEAN
IsCommentString(
	IN		ps1Byte			szStr
	);

BOOLEAN 
ParseQualifiedString(
    IN      ps1Byte 		In, 
    IN OUT  pu4Byte 		Start, 
    OUT     ps1Byte 		Out,
    OUT     const u4Byte 	MaxOutLen,    
    IN      s1Byte  		LeftQualifier, 
    IN      s1Byte  		RightQualifier
    );

BOOLEAN
GetU1ByteIntegerFromStringInDecimal(
	IN		ps1Byte Str,
	IN OUT	pu1Byte pInt
	);

BOOLEAN
GetS1ByteIntegerFromStringInDecimal(
	IN		ps1Byte Str,
	IN OUT	ps1Byte pInt
	);

BOOLEAN
isAllSpaceOrTab(
	pu1Byte data,
	u1Byte	size
	);

u4Byte
GenTag(
	IN	char	*pFunName
	);
	

#endif // #ifndef __INC_GENERALFUNC_H
