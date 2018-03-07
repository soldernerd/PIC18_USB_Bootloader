
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

void internalFlash_readPage(uint16_t page)
{
    
}

void internalFlash_erasePage(uint16_t page)
{
    uint32_t address;
    
    //Calculate address and set it
    address = page;
    address <<= 10;
    TBLPTR = address;

    //Erase the current block
    //EECON1 = 0x94;
    
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
    
    //generate some dummy data
    for(cntr=0; cntr<1024; ++cntr)
    {
        pageBuffer[cntr] = (cntr & 0xFF);
    }
    
    //Calculate address and set it
    address = page;
    address <<= 10;
    //address--;
    TBLPTR = address;
    
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

uint8_t internalFlash_read(uint32_t address, uint8_t data_length, uint8_t* buffer)
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
}//end SectorRead


uint8_t internalFlash_write(uint32_t address, uint8_t data_length, uint8_t* buffer)
{
    const uint8_t* dest;
    bool foundDifference;
    uint16_t blockCounter;
    uint16_t sectorCounter;
    uint8_t* p;

    //First, error check the resulting address, to make sure the MSD host isn't trying 
    //to erase/program illegal LBAs that are not part of the designated MSD volume space.
    if(address >= INTERNAL_FLASH_SIZE)
    {
        return false;
    }  

    //Compute pointer to location in flash memory we should modify
    dest = (const uint8_t*)(address);

    sectorCounter = 0;
    //Loop that actually does the flash programming, until all of the
    //intended sector data has been programmed
    
//    while(sectorCounter < FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
//    {
//        //First, read the contents of flash to see if they already match what the
//        //host is trying to write.  If every byte already matches perfectly,
//        //we can save flash endurance by not actually performing the reprogramming
//        //operation.
//        foundDifference = false;
//        for(blockCounter = 0; blockCounter < DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE; blockCounter++)
//        {
//            if(dest[sectorCounter] != buffer[sectorCounter])
//            {
//                foundDifference = true;
//                sectorCounter -= blockCounter;
//                break;
//            }
//            sectorCounter++;
//        }
//
//        //If the existing flash memory contents are different from what is waiting
//        //in the RAM buffer to be programmed.  We will need to do some flash reprogramming.
//        if(foundDifference == true)
//        {
//            uint8_t i;
//            PTR_SIZE address;
//
//            //The hardware erases more flash memory than the amount of a sector that
//            //we are programming.  Therefore, we will have to use a three step process:
//            //1. Read out the flash memory contents that are part of the erase page (but we don't need to modify)
//            //   and save it temporarily to a RAM buffer.
//            //2. Erase the flash memory page (which blows away multiple sectors worth of data in flash)
//            //3. Reprogram both the intended sector data, and the unmodified flash data that we didn't want to
//            //   modify, but had to temporarily erase (since it was sharing the erase page with our intended write location).
//
//            //Compute a pointer to the first byte on the erase page of interest
//            address = ((PTR_SIZE)(dest + sectorCounter) & ~((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));
//
//            //Read out the entire contents of the flash memory erase page of interest and save to RAM.
//            memcpy
//            (
//                (void*)file_buffer,
//                (const void*)address,
//                DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE
//            );
//
//            //Now erase the flash memory page
//            EraseBlock((const uint8_t*)address);
//
//            //Compute a pointer into the RAM buffer with the erased flash contents,
//            //to where we want to replace the existing data with the new data from the host.
//            address = ((PTR_SIZE)(dest + sectorCounter) & ((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));
//
//            //Overwrite part of the erased page RAM buffer with the new data being
//            //written from the host
//            memcpy
//            (
//                (void*)(&file_buffer[address]),
//                (void*)buffer,
//                FILEIO_CONFIG_MEDIA_SECTOR_SIZE
//            );
//
//            //Compute the number of write blocks that are in the erase page.
//            i = DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE / DRV_FILEIO_INTERNAL_FLASH_CONFIG_WRITE_BLOCK_SIZE;
//
//            #if defined(__XC8) || defined(__18CXX)
//                p = (uint8_t*)&file_buffer[0];
//                TBLPTR = ((PTR_SIZE)(dest + sectorCounter) & ~((uint32_t)DRV_FILEIO_INTERNAL_FLASH_CONFIG_ERASE_BLOCK_SIZE - 1));
//            #endif
//
//            //Commit each write block worth of data to the flash memory, one block at a time
//            while(i-- > 0)
//            {
//                //Write a block of the RAM bufferred data to the programming latches
//                for(blockCounter = 0; blockCounter < DRV_FILEIO_INTERNAL_FLASH_CONFIG_WRITE_BLOCK_SIZE; blockCounter++)
//                {
//                    //Write the data
//                    #if defined(__XC8)
//                        TABLAT = *p++;
//                        #asm
//                            tblwtpostinc
//                        #endasm
//                        sectorCounter++;
//                    #elif defined(__18CXX)
//                        TABLAT = *p++;
//                        _asm tblwtpostinc _endasm
//                        sectorCounter++;
//                    #endif
//
//                    #if defined(__C32__)
//                        NVMWriteWord((uint32_t*)KVA_TO_PA(FileAddress), *((uint32_t*)&file_buffer[sectorCounter]));
//                        FileAddress += 4;
//                        sectorCounter += 4;
//                    #endif
//                }
//
//                //Now commit/write the block of data from the programming latches into the flash memory
//                #if defined(__XC8)
//                    // Start the write process: for PIC18, first need to reposition tblptr back into memory block that we want to write to.
//                    #asm 
//                        tblrdpostdec 
//                    #endasm
//
//                    // Write flash memory, enable write control.
//                    EECON1 = 0x84;
//                    UnlockAndActivate(NVM_UNLOCK_KEY);
//                    TBLPTR++;                    
//                #elif defined(__18CXX)
//                    // Start the write process: for PIC18, first need to reposition tblptr back into memory block that we want to write to.
//                     _asm tblrdpostdec _endasm
//
//                    // Write flash memory, enable write control.
//                    EECON1 = 0x84;
//                    UnlockAndActivate(NVM_UNLOCK_KEY);
//                    TBLPTR++;
//                #endif
//            }//while(i-- > 0)
//        }//if(foundDifference == true)
//    }//while(sectorCounter < FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
    return true;
} //end SectorWrite



