

#include <stdint.h>
#include "hex.h"

static uint8_t hexCharToUint8(char c);
static uint8_t hexCharsToUint8(char c1, char c2);
static uint16_t hexCharsToUint16(char c1, char c2, char c3, char c4);

//Takes a single hexadecimal character and returns the corresponding value in the range [0,15]
static uint8_t hexCharToUint8(char c)
{
	uint8_t ascii = (uint8_t)c;

	//a,b,c,d,e,f
	if (ascii >= 97)
	{
		ascii -= 87;
	}
	//A,B,C,D,E,F
	else if (ascii >= 65)
	{
		ascii -= 55;
	}
	//0,1,2,3,4,5,6,7,8,9
	else
	{
		ascii -= 48;
	}
		
	if (ascii > 15)
	{
		return 0;
	}
	else
	{
		return ascii;
	}	
}

//Takes 2 single hexadecimal character and returns the corresponding value in the range [0,255]
static uint8_t hexCharsToUint8(char c1, char c2)
{
	uint8_t retVal = (hexCharToUint8(c1) << 4);
	retVal |= hexCharToUint8(c2);
	return retVal;
}

static uint16_t hexCharsToUint16(char c1, char c2, char c3, char c4)
{
	uint16_t retVal = (hexCharToUint8(c1) << 12);
	retVal |= (hexCharToUint8(c2) << 8);
	retVal |= (hexCharToUint8(c3) << 4);
	retVal |= hexCharToUint8(c4);
	return retVal;
}

//Parses the entry of a hex file starting at a given offset
uint32_t parseHexFileEntry(char *data, uint32_t offset, HexFileEntry_t *hexEntry)
{
	uint8_t i;

	//Check for start code
	if (data[offset] != ':')
	{
		return (uint32_t) RecordErrorStartCode;
	}

	//Get data length
	hexEntry->dataLength = hexCharsToUint8(data[offset + 1], data[offset + 2]);
	if (hexEntry->dataLength > 16)
	{
		return (uint32_t) RecordErrorDataTooLong;
	}
	
	//Get address
	hexEntry->address = hexCharsToUint16(data[offset + 3], data[offset + 4], data[offset + 5], data[offset + 6]);

	//Get record type
	hexEntry->recordType = (RecordType_t)hexCharsToUint8(data[offset + 7], data[offset + 8]);

	//Get data
	for (i = 0; i < hexEntry->dataLength; ++i)
	{
		hexEntry->data[i] = hexCharsToUint8(data[offset + 9 + i + i], data[offset + 10 + i + i]);
	}
	
	//Get checksum
	hexEntry->checksum = hexCharsToUint8(data[offset + 9 + i + i], data[offset + 10 + i + i]);

	//Calculate checksum check
	hexEntry->checksumCheck = hexEntry->dataLength;
	hexEntry->checksumCheck += (hexEntry->address >> 8);
	hexEntry->checksumCheck += (hexEntry->address & 0xFF);
	hexEntry->checksumCheck += hexEntry->recordType;
	for (i = 0; i < hexEntry->dataLength; ++i)
	{
		hexEntry->checksumCheck += hexEntry->data[i];
	}
	hexEntry->checksumCheck += hexEntry->checksum;

	//Throw an error if checksum does not match
	if (hexEntry->checksumCheck != 0)
	{
		return (uint32_t) RecordErrorChecksum;
	}

	//Find offset of the next entry
	if (hexEntry->recordType == RecordTypeEndOfFile)
	{
		//Indicate that the end of the file has been reached
		return 0;
	}
	else
	{
		offset += 11 + i + i;
		if (data[++offset] == ':')
		{
			return offset;
		}
		else if (data[++offset] == ':')
		{
			return offset;
		}
		else if (data[++offset] == ':')
		{
			return offset;
		}
		else
		{
			//There should be another record but there is not
			return (uint32_t) RecordErrorNoNextRecord;
		}
	}
}

//Parses an entire file determining the number of records and if the file is free of errors
void checkFile(char *data, FileCheckResult_t *checkResult)
{
	HexFileEntry_t hex_entry;
	uint32_t offset = 0;

	checkResult->number_of_entries = 0;
	checkResult->error = RecordErrorNoError;

	while(1)
	{
		offset = parseHexFileEntry(data, offset, &hex_entry);
		
		if (offset > (uint32_t) RecordErrorNoError)
		{
			checkResult->error = (RecordError_t)offset;
			return;
		}

		++checkResult->number_of_entries;

		if (offset == 0)
		{
			return;
		}
	}
}

