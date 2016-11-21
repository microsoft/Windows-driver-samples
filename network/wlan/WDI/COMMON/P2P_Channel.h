//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_CHANNEL_H
#define __INC_P2P_CHANNEL_H

// P2P_MAX_REG_CLASSES - Maximum number of regulatory classes
#define P2P_MAX_REG_CLASSES 10

// P2P_MAX_REG_CLASS_CHANNELS - Maximum number of channels per regulatory class
#define P2P_MAX_REG_CLASS_CHANNELS 20

#define P2P_REG_CLASS_NONE 0

typedef struct _P2P_CHANNEL
{
	u1Byte 			regClass;
	u1Byte			channel;
}P2P_CHANNEL;

typedef struct _P2P_REG_CLASS
{
	u1Byte 			regClass;
	u1Byte 			channel[P2P_MAX_REG_CLASS_CHANNELS];
	u1Byte 			channels;
}P2P_REG_CLASS;

typedef struct _P2P_CHANNELS
{
	P2P_REG_CLASS 	regClass[P2P_MAX_REG_CLASSES];
	u1Byte			regClasses;
}P2P_CHANNELS;


#endif	// #ifndef __INC_P2P_CHANNEL_H