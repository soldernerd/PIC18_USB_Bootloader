/* 
 * File:   spi.h
 * Author: luke
 *
 * Created on 26. Juli 2018, 15:18
 */

#ifndef SPI_H
#define	SPI_H

#include <stdint.h>

typedef enum 
{ 
    SPI_MODE_UNINITIALIZED,
    SPI_MODE_MASTER,
    SPI_MODE_SLAVE
} spiMode_t;

typedef enum 
{ 
    SPI_SPEED_12MHZ
} spiFrequency_t;

typedef enum
{
    SPI_POLARITY_ACTIVELOW,
    SPI_POLARITY_ACTIVEHIGH
} spiPolarity_t;

void spi_init(spiMode_t mode);
spiMode_t spi_get_mode(void);

void spi_tx(uint8_t *data, uint16_t length);
void spi_tx_tx(uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length);
void spi_tx_rx(uint8_t *command, uint16_t command_length, uint8_t *data, uint16_t data_length);

#endif	/* SPI_H */

