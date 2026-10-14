/*
 * eeprom_ex2.h
 *
 *  Created on: 12-Oct-2026
 *      Author: Shaswanth Mathav
 */
#ifndef EEPROM_EX2_H
#define EEPROM_EX2_H

#include "sdk_project_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef EEPROM_I2C_INSTANCE
#define EEPROM_I2C_INSTANCE      INST_LPI2C0
#endif
#ifndef EEPROM_DEVICE_BASE_ADDR
#define EEPROM_DEVICE_BASE_ADDR  0x50U
#endif
#ifndef EEPROM_SIZE_BYTES
#define EEPROM_SIZE_BYTES        512U
#endif
#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE         8U
#endif
#ifndef EEPROM_I2C_TIMEOUT_MS
#define EEPROM_I2C_TIMEOUT_MS    1000U
#endif
#ifndef EEPROM_ACK_POLL_MAX
#define EEPROM_ACK_POLL_MAX      200U
#endif

#if ((256U % EEPROM_PAGE_SIZE) != 0U)
#error "EEPROM_PAGE_SIZE must divide 256 so no page spans the A8 half boundary"
#endif

status_t EEPROM_WriteMultiPage(uint16_t startAddress, const uint8_t *data, uint16_t length);

status_t EEPROM_Read(uint16_t startAddress, uint8_t *data, uint16_t length);

uint8_t EEPROM_GetLastWriteChunks(void);

#endif /* EEPROM_EX2_H */
