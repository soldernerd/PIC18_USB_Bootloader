
#include <xc.h>
#include <stdint.h>
#include "os.h"
#include "i2c.h"
#include "rtcc.h"
#include "ui.h"
#include "flash.h"
#include "fat16.h"


 
//8ms for normal load, 1ms for short load
#define TIMER0_LOAD_HIGH_8MHZ 0xF8
#define TIMER0_LOAD_LOW_8MHZ 0x30
#define TIMER0_LOAD_SHORT_HIGH_8MHZ 0xFF
#define TIMER0_LOAD_SHORT_LOW_8MHZ 0x06

//8ms for normal load, 1ms for short load
#define TIMER0_LOAD_HIGH_48MHZ 0xD1
#define TIMER0_LOAD_LOW_48MHZ 0x20
#define TIMER0_LOAD_SHORT_HIGH_48MHZ 0xFA
#define TIMER0_LOAD_SHORT_LOW_48MHZ 0x24

//23ms in any case
//#define TIMER0_LOAD_HIGH_32KHZ 0xFF
//#define TIMER0_LOAD_LOW_32KHZ 0xFC
//#define TIMER0_LOAD_SHORT_HIGH_32KHZ 0xFF
//#define TIMER0_LOAD_SHORT_LOW_32KHZ 0xFC

//1second for normal load, 250ms for short load
#define TIMER0_LOAD_HIGH_32KHZ 0xFF
#define TIMER0_LOAD_LOW_32KHZ 0x80
#define TIMER0_LOAD_SHORT_HIGH_32KHZ 0xFF
#define TIMER0_LOAD_SHORT_LOW_32KHZ 0xE0

//Structs with calibration parameters
calibration_t calibrationParameters[7];



//There are no interrupts but we still use the timer and interrupt flag
void timer_pseudo_isr(void)
{
    //Do nothing if interrupt flat is not yet set
    if(INTCONbits.T0IF==0)
    {
        return;
    }
    
    //Timer 0
    //8ms until overflow
    TMR0H = TIMER0_LOAD_HIGH_48MHZ;
    TMR0L = TIMER0_LOAD_LOW_48MHZ;
    ++os.timeSlot;
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
        if(!ENCODER_B_BIT)
        {
            --os.encoderCount;
        }
        INTCON3bits.INT2IF = 0;
    }   
    if(INTCON3bits.INT3IF)
    {
        if(!ENCODER_A_BIT)
        {
            ++os.encoderCount;
        }
        INTCON3bits.INT3IF = 0;
    }
}



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
            
    //PPSInput(PPS_INT1, PPS_RP0); //Pushbutton
    //PPSInput(PPS_INT3, PPS_RP9); //Encoder A
    //PPSInput(PPS_INT2, PPS_RP10); //Encoder B

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

void system_encoder_disable(void)
{
    //Disable Interrupts  
    INTCON3bits.INT2IE = 0;
    INTCON3bits.INT3IE = 0;
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
    //INTCONbits.TMR0IE = 1;
    //INTCONbits.GIE = 1;
    
    //Initialize timeSlot
    os.timeSlot = 0;
}

void system_init(void)
{
    char filename[9] = "TEST    ";
    char extension[4] = "TXT";
    char filename2[9] = "NEWFILE ";
    char extension2[4] = "TXT";
    char appendtext[104] = "Just appended this text! Now this is super-long. it goes on and on and on ... but finally it ends HERE.";
    uint8_t file_number;
    uint8_t temp[50];
    
    //Configure ports
    VCC_HIGH_TRIS = PIN_OUTPUT;
    DISP_EN_TRIS = PIN_OUTPUT;
    
    USBCHARGER_EN_TRIS = PIN_OUTPUT;
    USBCHARGER_EN_PIN = 0;
    
    //Fan output
    FANOUT_PIN = 0;
    FANOUT_TRIS = PIN_OUTPUT;
    
    //Buck
    BUCK_ENABLE_PIN = 0;
    BUCK_ENABLE_TRIS = PIN_OUTPUT;
    BUCK_LOWFET_PIN = 0;
    BUCK_LOWFET_TRIS = PIN_OUTPUT;
    BUCK_HIGHFET_PIN = 0;
    BUCK_HIGHFET_TRIS = PIN_OUTPUT;
    
    //SPI Pins
    SPI_MISO_TRIS = PIN_INPUT;
    SPI_MOSI_TRIS = PIN_OUTPUT;
    SPI_SCLK_TRIS = PIN_OUTPUT;
    SPI_SS1_TRIS = PIN_OUTPUT;
    SPI_SS1_PIN = 1;
    
    //Pins for temperature sensing
    VOLTAGE_REFERENCE_TRIS = PIN_INPUT;
    VOLTAGE_REFERENCE_ANCON = PIN_ANALOG;
    TEMPERATURE1_TRIS = PIN_INPUT;
    TEMPERATURE1_ANCON = PIN_ANALOG;
    TEMPERATURE2_TRIS = PIN_INPUT;
    TEMPERATURE2_ANCON = PIN_ANALOG;
    TEMPERATURE3_TRIS = PIN_INPUT;
    TEMPERATURE3_ANCON = PIN_ANALOG;
    
    //Pins for power outputs
    PWROUT_ENABLE_PIN = 0;
    PWROUT_ENABLE_TRIS = 0;
    PWROUT_CH1_PIN = 1;
    PWROUT_CH1_TRIS = 0;
    PWROUT_CH2_PIN = 1;
    PWROUT_CH2_TRIS = 0;
    PWROUT_CH3_PIN = 1;
    PWROUT_CH3_TRIS = 0;
    PWROUT_CH4_PIN = 1;
    PWROUT_CH4_TRIS = 0;
    
    TRISAbits.TRISA0 = 1; //Push button 
    ANCON0bits.PCFG0 = 1; //Pushb button as digital input
    TRISBbits.TRISB6 = 1; //Encoder A
    TRISBbits.TRISB7 = 1; //Encoder B
    
    //Initialize variables
    os.bootloader_mode = BOOTLOADER_MODE_START;
    os.display_mode = DISPLAY_MODE_BOOTLOADER_START;

    //Configure timer 1
    //Clear interrupt flag
    PIR1bits.TMR1IF = 0; 
    //Load timer (1024 cycles until overflow)
    TMR1H = 0xFC; 
    TMR1L = 0x00;
    //Turn timer on
    T1CONbits.TMR1ON = 1;
    //Enable secondary (32kHz) oscillator
    T1CONbits.T1OSCEN = 1; 
    //Wait for secondary oscillator to have stabilized
    while(!PIR1bits.TMR1IF){};
    //Turn timer off
    T1CONbits.TMR1ON = 0;
    
    //Initialize I2C
    i2c_init();
    //Initialize display
    ui_init();
    
    //Initialize Real Time Clock
    rtcc_init();
    
    //Load calibration parameters
    i2c_eeprom_read_calibration();

    //Set up timer0 for timeSlots
    _system_timer0_init();
    
    //Set up encoder with interrupts
    _system_encoder_init();
    
    flash_init();
    fat_init();
    
    //Read from first file
    //file_number = fat_find_file(filename, extension);
    //fat_read_from_file(file_number, 5, 10, temp);
    
    //Append to second file
    //file_number = fat_find_file(filename2, extension2);
    //fat_append_to_file(file_number, 10, temp);
    
    //fat_rename_file(file_number, filename2, extension2);
}
