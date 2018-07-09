/*
 * File:   main.c
 * Author: Luke
 *
 * Created on 26. December 2016, 21:31
 */


/** INCLUDES *******************************************************/

#include "system.h"

#include "usb.h"
#include "usb_device_hid.h"
#include "usb_device_msd.h"
//
//#include "internal_flash.h"
//
#include "app_device_custom_hid.h"
#include "app_device_msd.h"

//User defined code
#include "os.h"
#include "i2c.h"
#include "display.h"
#include "ui.h"
#include "rtcc.h"
#include "flash.h"
#include "fat16.h"
#include "hex.h"
#include "bootloader.h"
#include "internal_flash.h"

static uint8_t normal_mode(void);

/*
//High priority ISR
void interrupt_high(void) __at(0x108)
{
    //asm("MOVLW 0x43"); 
    asm("goto 0x1008"); 
    return;
}

//Low priority ISR
//void interrupt low_priority interrupt_low(void)
void interrupt_low(void) __at(0x118)
{
    //asm("MOVLW 0x76"); 
    asm("goto 0x24");
    return;
}
 * */

#asm
    PSECT intcode
        goto    PROG_START + 0x08;
    PSECT intcodelo
        goto    PROG_START + 0x18;
#endasm


/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
MAIN_RETURN main(void)
{
    //This is a user defined function
    system_init();
    
    //Check if we should jump to normal software (i.e. not run bootloader)
    if(normal_mode())
    {
        //Start normal program. We're already done...
        #asm
            goto PROG_START;
        #endasm
    }
    
    SYSTEM_Initialize(SYSTEM_STATE_USB_START);

    USBDeviceInit();
    USBDeviceAttach();
    
    while(1)
    {

        #if defined(USB_POLLING)
            // Interrupt or polling method.  If using polling, must call
            // this function periodically.  This function will take care
            // of processing and responding to SETUP transactions
            // (such as during the enumeration process when you first
            // plug in).  USB hosts require that USB devices should accept
            // and process SETUP packets in a timely fashion.  Therefore,
            // when using polling, this function should be called
            // regularly (such as once every 1.8ms or faster** [see
            // inline code comments in usb_device.c for explanation when
            // "or faster" applies])  In most cases, the USBDeviceTasks()
            // function does not take very long to execute (ex: <100
            // instruction cycles) before it returns.
            USBDeviceTasks();
        #endif
        
        //Do this as often as possible
        APP_DeviceMSDTasks();
        APP_DeviceCustomHIDTasks();
        
        //Take care of timeslots, encoder and done flag
        //Usually, this happens in a timer ISR but we can't use interrupts here
        timer_pseudo_isr();

        if(!os.done)
        {
            //Run scheduled EEPROM write tasks
            i2c_eeprom_tasks();
            
            //Run user interface
            ui_run();

            //Run periodic tasks
            switch(os.timeSlot&0b00000111)
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

//This function decides if we should start in bootloader mode or not.
//Returns 0 for bootloader mode, 1 for normal program
static uint8_t normal_mode(void)
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
    else if(!PUSHBUTTON_BIT) //Button is pressed
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