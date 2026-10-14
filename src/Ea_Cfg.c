#include "Ea_Cfg.h"

static const Ea_BlockConfigType s_eaBlocks[] =
{
    { EA_BLOCK_EX2_VAR,       0x000U, 20U },
    { EA_BLOCK_EX2_UNALIGNED, 0x045U, 22U },
    { EA_BLOCK_EX2_CROSS,     0x0F8U, 22U }
};

const Ea_ConfigType Ea_Config = { s_eaBlocks, (uint16_t)(sizeof(s_eaBlocks) / sizeof(s_eaBlocks[0])) };
