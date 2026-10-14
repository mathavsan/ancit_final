#include "Ea.h"
#include "NvM_Cbk.h"
#include "eeprom_ex2.h"
#include <stddef.h>

typedef enum { EA_JOB_NONE = 0, EA_JOB_READ, EA_JOB_WRITE } Ea_JobType;

static const Ea_ConfigType *s_cfg    = NULL;
static MemIf_StatusType     s_status = MEMIF_UNINIT;
static MemIf_JobResultType  s_result = MEMIF_JOB_FAILED;
static Ea_JobType           s_job    = EA_JOB_NONE;
static const Ea_BlockConfigType *s_jobBlock = NULL;
static uint8_t             *s_jobBuf = NULL;
static uint16_t             s_jobOffset = 0U;
static uint16_t             s_jobLength = 0U;

static const Ea_BlockConfigType *Ea_Find(uint16_t blockNumber)
{
    if (s_cfg == NULL) { return NULL; }
    for (uint16_t i = 0U; i < s_cfg->blockCount; i++)
    {
        if (s_cfg->blocks[i].blockNumber == blockNumber) { return &s_cfg->blocks[i]; }
    }
    return NULL;
}

static boolean Ea_ConfigValid(const Ea_ConfigType *c)
{
    if ((c == NULL) || (c->blocks == NULL) || (c->blockCount == 0U)) { return FALSE; }
    for (uint16_t i = 0U; i < c->blockCount; i++)
    {
        const Ea_BlockConfigType *a = &c->blocks[i];
        if ((a->length == 0U) || (a->eepromAddress >= EEPROM_SIZE_BYTES) ||
           ((uint32_t)a->length > ((uint32_t)EEPROM_SIZE_BYTES - a->eepromAddress)))
        {
            return FALSE;
        }
        for (uint16_t j = (uint16_t)(i + 1U); j < c->blockCount; j++)
        {
            const Ea_BlockConfigType *b = &c->blocks[j];
            uint32_t aEnd = (uint32_t)a->eepromAddress + a->length;
            uint32_t bEnd = (uint32_t)b->eepromAddress + b->length;
            if ((a->blockNumber == b->blockNumber) ||
               ((a->eepromAddress < bEnd) && (b->eepromAddress < aEnd)))
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void Ea_Init(const Ea_ConfigType *ConfigPtr)
{
    s_job = EA_JOB_NONE;
    if (Ea_ConfigValid(ConfigPtr))
    {
        s_cfg = ConfigPtr; s_status = MEMIF_IDLE; s_result = MEMIF_JOB_OK;
    }
    else
    {
        s_cfg = NULL; s_status = MEMIF_UNINIT; s_result = MEMIF_JOB_FAILED;
    }
}

uint16_t Ea_GetBlockLength(uint16_t BlockNumber)
{
    const Ea_BlockConfigType *b = Ea_Find(BlockNumber);
    return (b != NULL) ? b->length : 0U;
}

Std_ReturnType Ea_Read(uint16_t BlockNumber, uint16_t BlockOffset, uint8_t *DataBufferPtr, uint16_t Length)
{
    const Ea_BlockConfigType *b = Ea_Find(BlockNumber);
    if ((s_status != MEMIF_IDLE) || (b == NULL) || (DataBufferPtr == NULL) || (Length == 0U) ||
        ((uint32_t)BlockOffset + Length > b->length))
    {
        return E_NOT_OK;
    }
    s_job = EA_JOB_READ; s_jobBlock = b; s_jobBuf = DataBufferPtr;
    s_jobOffset = BlockOffset; s_jobLength = Length;
    s_status = MEMIF_BUSY; s_result = MEMIF_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Ea_Write(uint16_t BlockNumber, const uint8_t *DataBufferPtr)
{
    const Ea_BlockConfigType *b = Ea_Find(BlockNumber);
    if ((s_status != MEMIF_IDLE) || (b == NULL) || (DataBufferPtr == NULL))
    {
        return E_NOT_OK;
    }
    s_job = EA_JOB_WRITE; s_jobBlock = b; s_jobBuf = (uint8_t *)DataBufferPtr;
    s_jobOffset = 0U; s_jobLength = b->length;
    s_status = MEMIF_BUSY; s_result = MEMIF_JOB_PENDING;
    return E_OK;
}

MemIf_StatusType Ea_GetStatus(void)       { return s_status; }
MemIf_JobResultType Ea_GetJobResult(void) { return s_result; }

void Ea_MainFunction(void)
{
    status_t st;
    uint16_t addr;

    if ((s_status != MEMIF_BUSY) || (s_job == EA_JOB_NONE) || (s_jobBlock == NULL)) { return; }

    addr = (uint16_t)(s_jobBlock->eepromAddress + s_jobOffset);
    if (s_job == EA_JOB_WRITE)
    {
        st = EEPROM_WriteMultiPage(addr, s_jobBuf, s_jobLength);
    }
    else
    {
        st = EEPROM_Read(addr, s_jobBuf, s_jobLength);
    }

    s_job = EA_JOB_NONE;
    s_status = MEMIF_IDLE;
    if (st == STATUS_SUCCESS)
    {
        s_result = MEMIF_JOB_OK;
        NvM_JobEndNotification();
    }
    else
    {
        s_result = MEMIF_JOB_FAILED;
        NvM_JobErrorNotification();
    }
}
