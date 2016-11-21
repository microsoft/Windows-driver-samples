#ifndef __INC_NDIS_OID_H
	#define __INC_NDIS_OID_H

#if 1//DBG
	// Check Build
	#define RT_OID_ENTRY_STRING_SIZE		80
	#define OID_STR_WRAPPER(oid_str)		oid_str
#else
	// Free Build (Reduce the memery usage)
	#define RT_OID_ENTRY_STRING_SIZE		1
	#define OID_STR_WRAPPER(oid_str)		""
#endif


typedef struct __RT_OID_ENTRY
{
	u4Byte 		Oid;	
	char			szID[RT_OID_ENTRY_STRING_SIZE]; 		// The string name of this OID

	NDIS_STATUS (*Func)(							// The OID handler function
		IN  PADAPTER pAdapter,
		IN  PNDIS_OID_REQUEST   NdisRequest
		);

}RT_OID_ENTRY, *PRT_OID_ENTRY;


	
#endif
