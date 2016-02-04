#ifndef __INC_EEPROM_H
#define __INC_EEPROM_H

#define	EEPROM_MAX_SIZE			256
#define	CLOCK_RATE					50			//100us		

//- EEPROM opcodes
#define EEPROM_READ_OPCODE		06
#define EEPROM_WRITE_OPCODE		05
#define EEPROM_ERASE_OPCODE		07
#define EEPROM_EWEN_OPCODE		19      // Erase/write enable
#define EEPROM_EWDS_OPCODE		16      // Erase/write disable

#define CSR_EEPROM_CONTROL_REG	(Adapter->HalFunc.pEEPROMOffset->CmdRegister)
#define EEDO							(Adapter->HalFunc.pEEPROMOffset->BIT_EEDO)	// EEPROM data out
#define EEDI							(Adapter->HalFunc.pEEPROMOffset->BIT_EEDI)		// EEPROM data in (set for writing data)
#define EECS							(Adapter->HalFunc.pEEPROMOffset->BIT_EECS)	// EEPROM chip select (1=high, 0=low)
#define EESK							(Adapter->HalFunc.pEEPROMOffset->BIT_EESK)	// EEPROM shift clock (1=high, 0=low)
#define EEM0							(Adapter->HalFunc.pEEPROMOffset->BIT_EEM0)
#define EEM1							(Adapter->HalFunc.pEEPROMOffset->BIT_EEM1)


u2Byte
GetEEpromAddressSize(
	u2Byte  Size
	);

u2Byte
GetEEpromSize(
	PADAPTER	Adapter
	);

u2Byte 
ReadEEprom(
	PADAPTER	Adapter,
	u2Byte 		Reg
	);



VOID 
ShiftOutBits(
	PADAPTER	Adapter,
	u2Byte		data,
	u2Byte		count
	);

u2Byte 
ShiftInBits(
	PADAPTER	Adapter
	);

VOID 
RaiseClock(
	PADAPTER	Adapter,
	u2Byte		*x
	);

VOID 
LowerClock(
	PADAPTER	Adapter,
	u2Byte		*x
	);

VOID 
EEpromCleanup(
	PADAPTER	Adapter
	);

VOID
UpdateChecksum(
	PADAPTER	Adapter
	);

VOID
WriteEEprom(
	PADAPTER	Adapter,
	u2Byte		reg,
	u2Byte		data
	);

u2Byte
WaitEEPROMCmdDone(
	PADAPTER	Adapter
	);

VOID
StandBy(
	PADAPTER	Adapter
	);

extern	BOOLEAN	
EEPROM_ProgramMap(
	IN	PADAPTER	Adapter,
	IN	ps1Byte 		pFileName);

#endif // #ifndef __INC_EEPROM_H
