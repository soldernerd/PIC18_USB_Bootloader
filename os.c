
#include <xc.h>
#include <stdint.h>
#include "hardware_config.h"
#include "os.h"
#include "i2c.h"
#include "ui.h"
#include "flash.h"
#include "display.h"
#include "internal_flash.h"
#include "fat16.h"


//8ms until overflow
#define TIMER0_LOAD_HIGH_48MHZ 0xD1
#define TIMER0_LOAD_LOW_48MHZ 0x20


//There are no interrupts but we still use the timer and interrupt flag
void timer_pseudo_isr(void)
{
    //Do nothing if interrupt flag is not yet set
    if(INTCONbits.T0IF==0)
    {
        return;
    }
    
    //Timer 0
    //8ms until overflow
    TMR0H = TIMER0_LOAD_HIGH_48MHZ;
    TMR0L = TIMER0_LOAD_LOW_48MHZ;
    ++os.timeSlot;
    //os.timeSlot &= TIMESLOT_MASK;
    os.done = 0;
    INTCONbits.T0IF = 0;

    //Push button
    if(INTCON3bits.INT1IF)
    {
        ++os.buttonCount;
        INTCON3bits.INT1IF = 0;
    }
    
    //Encoder
    if(INTCON3bits.INT2IF)
    {
        if(!ENCODER_B_PIN)
        {
            --os.encoderCount;
        }
        INTCON3bits.INT2IF = 0;
    }   
    if(INTCON3bits.INT3IF)
    {
        if(!ENCODER_A_PIN)
        {
            ++os.encoderCount;
        }
        INTCON3bits.INT3IF = 0;
    }
}

//Crystal is 8MHz but our clock is 48MHz so the standard __delay_ms() is too fast
void system_delay_ms(uint8_t ms)
{
    uint8_t cntr;
    for(cntr=0; cntr<ms; ++cntr)
    {
        __delay_ms(6);
    }
}


static void _system_encoder_init(void)
{
    PPSUnLock();
    RPINR1 = PUSHBUTTON_PPS;
    RPINR3 = ENCODER_A_PPS;
    RPINR2 = ENCODER_B_PPS;
    PPSUnLock()

    //We can't use interrupts, just use the flags
    INTCON2bits.INTEDG1 = 0; //0=falling
    INTCON3bits.INT1IF = 0;
    
    INTCON2bits.INTEDG2 = 1; //1=rising
    INTCON3bits.INT2IF = 0;
    
    INTCON2bits.INTEDG3 = 1; //1=rising
    INTCON3bits.INT3IF = 0;
    
    os.encoderCount = 0;
    os.buttonCount = 0;
}


void system_encoder_enable(void)
{
    //Clear interrupt flags
    INTCON3bits.INT2IF = 0;
    INTCON3bits.INT3IF = 0; 
    
    //Reset encoder count
    os.encoderCount = 0;
    
    //Enable Interrupts  
    INTCON3bits.INT2IE = 1;
    INTCON3bits.INT3IE = 1;
}



static void _system_timer0_init(void)
{
    //Clock source = Fosc/4
    T0CONbits.T0CS = 0;
    //Operate in 16bit mode
    T0CONbits.T08BIT = 0;
    //Prescaler=8
    T0CONbits.T0PS2 = 0;
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS0 = 0;
    //Use prescaler
    T0CONbits.PSA = 0;
    //8ms until overflow
    TMR0H = TIMER0_LOAD_HIGH_48MHZ;
    TMR0L = TIMER0_LOAD_LOW_48MHZ;
    //Turn timer0 on
    T0CONbits.TMR0ON = 1;
            
    //Enable timer0 interrupts
    //This is a boot loader, no interrupts allowed...
    INTCONbits.TMR0IF = 0;
    
    //Initialize timeSlot
    os.timeSlot = 0;
}

//Just configure the bare minimum needed to determine if we should jump to main program
void system_minimal_init(void)
{
    //Set board voltage high
    VCC_HIGH_TRIS = PIN_OUTPUT;
    VCC_HIGH_PIN = 1;
    
    //Configure pushbutton pin as digital input
    PUSHBUTTON_TRIS = PIN_INPUT;
    PUSHBUTTON_ANCON = PIN_DIGITAL;
    
    //Initialize I2C
    i2c_init();
    
    //Wait for voltages to have settled, also on display unit
    system_delay_ms(50);
}

//Undo everything the system_minimal_init() did)
void system_minimal_init_undo(void)
{
    //Reset and disable I2C module
    i2c_reset();
    
    //Undo pushbutton configuration
    PUSHBUTTON_TRIS = PIN_INPUT;
    PUSHBUTTON_ANCON = PIN_ANALOG;
    
    //Undo board voltage configuration
    VCC_HIGH_PIN = 0;
    VCC_HIGH_TRIS = PIN_INPUT;
}


void system_full_init(void)
{
    //Configure ports
    //Most pins will never be used in bootloader mode
    //Just make sure everything is in a safe and well-defined state
    
    //Enable display (display is active low)
    DISP_EN_TRIS = PIN_OUTPUT;
    DISP_EN_PIN = 0;
    
    //Disable USB charger
    USBCHARGER_EN_TRIS = PIN_OUTPUT;
    USBCHARGER_EN_PIN = 0;
    
    //Disable fan output
    FANOUT_TRIS = PIN_OUTPUT;
    FANOUT_PIN = 0;
    
    //Disable buck supply
    BUCK_ENABLE_TRIS = PIN_OUTPUT;
    BUCK_ENABLE_PIN = 0;
    
    //Drive buck mosfet signals low
    BUCK_LOWFET_TRIS = PIN_OUTPUT;
    BUCK_LOWFET_PIN = 0;
    BUCK_HIGHFET_TRIS = PIN_OUTPUT;
    BUCK_HIGHFET_PIN = 0;
    
    //SPI Pins
    SPI_MISO_TRIS = PIN_INPUT;
    SPI_MOSI_TRIS = PIN_OUTPUT;
    SPI_SCLK_TRIS = PIN_OUTPUT;
    SPI_SS1_TRIS = PIN_OUTPUT;
    SPI_SS1_PIN = 1;
    SPI_SS2_TRIS = PIN_INPUT;
    
    //Disable supply for power outputs
    PWROUT_ENABLE_TRIS = PIN_OUTPUT;
    PWROUT_ENABLE_PIN = 0;
    
    //Drive all power output mosfet signals high (drivers are inverting)
    PWROUT_CH1_TRIS = PIN_OUTPUT;
    PWROUT_CH1_PIN = 1;
    PWROUT_CH2_TRIS = PIN_OUTPUT;
    PWROUT_CH2_PIN = 1;
    PWROUT_CH3_TRIS = PIN_OUTPUT;
    PWROUT_CH3_PIN = 1;
    PWROUT_CH4_TRIS = PIN_OUTPUT;
    PWROUT_CH4_PIN = 1;
  
    //Configure encoder pins as digital inputs
    ENCODER_A_TRIS = PIN_INPUT;
    ENCODER_B_TRIS = PIN_INPUT;
    
    //Initialize variables
    os.bootloader_mode = BOOTLOADER_MODE_SEARCH;
    os.display_mode = DISPLAY_MODE_BOOTLOADER_START;
    
    //Initialize SPI / flash
    flash_init();
    
    //Initialize FAT16 file system on flash
    fat_init();
    
    //Set up encoder
    _system_encoder_init();
    
    //Initialize display
    ui_init();

    //Set up timer0 for timeSlots
    _system_timer0_init();
    
    //Make sure we reboot in bootloader mode if WDT reset occurs
    //i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE);
    
}

//This function must only be called after all initialization has been undone
void jump_to_main_program(void)
{
    //Start normal program. We're already done...
    #asm
        goto PROG_START;
    #endasm
}

void reboot(void)
{
    //Display message if display is on
    os.display_mode = DISPLAY_MODE_BOOTLOADER_REBOOTING;
    display_prepare(os.display_mode);
    if(ui_get_status()==USER_INTERFACE_STATUS_ON)
    {
        display_update();
    }
    
    //Just wait 2 seconds until the WDT resets the device
    while(1);
}
