
#include <xc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "i2c.h"
#include "os.h"
#include "spi.h"
#include "bootloader.h"

const char start_line1[] = "Bootloader Mode";
const char start_line2[] = "Version ";
const char start_line3[] = "";
const char start_line4[] = "soldernerd.com";

const char search_line1[] = "Bootloader Mode";
const char search_line2[] = "Looking for file";
const char search_line3[] = "FIRMWARE.HEX on USB";
const char search_line4[] = "drive...";

const char found_line1[] = "Bootloader Mode";
const char found_line2[] = "FIRMWARE.HEX found";
const char found_line3[] = "Size: ";
const char found_line3b[] = " bytes";
const char found_line4[] = "Press to use file";

const char verify_line1[] = "Bootloader Mode";
const char verify_line2[] = "Verifying...";
const char verify_line3[] = "Record";

const char checked_line1[] = "Bootloader Mode";
const char checked_line2[] = "File check completed";
const char checked_line3[] = "records";
const char checked_line4[] = "Press to program";

const char failed_line1[] = "Bootloader Mode";
const char failed_line2[] = "File check failed";
const char failed_line3_startCode[] = "Missing start code";
const char failed_line3_noNextRecord[] = "No next record";
const char failed_line3_checksum[] = "Checksum error";
const char failed_line3_dataTooLong[] = "Data too long";
const char failed_line3_addressRange[] = "Addr. outside range";
const char failed_line4[] = "Record ";

const char programming_line1[] = "Bootloader Mode";
const char programming_line2[] = "Programming flash";
const char programming_line3[] = "Entry ";
const char programming_line4[] = "Pages written: ";

const char done_line1[] = "Bootloader Mode";
const char done_line2[] = "Done!";
const char done_line3[] = "Pages written: ";
const char done_line4[] = "Press to reboot";

char display_content[4][20];

static void _display_start(void);
static void _display_search(void);
static void _display_found(void);
static void _display_verify(void);
static void _display_checked(void);
static void _display_failed(void);
static void _display_programming(void);
static void _display_done(void);

uint8_t display_get_character(uint8_t line, uint8_t position)
{
    return display_content[line][position];
}

static void _display_clear(void)
{
    uint8_t row;
    uint8_t col;
    for(row=0;row<4;++row)
    {
        for(col=0;col<20;++col)
        {
            display_content[row][col] = ' ';
        }
    }
}

static uint8_t _display_itoa_u16(uint32_t value,  char *text)
{
    itoa(text, value, 10);
    if(value>9999)
    {
        *(text+5) = ' ';
        return 5;
    }
    else if (value>999)
    {
        *(text+4) = ' ';
        return 4;
    }
    else if (value>99)
    {
        *(text+3) = ' ';
        return 3;
    }
    else if (value>9)
    {
        *(text+2) = ' ';
        return 2;
    }
    else
    {
        *(text+1) = ' ';
        return 1;
    }
}

static uint8_t _get_hex_char(uint8_t value)
{
    if(value&0xF0)
    {
        value >>= 4;
    }
    else
    {
        value &= 0x0F;
    }    
    switch(value)
    {
        case 0x00:
            return '0';
        case 0x01:
            return '1';
        case 0x02:
            return '2';
        case 0x03:
            return '3';
        case 0x04:
            return '4';         
        case 0x05:
            return '5';       
        case 0x06:
            return '6';  
        case 0x07:
            return '7'; 
        case 0x08:
            return '8';      
        case 0x09:
            return '9';         
        case 0x0A:
            return 'A'; 
        case 0x0B:
            return 'B';       
        case 0x0C:
            return 'C';          
        case 0x0D:
            return 'D';       
        case 0x0E:
            return 'E';         
        case 0x0F:
            return 'F'; 
    }
    return 'G';
}

static void _display_itoa_u8(uint8_t value,  char *text)
{
    *(text+0) = _get_hex_char(value & 0xF0);
    *(text+1) = _get_hex_char(value & 0x0F);
}

static uint8_t _display_itoa_u32(uint32_t value,  char *text)
{
    //Value can be handled by 16 bits
    if(value<=0x7FFF) //32767
    {
        return _display_itoa_u16((uint16_t) value,  text);
    }
        
    //We really need 32 bits
    if(value>100000000)
    {
        itoa(text, (uint16_t)(value/10000), 10);
        itoa(text+5, (uint16_t)(value%10000), 10);
        *(text+9) = ' ';
        return 9;
    }
    else if(value>10000000)
    {
        itoa(text, (uint16_t)(value/10000), 10);
        itoa(text+4, (uint16_t)(value%10000), 10);
        *(text+8) = ' ';
        return 8;
    }
    else if(value>1000000)
    {
        itoa(text, (uint16_t)(value/10000), 10);
        itoa(text+3, (uint16_t)(value%10000), 10);
        *(text+7) = ' ';
        return 7;
    }
    else if(value>100000)
    {
        itoa(text, (uint16_t)(value/10000), 10);
        itoa(text+2, (uint16_t)(value%10000), 10);
        *(text+6) = ' ';
        return 6;
    }
    else
    {
        itoa(text, (uint16_t)(value/10000), 10);
        itoa(text+1, (uint16_t)(value%10000), 10);
        *(text+5) = ' ';
        return 5;
    }
}

static void _display_itoa(int16_t value, uint8_t decimals, char *text)
{
    uint8_t pos;
    uint8_t len;
    int8_t missing;
    char tmp[10];
    itoa(tmp, value, 10);
    len = strlen(tmp);
    
    
    if(value<0) //negative values
    {
        missing = decimals + 2 - len;
        if(missing>0) //zero-padding needed
        {
            for(pos=decimals;pos!=0xFF;--pos)
            {
                if(pos>=missing) //there is a character to copy
                {
                    tmp[pos+1] = tmp[pos+1-missing];
                }
                else //there is no character
                {
                    tmp[pos+1] = '0';
                }
            }
            len = decimals + 2;
        }  
    }
    else
    {
        missing = decimals + 1 - len;
        if(missing>0) //zero-padding needed
        {
            for(pos=decimals;pos!=0xFF;--pos)
            {
                if(pos>=missing) //there is a character to copy
                {
                    tmp[pos] = tmp[pos-missing];
                }
                else //there is no character
                {
                    tmp[pos] = '0';
                }
            }
            len = decimals + 1;
        }       
    }
 
    decimals = len - decimals - 1;
    
    for(pos=0;pos<len;++pos)
    {
        text[pos] = tmp[pos];
        if(pos==decimals)
        {
            //Insert decimal point
            ++pos;
            text[pos] = '.';
            break;
        }
    }
    for(;pos<len;++pos)
    {
        text[pos+1] = tmp[pos];
    }
}

void display_prepare(uint8_t mode)
{
    _display_clear();
    
    switch(mode&0xF0)
    {
        case DISPLAY_MODE_BOOTLOADER_START:
            _display_start();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_SEARCH:
            _display_search();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_FILE_FOUND:
            _display_found();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_FILE_VERIFYING:
            _display_verify();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_CHECK_COMPLETE:
            _display_checked();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_CHECK_FAILED:
            _display_failed();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_PROGRAMMING:
            _display_programming();
            break;
            
        case DISPLAY_MODE_BOOTLOADER_DONE:
            _display_done();
            break;
    }
    
    //Pulse
    switch((os.timeSlot>>5)&0b11)
    {
        case 3:
            display_content[0][17] = '.';
            //fall through
        case 2:
            display_content[0][16] = '.';
            //fall through
        case 1:
            display_content[0][15] = '.';
            //fall through     
    }
    
    //Some debugging output
    //Warn if interrupts are enabled
    if(INTCONbits.GIE)
    {
        display_content[0][18] = 'I';
    }
}

static void _display_start(void)
{
    uint8_t cntr;
    cntr = 0;
    while(start_line1[cntr])
        display_content[0][cntr] = start_line1[cntr++];
    cntr = 0;
    while(start_line2[cntr])
        display_content[1][cntr] = start_line2[cntr++];
    cntr += _display_itoa_u32(FIRMWARE_VERSION_MAJOR, &display_content[1][cntr]);
    display_content[1][cntr++] = '.';
    cntr += _display_itoa_u32(FIRMWARE_VERSION_MINOR, &display_content[1][cntr]);
    display_content[1][cntr++] = '.';
    cntr += _display_itoa_u32(FIRMWARE_VERSION_FIX, &display_content[1][cntr]);
    cntr = 0;
    while(start_line3[cntr])
        display_content[2][cntr] = start_line3[cntr++];
    cntr = 0;
    while(start_line4[cntr])
        display_content[3][cntr] = start_line4[cntr++];
}

static void _display_search(void)
{
    uint8_t cntr;
    cntr = 0;
    while(search_line1[cntr])
        display_content[0][cntr] = search_line1[cntr++];
    cntr = 0;
    while(search_line2[cntr])
        display_content[1][cntr] = search_line2[cntr++];
    cntr = 0;
    while(search_line3[cntr])
        display_content[2][cntr] = search_line3[cntr++];
    cntr = 0;
    while(search_line4[cntr])
        display_content[3][cntr] = search_line4[cntr++];
}

static void _display_found(void)
{
    uint8_t cntr;
    uint8_t start;
    //First line
    cntr = 0;
    while(found_line1[cntr])
        display_content[0][cntr] = found_line1[cntr++];
    //Second line
    cntr = 0;
    while(found_line2[cntr])
        display_content[1][cntr] = found_line2[cntr++];
    //Third line (file size)
    cntr = 0;
    while(found_line3[cntr])
        display_content[2][cntr] = found_line3[cntr++];
    start = cntr;
    start += _display_itoa_u32(bootloader_get_file_size(), &display_content[2][cntr]);
    cntr = 0;
    while(found_line3b[cntr])
        display_content[2][start+cntr] = found_line3b[cntr++];
    //Fourth line
    cntr = 0;
    while(found_line4[cntr])
        display_content[3][cntr] = found_line4[cntr++];
}

static void _display_verify(void)
{
    uint8_t cntr;
    uint8_t start;
    cntr = 0;
    while(verify_line1[cntr])
        display_content[0][cntr] = verify_line1[cntr++];
    cntr = 0;
    while(verify_line2[cntr])
        display_content[1][cntr] = verify_line2[cntr++];
    cntr = 0;
    while(verify_line3[cntr])
        display_content[2][cntr] = verify_line3[cntr++];
    _display_itoa_u16(bootloader_get_entries(), &display_content[2][cntr+1]);
}

static void _display_checked(void)
{
    uint8_t cntr;
    uint8_t start;
    cntr = 0;
    while(checked_line1[cntr])
        display_content[0][cntr] = checked_line1[cntr++];
    cntr = 0;
    while(checked_line2[cntr])
        display_content[1][cntr] = checked_line2[cntr++];
    //Display number of records
    start = _display_itoa_u16(bootloader_get_total_entries(), &display_content[2][0]);
    start++;
    cntr = 0;
    while(checked_line3[cntr])
        display_content[2][start+cntr] = checked_line3[cntr++];
    //Display question
    cntr = 0;
    while(checked_line4[cntr])
        display_content[3][cntr] = checked_line4[cntr++];
}

static void _display_failed(void)
{
    uint8_t cntr;
    uint8_t start;
    cntr = 0;
    while(failed_line1[cntr])
        display_content[0][cntr] = failed_line1[cntr++];
    cntr = 0;
    while(failed_line2[cntr])
        display_content[1][cntr] = failed_line2[cntr++];
    //Display error
    cntr = 0;
    switch(bootloader_get_error())
    {
        case ShortRecordErrorStartCode:
            while(failed_line3_startCode[cntr])
            display_content[2][cntr] = failed_line3_startCode[cntr++];
            break;
            
        case ShortRecordErrorChecksum:
            while(failed_line3_checksum[cntr])
            display_content[2][cntr] = failed_line3_checksum[cntr++];
            //_itoa(&display_content[2][14], bootloader_get_rec_checksum(), 10);
            //itoa(&display_content[2][17], bootloader_get_rec_checksumCheck(), 10);
            break;
            
        case ShortRecordErrorNoNextRecord:
            while(failed_line3_noNextRecord[cntr])
            display_content[2][cntr] = failed_line3_noNextRecord[cntr++];
            break;
            
        case ShortRecordErrorDataTooLong:
            while(failed_line3_dataTooLong[cntr])
            display_content[2][cntr] = failed_line3_dataTooLong[cntr++];
            break;
            
        case ShortRecordErrorAddressRange:
            while(failed_line3_addressRange[cntr])
            display_content[2][cntr] = failed_line3_addressRange[cntr++];
            _display_itoa_u32(bootloader_get_rec_address(), &display_content[3][14]);
            break;
            
    }
    //Display record number
    cntr = 0;
    while(failed_line4[cntr])
        display_content[3][cntr] = failed_line4[cntr++];
    _display_itoa_u16(bootloader_get_entries(), &display_content[3][cntr]);
}

static void _display_programming(void)
{
    uint8_t cntr;
    cntr = 0;
    while(programming_line1[cntr])
        display_content[0][cntr] = programming_line1[cntr++];
    
    cntr = 0;
    while(programming_line2[cntr])
        display_content[1][cntr] = programming_line2[cntr++];
    
    cntr = 0;
    while(programming_line3[cntr])
        display_content[2][cntr] = programming_line3[cntr++];
    cntr += _display_itoa_u16(bootloader_get_entries(), &display_content[2][cntr]);
    display_content[2][cntr++] = '/';
    _display_itoa_u16(bootloader_get_total_entries(), &display_content[2][cntr]);
    
    cntr = 0;
    while(programming_line4[cntr])
        display_content[3][cntr] = programming_line4[cntr++];
    _display_itoa_u16(bootloader_get_flashPagesWritten(), &display_content[3][cntr]);
}

static void _display_done(void)
{
    uint8_t cntr;
    cntr = 0;
    while(done_line1[cntr])
        display_content[0][cntr] = done_line1[cntr++];
    cntr = 0;
    while(done_line2[cntr])
        display_content[1][cntr] = done_line2[cntr++];
    cntr = 0;
    while(done_line3[cntr])
        display_content[2][cntr] = done_line3[cntr++];
    _display_itoa_u16(bootloader_get_flashPagesWritten(), &display_content[2][cntr]);
    cntr = 0;
    while(done_line4[cntr])
        display_content[3][cntr] = done_line4[cntr++];
}

void display_update(void)
{
    i2c_display_cursor(0, 0);
    i2c_display_write_fixed(&display_content[0][0], 20);
    i2c_display_cursor(1, 0);
    i2c_display_write_fixed(&display_content[1][0], 20);
    i2c_display_cursor(2, 0);
    i2c_display_write_fixed(&display_content[2][0], 20);
    i2c_display_cursor(3, 0);
    i2c_display_write_fixed(&display_content[3][0], 20);
}