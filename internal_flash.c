
#include "string.h"
#include <internal_flash.h>
#include <stdint.h>
#include <stdbool.h>
#include <pic18f47j53.h>
 #include <xc.h>

/******************************************************************************
 * Global Variables
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/
static void _internalFlash_unlockAndActivate(uint8_t unlockKey);




//Arbitrary, but "uncommon" value.  Used by UnlockAndActivateWR() to enhance robustness.
#define INTERNAL_FLASH_UNLOCK_KEY  (uint8_t)0xB5
//volatile unsigned char file_buffer[DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE];
#define INTERNAL_FLASH_PROGRAM_WORD        0x4003
#define INTERNAL_FLASH_ERASE               0x4042
#define INTERNAL_FLASH_PROGRAM_PAGE        0x4001
#define PTR_SIZE uint32_t
const uint8_t *FileAddress = 0;

uint8_t pageBuffer[1024];

uint8_t* internalFlash_getBuffer(void)
{
    return pageBuffer;
}

void internalFlash_readPage(uint16_t page)
{
    uint32_t address;
    address = internalFlash_addressFromPage(page);
    internalFlash_read(address, 1024, pageBuffer);
}

void internalFlash_erasePage(uint16_t page)
{
    uint32_t address;
    
    //Calculate address and set it
    address = internalFlash_addressFromPage(page);
    TBLPTR = address;
    
    //Check if address falls into permitted range
    if((address<PROG_START) || (address+1023>=INTERNAL_FLASH_SIZE)) 
    {
        return;
    }
    
    //Set WREN and FREE bits
    EECON1bits.WREN = 1;
    EECON1bits.FREE = 1;
    
    _internalFlash_unlockAndActivate(INTERNAL_FLASH_UNLOCK_KEY);
}

void internalFlash_writePage(uint16_t page)
{
    uint32_t address;
    uint8_t gie;
    uint16_t cntr;
    uint8_t i;
    uint8_t block_cntr;
    uint8_t byte_cntr;
    
    //Calculate address and set it
    address = internalFlash_addressFromPage(page);
    TBLPTR = address;
    
    //Check if address falls into permitted range
    if((address<PROG_START) || (address+1023>=INTERNAL_FLASH_SIZE)) 
    {
        return;
    }
    
    //Write 16 times 64 bytes
    cntr = 0;
    for(block_cntr=0; block_cntr<16; ++block_cntr)
    {
        
        //Write a block of the RAM bufferred data to the programming latches
        for(byte_cntr=0; byte_cntr<64; ++byte_cntr)
        {
            //Write the data
            TABLAT = pageBuffer[cntr];
            #asm
                tblwtpostinc
            #endasm
            ++cntr;
        }
        
        // Write flash memory, enable write control.
        #asm 
            tblrdpostdec 
        #endasm
        EECON1 = 0x84;
        _internalFlash_unlockAndActivate(INTERNAL_FLASH_UNLOCK_KEY);
        TBLPTR++; 
        
    }
}

//Sample
static void _internalFlash_unlockAndActivate(uint8_t UnlockKey)
{
    uint8_t InterruptEnableSave;

    //Verify the UnlockKey is the correct value, to make sure this function is 
    //getting executed intentionally, from a calling function that knew it
    //should pass the correct INTERNAL_FLASH_UNLOCK_KEY value to this function.
    //If this function got executed unintentionally, then it would be unlikely
    //that the UnlockKey variable would have been loaded with the proper value.
    if(UnlockKey != INTERNAL_FLASH_UNLOCK_KEY)
    {
        EECON1bits.WREN = 0;
        return;
    }    

    InterruptEnableSave = INTCON;
    INTCONbits.GIEH = 0;    //Disable interrupts for unlock sequence.
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;      //CPU stalls until flash erase/write is complete
    EECON1bits.WREN = 0;    //Good practice to disable any further writes now.
    //Safe to re-enable interrupts now, if they were previously enabled.
    if(InterruptEnableSave & 0x80)  //Check if GIEH was previously set
    {
        INTCONbits.GIEH = 1;
    }     
} 

//Sample
void EraseBlock(const uint8_t* dest)
{
    TBLPTR = (unsigned long)dest;

    //Erase the current block
    EECON1 = 0x94;
    _internalFlash_unlockAndActivate(INTERNAL_FLASH_UNLOCK_KEY);
}

uint8_t internalFlash_read(uint32_t address, uint16_t data_length, uint8_t* buffer)
{
    //Error check.  Make sure the host is trying to read from a legitimate
    //address, which corresponds to the MSD volume (and not some other program
    //memory region beyond the end of the MSD volume).
    if(address >= INTERNAL_FLASH_SIZE)
    {
        return false;
    }   
    
    //Read a sector worth of data, and copy it to the specified RAM "buffer".
    memcpy
    (
        (void*)buffer,
        (const void*)(address),
        data_length
    );

	return true;
}

uint16_t internalFlash_pageFromAddress(uint32_t address)
{
    address >>= 10;
    return (uint16_t) address;
}

uint32_t internalFlash_addressFromPage(uint16_t page)
{
    uint32_t address;
    address = (uint32_t) page;
    address <<= 10;
    return address;
}

uint16_t internalFlash_addressWithinPage(uint32_t address, uint16_t page)
{
    uint32_t page_start_address;
    page_start_address = internalFlash_addressFromPage(page);
    address = address - page_start_address;
    return (uint16_t) address;
}


