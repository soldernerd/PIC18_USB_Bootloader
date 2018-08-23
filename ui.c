#include <xc.h>
#include <stdint.h>

#include "os.h"
#include "i2c.h"
#include "ui.h"
#include "internal_flash.h"

/* ****************************************************************************
 * Variables definitions
 * ****************************************************************************/

userInterfaceStatus_t userInterfaceStatus;
uint16_t system_ui_inactive_count;

/* ****************************************************************************
 * Static functions
 * ****************************************************************************/

//React if the push button has been pressed
static void _ui_encoder(void)
{
    switch(os.display_mode)
    {
        case DISPLAY_MODE_BOOTLOADER_START:
            if(os.buttonCount>0)
            {
                os.buttonCount = 0;
            }
            break;
            
        case DISPLAY_MODE_BOOTLOADER_FILE_FOUND:
            if(os.buttonCount>0)
            {
                os.bootloader_mode = BOOTLOADER_MODE_FILE_VERIFYING;
                os.display_mode = DISPLAY_MODE_BOOTLOADER_FILE_VERIFYING;
                os.buttonCount = 0;
            }
            break;
            
        case DISPLAY_MODE_BOOTLOADER_FILE_VERIFYING:
            if(os.buttonCount>0)
            {
                os.buttonCount = 0;
            }
            break;
            
        case DISPLAY_MODE_BOOTLOADER_CHECK_FAILED:
            if(os.buttonCount>0)
            {
                os.buttonCount = 0;
            }
            break;
            
        case DISPLAY_MODE_BOOTLOADER_CHECK_COMPLETE:
            if(os.buttonCount>0)
            {
                os.bootloader_mode = BOOTLOADER_MODE_PROGRAMMING;
                os.display_mode = DISPLAY_MODE_BOOTLOADER_PROGRAMMING;
                os.buttonCount = 0;
            }
            break;

        case BOOTLOADER_MODE_PROGRAMMING:
            if(os.buttonCount>0)
            {
                os.buttonCount = 0;
            }
            break; 
            
        case DISPLAY_MODE_BOOTLOADER_DONE:
            if(os.buttonCount>0)
            {
                jump_to_main_program();
            }
            break; 
    }    
}

/* ****************************************************************************
 * Public functions
 * ****************************************************************************/

//Return current UI status
userInterfaceStatus_t ui_get_status(void)
{
    return userInterfaceStatus;
}

//Start initialization of the display
void ui_init(void)
{
    system_ui_inactive_count = 0;
    //Enable high (3.3volts) board voltage
    VCC_HIGH_PIN = 1;
    userInterfaceStatus = USER_INTERFACE_STATUS_STARTUP_1;
}

//This function is periodically called to keep the user interface alive
void ui_run(void)
{
	switch(userInterfaceStatus)
	{
		case USER_INTERFACE_STATUS_OFF:
            //Wake up if button has been pressed
			if (os.buttonCount!=0)
			{
                ui_init();
                os.buttonCount = 0;
			}
			break;

		case USER_INTERFACE_STATUS_STARTUP_1:
            //Enable the user interface
            DISP_EN_PIN = 0;
            //Proceed to next state
			system_ui_inactive_count = 0;
			userInterfaceStatus = USER_INTERFACE_STATUS_STARTUP_2;
			break;
            
        case USER_INTERFACE_STATUS_STARTUP_2:
            //Put display into reset
            i2c_digipot_reset_on();
            //Proceed to next state
			system_ui_inactive_count = 0;
			userInterfaceStatus = USER_INTERFACE_STATUS_STARTUP_3;
			break;

		case USER_INTERFACE_STATUS_STARTUP_3:
			++system_ui_inactive_count;
            //Turn off reset after 32ms
			if (system_ui_inactive_count>3)
			{
				i2c_digipot_reset_off();
				system_ui_inactive_count = 0;
				userInterfaceStatus = USER_INTERFACE_STATUS_STARTUP_4;
			}
			break;

		case USER_INTERFACE_STATUS_STARTUP_4:
            //Send init sequence
            i2c_display_send_init_sequence();
            //Turn backlight on
            i2c_digipot_backlight(150); 
            //Enable rotary encoder inputs
            system_encoder_enable();
            //User interface is now up and running
            system_ui_inactive_count = 0;
            userInterfaceStatus = USER_INTERFACE_STATUS_ON;
			break;

		case USER_INTERFACE_STATUS_ON:
			if (os.encoderCount==0 && os.buttonCount==0)
			{
                if(system_ui_inactive_count<0xFFFF)
                    ++system_ui_inactive_count;
			}
			else
			{
				system_ui_inactive_count = 0;
				_ui_encoder();
			}
			break;
	}
}