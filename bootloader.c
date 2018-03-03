#include <xc.h>
#include "os.h"
#include "bootloader.h"
#include "fat16.h"
#include "hex.h"

uint8_t file_number = 0xFF;
uint8_t file_buffer[40];
uint16_t hex_file_entries = 0;
uint32_t hex_file_offset = 0;
uint32_t hex_file_size = 0;
HexFileEntry_t hex_file_entry;
ShortRecordError_t last_error;

typedef enum
{
    FILE_CHECK_STATUS_IN_PROGRESS,
    FILE_CHECK_STATUS_COMPLETED
} fileCheckStatus_t;
/*
typedef struct HexFileEntry
{
	uint16_t dataLength;
	uint16_t address;
	RecordType_t recordType;
	uint8_t data[16];
	uint8_t checksum;
	uint8_t checksumCheck;
} HexFileEntry_t;
*/
  
void _bootloader_find_file(void);
void _bootloader_verify_file(void);



void bootloader_run(void)
{
    switch(os.bootloader_mode)
    {
        case BOOTLOADER_MODE_START:
           _bootloader_find_file();
           break;
           
        case BOOTLOADER_MODE_FILE_FOUND:
            _bootloader_find_file();
           break;
            
        case BOOTLOADER_MODE_FILE_VERIFYING:
            _bootloader_verify_file();
            break;
            
        case BOOTLOADER_MODE_CHECK_COMPLETE:
            break;
            
        case BOOTLOADER_MODE_CHECK_FAILED:
            break;
    }
}

void _bootloader_find_file(void)
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

void _bootloader_verify_file(void)
{
    uint8_t rec_counter;
    //uint32_t offset = 0;
    uint32_t return_value = 0;
    
    //Find file size
    hex_file_size = fat_get_file_size(file_number);
    
    
    //Loop through a pre-defined number of records
    for(rec_counter=0; rec_counter<5; ++rec_counter)
    {
        //Read an entry
        if((hex_file_size-hex_file_offset)>=40)
            fat_read_from_file(file_number, hex_file_offset, 40, file_buffer);
        else
            fat_read_from_file(file_number, hex_file_offset, (hex_file_size-hex_file_offset), file_buffer);
        //Check that entry
        return_value = parseHexFileEntry(file_buffer, 0, &hex_file_entry);
        
        if(return_value==0)
        {
            //Last record has been reached without an error
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
            ++hex_file_entries;
            hex_file_offset += return_value;
        } 
    }
}

uint32_t bootloader_get_file_size(void)
{
    return hex_file_size;
}

uint16_t bootloader_get_entries(void)
{
    return hex_file_entries;
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