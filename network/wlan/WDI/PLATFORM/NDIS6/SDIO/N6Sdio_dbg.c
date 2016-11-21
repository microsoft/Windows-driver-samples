#include "Mp_Precomp.h"


// For SDIO framework development.
u1Byte GlobalSdioDbg				=	\
//									SDIO_DBG_CMD 			| 	// CMD52/53 R/W debugging
//									SDIO_DBG_ASYN_IO		|	// AsynIO debug
//									SDIO_DBG_IO_STRESS		|	// IO Stress test
//									SDIO_DBG_CCCR			|	// Dump CCCR registers
//									SDIO_DBG_FBR			|	// Dump FBR registers
//									SDIO_DBG_CIS			|	// Dump CIS registers
//									SDIO_DBG_LA_TRIGER		|	// Triger LA
									0;
