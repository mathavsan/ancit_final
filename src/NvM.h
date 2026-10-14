#ifndef NVM_H
#define NVM_H
#include "Std_Types.h"

typedef uint16_t NvM_BlockIdType;

typedef enum
{
    NVM_REQ_OK = 0, NVM_REQ_NOT_OK, NVM_REQ_PENDING, NVM_REQ_INTEGRITY_FAILED,
    NVM_REQ_BLOCK_SKIPPED, NVM_REQ_NV_INVALIDATED, NVM_REQ_CANCELED,
    NVM_REQ_REDUNDANCY_FAILED, NVM_REQ_RESTORED_FROM_ROM
} NvM_RequestResultType;

typedef struct
{
    uint16_t        nvLength;
    uint16_t        eaBlockNumber;
    boolean         useCrc;
    uint8_t        *ramBlock;
    const uint8_t  *romBlock;
} NvM_BlockDescriptorType;

typedef struct { const NvM_BlockDescriptorType *blocks; uint16_t blockCount; } NvM_ConfigType;

void           NvM_Init(const NvM_ConfigType *ConfigPtr);
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void *NvM_SrcPtr);
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void *NvM_DstPtr);
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged);
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr);
void           NvM_MainFunction(void);
#endif
