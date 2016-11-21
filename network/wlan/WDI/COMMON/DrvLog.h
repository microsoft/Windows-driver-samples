#ifndef __INC_DRV_LOG_H
#define __INC_DRV_LOG_H

typedef struct _DRV_LOG_TYPE_ATTRIBUTE_T
{
	unsigned int	MaxLogCountPwr; // 2's power, reprent for max number of logs of this type kept in driver, e.g. 5 means at most 2^5 logs kept.
	unsigned int	DescLen; // Length of bytes used in Description[] including EOS.
	char			Description[1]; // Null terminated string.
}DRV_LOG_TYPE_ATTRIBUTE_T, *PDRV_LOG_TYPE_ATTRIBUTE_T;

typedef struct _DRV_LOG_TYPE_LIST_T
{
	unsigned int				Count;
	DRV_LOG_TYPE_ATTRIBUTE_T	LogTypeAttributes[1];
}DRV_LOG_TYPE_LIST_T, *PDRV_LOG_TYPE_LIST_T;

typedef struct _DRV_LOG_ATTRIBUTE_T
{
	unsigned int	Type; // Key to assoicated with DRV_LOG_TYPE_ATTRIBUTE_T.
	unsigned int	DescLen; // Length of bytes used in Description[] including EOS.
	char			Description[1]; // Null terminated string.
}DRV_LOG_ATTRIBUTE_T, *PDRV_LOG_ATTRIBUTE_T;

typedef struct _DRV_LOG_ATTR_LIST_T
{
	unsigned int				Count;
	DRV_LOG_ATTRIBUTE_T			LogAttributes[1];
}DRV_LOG_ATTR_LIST_T, *PDRV_LOG_ATTR_LIST_T;

typedef struct _DRV_LOG_DATA_T
{
	unsigned int	Id; // Key to associated with DRV_LOG_ATTRIBUTE_T.
	unsigned int	TimeStampLow;
	unsigned int	TimeStampHigh;
	unsigned int	BufferLenUsed; 
	unsigned char	Buffer[1];
}DRV_LOG_DATA_T, *PDRV_LOG_DATA_T;

typedef struct _DRV_LOG_DATA_LIST_T
{
	unsigned int			Count;
	DRV_LOG_DATA_T			LogDatas[1];
}DRV_LOG_DATA_LIST_T, *PDRV_LOG_DATA_LIST_T;


#endif // #ifndef __INC_DRV_LOG_H
