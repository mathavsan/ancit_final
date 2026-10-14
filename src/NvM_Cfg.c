#include "NvM_Cfg.h"
#include "Ea_Cfg.h"
#include <stddef.h>

uint8_t NvM_RamBlock_Ex2Var[20];
static uint8_t s_ramUnaligned[20];
static const uint8_t s_romUnaligned[20] = { 0 };

static const NvM_BlockDescriptorType s_blocks[] =
{
    { 20U, EA_BLOCK_EX2_VAR,       FALSE, NvM_RamBlock_Ex2Var, NULL            },
    { 20U, EA_BLOCK_EX2_UNALIGNED, TRUE,  s_ramUnaligned,      s_romUnaligned   },
    { 20U, EA_BLOCK_EX2_CROSS,     TRUE,  NULL,                NULL            }
};

const NvM_ConfigType NvM_Config = { s_blocks, (uint16_t)(sizeof(s_blocks) / sizeof(s_blocks[0])) };
