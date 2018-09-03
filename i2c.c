

#include <xc.h>
#include "i2c.h"
#include "os.h"


//#define _XTAL_FREQ 8000000

/*
 * I2C read and write flags
 */
#define I2C_WRITE 0x00
#define I2C_READ 0x01

/*
 * I2C addresses
 */
#define I2C_DISPLAY_SLAVE_ADDRESS 0b01111000
#define I2C_DIGIPOT_SLAVE_ADDRESS 0b01011100
#define I2C_EEPROM_SLAVE_ADDRESS 0b10100000


/* ****************************************************************************
 * Variable definitions
 * ****************************************************************************/

i2cFrequency_t i2c_frequency;


/* ****************************************************************************
 * Replacements for depreciated PLIB functions
 * ****************************************************************************/

//Replaces OpenI2C1();
static void _i2c_open_1(void)
{
    SSP1STATbits.SMP = 0; //Enable slew rate control
    SSP1STATbits.CKE = 0; //Disable SMBus inputs
    SSP1ADD = 29; //400kHz at 48MHz system clock
    SSP1CON1bits.WCOL = 0; //Clear write collision bit
    SSP1CON1bits.SSPOV = 0; //Clear receive overflow bit bit
    SSP1CON1bits.SSPM = 0b1000; //I2C master mode
    SSP1CON1bits.SSPEN = 1; //Enable module
}

//Replaces IdleI2C();
static void _i2c_wait_idle(void)
{
    while(SSP1CON2bits.ACKEN | SSP1CON2bits.RCEN1 | SSP1CON2bits.PEN | SSP1CON2bits.RSEN | SSP1CON2bits.SEN | SSP1STATbits.R_W ){}
}

//Replaces StartI2C();
static void _i2c_start(void)
{
    SSP1CON2bits.SEN=1;
    while(SSP1CON2bits.SEN){}
}

//Replaces WriteI2C();
static void _i2c_send(uint8_t dat)
{
    SSP1BUF = dat;
}

//Replaces ReadI2C();
static uint8_t _i2c_get(void)
{
    SSP1CON2bits.RCEN = 1 ; //initiate I2C read
    while(SSP1CON2bits.RCEN){} //wait for read to complete
    return SSP1BUF; //return the value in the buffer
}

//Replaces StopI2C();
static void _i2c_stop(void)
{
    SSP1CON2bits.PEN = 1;
    while(SSP1CON2bits.PEN){}
}

//Replaces AckI2C();
static void _i2c_acknowledge(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
    while(SSP1CON2bits.ACKEN){}
}

//Replaces Not_i2c_acknowledge();
static void _i2c_not_acknowledge(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
    while(SSP1CON2bits.ACKEN){}
}


/* ****************************************************************************
 * General I2C functionality
 * ****************************************************************************/

void i2c_init(void)
{
    //Initialize I2C module
    _i2c_open_1();
    //Set baud rate to 100kHz
    i2c_set_frequency(I2C_FREQUENCY_100kHz);
}

void i2c_reset(void)
{
    SSP1STATbits.SMP = 0; //Enable slew rate control
    SSP1STATbits.CKE = 0; //Disable SMBus inputs
    SSP1CON1 = 0x00;
    SSP1ADD = 0x00;
}

i2cFrequency_t i2c_get_frequency(void)
{
    return i2c_frequency;
}

void i2c_set_frequency(i2cFrequency_t frequency)
{
    switch(frequency)
    {
        case I2C_FREQUENCY_100kHz:
            SSP1ADD = 119;
            break;
        case I2C_FREQUENCY_200kHz:
            SSP1ADD = 59;
            break;
        case I2C_FREQUENCY_400kHz:
            SSP1ADD = 29; 
            break;
    }
    //Save new frequency
    i2c_frequency = frequency;
}


static void _i2c_write(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(slave_address);
    _i2c_wait_idle();
    
    for(cntr=0; cntr<length; ++cntr)
    {
        _i2c_send(data[cntr]);
        _i2c_wait_idle();      
    } 
    
    _i2c_stop();
}

static void _i2c_read(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(slave_address | I2C_READ);
    _i2c_wait_idle();
    
    for(cntr=0; cntr<length-1; ++cntr)
    {
        data[cntr] = _i2c_get();
        _i2c_acknowledge();      
    } 
    data[cntr] = _i2c_get();
    _i2c_not_acknowledge();
     
    _i2c_stop();
}


/* ****************************************************************************
 * I2C Display Functionality
 * ****************************************************************************/

#define DISPLAY_RESET_PIN 0b00000100
#define DISPLAY_INSTRUCTION_REGISTER 0b00000000
#define DISPLAY_DATA_REGISTER 0b01000000
#define DISPLAY_CONTINUE_FLAG 0b10000000
#define DISPLAY_CLEAR 0b00000001
#define DISPLAY_SET_ADDRESS 0b10000000

static void _ic2_display_set_address(uint8_t address)
{
    uint8_t data_array[2];
    data_array[0] = DISPLAY_INSTRUCTION_REGISTER;
    data_array[1] = DISPLAY_SET_ADDRESS | address;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    //Set address
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, &data_array[0], 2);
}

void i2c_display_send_init_sequence(void)
{
    uint8_t init_sequence[9] = {
        0x3A, // 8bit, 4line, RE=1 
        //0x1E, // Set BS1 (1/6 bias) 
        0b00011110, //0b00011110,
        0x39, // 8bit, 4line, IS=1, RE=0 
        //0x1C, // Set BS0 (1/6 bias) + osc 
        0b00001100, //0b00011100
        0x74, //0b1110000, // Set contrast lower 4 bits 
        0b1011100, //Set ION, BON, contrast bits 4&5 
        0x6d, // Set DON and amp ratio
        0x0c, // Set display on
        0x01  // Clear display
    };
    
    //Set I2C frequency to 100kHz (display doesn't like 400kHz at startup)
    i2c_set_frequency(I2C_FREQUENCY_100kHz);

    //Write initialization sequence
    _i2c_write(I2C_DISPLAY_SLAVE_ADDRESS, &init_sequence[0], 9);
}

void i2c_display_cursor(uint8_t line, uint8_t position)
{
    uint8_t address;
    
    //Figure out display address
    line &= 0b11;
    address = line<<5;
    position &= 0b11111;
    address |= position;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    //Set address
    _ic2_display_set_address(address);
}

void i2c_display_write(char *data)
{
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_DISPLAY_SLAVE_ADDRESS);
    _i2c_wait_idle();
    _i2c_send(DISPLAY_DATA_REGISTER);
    _i2c_wait_idle();
    
    //Print entire (zero terminated) string
    while(*data)
    {
        _i2c_send(*data++);
        _i2c_wait_idle();        
    } 
    
    _i2c_stop();
}

void i2c_display_write_fixed(char *data, uint8_t length)
{
    uint8_t pos;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);

    _i2c_wait_idle();
    _i2c_start();
    _i2c_wait_idle();
    _i2c_send(I2C_DISPLAY_SLAVE_ADDRESS);
    _i2c_wait_idle();
    _i2c_send(DISPLAY_DATA_REGISTER);
    _i2c_wait_idle();
    
    //Print entire (zero terminated) string
    for(pos=0; pos<length; ++pos)
    {
        _i2c_send(data[pos]);
        _i2c_wait_idle();        
    } 
    
    _i2c_stop();    
}


/* ****************************************************************************
 * I2C Dual Digipot Functionality
 * ****************************************************************************/

#define DIGIPOT_MEMORY_ADDRESS_VOLATILE_WIPER_0 0x00
#define DIGIPOT_MEMORY_ADDRESS_VOLATILE_WIPER_1 0x10
#define DIGIPOT_MEMORY_ADDRESS_NONVOLATILE_WIPER_0 0x20
#define DIGIPOT_MEMORY_ADDRESS_NONVOLATILE_WIPER_1 0x30

#define DIGIPOT_COMMAND_WRITE 0x00
#define DIGIPOT_COMMAND_INCREMENT 0x01
#define DIGIPOT_COMMAND_DECREMENT 0x02
#define DIGIPOT_COMMAND_READ 0x03

void i2c_digipot_reset_on(void)
{
    uint8_t data_array[2];
    data_array[0] = DIGIPOT_MEMORY_ADDRESS_VOLATILE_WIPER_1 | DIGIPOT_COMMAND_WRITE;
    //data_array[0] = 0b00010000;
    data_array[1] = 0x00;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(I2C_DIGIPOT_SLAVE_ADDRESS, &data_array[0], 2);
}

void i2c_digipot_reset_off(void)
{
    uint8_t data_array[2];
    data_array[0] = DIGIPOT_MEMORY_ADDRESS_VOLATILE_WIPER_1 | DIGIPOT_COMMAND_WRITE;
    data_array[1] = 0x80;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(I2C_DIGIPOT_SLAVE_ADDRESS, &data_array[0], 2);  
}

void i2c_digipot_backlight(uint8_t level)
{
    uint8_t data_array[2];
    data_array[0] = DIGIPOT_MEMORY_ADDRESS_VOLATILE_WIPER_0 | DIGIPOT_COMMAND_WRITE;
    data_array[1] = level>>1; //There are only 128 levels on this digipot
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(I2C_DIGIPOT_SLAVE_ADDRESS, &data_array[0], 2);
}

/* ****************************************************************************
 * I2C EEPROM Functionality
 * ****************************************************************************/

 
void i2c_eeprom_writeByte(uint16_t address, uint8_t data)
{
    uint8_t slave_address;
    uint8_t dat[2];
    
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    dat[0] = address & 0xFF;
    dat[1] = data;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(slave_address, &dat[0], 2);
}

uint8_t i2c_eeprom_readByte(uint16_t address)
{
    uint8_t slave_address;
    uint8_t addr;
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    addr = address & 0xFF;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(slave_address, &addr, 1);
    _i2c_read(slave_address, &addr, 1);
    return addr;
}

void i2c_eeprom_write(uint16_t address, uint8_t *data, uint8_t length)
{
    uint8_t cntr;
    uint8_t slave_address;
    uint8_t dat[17];

    slave_address = I2C_EEPROM_SLAVE_ADDRESS | ((address&0b0000011100000000)>>7);
    dat[0] = address & 0xFF;

    length &= 0b00001111;
    for(cntr=0; cntr<length; ++cntr)
    {
        dat[cntr+1] = data[cntr];
    }
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(slave_address, &dat[0], length+1);
}

void i2c_eeprom_read(uint16_t address, uint8_t *data, uint8_t length)
{
    uint8_t slave_address;
    uint8_t addr;
    addr = address & 0xFF;
    address &= 0b0000011100000000;
    address >>= 7;
    slave_address = I2C_EEPROM_SLAVE_ADDRESS | address;
    
    //Set I2C frequency to 400kHz
    i2c_set_frequency(I2C_FREQUENCY_400kHz);
    
    _i2c_write(slave_address, &addr, 1);
    _i2c_read(slave_address, &data[0], length);
}