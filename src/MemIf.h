#ifndef MEMIF_H
#define MEMIF_H
#include "Std_Types.h"

typedef enum { MEMIF_UNINIT = 0, MEMIF_IDLE, MEMIF_BUSY, MEMIF_BUSY_INTERNAL } MemIf_StatusType;
typedef enum { MEMIF_JOB_OK = 0, MEMIF_JOB_FAILED, MEMIF_JOB_PENDING, MEMIF_JOB_CANCELED,
               MEMIF_BLOCK_INCONSISTENT, MEMIF_BLOCK_INVALID } MemIf_JobResultType;

Std_ReturnType      MemIf_Read(uint8_t DeviceIndex, uint16_t BlockNumber, uint16_t BlockOffset,
                               uint8_t *DataBufferPtr, uint16_t Length);
Std_ReturnType      MemIf_Write(uint8_t DeviceIndex, uint16_t BlockNumber, const uint8_t *DataBufferPtr);
MemIf_StatusType    MemIf_GetStatus(uint8_t DeviceIndex);
MemIf_JobResultType MemIf_GetJobResult(uint8_t DeviceIndex);
#endif
