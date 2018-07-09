/* 
 * File:   bootloader.h
 * Author: luke
 *
 * Created on 19. Februar 2018, 20:56
 */

#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#include "hex.h"

typedef enum ShortRecordError
{
	ShortRecordErrorStartCode = 0xF,
	ShortRecordErrorChecksum = 0xE,
	ShortRecordErrorNoNextRecord = 0xD,
	ShortRecordErrorDataTooLong = 0xC,
    ShortRecordErrorAddressRange = 0xB,        
	ShortRecordErrorNoError = 0x0
} ShortRecordError_t;

const char bootloader_filename[9] = "FIRMWARE";
const char bootloader_extension[4] = "HEX";

void bootloader_run(uint8_t timeslot);
uint32_t bootloader_get_file_size(void);
uint16_t bootloader_get_entries(void);
uint16_t bootloader_get_total_entries(void);
ShortRecordError_t bootloader_get_error(void);
uint16_t bootloader_get_flashPagesWritten(void);

//Functions that give access to last record
uint16_t bootloader_get_rec_dataLength(void);
uint16_t bootloader_get_rec_address(void);
RecordType_t bootloader_get_rec_recordType(void);
uint8_t bootloader_get_rec_data(uint8_t index);
uint8_t bootloader_get_rec_checksum(void);
uint8_t bootloader_get_rec_checksumCheck(void);





#endif	/* BOOTLOADER_H */

