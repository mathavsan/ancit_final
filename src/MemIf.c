#include "MemIf.h"
#include "Ea.h"

Std_ReturnType MemIf_Read(uint8_t DeviceIndex, uint16_t BlockNumber, uint16_t BlockOffset,
                          uint8_t *DataBufferPtr, uint16_t Length)
{
    return (DeviceIndex == 0U) ? Ea_Read(BlockNumber, BlockOffset, DataBufferPtr, Length) : E_NOT_OK;
}

Std_ReturnType MemIf_Write(uint8_t DeviceIndex, uint16_t BlockNumber, const uint8_t *DataBufferPtr)
{
    return (DeviceIndex == 0U) ? Ea_Write(BlockNumber, DataBufferPtr) : E_NOT_OK;
}

MemIf_StatusType MemIf_GetStatus(uint8_t DeviceIndex)
{
    return (DeviceIndex == 0U) ? Ea_GetStatus() : MEMIF_UNINIT;
}

MemIf_JobResultType MemIf_GetJobResult(uint8_t DeviceIndex)
{
    return (DeviceIndex == 0U) ? Ea_GetJobResult() : MEMIF_JOB_FAILED;
}
