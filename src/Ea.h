#ifndef EA_H
#define EA_H
#include "Std_Types.h"
#include "MemIf.h"

typedef struct { uint16_t blockNumber; uint16_t eepromAddress; uint16_t length; } Ea_BlockConfigType;
typedef struct { const Ea_BlockConfigType *blocks; uint16_t blockCount; } Ea_ConfigType;

void             Ea_Init(const Ea_ConfigType *ConfigPtr);
Std_ReturnType   Ea_Read(uint16_t BlockNumber, uint16_t BlockOffset, uint8_t *DataBufferPtr, uint16_t Length);
Std_ReturnType   Ea_Write(uint16_t BlockNumber, const uint8_t *DataBufferPtr);
MemIf_StatusType Ea_GetStatus(void);
MemIf_JobResultType Ea_GetJobResult(void);
void             Ea_MainFunction(void);

uint16_t         Ea_GetBlockLength(uint16_t BlockNumber);
#endif
