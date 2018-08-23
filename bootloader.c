#include <xc.h>
#include "os.h"
#include "bootloader.h"
#include "fat16.h"
#include "hex.h"
#include "internal_flash.h"

#define BOOTLOADER_CHARACTER_BUFFER_SIZE 50
#define BOOTLOADER_NUMBER_OF_RECORDS_PER_CALL 1
#define BOOTLOADER_MINIMUM_ADDRESS_ALLOWED 0x09000
#define BOOTLOADER_MAXIMUM_ADDRESS_ALLOWED 0x1FFF7
#define BOOTLOADER_CONFIGURATIONBITS_ADDRESS_MIN 0x1FFF8
#define BOOTLOADER_CONFIGURATIONBITS_ADDRESS_MAX 0x1FFFF

uint8_t file_number = 0xFF;
uint8_t file_buffer[BOOTLOADER_CHARACTER_BUFFER_SIZE];
uint32_t file_minimum_address;
uint32_t file_maximum_address;
uint16_t hex_file_entries = 0;
uint16_t total_hex_file_entries = 0;
uint32_t hex_file_offset = 0;
uint32_t hex_file_size = 0;
HexFileEntry_t hex_file_entry;
ShortRecordError_t last_error;
uint32_t extended_linear_address;
uint8_t start_from_byte_next = 0;

uint16_t flash_pages_written;

typedef enum
{
    FILE_CHECK_STATUS_IN_PROGRESS,
    FILE_CHECK_STATUS_COMPLETED
} fileCheckStatus_t;

typedef enum
{
    COMPARE_RESULT_DATA_MATCHES,
    COMPARE_RESULT_DATA_DOES_NOT_MATCH
} compareResult_t;

typedef enum
{
    ADDRESS_CHECK_RESULT_OK = 0x00,       
    ADDRESS_CHECK_RESULT_CONFIGURATION_BITS = 0x01,
    ADDRESS_CHECK_RESULT_ERROR = 0xFF
} addressCheckResult_t;

static addressCheckResult_t _bootloader_check_address(uint32_t address,  uint8_t dataLength);
  
static void _bootloader_find_file(void);
static void _bootloader_verify_file(void);
static void _bootloader_program(void);

static compareResult_t _bootloader_verify_program_memory(uint32_t addressOffset, HexFileEntry_t *hexFileEntry);




void bootloader_run(uint8_t timeslot)
{
    switch(os.bootloader_mode)
    {
        case BOOTLOADER_MODE_START:
           if(timeslot==0)
            { 
                _bootloader_find_file();
            }
           break;
           
        case BOOTLOADER_MODE_FILE_FOUND:
            if(timeslot==0)
            { 
                _bootloader_find_file();
            }
           break;
            
        case BOOTLOADER_MODE_FILE_VERIFYING:
            _bootloader_verify_file();
            break;
            
        case BOOTLOADER_MODE_CHECK_COMPLETE:
            break;
            
        case BOOTLOADER_MODE_CHECK_FAILED:
            break;
            
        case BOOTLOADER_MODE_PROGRAMMING:
            if(timeslot==0)
            {
                _bootloader_program();
            }
            break;

        case BOOTLOADER_MODE_DONE:
            break;
    }
}

static addressCheckResult_t _bootloader_check_address(uint32_t address, uint8_t dataLength)
{   
    addressCheckResult_t byte_status;
    addressCheckResult_t worst_case;
    uint8_t cntr;
    
    worst_case = ADDRESS_CHECK_RESULT_OK;
    
    //Check needs to pass for every byte in data range
    for(cntr=0; cntr<dataLength; ++cntr)
    {
        //Allowed range: At interrupt vectors and slightly beyond
        if(((address+cntr)>=BOOTLOADER_MINIMUM_ADDRESS_ALLOWED) && ((address+cntr)<=BOOTLOADER_MAXIMUM_ADDRESS_ALLOWED))
        {
            byte_status = ADDRESS_CHECK_RESULT_OK;
        }
        //Configuration bits. Don't program but allowed in file
        //These will be checked to ensure they match
        else if(((address+cntr)>=BOOTLOADER_CONFIGURATIONBITS_ADDRESS_MIN) && ((address+cntr)<=BOOTLOADER_CONFIGURATIONBITS_ADDRESS_MAX))
        {
            byte_status = ADDRESS_CHECK_RESULT_CONFIGURATION_BITS;
        }
        else
        {
            byte_status = ADDRESS_CHECK_RESULT_ERROR;
        }
        
        //Immediately return if a byte falls outside the permitted range
        if(byte_status == ADDRESS_CHECK_RESULT_ERROR)
        {
            return ADDRESS_CHECK_RESULT_ERROR;
        }
        
        //Update worst case
        if(byte_status > worst_case)
        {
            worst_case = byte_status;
        }
    }
    
    return worst_case;
}

static void _bootloader_find_file(void)
{
    //Try to locate file
    file_number = fat_find_file(bootloader_filename, bootloader_extension);
    
    //File has been found
    if(file_number!=0xFF)
    {
        //Find file size
        hex_file_size = fat_get_file_size(file_number);
        //Let user decide if the file is to be used
        hex_file_entries = 0;
        hex_file_offset = 0;
        file_minimum_address = 0xFFFFFFFF;
        file_maximum_address = 0x00000000;
        extended_linear_address = 0x00000000;
        os.bootloader_mode = BOOTLOADER_MODE_FILE_FOUND;
        os.display_mode = DISPLAY_MODE_BOOTLOADER_FILE_FOUND;
    }
    //File has not been found
    else
    {
        //Reset hex file size
        hex_file_size = 0;
        //Return to (or remain in) startup state
        os.bootloader_mode = BOOTLOADER_MODE_START;
        os.display_mode = DISPLAY_MODE_BOOTLOADER_START;
    }
}

static void _bootloader_verify_file(void)
{
    uint8_t rec_counter;
    uint32_t return_value = 0;
    uint32_t address32;
    
    //Find file size
    hex_file_size = fat_get_file_size(file_number);
    
    //Loop through a pre-defined number of records
    for(rec_counter=0; rec_counter<BOOTLOADER_NUMBER_OF_RECORDS_PER_CALL; ++rec_counter)
    {
        //Read an entry
        if((hex_file_size-hex_file_offset)>=BOOTLOADER_CHARACTER_BUFFER_SIZE)
            fat_read_from_file(file_number, hex_file_offset, BOOTLOADER_CHARACTER_BUFFER_SIZE, file_buffer);
        else
            fat_read_from_file(file_number, hex_file_offset, (hex_file_size-hex_file_offset), file_buffer);
        //Check that entry
        return_value = parseHexFileEntry(file_buffer, 0, &hex_file_entry);
        
        //Keep track of number of records
        ++hex_file_entries;
        
        //Keep track of extended linear address
        if(hex_file_entry.recordType==RecordTypeExtendedLinearAddress)
        {
            extended_linear_address = hex_file_entry.data[0];
            extended_linear_address <<= 8;
            extended_linear_address |= hex_file_entry.data[1];
            extended_linear_address <<= 8;
            extended_linear_address <<= 8;
        }
        
        //Keep track of address range
        if(hex_file_entry.recordType==RecordTypeData)
        {
            //Calculate 32-bit address
            address32 = extended_linear_address + hex_file_entry.address;
            
            //Update ranges
            if(address32<file_minimum_address)
            {
                file_minimum_address = address32;
            }
            if(address32>file_maximum_address)
            {
                file_maximum_address = address32;
            }
            
            //Check if address is valid
            if(_bootloader_check_address(address32, hex_file_entry.dataLength) == ADDRESS_CHECK_RESULT_ERROR)
            {
                //Address outside allowed range
                last_error = ShortRecordErrorAddressRange;
                //Change mode
                os.bootloader_mode = BOOTLOADER_MODE_CHECK_FAILED;
                os.display_mode = DISPLAY_MODE_BOOTLOADER_CHECK_FAILED;
                break;
            }
        }

        if(return_value==0)
        {
            //Last record has been reached without an error
            //Prepare variables for programming and change mode
            total_hex_file_entries = hex_file_entries;
            hex_file_entries = 0;
            hex_file_offset = 0;
            extended_linear_address = 0;
            flash_pages_written = 0;
            start_from_byte_next = 0;
            
            os.bootloader_mode = BOOTLOADER_MODE_CHECK_COMPLETE;
            os.display_mode = DISPLAY_MODE_BOOTLOADER_CHECK_COMPLETE;
            break;
        }
        else if(return_value>0xFFFFFFF0)
        {
            //There is some error - save it
            last_error = return_value & 0xF;
            //Change mode
            os.bootloader_mode = BOOTLOADER_MODE_CHECK_FAILED;
            os.display_mode = DISPLAY_MODE_BOOTLOADER_CHECK_FAILED;
            break;
        }
        else
        {
            //No error but end of file has not yet been reached
            hex_file_offset += return_value;
        } 
    }
}

static void _bootloader_program(void)
{
    uint16_t cntr;
    uint8_t* buffer;
    uint16_t entry_page;
    uint16_t page_to_write = 0;
    uint8_t start_from_byte;
    uint32_t address32;
    uint32_t return_value = 0;
    uint16_t address_within_page;

    //Loop through records
    //for(rec_counter=0; rec_counter<5; ++rec_counter)
    while(1)
    {
        //Read an entry
        if((hex_file_size-hex_file_offset)>=BOOTLOADER_CHARACTER_BUFFER_SIZE)
        {
            fat_read_from_file(file_number, hex_file_offset, BOOTLOADER_CHARACTER_BUFFER_SIZE, file_buffer);
        }
        else
        {
            fat_read_from_file(file_number, hex_file_offset, (hex_file_size-hex_file_offset), file_buffer);
        }
        
        //Parse that entry
        return_value = parseHexFileEntry(file_buffer, 0, &hex_file_entry);
        
        //Keep track of number of records
        ++hex_file_entries;
        
        //Handle return value
        if(return_value>RecordErrorNoError)
        {
            //An error has occurred
            os.bootloader_mode = BOOTLOADER_MODE_CHECK_FAILED;
            os.display_mode = DISPLAY_MODE_BOOTLOADER_CHECK_FAILED;
            return;
        }
//        else if(return_value==0)
//        {
//            //Last record has been reached without an error
//            //Don't return, just break. We still need to write that data to flash
//            break;
//        }
        else
        {
            //No error and end of file has not yet been reached
            hex_file_offset += return_value;
        } 
        
        switch(hex_file_entry.recordType)
        {
            //Extended linear address
            case RecordTypeExtendedLinearAddress:
                //Keep track of extended linear address
                //After that we are done with this record
                extended_linear_address = hex_file_entry.data[0];
                extended_linear_address <<= 8;
                extended_linear_address |= hex_file_entry.data[1];
                extended_linear_address <<= 8;
                extended_linear_address <<= 8;
                continue;
                break;
                
            //Data
            case RecordTypeData:                
                //Recall where we should start
                start_from_byte = start_from_byte_next;
                //Remember where to start from next time (default, may be changed below)
                start_from_byte_next = 0;
                
                //Calculate 32-bit address and corresponding page
                address32 = extended_linear_address + hex_file_entry.address;
                entry_page = internalFlash_pageFromAddress(address32 + start_from_byte);
                
                //Check address range
                if(_bootloader_check_address(address32+start_from_byte, hex_file_entry.dataLength-start_from_byte) != ADDRESS_CHECK_RESULT_OK)
                {
                    break;
                }
                
                if(page_to_write==0)
                {
                    //This is the first data record
                    //Prepare the buffer
                    //Obtain a handle to a 1024 byte buffer
                    page_to_write = entry_page;
                    internalFlash_readPage(page_to_write);
                    buffer = internalFlash_getBuffer();
                }

                for(cntr=start_from_byte; cntr<hex_file_entry.dataLength; ++cntr)
                {
                    //Make sure that byte is still on the same page
                    //Hex file entries often span across page boundaries
                    if(internalFlash_pageFromAddress(address32+cntr) == page_to_write)
                    {
                        address_within_page = internalFlash_addressWithinPage(address32+cntr, page_to_write);
                        buffer[address_within_page] = hex_file_entry.data[cntr];
                    }
                    else
                    {
                        //Make sure we re-visit this hex file entry
                        hex_file_offset -= return_value;
                        --hex_file_entries;
                        //Remember where to start from next time
                        start_from_byte_next = cntr;
                        //Write data to flash
                        internalFlash_erasePage(page_to_write);
                        internalFlash_writePage(page_to_write);
                        ++flash_pages_written;
                        //Return from function
                        return;
                    }
                }
                    
                break;
                
            case RecordTypeEndOfFile:
                //Only write if data has been prepared before
                if(page_to_write!=0)
                {
                    //Write data to flash
                    internalFlash_erasePage(page_to_write);
                    internalFlash_writePage(page_to_write);
                    ++flash_pages_written;
                    //Change mode
                    os.bootloader_mode = BOOTLOADER_MODE_DONE;
                    os.display_mode = DISPLAY_MODE_BOOTLOADER_DONE;
                }
                //Return from function
                return;
                break;
        }
    }
}

static compareResult_t _bootloader_verify_program_memory(uint32_t addressOffset, HexFileEntry_t *hexFileEntry)
{
    uint8_t buffer[16];
    uint32_t address; 
    uint8_t cntr;
    
    //Calculate address
    address = addressOffset + hexFileEntry->address;
    
    //Read from internal flash into buffer
    internalFlash_read(address, hexFileEntry->dataLength, buffer);
    
    for(cntr=0; cntr<hexFileEntry->dataLength; ++cntr)
    {
        if(hexFileEntry->data[cntr] != buffer[cntr])
        {
            return COMPARE_RESULT_DATA_DOES_NOT_MATCH;
        }
    }
    
    //If we get to here, all bytes match
    return COMPARE_RESULT_DATA_MATCHES;
}

uint32_t bootloader_get_file_size(void)
{
    return hex_file_size;
}

uint16_t bootloader_get_entries(void)
{
    return hex_file_entries;
}

uint16_t bootloader_get_total_entries(void)
{
    return total_hex_file_entries;
}

ShortRecordError_t bootloader_get_error(void)
{
    return last_error;
}

uint16_t bootloader_get_rec_dataLength(void)
{
    return hex_file_entry.dataLength;
}

uint16_t bootloader_get_rec_address(void)
{
    //return 12345;
    return hex_file_entry.address;
}

RecordType_t bootloader_get_rec_recordType(void)
{
    return hex_file_entry.recordType;
}

uint8_t bootloader_get_rec_data(uint8_t index)
{
    return hex_file_entry.data[index];
}

uint8_t bootloader_get_rec_checksum(void)
{
    return hex_file_entry.checksum;
}

uint8_t bootloader_get_rec_checksumCheck(void)
{
    return hex_file_entry.checksumCheck;
}

uint16_t bootloader_get_flashPagesWritten(void)
{
    return flash_pages_written;
}