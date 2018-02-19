#include <xc.h>
#include "os.h"
#include "bootloader.h"
#include "fat16.h"
#include "hex.h"

uint8_t file_number = 0xFF;
uint8_t file_buffer[40];
uint16_t hex_file_entries = 0;
uint32_t hex_file_size = 0;
HexFileEntry_t hex_file_entry;
ShortRecordError_t last_error;
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
            _bootloader_verify_file();
            break;
    }
}

void _bootloader_find_file(void)
{
    file_number = fat_find_file(bootloader_filename, bootloader_extension);
    if(file_number!=0xFF)
    {
        os.bootloader_mode = BOOTLOADER_MODE_FILE_FOUND;
        os.display_mode = DISPLAY_MODE_BOOTLOADER_FILE_FOUND;
    }
}

void _bootloader_verify_file(void)
{
    uint32_t offset = 0;
    uint32_t return_value = 0;
    
    //Find file size
    hex_file_size = fat_get_file_size(file_number);
    
    
    //Loop through all entries in the hex file
    while(1)
    {
        //Read an entry
        if((hex_file_size-offset)>=40)
            fat_read_from_file(file_number, offset, 40, file_buffer);
        else
            fat_read_from_file(file_number, offset, (hex_file_size-offset), file_buffer);
        //Check that entry
        return_value = parseHexFileEntry(file_buffer, 0, &hex_file_entry);
        
        ++hex_file_entries;
        
        if(return_value==0)
        {
            //Last record has been reached
            break;
        }
        else if(return_value>0xFFFFFFF0)
        {
            //There is some error
            last_error = return_value & 0xF;
            break;
        }
        else
        {
            offset += return_value;
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