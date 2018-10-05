
#include <string.h>

#include "system.h"
#include "api.h"
#include "display.h"
#include "os.h"
#include "ui.h"
#include "i2c.h"
#include "bootloader.h"
#include "fat16.h"
#include "flash.h"



/******************************************************************************
 * Static function prototypes
 ******************************************************************************/

static void _fill_buffer_get_status(uint8_t *outBuffer);
static void _fill_buffer_get_display(uint8_t *outBuffer, uint8_t secondHalf);
static void _fill_buffer_get_bootloader_details(uint8_t *outBuffer);
static void _fill_buffer_get_configuration(uint8_t *outBuffer);

static void _fill_buffer_get_file_details(uint8_t *inBuffer, uint8_t *outBuffer);
static void _fill_buffer_find_file(uint8_t *inBuffer, uint8_t *outBuffer);
static void _fill_buffer_read_file(uint8_t *inBuffer, uint8_t *outBuffer);
static void _fill_buffer_read_buffer(uint8_t *inBuffer, uint8_t *outBuffer);

static void _parse_command_short(uint8_t cmd);
static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

static uint8_t _parse_file_resize(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_delete(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_create(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_rename(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_append(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_modify(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_sector_to_buffer(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_buffer_to_sector(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_write_buffer(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_file_copy(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

static uint8_t _parse_settings_spi_mode(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_spi_frequency(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_spi_polarity(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_i2c_mode(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_i2c_frequency(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_i2c_slaveModeSlaveAddress(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);
static uint8_t _parse_settings_i2c_masterModeSlaveAddress(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr);

/******************************************************************************
 * Public functions implementation
 ******************************************************************************/


void api_prepare(uint8_t *inBuffer, uint8_t *outBuffer)
{
    apiDataRequest_t command = (apiDataRequest_t) inBuffer[0]; 

    if(command>0x7F)
    {
        //Extended data request, may be followed by parameters (no commands allowed to follow)
        
//        //Only allowed if flash is not busy, return normal status if flash is busy
//        if(flash_is_busy())
//        {
//            _fill_buffer_get_status(outBuffer);
//            return;
//        }

        switch(command)
        {
            case DATAREQUEST_GET_FILE_DETAILS:
                //Get file details
                _fill_buffer_get_file_details(inBuffer, outBuffer);
                break;
                
            case DATAREQUEST_FIND_FILE:
                //Find file
                _fill_buffer_find_file(inBuffer, outBuffer);
                break;
                
            case DATAREQUEST_READ_FILE:
                //Read file
                _fill_buffer_read_file(inBuffer, outBuffer);
                break;
                
            case DATAREQUEST_READ_BUFFER:
                //Read file
                _fill_buffer_read_buffer(inBuffer, outBuffer);
                break;
                
            default:
                outBuffer[0] = 0x99;
                outBuffer[1] = 0x99;
        }
    }
    else
    {
        //Normal data request, need no parameters, may be followed by commands
        switch(command)				
        {
            case DATAREQUEST_GET_COMMAND_RESPONSE:
                //nothing to do
                break;
            
            case DATAREQUEST_GET_STATUS:
                //Call function to fill the buffer with general information
                _fill_buffer_get_status(outBuffer);
                break;

            case DATAREQUEST_GET_DISPLAY_1:
                //Call function to fill the buffer with general information
                _fill_buffer_get_display(outBuffer, 0);
                break;

            case DATAREQUEST_GET_DISPLAY_2:
                //Call function to fill the buffer with general information
                _fill_buffer_get_display(outBuffer, 1);
                break;
                
            case DATAREQUEST_GET_BOOTLOADER_DETAILS:
                //Call function to fill the buffer with bootloader details
                _fill_buffer_get_bootloader_details(outBuffer);
                break;
            
            case DATAREQUEST_GET_CONFIGURATION:
                //Call function to fill the buffer with configuration details
                _fill_buffer_get_configuration(outBuffer);
                break;
                
            case DATAREQUEST_GET_ECHO:
                //Copy received data to outBuffer
                memcpy(outBuffer, inBuffer, 64);
                break;                
                
            default:
                outBuffer[0] = 0x99;
                outBuffer[1] = 0x99;
        }
    }
}

void api_parse(uint8_t *inBuffer, uint8_t receivedDataLength, uint8_t *outBuffer)
{
    //Check if the host expects us to do anything else

    uint8_t in_idx;
    uint8_t out_idx;
    uint8_t *out_idx_ptr;
    
    out_idx = 0;
    out_idx_ptr = &out_idx;
    
    if(inBuffer[0]>0x7F)
    {
        //Extended data request. May not be followed by commands.
        //Nothing for us to do here
        return;
    }
    
    if(inBuffer[0]==DATAREQUEST_GET_ECHO)
    {
        //Connectivity is beeing tested. Just echo back whatever we've received
        //Do not try to interpret any of the following bytes as commands
        return;
    }
    
    if(inBuffer[0]==DATAREQUEST_GET_COMMAND_RESPONSE)
    {
        //Echo back to the host PC the command we are fulfilling in the first uint8_t
        outBuffer[0] = DATAREQUEST_GET_COMMAND_RESPONSE;
    
        //Bootloader signature
        outBuffer[1] = BOOTLOADER_SIGNATURE >> 8; //MSB
        outBuffer[2] = (uint8_t) BOOTLOADER_SIGNATURE; //LSB
        
        //Set out_idx, also indicating that we want to provide feedback on the command's execution
        out_idx = 3;
    }
    
    in_idx = 1;
    while(in_idx<receivedDataLength)
    {
        //Check if there is anything more to parse
        if(inBuffer[in_idx]==COMMAND_STOP_PARSING)
        {
            return;
        }
        
        switch(inBuffer[in_idx] & 0xF0)
        {
            case 0x20:
                _parse_command_short(inBuffer[in_idx]);
                ++in_idx;
                break;
                
            case 0x30:
                _parse_command_short(inBuffer[in_idx]);
                ++in_idx;
                break;
                
            case 0x50:
                in_idx += _parse_command_long(&inBuffer[in_idx], outBuffer, out_idx_ptr);
                break;
                
            default:
                //We should never end up here
                //If we still do, stop parsing this buffer
                return;
        }
    }
}


/******************************************************************************
 * Static functions implementation
 ******************************************************************************/

//Fill buffer with general status information
static void _fill_buffer_get_status(uint8_t *outBuffer)
{
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_STATUS;
    
    //Bootloader signature
    outBuffer[1] = BOOTLOADER_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) BOOTLOADER_SIGNATURE; //LSB
    
    //Flash busy or not
    outBuffer[3] = (uint8_t) flash_is_busy();
    
    //Firmware version
    outBuffer[4] = FIRMWARE_VERSION_MAJOR;
    outBuffer[5] = FIRMWARE_VERSION_MINOR;
    outBuffer[6] = FIRMWARE_VERSION_FIX;
    
    //Display status, display off, startup etc
    outBuffer[7] = ui_get_status();
    
    //Entire os struct
    outBuffer[8] = os.encoderCount;
    outBuffer[9] = os.buttonCount;
    outBuffer[10] = os.timeSlot;
    outBuffer[11] = os.done;
    outBuffer[12] = os.bootloader_mode;
    outBuffer[13] = os.display_mode;
}

//Fill buffer with display content
static void _fill_buffer_get_display(uint8_t *outBuffer, uint8_t secondHalf)
{
    uint8_t cntr;
    uint8_t line;
    uint8_t start_line;
    uint8_t position;
    
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    if(secondHalf)
    {
        outBuffer[0] = DATAREQUEST_GET_DISPLAY_2;
    }
    else
    {
        outBuffer[0] = DATAREQUEST_GET_DISPLAY_1;
    }
   
    //Bootloader signature
    outBuffer[1] = BOOTLOADER_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) BOOTLOADER_SIGNATURE; //LSB
   
    //Get display data
    cntr = 3;
    if(secondHalf)
    {
        start_line = 2;
    }
    else
    {
        start_line = 0;
    }
    for(line=start_line; line<start_line+2; ++line)
    {
        for(position=0; position<20; ++position)
        {
            outBuffer[cntr] = display_get_character(line, position);
            ++cntr;
        }
    }
}

static void _fill_buffer_get_bootloader_details(uint8_t *outBuffer)
{
    uint8_t cntr;
    uint8_t data_length;
    uint16_t buffer_small;
    uint32_t buffer_large;
    
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_BOOTLOADER_DETAILS;
    
    //Bootloader signature
    outBuffer[1] = HIGH_BYTE(BOOTLOADER_SIGNATURE); //MSB
    outBuffer[2] = LOW_BYTE(BOOTLOADER_SIGNATURE); //LSB
   
    //Bootloader information (high level)
    buffer_large = bootloader_get_file_size();
    outBuffer[3] = HIGH_BYTE(HIGH_WORD(buffer_large));
    outBuffer[4] = LOW_BYTE(HIGH_WORD(buffer_large));
    outBuffer[5] = HIGH_BYTE(LOW_WORD(buffer_large));
    outBuffer[6] = LOW_BYTE(LOW_WORD(buffer_large));
    
    buffer_small = bootloader_get_entries();
    outBuffer[7] = HIGH_BYTE(buffer_small);
    outBuffer[8] = LOW_BYTE(buffer_small);
    
    buffer_small = bootloader_get_total_entries();
    outBuffer[9] = HIGH_BYTE(buffer_small);
    outBuffer[10] = LOW_BYTE(buffer_small);
    
    outBuffer[11] = (uint8_t) bootloader_get_error();
    
    buffer_small = bootloader_get_flashPagesWritten();
    outBuffer[12] = HIGH_BYTE(buffer_small);
    outBuffer[13] = LOW_BYTE(buffer_small);
    
    //Bootloader information (last record)
    buffer_small = bootloader_get_rec_dataLength();
    outBuffer[14] = HIGH_BYTE(buffer_small);
    outBuffer[15] = LOW_BYTE(buffer_small);
    
    buffer_small = bootloader_get_rec_address();
    outBuffer[16] = HIGH_BYTE(buffer_small);
    outBuffer[17] = LOW_BYTE(buffer_small);
    
    outBuffer[18] = (uint8_t) bootloader_get_rec_recordType();
    outBuffer[19] = bootloader_get_rec_checksum();
    outBuffer[20] = bootloader_get_rec_checksumCheck();

    data_length = (uint8_t) bootloader_get_rec_dataLength();
    if(data_length>43)
    {
        //More will not fit into our 64byte buffer
        data_length = 43;
    }
    for(cntr=0; cntr<data_length; ++cntr)
    {
        outBuffer[21+cntr] = bootloader_get_rec_data(cntr);
    }
}

static void _fill_buffer_get_configuration(uint8_t *outBuffer)
{
    //Echo back to the host PC the command we are fulfilling in the first uint8_t
    outBuffer[0] = DATAREQUEST_GET_CONFIGURATION;
    
    //Bootloader signature
    outBuffer[1] = BOOTLOADER_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) BOOTLOADER_SIGNATURE; //LSB
   
    //SPI settings
    outBuffer[3] = os.communicationSettings.spiMode;
    outBuffer[4] = os.communicationSettings.spiFrequency;
    outBuffer[5] = os.communicationSettings.spiPolarity;
    
    //I2C settings
    outBuffer[6] = os.communicationSettings.i2cMode;
    outBuffer[7] = os.communicationSettings.i2cFrequency;
    outBuffer[8] = os.communicationSettings.i2cSlaveModeSlaveAddress;
    outBuffer[9] = os.communicationSettings.i2cMasterModeSlaveAddress;
}

static void _fill_buffer_get_file_details(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint8_t file_number = inBuffer[1];
    
    //Echo command
    outBuffer[0] = DATAREQUEST_GET_FILE_DETAILS;
   
    //Bootloader signature
    outBuffer[1] = BOOTLOADER_SIGNATURE >> 8; //MSB
    outBuffer[2] = (uint8_t) BOOTLOADER_SIGNATURE; //LSB
    
    //Echo file number
    outBuffer[3] = file_number;
    
    //Return desired data, i.e. entire 32-bit file information struct
    outBuffer[4] = fat_get_file_information(file_number, (rootEntry_t*) &outBuffer[5]);
}

static void _fill_buffer_find_file(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint8_t cntr;
    
    //Echo command
    outBuffer[0] = DATAREQUEST_FIND_FILE;
   
    //Bootloader signature
    outBuffer[1] = HIGH_BYTE(BOOTLOADER_SIGNATURE); //MSB
    outBuffer[2] = LOW_BYTE(BOOTLOADER_SIGNATURE); //LSB
    
    //Find and return file number
    outBuffer[3] = fat_find_file(&inBuffer[1], &inBuffer[9]);
    
    //Echo file name and extention
    for(cntr=0; cntr<11; ++cntr)
    {
        outBuffer[cntr+12] = inBuffer[cntr+1];
    }
}

static void _fill_buffer_read_file(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint32_t start;
    uint32_t file_size;
    uint32_t data_length;
    
    //Echo command
    outBuffer[0] = DATAREQUEST_READ_FILE;
   
    //Bootloader signature
    outBuffer[1] = HIGH_BYTE(BOOTLOADER_SIGNATURE); //MSB
    outBuffer[2] = LOW_BYTE(BOOTLOADER_SIGNATURE); //LSB
    
    //Echo file number
    outBuffer[3] = inBuffer[1];
    
    //Echo start
    outBuffer[4] = inBuffer[2];
    outBuffer[5] = inBuffer[3];
    outBuffer[6] = inBuffer[4];
    outBuffer[7] = inBuffer[5];
    
    //Calculate start
    start = inBuffer[2];
    start <<= 8;
    start |= inBuffer[3];
    start <<= 8;
    start |= inBuffer[4];
    start <<= 8;
    start |= inBuffer[5];
    
    //Get file size and calculate number of bytes to get
    file_size = fat_get_file_size(inBuffer[1]);
    data_length = file_size - start;
    if(data_length>54)
    {
        //More will not fit into our 64 byte buffer
        data_length = 54;
    }
    
    //Echo data length
    outBuffer[8] = (uint8_t) data_length;
    
    //Read data from file
    outBuffer[9] = fat_read_from_file(inBuffer[1], start, data_length, &outBuffer[10]);
}

static void _fill_buffer_read_buffer(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint16_t start;
    uint16_t data_length;
    
    //Echo command
    outBuffer[0] = DATAREQUEST_READ_BUFFER;
   
    //Bootloader signature
    outBuffer[1] = HIGH_BYTE(BOOTLOADER_SIGNATURE); //MSB
    outBuffer[2] = LOW_BYTE(BOOTLOADER_SIGNATURE); //LSB

    //Echo start
    outBuffer[3] = inBuffer[1];
    outBuffer[4] = inBuffer[2];
    
    //Calculate start
    start = inBuffer[1];
    start <<= 8;
    start |= inBuffer[2];
    
    //Get file size and calculate number of bytes to get
    data_length = 512 - start;
    if(data_length>54)
    {
        //More will not fit into our 64 byte buffer
        data_length = 58;
    }
    
    //Echo data length
    outBuffer[5] = (uint8_t) data_length;
    
    //Read data from file
    fat_read_from_buffer(start, data_length, &outBuffer[6]);
}


static void _parse_command_short(uint8_t cmd)
{
    switch(cmd)
    {
        case COMMAND_REBOT:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, 0x00); //0x00 is a neutral value
            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
            
        case COMMAND_REBOT_BOOTLOADER_MODE:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_BOOTLOADER_MODE);
            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
                
        case COMMAND_REBOT_NORMAL_MODE:
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_NORMAL_MODE);
            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
            
        case COMMAND_JUMP_TO_MAIN_PROGRAM:
            //We can't just jump there. We need a proper reset first
            i2c_eeprom_writeByte(EEPROM_BOOTLOADER_BYTE_ADDRESS, BOOTLOADER_BYTE_FORCE_NORMAL_MODE);
            system_delay_ms(10); //ensure data has been written before rebooting
            reboot();
            break;
            
        case COMMAND_SUSPEND_BOOTLOADER:
            os.bootloader_mode = BOOTLOADER_MODE_SUSPENDED;
            os.display_mode = DISPLAY_MODE_BOOTLOADER_SUSPENDED;
            break;
                
        case COMMAND_ENCODER_CCW:
            --os.encoderCount;
            break;
            
        case COMMAND_ENCODER_CW:
            ++os.encoderCount;
            break;
            
        case COMMAND_ENCODER_PUSH:
            ++os.buttonCount;
            break;
    }
}

static uint8_t _parse_command_long(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    uint8_t length = 65;
    
    switch(data[0])
    {
        case COMMAND_FILE_RESIZE:
            length = _parse_file_resize(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_DELETE:
            length = _parse_file_delete(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_CREATE:
            length = _parse_file_create(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_RENAME:
            length = _parse_file_rename(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_APPEND:
            length = _parse_file_append(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_MODIFY:
            length = _parse_file_modify(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FORMAT_DRIVE:
            length = _parse_format_drive(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_SECTOR_TO_BUFFER:
            length = _parse_sector_to_buffer(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_BUFFER_TO_SECTOR:
            length = _parse_buffer_to_sector(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_WRITE_BUFFER:
            length = _parse_write_buffer(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_FILE_COPY:
            length = _parse_file_copy(data, out_buffer, out_idx_ptr);
            break;

        case COMMAND_SET_SPI_MODE:
            length = _parse_settings_spi_mode(data, out_buffer, out_idx_ptr);
            break;
            
        case COMMAND_SET_SPI_FREQUENCY:
            length = _parse_settings_spi_frequency(data, out_buffer, out_idx_ptr);
            break;
          
        case COMMAND_SET_SPI_POLARITY:
            length = _parse_settings_spi_polarity(data, out_buffer, out_idx_ptr);
            break;
          
        case COMMAND_SET_I2C_MODE:
            length = _parse_settings_i2c_mode(data, out_buffer, out_idx_ptr);
            break;
          
        case COMMAND_SET_I2C_FREQUENCY:
            length = _parse_settings_i2c_frequency(data, out_buffer, out_idx_ptr);
            break;
          
        case COMMAND_SET_I2C_SLAVE_MODE_ADDRESS:
            length = _parse_settings_i2c_slaveModeSlaveAddress(data, out_buffer, out_idx_ptr);
            break;
          
        case COMMAND_SET_I2C_MASTER_MODE_ADDRESS:
            length = _parse_settings_i2c_masterModeSlaveAddress(data, out_buffer, out_idx_ptr);
            break; 
    }    
    
    return length;
}

static uint8_t _parse_file_resize(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x50: Resize file. Parameters: uint8_t FileNumber, uint32_t newFileSize, 0x4CEA
    uint32_t file_size;
    uint8_t return_value;
    uint8_t file_number;
    
    if((data[0]!=COMMAND_FILE_RESIZE) || (data[6]!=0x4C) || (data[7]!=0xEA))
    {
        return 8;
    }
    
    //Get file number
    file_number = data[1];
    
    //Calculate file size
    file_size = data[2];
    file_size <<= 8;
    file_size |= data[3];
    file_size <<= 8;
    file_size |= data[4];
    file_size <<= 8;
    file_size |= data[5];
    
    //Resize file
    return_value = fat_resize_file(file_number, file_size);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<58))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_RESIZE;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(HIGH_WORD(file_size));
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(HIGH_WORD(file_size));
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(LOW_WORD(file_size));
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(LOW_WORD(file_size));
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 8;
}

static uint8_t _parse_file_delete(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x51: Delete file. Parameters: uint8_t FileNumber, 0x66A0
    
    uint8_t return_value;
    uint8_t file_number;
    
    if((data[0]!=COMMAND_FILE_DELETE) || (data[2]!=0x66) || (data[3]!=0xA0))
    {
        return 4;
    }
    
    //Save file number
    file_number = data[1];
    
    //Delete file
    return_value = fat_delete_file(file_number);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<62))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_DELETE;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 4;
}

static uint8_t _parse_file_create(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x52: Create file. Parameters: char[8] FileName, char[3] FileExtention, uint32_t FileSize, 0xBD4F 
    uint8_t file_number;
    
    uint32_t file_size;
    if((data[0]!=COMMAND_FILE_CREATE) || (data[16]!=0xBD) || (data[17]!=0x4F))
    {
        return 18;
    }
    
    //Calculate file size
    file_size = data[12];
    file_size <<= 8;
    file_size |= data[13];
    file_size <<= 8;
    file_size |= data[14];
    file_size <<= 8;
    file_size |= data[15];
    
    //Create file
    file_number = fat_create_file(&data[1], &data[9], file_size);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_CREATE;
        out_buffer[(*out_idx_ptr)++] = file_number;
    }
    
    return 18;
}

static uint8_t _parse_file_rename(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x53: Rename file. Parameters: uint8_t FileNumber, char[8] NewFileName, char[3] NewFileExtention, 0x7E18
    
    uint8_t file_number;
    uint8_t return_value;
    
    if((data[0]!=COMMAND_FILE_RENAME) || (data[13]!=0x7E) || (data[14]!=0x18))
    {
        return 15;
    }
    
    //Save file number
    file_number = data[1];
    
    //Rename file
    return_value = fat_rename_file(file_number, &data[2], &data[10]);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<62))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_RENAME;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 15;
}

static uint8_t _parse_file_append(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x54: Append to file. Parameters: uint8_t FileNumber, uint8_t NumberOfBytes, 0xFE4B, DATA
    
    uint8_t file_number;
    uint8_t return_value;
    uint16_t number_of_bytes;
    
    if((data[0]!=COMMAND_FILE_APPEND) || (data[3]!=0xFE) || (data[4]!=0x4B))
    {
        //Can't trust the number of bytes in this case
        return 65;
    }
    
    //Save file number
    file_number = data[1];
    
    //Get number of bytes
    number_of_bytes = (uint16_t) data[2];
    
    //Append to file
    return_value = fat_append_to_file(file_number, number_of_bytes, &data[5]);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<60))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_APPEND;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(number_of_bytes);
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(number_of_bytes);
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return data[2] + 5;
}

static uint8_t _parse_file_modify(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    uint16_t number_of_bytes;
    uint32_t start_byte;
    uint8_t file_number;
    uint8_t return_value;
    
    //0x55: Modify file. Parameters: uint8_t FileNumber, uint32_t StartByte, uint8_t NumerOfBytes, 0x0F9B, DATA
    if((data[0]!=COMMAND_FILE_MODIFY) || (data[7]!=0x0F) || (data[8]!=0x9B))
    {
        //Can't trust the number of bytes in this case
        return 65;
    }
    
    //Save file number
    file_number = data[1];
    
    //Get number_of_bytes
    number_of_bytes = data[6];
    
    //Calculate start byte
    start_byte = data[2];
    start_byte <<= 8;
    start_byte |= data[3];
    start_byte <<= 8;
    start_byte |= data[4];
    start_byte <<= 8;
    start_byte |= data[5];
    
    return_value = fat_modify_file(file_number, start_byte, number_of_bytes, &data[9]);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<56))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_MODIFY;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(HIGH_WORD(start_byte));
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(HIGH_WORD(start_byte));
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(LOW_WORD(start_byte));
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(LOW_WORD(start_byte));
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(number_of_bytes);
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(number_of_bytes);
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return number_of_bytes + 9;
}

static uint8_t _parse_format_drive(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x56: Format drive. Parameters: none, 0xDA22
    
    uint8_t return_value;
    
    if((data[0]!=COMMAND_FORMAT_DRIVE) || (data[1]!=0xDA) || (data[2]!=0x22))
    {
        return 3;
    }
    
    return_value = fat_format();
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FORMAT_DRIVE;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 3;
} 

static uint8_t _parse_sector_to_buffer(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x57: Read file sector to buffer. Parameters: uint8_t file_number, uint16_t sector, 0x1B35
    uint8_t file_number;
    uint16_t sector;
    uint8_t return_value;
    
    if((data[0]!=COMMAND_SECTOR_TO_BUFFER) || (data[4]!=0x1B) || (data[5]!=0x35))
    {
        return 6;
    }
    
    //Get file number
    file_number = data[1];
    
    //Calculate sector
    sector = data[2];
    sector <<= 8;
    sector |= data[3];

    //Read desired sector from flash into buffer 2
    return_value = fat_copy_sector_to_buffer(data[1], sector);
    
    //Return result if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<60))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SECTOR_TO_BUFFER;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(sector);
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(sector);
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 6;
}

static uint8_t _parse_buffer_to_sector(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x58: Write buffer to file sector. Parameters: uint8_t file_number, uint16_t sector, 0x6A6D
    uint8_t file_number;
    uint16_t sector;
    uint8_t return_value;
    
    if((data[0]!=COMMAND_BUFFER_TO_SECTOR) || (data[4]!=0x6A) || (data[5]!=0x6D))
    {
        return 6;
    }
    
    //Get file number
    file_number = data[1];
    
    //Calculate sector
    sector = data[2];
    sector <<= 8;
    sector |= data[3];

    //Write content of buffer 2 to desired flash location
    return_value = fat_write_sector_from_buffer(file_number, sector);
    
    //Return result if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<60))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_BUFFER_TO_SECTOR;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(sector);
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(sector);
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 6;
}

static uint8_t _parse_write_buffer(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    uint16_t start_byte;
    uint16_t number_of_bytes;
    
    //0x59: Modify buffer. Parameters: uint16_t StartByte, uint8_t NumerOfBytes, 0xE230, DATA
    if((data[0]!=COMMAND_WRITE_BUFFER) || (data[4]!=0xE2) || (data[5]!=0x30))
    {
        //Can't trust the number of bytes in this case
        return 65;
    }
    
    //Calculate sector
    start_byte = data[1];
    start_byte <<= 8;
    start_byte |= data[2];
    
    //Get number of bytes
    number_of_bytes = data[3];
    
    //Modify content of buffer 2, i.e. write to buffer
    fat_write_to_buffer(start_byte, number_of_bytes, &data[6]);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<61))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_WRITE_BUFFER;
        out_buffer[(*out_idx_ptr)++] = HIGH_BYTE(start_byte);
        out_buffer[(*out_idx_ptr)++] = LOW_BYTE(start_byte);
        out_buffer[(*out_idx_ptr)++] = number_of_bytes;
    }
    
    //Return actual command length
    return 6 + number_of_bytes;
}

static uint8_t _parse_file_copy(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x5A: Copy file. PParameters: uint8_t FileNumber, char[8] NewFileName, char[3] NewFileExtention, 0x54D9
    
    uint8_t file_number;
    uint8_t return_value;
    
    if((data[0]!=COMMAND_FILE_COPY) || (data[13]!=0x54) || (data[14]!=0xD9))
    {
        return 15;
    }
    
    //Save file number
    file_number = data[1];
    
    //Rename file
    return_value = fat_copy_file(file_number, &data[2], &data[10]);
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<62))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_FILE_COPY;
        out_buffer[(*out_idx_ptr)++] = file_number;
        out_buffer[(*out_idx_ptr)++] = return_value;
    }
    
    return 15;
}

static uint8_t _parse_settings_spi_mode(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x70: Change SPI mode. Parameters: uint8_t NewMode, 0x88E2
    
    if((data[0]!=COMMAND_SET_SPI_MODE) || (data[2]!=0x88) || (data[3]!=0xE2))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.spiMode = (externalMode_t) data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_SPI_MODE;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}

static uint8_t _parse_settings_spi_frequency(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x71: Change SPI frequency. Parameters: uint8_t NewFrequency, 0xAEA8
    
    
    if((data[0]!=COMMAND_SET_SPI_FREQUENCY) || (data[2]!=0xAE) || (data[3]!=0xA8))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.spiFrequency = (spiFrequency_t) data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_SPI_FREQUENCY;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}

static uint8_t _parse_settings_spi_polarity(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x72: Change SPI polarity. Parameters: uint8_t NewPolarity, 0x0DBB
    
    if((data[0]!=COMMAND_SET_SPI_POLARITY) || (data[2]!=0x0D) || (data[3]!=0xBB))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.spiFrequency = (spiFrequency_t) data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_SPI_POLARITY;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}
            
static uint8_t _parse_settings_i2c_mode(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x73: Change I2C mode. Parameters: uint8_t NewMode, 0xB6B9
    
    if((data[0]!=COMMAND_SET_I2C_MODE) || (data[2]!=0xB6) || (data[3]!=0xB9))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.i2cMode = (externalMode_t) data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_I2C_MODE;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}
    
static uint8_t _parse_settings_i2c_frequency(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x74: Change I2C frequency. Parameters: uint8_t NewFrequency, 0x4E03
    
    if((data[0]!=COMMAND_SET_I2C_FREQUENCY) || (data[2]!=0x4E) || (data[3]!=0x03))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.i2cFrequency = (i2cFrequency_t) data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_I2C_FREQUENCY;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}

static uint8_t _parse_settings_i2c_slaveModeSlaveAddress(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x75: Change I2C slave mode slave address. Parameters: uint8_t NewAddress, 0x88E2
    
    if((data[0]!=COMMAND_SET_I2C_SLAVE_MODE_ADDRESS) || (data[2]!=0x88) || (data[3]!=0xE2))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.i2cSlaveModeSlaveAddress = data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_I2C_SLAVE_MODE_ADDRESS;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}

static uint8_t _parse_settings_i2c_masterModeSlaveAddress(uint8_t *data, uint8_t *out_buffer, uint8_t *out_idx_ptr)
{
    //0x76: Change I2C master mode slave address. Parameters: uint8_t NewAddress, 0x540D
    
    if((data[0]!=COMMAND_SET_I2C_MASTER_MODE_ADDRESS) || (data[2]!=0x54) || (data[3]!=0x0D))
    {
        return 4;
    }
    
    //Save new setting
    os.communicationSettings.i2cMasterModeSlaveAddress = data[1];
    
    //Return confirmation if desired
    if(((*out_idx_ptr)>0) && ((*out_idx_ptr)<63))
    {
        out_buffer[(*out_idx_ptr)++] = COMMAND_SET_I2C_MASTER_MODE_ADDRESS;
        out_buffer[(*out_idx_ptr)++] = data[1];
    }
    
    return 4;
}

