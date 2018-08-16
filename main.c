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
    //Initialize low level hardware so that we have a functional system
    //We need that in order to decide in which mode to run (bootloader or normal)
    system_init();
    
    //Check if we should jump to normal software (i.e. not run bootloader)
    if(_normal_mode())
    {
        //Start normal program. We're already done...
        #asm
            goto PROG_START;
        #endasm
    }
    
    //We will run in bootloader mode
    //Now we need to get USB running as well
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);
    USBDeviceInit();
    USBDeviceAttach();
    
    //System is now fully up and running
    //We can enter an endless loop
    while(1)
    {
        //We don't have interrupts
        //So we need to keep USB alive by regularly calling these functions
        //This should be done every 1.8ms
        USBDeviceTasks();
        APP_DeviceMSDTasks();
        APP_DeviceCustomHIDTasks();
        
        //Take care of timeslots, encoder and done flag
        //Usually, this happens in a timer ISR but we can't use interrupts here
        timer_pseudo_isr();

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
                    if(ui_get_status()==USER_INTERFACE_STATUS_ON)
                    {
                        display_prepare(os.display_mode);
                    }
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