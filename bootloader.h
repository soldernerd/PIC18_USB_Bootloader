/* 
 * File:   bootloader.h
 * Author: luke
 *
 * Created on 19. Februar 2018, 20:56
 */

#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

typedef enum ShortRecordError
{
	ShortRecordErrorStartCode = 0xF,
	ShortRecordErrorChecksum = 0xE,
	ShortRecordErrorNoNextRecord = 0xD,
	ShortRecordErrorDataTooLong = 0xC,
	ShortRecordErrorNoError = 0x0
} ShortRecordError_t;

const char bootloader_filename[9] = "FIRMWARE";
const char bootloader_extension[4] = "HEX";

void bootloader_run(void);
uint32_t bootloader_get_file_size(void);
uint16_t bootloader_get_entries(void);
ShortRecordError_t bootloader_get_error(void);


#endif	/* BOOTLOADER_H */

