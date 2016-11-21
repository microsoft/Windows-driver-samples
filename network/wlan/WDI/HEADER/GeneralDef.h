#ifndef __INC_GENERALDEF_H
#define __INC_GENERALDEF_H

/*
 *	Note:	1.Only pure value definition can be put here.
 *			2.The definition here should be hardware and platform independent.
 *
*/

#define IN
#define OUT

#ifndef TRUE
	#define TRUE		1
#endif
#ifndef FALSE
	#define FALSE	0
#endif

#ifndef true
	#define true		1
#endif
#ifndef false
	#define false		0
#endif

#define BIT0		0x00000001
#define BIT1		0x00000002
#define BIT2		0x00000004
#define BIT3		0x00000008
#define BIT4		0x00000010
#define BIT5		0x00000020
#define BIT6		0x00000040
#define BIT7		0x00000080
#define BIT8		0x00000100
#define BIT9		0x00000200
#define BIT10	0x00000400
#define BIT11	0x00000800
#define BIT12	0x00001000
#define BIT13	0x00002000
#define BIT14	0x00004000
#define BIT15	0x00008000
#define BIT16	0x00010000
#define BIT17	0x00020000
#define BIT18	0x00040000
#define BIT19	0x00080000
#define BIT20	0x00100000
#define BIT21	0x00200000
#define BIT22	0x00400000
#define BIT23	0x00800000
#define BIT24	0x01000000
#define BIT25	0x02000000
#define BIT26	0x04000000
#define BIT27	0x08000000
#define BIT28	0x10000000
#define BIT29	0x20000000
#define BIT30	0x40000000
#define BIT31	0x80000000

#define BIT32	UINT64_C(0x0000000100000000)
#define BIT33	UINT64_C(0x0000000200000000)
#define BIT34	UINT64_C(0x0000000400000000)
#define BIT35	UINT64_C(0x0000000800000000)
#define BIT36	UINT64_C(0x0000001000000000)
#define BIT37	UINT64_C(0x0000002000000000)
#define BIT38	UINT64_C(0x0000004000000000)
#define BIT39	UINT64_C(0x0000008000000000)
#define BIT40	UINT64_C(0x0000010000000000)
#define BIT41	UINT64_C(0x0000020000000000)
#define BIT42	UINT64_C(0x0000040000000000)
#define BIT43	UINT64_C(0x0000080000000000)
#define BIT44	UINT64_C(0x0000100000000000)
#define BIT45	UINT64_C(0x0000200000000000)
#define BIT46	UINT64_C(0x0000400000000000)
#define BIT47	UINT64_C(0x0000800000000000)
#define BIT48	UINT64_C(0x0001000000000000)
#define BIT49	UINT64_C(0x0002000000000000)
#define BIT50	UINT64_C(0x0004000000000000)
#define BIT51	UINT64_C(0x0008000000000000)
#define BIT52	UINT64_C(0x0010000000000000)
#define BIT53	UINT64_C(0x0020000000000000)
#define BIT54	UINT64_C(0x0040000000000000)
#define BIT55	UINT64_C(0x0080000000000000)
#define BIT56	UINT64_C(0x0100000000000000)
#define BIT57	UINT64_C(0x0200000000000000)
#define BIT58	UINT64_C(0x0400000000000000)
#define BIT59	UINT64_C(0x0800000000000000)
#define BIT60	UINT64_C(0x1000000000000000)
#define BIT61	UINT64_C(0x2000000000000000)
#define BIT62	UINT64_C(0x4000000000000000)
#define BIT63	UINT64_C(0x8000000000000000)


//-----------------------------------------------------------------------------------------
// Use one of the following value to define the flag, RT_PLATFORM.
//-----------------------------------------------------------------------------------------
#define PLATFORM_WINDOWS		0
#define PLATFORM_LINUX			1
#define PLATFORM_FREEBSD		3
#define PLATFORM_MACOSX		4

//-----------------------------------------------------------------------------------------
// OS versions
//----------------------------------------------------------------------------------------
//	It is mainly for the conditional compilation.

// Unknown OS
#define OS_UNKNOWN			0x00

// Windows Version (1st byte):
#define OS_WIN_XP			0x01
#define OS_WIN_VISTA		0x02
#define OS_WIN_7			0x03
#define OS_WIN_8			0x04
#define OS_WIN_BLUE		0x05
#define OS_WIN_10			0x06

// MAC OS Version (2nd byte):
#define OS_MAC_LEOPARD	(0x01 << 8)
#define OS_MAC_TIGER		(0x02 << 8)
#define OS_MAC_PANTHER	(0x03 << 8)
#define OS_MAC_JAGUAR		(0x04 << 8)

// Linux OS Version (3rd byte):
#define OS_LINUX_24			OS_WIN_XP	// TODO: We should modify COMMON code and change this definition
#define OS_LINUX_26			OS_WIN_XP	// TODO: We should modify COMMON code and change this definition

#define OS_WIN_FROM_VISTA(v) 		((v & 0xFF) >= OS_WIN_VISTA)
#define OS_WIN_FROM_WIN7(v)		((v & 0xFF) >= OS_WIN_7)
#define OS_WIN_FROM_WIN8(v)		((v & 0xFF) >= OS_WIN_8)
#define OS_WIN_FROM_WIN_BLUE(v)	((v & 0xFF) >= OS_WIN_BLUE)
#define OS_WIN_FROM_WIN10(v)	((v & 0xFF) >= OS_WIN_10)


#define RUNTIME_OS_WIN_FROM_WIN8(_pAdapter) ((_pAdapter)->MgntInfo.NdisVersion >= RT_NDIS_VERSION_6_30)
#define RUNTIME_OS_WIN_FROM_WINBLUE(_pAdapter) ((_pAdapter)->MgntInfo.NdisVersion >= RT_NDIS_VERSION_6_40)


//-----------------------------------------------------------------------------------------
// Use one of the following value to define the flag, HAL_CODE_BASE.
//-----------------------------------------------------------------------------------------
#define RTL818X					BIT0	// For 8185/8187.
#define RTL818X_B				BIT1	// For 8185B/8187B. 
#define RTL818X_S				(BIT1 | BIT2)     	// For RTL8187S (PCIe & USB)
#define RTL819X					BIT3			// For 819x series, 8190, 8192E, 8192U, 8192SP, 8192SU, 8193SP, 8193SU 
#define RTL8190					(BIT3 | BIT4)	// For 8190 PCI
#define RTL8192					(BIT3 | BIT5)	// For 8192 (PCI-E & USB)
#define RTL8192_S				(BIT3 | BIT6)	// For 8192_s (PCI-E & USB)
#define RTL8192_C				(BIT3 | BIT7)	// For 8192_c (PCI-E & USB)
#define RTL8193					(BIT3 | BIT8)	// For 8193_s (PCI-E & USB)

//-----------------------------------------------------------------------------------------
// Use one of the following value to define the flag, DEV_BUS_TYPE.
//-----------------------------------------------------------------------------------------
#define RT_PCI_INTERFACE				1
#define RT_USB_INTERFACE				2
#define RT_SDIO_INTERFACE				3

#endif // #ifndef __INC_GENERALDEF_H
