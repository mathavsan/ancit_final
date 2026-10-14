#include "eeprom_ex2.h"
#include <stddef.h>
#include <string.h>

static uint8_t s_lastChunks = 0U;

static bool EEPROM_RangeValid(uint16_t startAddress, uint16_t length)
{
    return (length != 0U) &&
           (startAddress < EEPROM_SIZE_BYTES) &&
           ((uint32_t)length <= ((uint32_t)EEPROM_SIZE_BYTES - (uint32_t)startAddress));
}

static void EEPROM_SelectDevice(uint16_t address)
{
    uint16_t dev = (uint16_t)(EEPROM_DEVICE_BASE_ADDR | ((address >> 8U) & 0x01U));
    LPI2C_DRV_MasterSetSlaveAddr(EEPROM_I2C_INSTANCE, dev, false);
}

static status_t EEPROM_WaitReady(uint16_t address)
{
    uint8_t wordAddr = (uint8_t)address;

    for (uint32_t i = 0U; i < EEPROM_ACK_POLL_MAX; i++)
    {
        if (LPI2C_DRV_MasterSendDataBlocking(EEPROM_I2C_INSTANCE, &wordAddr, 1U,
                                             true, EEPROM_I2C_TIMEOUT_MS) == STATUS_SUCCESS)
        {
            return STATUS_SUCCESS;
        }
    }
    return STATUS_TIMEOUT;
}

static status_t EEPROM_WritePage(uint16_t address, const uint8_t *data, uint8_t length)
{
    uint8_t buf[EEPROM_PAGE_SIZE + 1U];
    status_t result;

    buf[0] = (uint8_t)address;
    (void)memcpy(&buf[1], data, length);

    EEPROM_SelectDevice(address);
    result = LPI2C_DRV_MasterSendDataBlocking(EEPROM_I2C_INSTANCE, buf,(uint32_t)length + 1U, true,EEPROM_I2C_TIMEOUT_MS);
    if (result != STATUS_SUCCESS)
    {
        return result;
    }
    return EEPROM_WaitReady(address);
}

status_t EEPROM_WriteMultiPage(uint16_t startAddress, const uint8_t *data, uint16_t length)
{
    uint16_t offset = 0U;

    s_lastChunks = 0U;

    if ((data == NULL) || !EEPROM_RangeValid(startAddress, length))
    {
        return STATUS_ERROR;
    }

    while (offset < length)
    {
        uint16_t cur    = (uint16_t)(startAddress + offset);
        uint16_t room   = (uint16_t)(EEPROM_PAGE_SIZE - (cur % EEPROM_PAGE_SIZE));
        uint16_t remain = (uint16_t)(length - offset);
        uint8_t  chunk  = (uint8_t)((remain < room) ? remain : room);
        status_t result = EEPROM_WritePage(cur, &data[offset], chunk);

        if (result != STATUS_SUCCESS)
        {
            return result;
        }
        s_lastChunks++;
        offset = (uint16_t)(offset + chunk);
    }
    return STATUS_SUCCESS;
}

status_t EEPROM_Read(uint16_t startAddress, uint8_t *data, uint16_t length)
{
    uint16_t offset = 0U;

    if ((data == NULL) || !EEPROM_RangeValid(startAddress, length))
    {
        return STATUS_ERROR;
    }

    while (offset < length)
    {
        uint16_t cur      = (uint16_t)(startAddress + offset);
        uint16_t room     = (uint16_t)(256U - (cur % 256U));
        uint16_t remain   = (uint16_t)(length - offset);
        uint16_t chunk    = (remain < room) ? remain : room;
        uint8_t  wordAddr = (uint8_t)cur;
        status_t result;

        EEPROM_SelectDevice(cur);
        result = LPI2C_DRV_MasterSendDataBlocking(EEPROM_I2C_INSTANCE, &wordAddr, 1U,false, EEPROM_I2C_TIMEOUT_MS);
        if (result != STATUS_SUCCESS)
        {
            return result;
        }
        result = LPI2C_DRV_MasterReceiveDataBlocking(EEPROM_I2C_INSTANCE, &data[offset],(uint32_t)chunk, true,EEPROM_I2C_TIMEOUT_MS);
        if (result != STATUS_SUCCESS)
        {
            return result;
        }
        offset = (uint16_t)(offset + chunk);
    }
    return STATUS_SUCCESS;
}

uint8_t EEPROM_GetLastWriteChunks(void)
{
    return s_lastChunks;
}
