
#include <stdint.h>
#include <xc.h>
#include "string.h"
#include "os.h"
#include "spi.h"

/*****************************************************************************
 * Global Variables                                                          *
 *****************************************************************************/

spiMode_t spi_mode;
spiFrequency_t spi_speed;

//Need to know:
// - SPI mode: master / slave
// - External baud rate

/*****************************************************************************
 * Utility functions                                                         *
 * These are for internal use only and are used to implement the actual      *  
 * flash functionality                                                       *
 *****************************************************************************/

static void _spi_init_pps(void)
{
    //Associate pins with MSSP module
    PPSUnLock();
    PPS_FUNCTION_SPI2_MISO_INPUT = SPI_MISO_PPS;
    SPI_MOSI_PPS = PPS_FUNCTION_SPI2_MOSI_OUTPUT;
    //Careful: Clock needs to be mapped as an output AND an input
    SPI_SCLK_PPS_OUT = PPS_FUNCTION_SPI2_SCLK_OUTPUT;
    PPS_FUNCTION_SPI2_SCLK_INPUT = SPI_SCLK_PPS_IN;
    //SPI_SS_PPS = PPS_FUNCTION_SPI2_SS_OUTPUT;
    PPSLock();
}

static void _spi_init_master(void)
{
    //Configure and enable MSSP module in master mode
    SSP2STATbits.SMP = 1; //Sample at end
    SSP2STATbits.CKE = 1; //Active to idle
    SSP2CON1bits.CKP = 0; //Idle clock is low
    SSP2CON1bits.SSPM =0b0000; //SPI master mode, Fosc/4
    SSP2CON1bits.SSPEN = 1; //Enable SPI module
}

static void _spi_init_slave(void)
{
    //Yet to be implemented
}

static void _switch_mastermode(void)
{
    //Yet to be implemented...
}

static void _switch_slavemode(void)
{
    //Yet to be implemented...
}

/*****************************************************************************
 * Public functions                                                          *
 * Only these functions are made accessible via spi.h                        *  
 *****************************************************************************/

//Set up PPS pin mappings
//Initialize the SPI
void spi_init(spiMode_t mode)
{
    //Return immediately if there is nothing to do
    if(spi_mode == mode)
    {
        return;
    }
    
    //Set up PPS if necessary
    if(spi_mode == SPI_MODE_UNINITIALIZED)
    {
        _spi_init_pps();
    }
    
    switch(mode)
    {
        case SPI_MODE_MASTER:
            _spi_init_master();
            spi_mode = mode;
            break;
           
        case SPI_MODE_SLAVE:
            _spi_init_slave();
            spi_mode = mode;
            break;
    }
}

spiMode_t spi_get_mode()
{
    return spi_mode;
}

/*****************************************************************************
 * SPI / DMA Functions                                                       *
 *****************************************************************************/

//Transmits a number of bytes via SPI using the DMA module
//Typically used to send a command
//Caution: this function does NOT check if the flash is busy or in low-power mode
void spi_tx(uint8_t *data, uint16_t length)
{
    //Make sure we are in master mode
    _switch_mastermode();
    
    //Slave Select not controlled by DMA module
    DMACON1bits.SSCON1 = 0;
    DMACON1bits.SSCON0 = 0; 
    //Do increment TX address
    DMACON1bits.TXINC = 1; 
    //Do not increment RX address
    DMACON1bits.RXINC = 0; 
    //Half duplex, transmit only
    DMACON1bits.DUPLEX1 = 0;
    DMACON1bits.DUPLEX0 = 1;
    //Disable delay interrupts
    DMACON1bits.DLYINTEN = 0; 
    //1 cycle delay only
    DMACON2bits.DLYCYC = 0b0000; 
    //Only interrupt after transfer is completed
    DMACON2bits.INTLVL = 0b0000; 
    
    //Set TX buffer address
    TXADDRH =  HIGH_BYTE((uint16_t) data);
    TXADDRL =  LOW_BYTE((uint16_t) data);
    
    //Set number of bytes to transmit
    DMABCH = HIGH_BYTE((uint16_t) (length-1));
    DMABCL = LOW_BYTE((uint16_t) (length-1));
    
    //Perform actual transfer
    SPI_SS1_PIN = 0; //Enable slave select pin 
    DMACON1bits.DMAEN = 1; //Start transfer
    while(DMACON1bits.DMAEN); //Wait for transfer to complete
    SPI_SS1_PIN = 1; //Disable slave select pin 
}

//Transmits a number of bytes via SPI using the DMA module
//Similar to _flash_spi_tx but uses two different data sources
//Typically used to send a command followed by data
//Caution: this function does NOT check if the flash is busy or in low-power mode
void spi_tx_tx(uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length)
{
    //Make sure we are in master mode
    _switch_mastermode();
    
    //Slave Select not controlled by DMA module
    DMACON1bits.SSCON1 = 0;
    DMACON1bits.SSCON0 = 0; 
    //Do increment TX address
    DMACON1bits.TXINC = 1; 
    //Do not increment RX address
    DMACON1bits.RXINC = 0; 
    //Half duplex, transmit only
    DMACON1bits.DUPLEX1 = 0;
    DMACON1bits.DUPLEX0 = 1;
    //Disable delay interrupts
    DMACON1bits.DLYINTEN = 0; 
    //1 cycle delay only
    DMACON2bits.DLYCYC = 0b0000; 
    //Only interrupt after transfer is completed
    DMACON2bits.INTLVL = 0b0000; 
    
    //Set TX buffer address for command
    TXADDRH =  HIGH_BYTE((uint16_t) command);
    TXADDRL =  LOW_BYTE((uint16_t) command);
    
    //Set number of bytes to transmit for command
    DMABCH = HIGH_BYTE((uint16_t) (command_length-1));
    DMABCL = LOW_BYTE((uint16_t) (command_length-1));
    
    //Enable slave select pin 
    SPI_SS1_PIN = 0; 
    
    //Perform transfer of command
    DMACON1bits.DMAEN = 1; //Start transfer
    while(DMACON1bits.DMAEN); //Wait for transfer to complete  
    
    //Set TX buffer address for data
    TXADDRH =  HIGH_BYTE((uint16_t) data);
    TXADDRL =  LOW_BYTE((uint16_t) data);
    
    //Set number of bytes to transmit for actual data
    DMABCH = HIGH_BYTE((uint16_t) (data_length-1));
    DMABCL = LOW_BYTE((uint16_t) (data_length-1));
    
    //Perform transfer of data
    DMACON1bits.DMAEN = 1; //Start transfer
    while(DMACON1bits.DMAEN); //Wait for transfer to complete
    
    //Disable slave select pin 
    SPI_SS1_PIN = 1;  
}

//Transmits and then receives a number of bytes via SPI using the DMA module
//Typically used to send a command in order to receive data
//Caution: this function does NOT check if the flash is busy or in low-power mode
void spi_tx_rx(uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length)
{
    //Make sure we are in master mode
    _switch_mastermode();
    
    //Slave Select not controlled by DMA module
    DMACON1bits.SSCON1 = 0;
    DMACON1bits.SSCON0 = 0; 
    //Do increment TX address
    DMACON1bits.TXINC = 1; 
    //Do not increment RX address
    DMACON1bits.RXINC = 0; 
    //Half duplex, transmit only
    DMACON1bits.DUPLEX1 = 0;
    DMACON1bits.DUPLEX0 = 1;
    //Disable delay interrupts
    DMACON1bits.DLYINTEN = 0; 
    //1 cycle delay only
    DMACON2bits.DLYCYC = 0b0000; 
    //Only interrupt after transfer is completed
    DMACON2bits.INTLVL = 0b0000; 
    
    //Set TX buffer address
    TXADDRH =  HIGH_BYTE((uint16_t) command);
    TXADDRL =  LOW_BYTE((uint16_t) command);
    
    //Set number of bytes to transmit
    DMABCH = HIGH_BYTE((uint16_t) (command_length-1));
    DMABCL = LOW_BYTE((uint16_t) (command_length-1));
    
    //Enable slave select pin 
    SPI_SS1_PIN = 0; 
    
    //Perform transfer of command
    DMACON1bits.DMAEN = 1; //Start transfer
    while(DMACON1bits.DMAEN); //Wait for transfer to complete  
    
    //Do not increment TX address
    DMACON1bits.TXINC = 0; 
    //Do increment RX address
    DMACON1bits.RXINC = 1; 
    //Half duplex, receive only
    DMACON1bits.DUPLEX1 = 0;
    DMACON1bits.DUPLEX0 = 0;
    
    //Set RX buffer address
    RXADDRH =  HIGH_BYTE((uint16_t) data);
    RXADDRL =  LOW_BYTE((uint16_t) data);
    
    //Set number of bytes to transmit
    DMABCH = HIGH_BYTE((uint16_t) (data_length-1));
    DMABCL = LOW_BYTE((uint16_t) (data_length-1));
    
    //Perform transfer of data
    DMACON1bits.DMAEN = 1; //Start transfer
    while(DMACON1bits.DMAEN); //Wait for transfer to complete
    
    //Disable slave select pin 
    SPI_SS1_PIN = 1;
}