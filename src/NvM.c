#include "NvM.h"
#include "NvM_Cbk.h"
#include "MemIf.h"
#include "Ea.h"
#include <stddef.h>
#include <string.h>

#define NVM_MAX_BLOCKS      8U
#define NVM_QUEUE_SIZE      4U
#define NVM_MAX_NV_BYTES    64U

typedef enum { NVM_OP_READ = 0, NVM_OP_WRITE } NvM_OpType;
typedef struct { NvM_BlockIdType id; NvM_OpType op; void *ptr; } NvM_JobType;
typedef enum { NVM_ST_IDLE = 0, NVM_ST_WAIT_MEMIF } NvM_StateType;

static const NvM_ConfigType *s_cfg = NULL;
static NvM_RequestResultType s_result[NVM_MAX_BLOCKS + 1U];
static boolean s_ramValid[NVM_MAX_BLOCKS + 1U];
static boolean s_ramChanged[NVM_MAX_BLOCKS + 1U];
static NvM_JobType s_queue[NVM_QUEUE_SIZE];
static uint8_t s_qHead = 0U, s_qCount = 0U;
static NvM_JobType s_cur;
static NvM_StateType s_state = NVM_ST_IDLE;
static volatile boolean s_memifDone = FALSE, s_memifOk = FALSE;
static uint8_t s_buf[NVM_MAX_NV_BYTES];

static uint16_t NvM_Crc16(const uint8_t *d, uint16_t n)
{
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0U; i < n; i++)
    {
        crc = (uint16_t)(crc ^ ((uint16_t)d[i] << 8));
        for (uint8_t b = 0U; b < 8U; b++)
        {
            uint32_t t = (uint32_t)crc << 1;
            if ((crc & 0x8000U) != 0U) { t ^= 0x1021U; }
            crc = (uint16_t)t;
        }
    }
    return crc;
}

static const NvM_BlockDescriptorType *NvM_Desc(NvM_BlockIdType id)
{
    if ((s_cfg == NULL) || (id == 0U) || (id > s_cfg->blockCount)) { return NULL; }
    return &s_cfg->blocks[id - 1U];
}

static uint16_t NvM_NvBytes(const NvM_BlockDescriptorType *d)
{
    return (uint16_t)(d->nvLength + ((d->useCrc != FALSE) ? 2U : 0U));
}

void NvM_Init(const NvM_ConfigType *ConfigPtr)
{
    boolean ok = TRUE;

    s_cfg = NULL; s_qHead = 0U; s_qCount = 0U; s_state = NVM_ST_IDLE;
    s_memifDone = FALSE; s_memifOk = FALSE;

    if ((ConfigPtr == NULL) || (ConfigPtr->blocks == NULL) || (ConfigPtr->blockCount == 0U) ||
        (ConfigPtr->blockCount > NVM_MAX_BLOCKS))
    {
        return;
    }
    for (uint16_t i = 0U; i < ConfigPtr->blockCount; i++)
    {
        const NvM_BlockDescriptorType *d = &ConfigPtr->blocks[i];
        if ((d->nvLength == 0U) || (NvM_NvBytes(d) > NVM_MAX_NV_BYTES) ||
            (Ea_GetBlockLength(d->eaBlockNumber) != NvM_NvBytes(d)))
        {
            ok = FALSE;
        }
    }
    if (ok == FALSE) { return; }

    for (uint16_t i = 0U; i <= NVM_MAX_BLOCKS; i++)
    {
        s_result[i] = NVM_REQ_OK; s_ramValid[i] = FALSE; s_ramChanged[i] = FALSE;
    }
    s_cfg = ConfigPtr;
}

static Std_ReturnType NvM_Enqueue(NvM_BlockIdType id, NvM_OpType op, void *ptr)
{
    const NvM_BlockDescriptorType *d = NvM_Desc(id);
    uint8_t tail;

    if ((d == NULL) || (MemIf_GetStatus(0U) == MEMIF_UNINIT)) { return E_NOT_OK; }
    if (ptr == NULL) { ptr = d->ramBlock; }
    if (ptr == NULL) { return E_NOT_OK; }
    if ((s_result[id] == NVM_REQ_PENDING) || (s_qCount >= NVM_QUEUE_SIZE)) { return E_NOT_OK; }

    tail = (uint8_t)((s_qHead + s_qCount) % NVM_QUEUE_SIZE);
    s_queue[tail].id = id; s_queue[tail].op = op; s_queue[tail].ptr = ptr;
    s_qCount++;
    s_result[id] = NVM_REQ_PENDING;
    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void *NvM_SrcPtr)
{
    return NvM_Enqueue(BlockId, NVM_OP_WRITE, (void *)NvM_SrcPtr);
}

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void *NvM_DstPtr)
{
    return NvM_Enqueue(BlockId, NVM_OP_READ, NvM_DstPtr);
}

Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)
{
    const NvM_BlockDescriptorType *d = NvM_Desc(BlockId);
    if ((d == NULL) || (d->ramBlock == NULL)) { return E_NOT_OK; }
    if (BlockChanged != FALSE) { s_ramValid[BlockId] = TRUE; s_ramChanged[BlockId] = TRUE; }
    else                       { s_ramChanged[BlockId] = FALSE; }
    return E_OK;
}

Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType *RequestResultPtr)
{
    if ((RequestResultPtr == NULL) || (NvM_Desc(BlockId) == NULL)) { return E_NOT_OK; }
    *RequestResultPtr = s_result[BlockId];
    return E_OK;
}

void NvM_JobEndNotification(void)   { s_memifOk = TRUE;  s_memifDone = TRUE; }
void NvM_JobErrorNotification(void) { s_memifOk = FALSE; s_memifDone = TRUE; }

static void NvM_Finish(NvM_BlockIdType id, NvM_RequestResultType r)
{
    s_result[id] = r;
    s_state = NVM_ST_IDLE;
}

static void NvM_StartJob(void)
{
    const NvM_BlockDescriptorType *d;
    Std_ReturnType r;

    s_cur = s_queue[s_qHead];
    s_qHead = (uint8_t)((s_qHead + 1U) % NVM_QUEUE_SIZE);
    s_qCount--;
    d = NvM_Desc(s_cur.id);
    s_memifDone = FALSE;

    if (s_cur.op == NVM_OP_WRITE)
    {
        (void)memcpy(s_buf, s_cur.ptr, d->nvLength);
        if (d->useCrc != FALSE)
        {
            uint16_t crc = NvM_Crc16(s_buf, d->nvLength);
            s_buf[d->nvLength]      = (uint8_t)(crc >> 8);
            s_buf[d->nvLength + 1U] = (uint8_t)(crc & 0xFFU);
        }
        r = MemIf_Write(0U, d->eaBlockNumber, s_buf);
    }
    else
    {
        r = MemIf_Read(0U, d->eaBlockNumber, 0U, s_buf, NvM_NvBytes(d));
    }

    if (r != E_OK) { NvM_Finish(s_cur.id, NVM_REQ_NOT_OK); }
    else           { s_state = NVM_ST_WAIT_MEMIF; }
}

static void NvM_CompleteJob(void)
{
    const NvM_BlockDescriptorType *d = NvM_Desc(s_cur.id);
    boolean toRam = (boolean)((d->ramBlock != NULL) && (s_cur.ptr == (void *)d->ramBlock));

    if (s_memifOk == FALSE) { NvM_Finish(s_cur.id, NVM_REQ_NOT_OK); return; }

    if (s_cur.op == NVM_OP_WRITE)
    {
        if (toRam != FALSE) { s_ramValid[s_cur.id] = TRUE; s_ramChanged[s_cur.id] = FALSE; }
        NvM_Finish(s_cur.id, NVM_REQ_OK);
        return;
    }

    if ((d->useCrc != FALSE) &&
        (NvM_Crc16(s_buf, d->nvLength) !=
         (uint16_t)(((uint16_t)s_buf[d->nvLength] << 8) | s_buf[d->nvLength + 1U])))
    {
        if (d->romBlock != NULL)
        {
            (void)memcpy(s_cur.ptr, d->romBlock, d->nvLength);
            if (toRam != FALSE) { s_ramValid[s_cur.id] = TRUE; s_ramChanged[s_cur.id] = TRUE; }
            NvM_Finish(s_cur.id, NVM_REQ_RESTORED_FROM_ROM);
        }
        else
        {
            if (toRam != FALSE) { s_ramValid[s_cur.id] = FALSE; }
            NvM_Finish(s_cur.id, NVM_REQ_INTEGRITY_FAILED);
        }
        return;
    }
    (void)memcpy(s_cur.ptr, s_buf, d->nvLength);
    if (toRam != FALSE) { s_ramValid[s_cur.id] = TRUE; s_ramChanged[s_cur.id] = FALSE; }
    NvM_Finish(s_cur.id, NVM_REQ_OK);
}

void NvM_MainFunction(void)
{
    if (s_cfg == NULL) { return; }

    if (s_state == NVM_ST_IDLE)
    {
        if (s_qCount > 0U) { NvM_StartJob(); }
    }
    else if (s_memifDone != FALSE)
    {
        s_memifDone = FALSE;
        NvM_CompleteJob();
    }
    else
    {
    }
}
