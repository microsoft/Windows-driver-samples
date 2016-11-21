#ifndef __INC_ETHERNET_H
#define __INC_ETHERNET_H

#define ETHERNET_ADDRESS_LENGTH			6
#define ETHERNET_HEADER_SIZE				14
#define LLC_HEADER_SIZE						6
#define TYPE_LENGTH_FIELD_SIZE				2
#define MINIMUM_ETHERNET_PACKET_SIZE		60
#define MAXIMUM_ETHERNET_PACKET_SIZE		1514

#define RT_ETH_IS_MULTICAST(_pAddr)	((((pu1Byte)(_pAddr))[0]&0x01)!=0)
#define RT_ETH_IS_BROADCAST(_pAddr)	(										\
											((pu1Byte)(_pAddr))[0]==0xff	&& 		\
											((pu1Byte)(_pAddr))[1]==0xff	&&		\
											((pu1Byte)(_pAddr))[2]==0xff	&&		\
											((pu1Byte)(_pAddr))[3]==0xff	&&		\
											((pu1Byte)(_pAddr))[4]==0xff	&&		\
											((pu1Byte)(_pAddr))[5]==0xff		)

//
// TOSHIBA_PWR_INDEX: customized for Toshiba. Power index "0" at channel 12~14 presents
// disableing the specified channel(no receive and send). By Bruce, 2007-09-03.
// The define is always be set enable
#define TOSHIBA_PWR_INDEX	1    //Merge by Jacken 2008/01/31

#endif // #ifndef __INC_ETHERNET_H
