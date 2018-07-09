/* 
 * File:   internal_flash.h
 * Author: luke
 *
 * Created on 6. März 2018, 19:04
 */

#ifndef INTERNAL_FLASH_H
#define	INTERNAL_FLASH_H

#include <stdint.h>

#define INTERNAL_FLASH_SIZE 0x1FFFF
#define PROG_START 0x9000

uint8_t* internalFlash_getBuffer(void);

void internalFlash_readPage(uint16_t page);
void internalFlash_erasePage(uint16_t page);
void internalFlash_writePage(uint16_t page);

uint16_t internalFlash_pageFromAddress(uint32_t address);
uint32_t internalFlash_addressFromPage(uint16_t page);
uint16_t internalFlash_addressWithinPage(uint32_t address, uint16_t page);

uint8_t internalFlash_read(uint32_t address, uint16_t data_length, uint8_t* buffer);
uint8_t internalFlash_write(uint32_t address, uint8_t data_length, uint8_t* buffer);



#endif	/* INTERNAL_FLASH_H */