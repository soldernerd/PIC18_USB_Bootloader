/*
 * File:   main.c
 * Author: Luke
 *
 * Created on 26. December 2016, 21:31
 */

/* ****************************************************************************
 * Includes
 * ****************************************************************************/

#include "system.h"
#include "usb.h"
#include "usb_device_hid.h"
#include "usb_device_msd.h"
#include "app_device_custom_hid.h"
#include "app_device_msd.h"
#include "os.h"
#include "i2c.h"
#include "display.h"
#include "ui.h"
#include "flash.h"
#include "fat16.h"
#include "hex.h"
#include "bootloader.h"
#include "internal_flash.h"
#include "api.h"


/* ****************************************************************************
 * Re-direct interrupts
 * ****************************************************************************/

#asm
    PSECT intcode
        goto    PROG_START + 0x08;
    PSECT intcodelo
        goto    PROG_START + 0x18;
#endasm


/* ****************************************************************************
 * Static function prototypes
 * ****************************************************************************/

//This function decides if we should start in bootloader mode or not.
//Returns 0 for bootloader mode, 1 for normal program
static uint8_t _normal_mode(void);


/* ****************************************************************************
 * Main function definition
 * ****************************************************************************/
void main(void)
{
    uint16_t bytes_transmitted;
    uint16_t tx_start_addr;
    
    uint8_t *rx_buffer;
    uint8_t *tx_buffer;
    
    rx_buffer = spi_get_external_rx_buffer();
    tx_buffer = spi_get_external_tx_buffer();
    tx_start_addr = (uint16_t) tx_buffer;
    
    //Initialize low level hardware so that we have a (minimally) functional system
    //We need that in order to decide in which mode to run (bootloader or normal)
    system_minimal_init();

    //Check if we should jump to normal software (i.e. not run bootloader)
    if(_normal_mode())
    {
        //Undo any initialization prior to starting main program
        system_minimal_init_undo();
        jump_to_main_program();
    }
    
    //If we are still here, we will operate in bootloader mode
    //Initialize low level hardware so that we have a (fully) functional system
    system_full_init();
    
    //We will run in bootloader mode
    //Now we need to get USB running as well
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);
    USBDeviceInit();
    USBDeviceAttach();
    
    //System is now fully up and running
    //We can enter an endless loop
    while(1)
    {
        //Clear watchdog timer
        ClrWdt();
        
        //We don't have interrupts
        //So we need to keep USB alive by regularly calling these functions
        //This should be done every 1.8ms
        USBDeviceTasks();
        APP_DeviceMSDTasks();
        APP_DeviceCustomHIDTasks();
        
        //Take care of timeslots, encoder and done flag
        //Usually, this happens in a timer ISR but we can't use interrupts here
        timer_pseudo_isr();
        
        //Check for data received via external SPI
        if(SPI_SS2_PORT)
        {
            //There is not communication in progress
            //Calculate the number of bytes transmitted
            bytes_transmitted = TXADDRH;
            bytes_transmitted <<= 8;
            bytes_transmitted |= TXADDRL;
            --bytes_transmitted;
            bytes_transmitted -= (uint16_t) spi_get_external_tx_buffer();
            
            //If data has been transmitted process that data and reset connection
            if(bytes_transmitted>0)
            {
                //Disable DMA module
                DMACON1bits.DMAEN = 0;

                //Process data
                api_prepare(rx_buffer, tx_buffer);
                api_parse(rx_buffer, (uint8_t) bytes_transmitted);
                
                //Set TX buffer address
                TXADDRH =  HIGH_BYTE((uint16_t) tx_buffer);
                TXADDRL =  LOW_BYTE((uint16_t) tx_buffer);

                //Set RX buffer address
                RXADDRH =  HIGH_BYTE((uint16_t) rx_buffer);
                RXADDRL =  LOW_BYTE((uint16_t) rx_buffer);

                //Set number of bytes to transmit
                DMABCH = HIGH_BYTE((uint16_t) (64-1));
                DMABCL = LOW_BYTE((uint16_t) (64-1));

                //Clear interrupt flag
                PIR3bits.SSP2IF = 0;
                //Re-enable DMA module
                DMACON1bits.DMAEN = 1;
            }
        }

        //Time is divided into 8 timeslots of 8ms each before starting again
        //This can be used to schedule tasks
        if(!os.done)
        {
            //Run user interface every time (i.e every 8ms)
            ui_run();
            
            //Run the following tasks only in certain time slots
            //All the actual work happens in time slots 0 to 5
            //Time slots 6 and 7 are used to update the user interface
            switch(os.timeSlot&TIMESLOT_MASK)
            {
                case 0:
                    bootloader_run(0);
                    break;

                case 1:
                    bootloader_run(1);
                    break;
                    
                case 2:
                    bootloader_run(2);
                    break;
                    
                case 3:
                    bootloader_run(3);
                    break;
                    
                case 4:
                    bootloader_run(4);
                    break;
                    
                case 5:
                    bootloader_run(5);
                    break;

                case 6:
                    display_prepare(os.display_mode);
                    break;

                case 7:
                    if(ui_get_status()==USER_INTERFACE_STATUS_ON)
                    {
                        display_update();
                    }
                    break;
            }
            os.done = 1;
        }
    }//end while(1)
}//end main


/* ****************************************************************************
 * Static function implementations
 * ****************************************************************************/

//This function decides if we should start in bootloader mode or not.
//Returns 0 for bootloader mode, 1 for normal program
static uint8_t _normal_mode(void)
{
    if(i2c_eeprom_readByte(EEPROM_BOOTLOADER_BYTE_ADDRESS)==BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE)
    {
        //Change value so we don't start in bootloader mode indefinitely
        i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, 0x00);
        //But start in bootloader mode this time
        return 0;
    }
    else if(i2c_eeprom_readByte(EEPROM_BOOTLOADER_BYTE_ADDRESS)==BOOTLOADER_BYTE_FORCE_NORMAL_MODE)
    {
        //Change value so we don't start in normal mode indefinitely
        i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, 0x00);
        //But start in normal mode this time
        return 1;
    }
    else if(!PUSHBUTTON_PIN) //Button is pressed
    {
        //Bootloader mode if pushbutton is pressed
        return 0;
    }
    else
    {
        //Normal program
        return 1;
    }
}