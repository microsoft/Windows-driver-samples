#ifndef __INC_PRECOMP_H
#define __INC_PRECOMP_H


//========================= Platform OS version definition ============================//
// The following definitions come from the batch-file-created file.                    //
// 		#define RT_PLATFORM		<RT_PLATFORM>                                          //
// 		#define OS_VERSION		<OS_VERSION_VALUE>                                     //
//=====================================================================================//
#include "GeneralDef.h"	       // Define the RT_PLATFORM and the OS_VERSION values
#include "platform-os-version.h"	

#define	WPP_SOFTWARE_TRACE					1
#define	SOFTWARE_TRACE_LOGGING				1

//=====================================================================================//
//                                Common definitions                                   //
//=====================================================================================//
#define	DM_ODM_SUPPORT_TYPE					0x08
#define DEV_BUS_TYPE								RT_SDIO_INTERFACE

#define	DRV_LOG											1


// Statistic
#define STATISTIC_SUPPORT       					1  	// counter caculate.    

// Auto Channel Select by NHM statistic
#define AUTO_CHNL_SEL_NHM					1

// Debug for Rockchip RK platform temporarily, this define shall be removed later
#define RK_PLATFORM_SUPPORT					0

// LE do not cache BSS List if this macro is 0, which is recommendation setting.
#define BSS_LIST_CACHE						0

// LE do not override BSS selection set by UE if this macro is 0, which is recommendation settting.
#define CONNECT_BSS_SELECTION_OVERRIDE		0

#ifndef ELIMINATE_THREAD_FOR_COMMAND_HANDLE
#define ELIMINATE_THREAD_FOR_COMMAND_HANDLE
#endif

// only turn on this flag when driver capable to detect FW stall
#define ALLOW_INDICATE_FW_STALL				0

#define		REMOVE_PACK							1
#define		DFS_SUPPORT							1
#define		DFS_SUPPORT_STA_MODE		1

#define		P2P_SUPPORT							1
#define		WPS_SUPPORT							1


#define MUTIPLE_BULK_OUT

#define TX_AGGREGATION						1	// For Tx Aggregation
#define RX_AGGREGATION						1	// For Rx Aggregation 

#define RTL8723_SDIO_IO_THREAD_ENABLE	            1 


#define MULTIPORT_SUPPORT									1
#define MULTICHANNEL_SUPPORT							1

#define	SUPPORT_SDIO_BUS_SPEED_SDR25			1	// For Intel Baytrail option. SDR25 = 50MHz.

#define NDIS650														1	// WDK Check Definition
#define WDI_SUPPORT												1


//================================= End of Others section =============================//
#include "PrecompInc_sample.h"
#endif // #ifndef __INC_PRECOMP_H
