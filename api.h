/* 
 * File:   api.h
 * Author: luke
 *
 * Created on 25. Juli 2018, 11:07
 */

#ifndef API_H
#define	API_H

/******************************************************************************
 * API description
 ******************************************************************************
 * 
 * Data requests. These must be sent as the first byte. Any commands may follow
 *  0x10: General status information
 *  0x11: First 2 lines of display content
 *  0x12: Last 2 lines of display content
 *  0x13: Bootloader details
 *  0x15: External communication configuration
 *  0x20: Echo (i.e. send back) all data received. Used to test connection.
 * 
 * Extended data requests. Only parameters may follow
 *  0x80: Get information for a specific file. Parameter: uint8_t FileNumber
 *  0x81: Find file. Parameter: char[8] FileName, char[3] FileExtention
 *  0x82: Read file. Parameters: uint8_t FileNumber, uint32_t StartByte
 *  0x83: Read buffer. Parameters: uint16_t StartByte
 * 
 * Single byte commands
 *  0x20: Reboot
 *  0x21: Reboot in bootloader mode
 *  0x22: Reboot in normal mode
 *  0x23: Jump to main program
 *  0x24: Suspend bootloader
 *  0x3C: Turn encoder CCW
 *  0x3D: Turn encoder CW
 *  0x3E: Press push button
 *  0x99: Stop parsing (there are no more commands in this buffer)
 * 
 * Multi byte commands (followed by a 16 bit constant to prevent unintended use)
 *  0x50: Resize file. Parameters: uint8_t FileNumber, uint32_t newFileSize, 0x4CEA
 *  0x51: Delete file. Parameters: uint8_t FileNumber, 0x66A0
 *  0x52: Create file. Parameters: char[8] FileName, char[3] FileExtention, uint32_t FileSize, 0xBD4F
 *  0x53: Rename file. Parameters: uint8_t FileNumber, char[8] NewFileName, char[3] NewFileExtention, 0x7E18
 *  0x54: Append to file. Parameters: uint8_t FileNumber, uint8_t NumberOfBytes, 0xFE4B, DATA
 *  0x55: Modify file. Parameters: uint8_t FileNumber, uint32_t StartByte, uint8_t NumerOfBytes, 0x0F9B, DATA
 *  0x56: Format drive. Parameters: none, 0xDA22
 *  0x57: Read file sector to buffer. Parameters: uint8_t file_number, uint16_t sector, 0x1B35
 *  0x58: Write buffer to file sector. Parameters: uint8_t file_number, uint16_t sector, 0x6A6D
 *  0x59: Modify buffer. Parameters: uint16_t StartByte, uint8_t NumerOfBytes, 0xE230, DATA
 *  0x5A: Copy file. PParameters: uint8_t FileNumber, char[8] NewFileName, char[3] NewFileExtention, 0x54D9
 *  0x70: Change SPI mode. Parameters: uint8_t NewMode, 0x88E2
 *  0x71: Change SPI frequency. Parameters: uint8_t NewFrequency, 0xAEA8
 *  0x72: Change SPI polarity. Parameters: uint8_t NewPolarity, 0x0DBB
 *  0x73: Change I2C mode. Parameters: uint8_t NewMode, 0xB6B9
 *  0x74: Change I2C frequency. Parameters: uint8_t NewFrequency, 0x4E03
 *  0x75: Change I2C slave mode slave address. Parameters: uint8_t NewAddress, 0x88E2
 *  0x76: Change I2C master mode slave address. Parameters: uint8_t NewAddress, 0x540D
 *  
 ******************************************************************************/

/******************************************************************************
 * Type definitions
 ******************************************************************************/

typedef enum
{
    DATAREQUEST_GET_COMMAND_RESPONSE = 0x00,
    DATAREQUEST_GET_STATUS = 0x10,
    DATAREQUEST_GET_DISPLAY_1 = 0x11,
    DATAREQUEST_GET_DISPLAY_2 = 0x12,
    DATAREQUEST_GET_BOOTLOADER_DETAILS = 0x13,
    DATAREQUEST_GET_CONFIGURATION = 0x15,
    DATAREQUEST_GET_ECHO = 0x20,
    DATAREQUEST_GET_FILE_DETAILS = 0x80,
    DATAREQUEST_FIND_FILE = 0x81,
    DATAREQUEST_READ_FILE = 0x82,
    DATAREQUEST_READ_BUFFER = 0x83
} apiDataRequest_t;

typedef enum
{
    COMMAND_REBOT = 0x20,
    COMMAND_REBOT_BOOTLOADER_MODE = 0x21,
    COMMAND_REBOT_NORMAL_MODE = 0x22,
    COMMAND_JUMP_TO_MAIN_PROGRAM = 0x23,
    COMMAND_SUSPEND_BOOTLOADER = 0x24,
    COMMAND_ENCODER_CCW = 0x3C,
    COMMAND_ENCODER_CW = 0x3D,
    COMMAND_ENCODER_PUSH = 0x3E,
    COMMAND_STOP_PARSING = 0x99,
    COMMAND_FILE_RESIZE = 0x50,
    COMMAND_FILE_DELETE = 0x51,
    COMMAND_FILE_CREATE = 0x52,
    COMMAND_FILE_RENAME = 0x53,
    COMMAND_FILE_APPEND = 0x54,
    COMMAND_FILE_MODIFY = 0x55,
    COMMAND_FORMAT_DRIVE = 0x56,
    COMMAND_SECTOR_TO_BUFFER = 0x57,
    COMMAND_BUFFER_TO_SECTOR = 0x58,
    COMMAND_WRITE_BUFFER = 0x59,
    COMMAND_FILE_COPY = 0x5A,    
    COMMAND_SET_SPI_MODE = 0x70,
    COMMAND_SET_SPI_FREQUENCY = 0x71,
    COMMAND_SET_SPI_POLARITY = 0x72,
    COMMAND_SET_I2C_MODE = 0x73,
    COMMAND_SET_I2C_FREQUENCY = 0x74,
    COMMAND_SET_I2C_SLAVE_MODE_ADDRESS = 0x75,
    COMMAND_SET_I2C_MASTER_MODE_ADDRESS = 0x76       
} apiCommand_t;


/******************************************************************************
 * Function prototypes
 ******************************************************************************/

void api_prepare(uint8_t *inBuffer, uint8_t *outBuffer);
void api_parse(uint8_t *inBuffer, uint8_t receivedDataLength, uint8_t *outBuffer);


#endif	/* API_H */

