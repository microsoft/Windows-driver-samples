#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Channel.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
VOID
p2p_channel_AddRegClass(
	IN  P2P_REG_CLASS 			*pRegCls,
	IN  u1Byte					regClass,
	IN  u1Byte					nChannels,
	IN  const u1Byte			*channelList
	)
{
	pRegCls->regClass = regClass;
	pRegCls->channels = nChannels;
	p2p_MoveMemory(pRegCls->channel, channelList, nChannels);

	return;
}

VOID
p2p_channel_IntersectRegClass(
	IN  const P2P_REG_CLASS		*a,
	IN  const P2P_REG_CLASS		*b,
	OUT P2P_REG_CLASS			*res
	)
{
	u1Byte						ita = 0;
	u1Byte						itb = 0;

	PlatformZeroMemory(res, sizeof(*res));

	res->regClass = a->regClass;
	res->channels = 0;

	for(ita = 0; ita < a->channels; ita++)
	{
		for(itb = 0; itb < b->channels; itb++)
		{
			if(a->channel[ita] != b->channel[itb])
				continue;
			
			res->channel[res->channels++] = a->channel[ita];
			if(P2P_MAX_REG_CLASS_CHANNELS == res->channels)
				return;
		}
	}
}

VOID
p2p_channel_Dump(
	IN  const P2P_CHANNELS		*channels
	)
{
	u1Byte						itRegCls = 0;
	u1Byte						itChnl = 0;
	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("# reg class: %u\n", channels->regClasses));

	for(itRegCls = 0; itRegCls < channels->regClasses; itRegCls++)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("reg: %u, ", channels->regClass[itRegCls].regClass));
		for(itChnl = 0; itChnl < channels->regClass[itRegCls].channels; itChnl++)
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%u ", channels->regClass[itRegCls].channel[itChnl]));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("\n"));
	}
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

BOOLEAN
p2p_Channel_Add(
	IN  P2P_CHANNELS			*channels,
	IN  u1Byte					regClass,
	IN  u1Byte					nChannels,
	IN  const u1Byte			*channelList
	)
{
	u1Byte						itRegClass = 0;
	u1Byte						itChannel = 0;
	BOOLEAN						bAdded = FALSE;

	if(P2P_MAX_REG_CLASS_CHANNELS < nChannels)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("too many channels: %u\n", nChannels));
		return FALSE;
	}

	for(itRegClass = 0; itRegClass < channels->regClasses; itRegClass++)
	{
		P2P_REG_CLASS			*pRegCls = &channels->regClass[itRegClass];
		
		if(regClass != pRegCls->regClass)
			continue;

		// Override all the orignal channels
		p2p_channel_AddRegClass(pRegCls, regClass, nChannels, channelList);
		bAdded = TRUE;
		break;
	}

	if(!bAdded)
	{
		if(P2P_MAX_REG_CLASSES <= channels->regClasses)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("no more reg class could be added\n"));
			return FALSE;
		}

		p2p_channel_AddRegClass(&channels->regClass[channels->regClasses++], regClass, nChannels, channelList);
		bAdded = TRUE;
	}

	return bAdded;
}

VOID
p2p_Channel_Reset(
	IN  P2P_CHANNELS			*channels
	)
{
	PlatformZeroMemory(channels, sizeof(*channels));

	return;
}

VOID
p2p_Channel_Intersect(
	IN  const P2P_CHANNELS		*a,
	IN  const P2P_CHANNELS		*b,
	OUT P2P_CHANNELS			*res
	)
{
	u1Byte						itRegClsA = 0;
	u1Byte						itRegClsB = 0;
	
	p2p_Channel_Reset(res);
	
	if(!a->regClasses || !b->regClasses)
		return;

	for(itRegClsA = 0; itRegClsA < a->regClasses; itRegClsA++)
	{
		// Match reg class
		for(itRegClsB = 0; itRegClsB < b->regClasses; itRegClsB++)
			if(a->regClass[itRegClsA].regClass == b->regClass[itRegClsB].regClass)
				break;

		// The class can't be found in b
		if(itRegClsB == b->regClasses)
			continue;

		p2p_channel_IntersectRegClass(&a->regClass[itRegClsA], &b->regClass[itRegClsB], &res->regClass[res->regClasses]);
		if(res->regClass[res->regClasses].channels)
		{
			res->regClasses++;
			if(P2P_MAX_REG_CLASSES == res->regClasses)
				return;
		}
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("a:\n"));
	p2p_channel_Dump(a);
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("b:\n"));
	p2p_channel_Dump(b);
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("res:\n"));
	p2p_channel_Dump(res);

	return;
}

BOOLEAN
p2p_Channel_ChannelListAttrToChannels(
	IN  u2Byte					chnlListAttrLen,
	IN  const u1Byte			*chnlListAttr,
	OUT P2P_CHANNELS			*pChannels
	)
{
	const u1Byte				*pos = chnlListAttr;
	const u1Byte				*end = chnlListAttr + chnlListAttrLen;
	
	if(!chnlListAttr)
		return FALSE;

	if(!chnlListAttrLen)
		return FALSE;

	pChannels->regClasses = 0;

	while(pos < end)
	{
		P2P_REG_CLASS			*regClass = pChannels->regClass + pChannels->regClasses;
		u1Byte					channels = 0;

		// class
		regClass->regClass = pos[0];
		pos++;

		// number of channels
		channels = pos[1];
		pos++;
		
		if(sizeof(regClass->channel) < channels)
			channels = sizeof(regClass->channel);

		// channel list
		for(regClass->channels = 0; regClass->channels < channels; regClass->channels++)
		{
			regClass->channel[regClass->channels] = pos[0];
			pos++;
		}

		pChannels->regClasses++;
	}

	return TRUE;
}

BOOLEAN
p2p_Channel_InChannelEntryList(
	IN  u1Byte					channel,
	IN  const P2P_CHANNELS		*pChannels
	)
{
	u1Byte						itRegClass = 0;

	for(itRegClass = 0; itRegClass < pChannels->regClasses; itRegClass++)
	{
		const P2P_REG_CLASS		*regClass = pChannels->regClass + itRegClass;
		u1Byte					itChannel = 0;

		for(itChannel = 0; itChannel < regClass->channels; itChannel++)
		{
			if(regClass->channel[itChannel] == channel)
				return TRUE;
		}
	}

	return FALSE;
}

#endif
