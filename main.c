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
                    bootloader_run();
                    break;

                case 1:
                    break;

                case 6:
                    break;

                case 7:
                    if(ui_get_status()==USER_INTERFACE_STATUS_ON)
                    {
                        display_prepare(os.display_mode);
                        display_update();
                    }
                    break;
            }
            os.done = 1;
        }
    }//end while(1)
}//end main

/*******************************************************************************
 End of File
*/
