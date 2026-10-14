/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms. By expressly
 * accepting such terms or by downloading, installing, activating and/or otherwise
 * using the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms. If you do not agree to be
 * bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software. The production use license in
 * Section 2.3 is expressly granted for this software.
 */


#include "sdk_project_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "eeprom_ex2.h"
#include "Std_Types.h"
#include "MemIf.h"
#include "Ea.h"
#include "Ea_Cfg.h"
#include "NvM.h"
#include "NvM_Cfg.h"

#define DATA_SIZE          20U
#define WAIT_GUARD         1000U
#define UART_TIMEOUT       1000U
#define UART_LINE_SIZE     100U

#define I2C_INSTANCE       INST_LPI2C0
#define UART_INSTANCE      INST_LPUART_1

#define STORAGE_LED_PIN    0U

static lpi2c_master_state_t lpi2c0MasterState;
static uint8_t writeData[DATA_SIZE];
static uint8_t readData[DATA_SIZE];
static char txBuffer[100];
static char rxLine[UART_LINE_SIZE];

static void StorageLED_Init(void)
{
    PINS_DRV_WritePin(PTD, STORAGE_LED_PIN, 1U);
    PINS_DRV_SetMuxModeSel(PORTD, STORAGE_LED_PIN, PORT_MUX_AS_GPIO);
    PINS_DRV_SetPinDirection(PTD, STORAGE_LED_PIN, 1U);
}

static void StorageLED_Set(bool on)
{
    PINS_DRV_WritePin(PTD, STORAGE_LED_PIN, on ? 0U : 1U);
}

static void StorageLED_BlinkOnce(void)
{
    StorageLED_Set(true);
    OSIF_TimeDelay(500U);
    StorageLED_Set(false);
}

static void UART_Print(const char *text)
{
    (void)LPUART_DRV_SendDataBlocking(UART_INSTANCE,(const uint8_t *)text,(uint32_t)strlen(text),UART_TIMEOUT);
}

static void UART_PutChar(uint8_t value)
{
    (void)LPUART_DRV_SendDataBlocking(UART_INSTANCE, &value, 1U, UART_TIMEOUT);
}

static void UART_ReadLine(char *line, uint16_t capacity)
{
    uint16_t used = 0U;
    uint8_t ch;

    while (1)
    {
        status_t status = LPUART_DRV_ReceiveDataBlocking(UART_INSTANCE, &ch, 1U, UART_TIMEOUT);

        if (status != STATUS_SUCCESS)
        {
            continue;
        }

        if ((ch == (uint8_t)'\r') || (ch == (uint8_t)'\n'))
        {
            if (used == 0U)
            {
                continue;
            }
            line[used] = '\0';
            UART_Print("\r\n");
            return;
        }

        if ((ch == 8U) || (ch == 127U))
        {
            if (used != 0U)
            {
                used--;
                UART_Print("\b \b");
            }
            continue;
        }

        if ((ch >= 32U) && (ch <= 126U))
        {
            if (used < (capacity - 1U))
            {
                line[used++] = (char)ch;
                UART_PutChar(ch);
            }
            else
            {
                UART_PutChar(7U);
            }
        }
    }
}

static int8_t HexDigit(char c)
{
    if ((c >= '0') && (c <= '9')) return (int8_t)(c - '0');
    if ((c >= 'a') && (c <= 'f')) return (int8_t)(c - 'a' + 10);
    if ((c >= 'A') && (c <= 'F')) return (int8_t)(c - 'A' + 10);
    return -1;
}

static bool Parse20HexBytes(const char *line, uint8_t *data)
{
    uint16_t count = 0U;

    while (*line != '\0')
    {
        while ((*line == ' ') || (*line == '\t') || (*line == ','))
        {
            line++;
        }

        if (*line == '\0') break;
        if (count >= DATA_SIZE) return false;

        int8_t high = HexDigit(*line++);
        if (high < 0) return false;

        if (*line == '\0') return false;
        int8_t low = HexDigit(*line++);
        if (low < 0) return false;

        if ((*line != '\0') && (*line != ' ') &&
            (*line != '\t') && (*line != ','))
        {
            return false;
        }

        data[count++] = (uint8_t)((((uint8_t)high) << 4U) | (uint8_t)low);
    }

    return (count == DATA_SIZE);
}

static void PrintBytes(const uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0U; i < size; i++)
    {
        (void)sprintf(txBuffer, "%02X ", (unsigned int)data[i]);
        UART_Print(txBuffer);
    }
    UART_Print("\r\n");
}

static void PrintPages(const uint8_t *data, uint16_t size)
{
    uint16_t offset = 0U;
    uint8_t page = 1U;

    while (offset < size)
    {
        uint16_t left = (uint16_t)(size - offset);
        uint8_t length = (uint8_t)((left > EEPROM_PAGE_SIZE)?
        		         EEPROM_PAGE_SIZE : left);
                         (void)sprintf(txBuffer, "Page %u (%u bytes): ",(unsigned int)page, (unsigned int)length);
        UART_Print(txBuffer);
        PrintBytes(&data[offset], length);
        offset += length;
        page++;
    }
}

static NvM_RequestResultType WaitBlock(NvM_BlockIdType id)
{
    NvM_RequestResultType r = NVM_REQ_PENDING;
    uint32_t guard = WAIT_GUARD;

    while ((r == NVM_REQ_PENDING) && (guard > 0U))
    {
        Ea_MainFunction();
        NvM_MainFunction();
        if (NvM_GetErrorStatus(id, &r) != E_OK)
        {
            return NVM_REQ_NOT_OK;
        }
        guard--;
    }
    return (r == NVM_REQ_PENDING) ? NVM_REQ_NOT_OK : r;
}

static bool NvmWriteAndWait(NvM_BlockIdType id, const uint8_t *data)
{
    if (NvM_WriteBlock(id, data) != E_OK) { return false; }
    return (WaitBlock(id) == NVM_REQ_OK);
}

static bool NvmReadAndWait(NvM_BlockIdType id, uint8_t *data)
{
    if (NvM_ReadBlock(id, data) != E_OK) { return false; }
    return (WaitBlock(id) == NVM_REQ_OK);
}

int main(void)
{
    status_t status;

    CLOCK_DRV_Init(&clockMan1_InitConfig0);
    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
    StorageLED_Init();

    status = LPUART_DRV_Init(UART_INSTANCE, &lpUartState0,&lpuart_0_InitConfig0);
    if (status != STATUS_SUCCESS)
    {
        while (1) { }
    }

    UART_Print("\r\n============================\r\n");
    UART_Print(" EEPROM EXERCISE 2\r\n");
    UART_Print("============================\r\n");
    UART_Print("UART Initialized\r\n");

    status = LPI2C_DRV_MasterInit(I2C_INSTANCE, &lpi2c0_MasterConfig0,&lpi2c0MasterState);
    if (status != STATUS_SUCCESS)
    {
        UART_Print("I2C Initialization FAILED\r\n");
        while (1) { }
    }
    UART_Print("I2C Initialized\r\n");

    Ea_Init(&Ea_Config);
    NvM_Init(&NvM_Config);
    if (MemIf_GetStatus(0U) != MEMIF_IDLE)
    {
        UART_Print("Ea configuration INVALID\r\n");
        while (1) { }
    }

    UART_Print("\r\nPreviously Stored EEPROM Data (20 bytes):\r\n");
    if (NvmReadAndWait(NVM_BLOCK_EX2_VAR, readData))
    {
        PrintPages(readData, DATA_SIZE);
        StorageLED_BlinkOnce();
        UART_Print("EEPROM startup read successful: BLUE LED BLINKED ONCE\r\n");
    }
    else
    {
        StorageLED_Set(false);
        UART_Print("EEPROM startup read FAILED; BLUE LED OFF\r\n");
    }

    do
    {
        UART_Print("\r\nEnter 20 HEX bytes separated by spaces.\r\n");
        UART_Print("Example: 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14\r\n");
        UART_Print("Input > ");
        UART_ReadLine(rxLine, UART_LINE_SIZE);

        if (Parse20HexBytes(rxLine, writeData))
        {
            break;
        }
        UART_Print("INVALID INPUT: Enter exactly 20 two-digit HEX bytes.\r\n");
    } while (1);

    StorageLED_Set(false);
    UART_Print("\r\nWriting 20 Bytes...\r\n");
    if (!NvmWriteAndWait(NVM_BLOCK_EX2_VAR, writeData))
    {
        UART_Print("EEPROM WRITE FAILED\r\n");
        while (1) { }
    }

    (void)sprintf(txBuffer, "Page writes used: %u\r\n", (unsigned int)EEPROM_GetLastWriteChunks());
    UART_Print(txBuffer);

    UART_Print("\r\nReading EEPROM...\r\n");
    memset(readData, 0, sizeof(readData));
    if (!NvmReadAndWait(NVM_BLOCK_EX2_VAR, readData))
    {
        UART_Print("EEPROM READ FAILED\r\n");
        while (1) { }
    }

    UART_Print("\r\nWritten Data (by page):\r\n");
    PrintPages(writeData, DATA_SIZE);
    UART_Print("\r\nRead Data (by page):\r\n");
    PrintPages(readData, DATA_SIZE);

    if (memcmp(writeData, readData, DATA_SIZE) == 0)
    {
        UART_Print("\r\nVerification: PASSED\r\n");
    }
    else
    {
        UART_Print("\r\nVerification: FAILED\r\n");
    }
    StorageLED_Set(false);
    UART_Print("\r\nTEST COMPLETED\r\n");

    while (1) { }
}
