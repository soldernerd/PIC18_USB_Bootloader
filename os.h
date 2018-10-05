/* 
 * File:   os.h
 * Author: Luke
 *
 * Created on 5. September 2016, 21:17
 */

#ifndef OS_H
#define	OS_H

#include <stdint.h>
#include "i2c.h"
#include "spi.h"




/*
 * Type definitions
 */

typedef enum 
{ 
    DISPLAY_MODE_BOOTLOADER_START = 0x00,
    DISPLAY_MODE_BOOTLOADER_SEARCH = 0x10,
    DISPLAY_MODE_BOOTLOADER_FILE_FOUND = 0x20,
    DISPLAY_MODE_BOOTLOADER_FILE_VERIFYING = 0x30,
    DISPLAY_MODE_BOOTLOADER_CHECK_COMPLETE = 0x40,
    DISPLAY_MODE_BOOTLOADER_CHECK_FAILED= 0x50,
    DISPLAY_MODE_BOOTLOADER_PROGRAMMING= 0x60,
    DISPLAY_MODE_BOOTLOADER_DONE = 0x70,
    DISPLAY_MODE_BOOTLOADER_REBOOTING = 0x80,
    DISPLAY_MODE_BOOTLOADER_SUSPENDED = 0x90 
} displayMode_t;

typedef enum 
{ 
    BOOTLOADER_MODE_SEARCH = 0x10,
    BOOTLOADER_MODE_FILE_FOUND = 0x20,
    BOOTLOADER_MODE_FILE_VERIFYING = 0x30,
    BOOTLOADER_MODE_CHECK_COMPLETE = 0x40,
    BOOTLOADER_MODE_CHECK_FAILED = 0x50,
    BOOTLOADER_MODE_PROGRAMMING = 0x60,
    BOOTLOADER_MODE_DONE = 0x70,
    BOOTLOADER_MODE_SUSPENDED = 0x90
} bootloaderMode_t;

typedef enum
{
    EXTERNAL_MODE_SLAVE,
    EXTERNAL_MODE_MASTER
} externalMode_t;

typedef struct
{
    //SPI settings
    externalMode_t spiMode;
    spiFrequency_t spiFrequency;
    spiPolarity_t spiPolarity;
    //I2C settings
    externalMode_t i2cMode;
    i2cFrequency_t i2cFrequency;
    uint8_t i2cSlaveModeSlaveAddress;
    uint8_t i2cMasterModeSlaveAddress;
} communicationSettings_t;


typedef struct
{
    int8_t encoderCount;
    int8_t buttonCount;
    uint8_t timeSlot;
    uint8_t done;
    bootloaderMode_t bootloader_mode;
    displayMode_t display_mode;
    communicationSettings_t communicationSettings;
} os_t;


/*
 * Global variables
 */

os_t os;

/*
 * Function prototypes
 */


void timer_pseudo_isr(void);
void system_minimal_init(void);
void system_minimal_init_undo(void);
void system_full_init(void);
void system_delay_ms(uint8_t ms);
void system_encoder_enable(void);
void jump_to_main_program(void);
void reboot(void);

#endif	/* OS_H */

