#ifndef __INC_ADCSMP_H
#define __INC_ADCSMP_H

typedef struct _RT_ADCSMP_STRING{
	pu4Byte		Octet;
	u4Byte		Length;
	u4Byte		buffer_size;
	u4Byte		start_pos;
}RT_ADCSMP_STRING, *PRT_ADCSMP_STRING;


typedef enum _RT_ADCSMP_TRIG_SEL
{
	ADCSMP_BB_TRIG, 
	ADCSMP_MAC_TRIG, 
}RT_ADCSMP_TRIG_SEL, *PRT_ADCSMP_TRIG_SEL;


typedef enum _RT_ADCSMP_TRIG_SIG_SEL
{
	ADCSMP_TRIG_CRCOK, 
	ADCSMP_TRIG_CRCFAIL, 
	ADCSMP_TRIG_CCA,
	ADCSMP_TRIG_REG,
}RT_ADCSMP_TRIG_SIG_SEL, *PRT_ADCSMP_TRIG_SIG_SEL;


typedef enum _RT_ADCSMP_STATE
{
	ADCSMP_STATE_IDLE, 
	ADCSMP_STATE_SET, 
	ADCSMP_STATE_QUERY,
}RT_ADCSMP_STATE, *PRT_ADCSMP_STATE;


typedef struct _RT_ADCSMP
{
	RT_ADCSMP_STRING		ADCSmpBuf;
	RT_ADCSMP_STATE			ADCSmpState;
	RT_ADCSMP_TRIG_SEL		ADCSmpTrigSel;
	RT_ADCSMP_TRIG_SIG_SEL 	ADCSmpTrigSigSel;
	u1Byte					ADCSmpDmaDataSigSel;
	u1Byte					ADCSmpTriggerTime;
	u2Byte					ADCSmpPollingTime;
}RT_ADCSMP, *PRT_ADCSMP;


VOID
ADCSmpWorkItemCallback(
	IN	PVOID	pContext
	);


VOID
ADCSmp_Set(
	IN	PADAPTER				Adapter,
	IN	RT_ADCSMP_TRIG_SEL		TrigSel,
	IN	RT_ADCSMP_TRIG_SIG_SEL	TrigSigSel,
	IN	u1Byte					DmaDataSigSel,
	IN	u1Byte					TriggerTime,
	IN	u2Byte					PollingTime
	);


RT_STATUS
ADCSmp_Query(
	IN	PADAPTER			Adapter,
	IN 	ULONG				InformationBufferLength, 
	OUT	PVOID				InformationBuffer, 
	OUT	PULONG				BytesWritten
	);

VOID
ADCSmp_Stop(
	IN	PADAPTER			Adapter
	);

VOID
ADCSmp_Init(
	PADAPTER		Adapter
	);


VOID
ADCSmp_DeInit(
	PADAPTER		Adapter
	);

VOID
Dump_MAC(
	PADAPTER		Adapter
	);

VOID
Dump_BB(
	PADAPTER		Adapter
	);

VOID
Dump_RF(
	PADAPTER		Adapter
	);

#endif

