/* 
 * File:   application_config.h
 * Author: luke
 *
 * Created on 3. Oktober 2018, 17:30
 */

#ifndef APPLICATION_CONFIG_H
#define	APPLICATION_CONFIG_H

/*
 * A random generated signature to distinguish between bootloader and normal firmware
 */

#define BOOTLOADER_SIGNATURE 0xC125

/*
 * Firmware version
 */

#define FIRMWARE_VERSION_MAJOR 0x00
#define FIRMWARE_VERSION_MINOR 0x04
#define FIRMWARE_VERSION_FIX 0x01

/*
 * Application specific settings
 */

#define NUMBER_OF_TIMESLOTS 8
#define TIMESLOT_MASK 0b00000111

#define EEPROM_BOOTLOADER_BYTE_ADDRESS 0x100
#define BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE 0x94
#define BOOTLOADER_BYTE_FORCE_NORMAL_MODE 0x78

#endif	/* APPLICATION_CONFIG_H */

