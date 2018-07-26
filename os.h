/* 
 * File:   os.h
 * Author: Luke
 *
 * Created on 5. September 2016, 21:17
 */

#ifndef OS_H
#define	OS_H

#include <stdint.h>
#include "spi.h"
#include "i2c.h"

/*
 * Define your board revision here
 */

#define BOARD_REVISION_F

/******************************************************************************
 * Nothing should need to be changed below here                               *
 ******************************************************************************/

/*
 * A random generated signature to distinguish between bootloader and normal firmware
 */

#define BOOTLOADER_SIGNATURE 0xC125

/*
 * Firmware version
 */

#define FIRMWARE_VERSION_MAJOR 0x00
#define FIRMWARE_VERSION_MINOR 0x01
#define FIRMWARE_VERSION_FIX 0x00


/*
 * Replacement for some depreciated PLIB macros
 */

#define  PPSUnLock()    {EECON2 = 0b01010101; EECON2 = 0b10101010; PPSCONbits.IOLOCK = 0;}
#define  PPSLock() 		{EECON2 = 0b01010101; EECON2 = 0b10101010; PPSCONbits.IOLOCK = 1;}

/*
 * General definitions for setting pin functions
 */

#define PIN_INPUT           1
#define PIN_OUTPUT          0
#define PIN_DIGITAL         1
#define PIN_ANALOG          0

/*
 * Physical properties, pinout
 */

#define _XTAL_FREQ 8000000

#define PPS_FUNCTION_SPI2_MISO_INPUT RPINR21
#define PPS_FUNCTION_SPI2_MOSI_OUTPUT 10
#define PPS_FUNCTION_SPI2_SCLK_OUTPUT 11
#define PPS_FUNCTION_SPI2_SCLK_INPUT RPINR22
#define PPS_FUNCTION_SPI2_SS_OUTPUT 12

#define DISP_EN_TRIS TRISDbits.TRISD0
#define DISP_EN_PIN LATDbits.LD0

#define VCC_HIGH_TRIS TRISCbits.TRISC2
#define VCC_HIGH_PIN LATCbits.LC2

#define BUCK_ENABLE_TRIS TRISBbits.TRISB1
#define BUCK_ENABLE_PIN LATBbits.LB1
#define BUCK_LOWFET_TRIS TRISBbits.TRISB2
#define BUCK_LOWFET_PIN LATBbits.LB2
#define BUCK_LOWFET_PPS RPOR6
#define BUCK_HIGHFET_TRIS TRISBbits.TRISB3
#define BUCK_HIGHFET_PIN LATBbits.LB3
#define BUCK_HIGHFET_PPS RPOR5

#define PWROUT_ENABLE_TRIS TRISCbits.TRISC7
#define PWROUT_ENABLE_PIN LATCbits.LC7
#define PWROUT_CH1_TRIS TRISEbits.TRISE2
#define PWROUT_CH1_PIN LATEbits.LE2
#define PWROUT_CH2_TRIS TRISEbits.TRISE1
#define PWROUT_CH2_PIN LATEbits.LE1
#define PWROUT_CH3_TRIS TRISEbits.TRISE0
#define PWROUT_CH3_PIN LATEbits.LE0
#define PWROUT_CH4_TRIS TRISAbits.TRISA5
#define PWROUT_CH4_PIN LATAbits.LA5

#define USBCHARGER_EN_TRIS TRISDbits.TRISD3
#define USBCHARGER_EN_PIN LATDbits.LD3

#define SPI_MISO_TRIS TRISDbits.TRISD6
#define SPI_MISO_PORT PORTDbits.RD6
#define SPI_MISO_PPS 23
#define SPI_MOSI_TRIS TRISDbits.TRISD4
#define SPI_MOSI_PORT LATDbits.LD4
#define SPI_MOSI_PPS RPOR21
#define SPI_SCLK_TRIS TRISDbits.TRISD5
#define SPI_SCLK_PORT LATDbits.LD5
#define SPI_SCLK_PPS_OUT RPOR22
#define SPI_SCLK_PPS_IN 22
#define SPI_SS1_TRIS TRISDbits.TRISD7
#define SPI_SS1_PIN LATDbits.LD7
#define SPI_SS1_PPS RPOR24

#ifdef BOARD_REVISION_E
    #define SPI_SS2_TRIS TRISDbits.TRISD1
    #define SPI_SS2_PIN LATDbits.LD1
    #define FANOUT_TRIS TRISDbits.TRISD2
    #define FANOUT_PIN LATDbits.LD2
#endif /* BOARD_REVISION_E */

#ifdef BOARD_REVISION_F
    #define SPI_SS2_TRIS TRISDbits.TRISD2
    #define SPI_SS2_PIN LATDbits.LD2
    #define SPI_SS2_PPS RPOR19
    #define FANOUT_TRIS TRISDbits.TRISD1
    #define FANOUT_PIN LATDbits.LD1
#endif /* BOARD_REVISION_F */

#define PUSHBUTTON_TRIS TRISAbits.TRISA0
#define PUSHBUTTON_PIN PORTAbits.RA0
#define PUSHBUTTON_ANCON ANCON0bits.PCFG0
#define PUSHBUTTON_PPS 0
#define ENCODER_A_TRIS TRISBbits.TRISB7
#define ENCODER_A_PIN PORTBbits.RB7
#define ENCODER_A_PPS 9
#define ENCODER_B_TRIS TRISBbits.TRISB6
#define ENCODER_B_PIN PORTBbits.RB6
#define ENCODER_B_PPS 10

/*
 * Application specific settings
 */

#define NUMBER_OF_TIMESLOTS 8
#define TIMESLOT_MASK 0b00000111

#define EEPROM_BOOTLOADER_BYTE_ADDRESS 0x100
#define BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE 0x94
#define BOOTLOADER_BYTE_FORCE_NORMAL_MODE 0x78


/*
 * Type definitions
 */

typedef enum 
{ 
    DISPLAY_MODE_BOOTLOADER_START = 0x00,
    DISPLAY_MODE_BOOTLOADER_FILE_FOUND = 0x10,
    DISPLAY_MODE_BOOTLOADER_FILE_VERIFYING = 0x20,
    DISPLAY_MODE_BOOTLOADER_CHECK_COMPLETE = 0x30,
    DISPLAY_MODE_BOOTLOADER_CHECK_FAILED= 0x40,
    DISPLAY_MODE_BOOTLOADER_PROGRAMMING= 0x50,
    DISPLAY_MODE_BOOTLOADER_DONE = 0x60      
} displayMode_t;

typedef enum 
{ 
    BOOTLOADER_MODE_START = 0x00,
    BOOTLOADER_MODE_FILE_FOUND = 0x10,
    BOOTLOADER_MODE_FILE_VERIFYING = 0x20,
    BOOTLOADER_MODE_CHECK_COMPLETE = 0x30,
    BOOTLOADER_MODE_CHECK_FAILED = 0x40,
    BOOTLOADER_MODE_PROGRAMMING = 0x50,
    BOOTLOADER_MODE_DONE = 0x60
} bootloaderMode_t;

typedef struct
{
    volatile int8_t encoderCount;
    volatile int8_t buttonCount;
    volatile uint8_t timeSlot;
    volatile uint8_t done;
    bootloaderMode_t bootloader_mode;
    displayMode_t display_mode;
} os_t;

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


/*
 * Global variables
 */

os_t os;

communicationSettings_t communicationSettings;

/*
 * Function prototypes
 */


void timer_pseudo_isr(void);
void system_init(void);
void system_delay_ms(uint8_t ms);
void system_encoder_enable(void);

#endif	/* OS_H */

