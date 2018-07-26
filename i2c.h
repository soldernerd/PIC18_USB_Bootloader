/* 
 * File:   i2c.h
 * Author: Luke
 *
 * Created on 4. September 2016, 09:26
 */

#ifndef I2C_H
#define	I2C_H

#include <stdint.h>
#include "os.h"

/* ****************************************************************************
 * Type definitions
 * ****************************************************************************/

typedef enum
{
    I2C_FREQUENCY_100kHz,
    I2C_FREQUENCY_200kHz,
    I2C_FREQUENCY_400kHz
} i2cFrequency_t;


/* ****************************************************************************
 * General I2C functionality
 * ****************************************************************************/

void i2c_init(void);
i2cFrequency_t i2c_get_frequency(void);
void i2c_set_frequency(i2cFrequency_t frequency);


/* ****************************************************************************
 * I2C Display Functionality
 * ****************************************************************************/

void i2c_display_send_init_sequence(void);
void i2c_display_cursor(uint8_t line, uint8_t position);
void i2c_display_write(char *data);
void i2c_display_write_fixed(char *data, uint8_t length);


/* ****************************************************************************
 * I2C Dual Digipot Functionality
 * ****************************************************************************/

void i2c_digipot_reset_on(void);
void i2c_digipot_reset_off(void);
void i2c_digipot_backlight(uint8_t level);


/* ****************************************************************************
 * I2C EEPROM Functionality
 * ****************************************************************************/

void i2c_eeprom_writeByte(uint16_t address, uint8_t data);
uint8_t i2c_eeprom_readByte(uint16_t address);
void i2c_eeprom_write(uint16_t address, uint8_t *data, uint8_t length);
void i2c_eeprom_read(uint16_t address, uint8_t *data, uint8_t length);

#endif	/* I2C_H */

