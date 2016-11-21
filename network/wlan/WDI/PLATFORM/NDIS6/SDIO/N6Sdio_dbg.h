#ifndef __INC_N6SDIO_DBG_H
#define __INC_N6SDIO_DBG_H

extern u1Byte GlobalSdioDbg;




// For SDIO debug, added by Roger, 2011.01.11.
#define SDIO_DBG_CMD			BIT0
#define SDIO_DBG_ASYN_IO		BIT1
#define SDIO_DBG_IO_STRESS		BIT2
#define SDIO_DBG_CCCR			BIT3
#define SDIO_DBG_FBR			BIT4
#define SDIO_DBG_CIS			BIT5
#define SDIO_DBG_LA_TRIGER		BIT6


// SDIO Debug control for Command Line, added by Roger, 2011.05.20.
#define SDIO_REG_CTRL_MASK				0x0F
#define SDIO_REG_CTRL_LOCAL			BIT(0)
#define SDIO_REG_CTRL_CMD52			BIT(1)

#endif
