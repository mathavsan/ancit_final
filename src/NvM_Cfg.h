#ifndef NVM_CFG_H
#define NVM_CFG_H
#include "NvM.h"
#define NVM_BLOCK_EX2_VAR        1U
#define NVM_BLOCK_EX2_UNALIGNED  2U
#define NVM_BLOCK_EX2_CROSS      3U
extern const NvM_ConfigType NvM_Config;
extern uint8_t NvM_RamBlock_Ex2Var[20];
#endif
