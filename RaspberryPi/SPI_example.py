import ctypes
import random

import libbcm2835._bcm2835 as io
print('Library loaded')

pin_ss = io.RPI_GPIO_P1_24
pin_sclk = io.RPI_GPIO_P1_23
pin_miso = io.RPI_GPIO_P1_21
pin_mosi = io.RPI_GPIO_P1_19

dir_input = io.BCM2835_GPIO_FSEL_INPT
dir_output = io.BCM2835_GPIO_FSEL_OUTP

high = io.HIGH
low = io.LOW

pullup = io.BCM2835_GPIO_PUD_UP
pulldown = io.BCM2835_GPIO_PUD_DOWN
neither = io.BCM2835_GPIO_PUD_OFF

set_direction = io.bcm2835_gpio_fsel
set_pull = io.bcm2835_gpio_set_pud
set_pin = io.bcm2835_gpio_write

def delay_ms(ms):
    io.bcm2835_delay(ms)


def init():
    if io.bcm2835_init()==0:
        print('Module failed to initialize')
    else:
        print('Module sucessfully initialized')
        

def configure_pins():    
    set_direction = io.bcm2835_gpio_fsel
    set_pull = io.bcm2835_gpio_set_pud
    set_pin = io.bcm2835_gpio_write

    set_direction(pin_ss, dir_input)
    set_pull(pin_ss, neither)

    set_direction(pin_sclk, dir_input)
    set_pull(pin_sclk, neither)

    set_direction(pin_miso, dir_input)
    set_pull(pin_miso, neither)

    set_direction(pin_mosi, dir_input)
    set_pull(pin_mosi, neither)

    print('Pins configured')
    
def list_to_string(input):
    s = '['
    for v in input:
        s += '0x{0:02X}, '.format(v)
    s = s[:-2]
    return s+']'
    
    
def spi_init():
    if io.bcm2835_spi_begin()==0:
        print('SPI failed to initialize')
    else:
        print('SPI sucessfully initialized')
        
    io.bcm2835_spi_setBitOrder(io.BCM2835_SPI_BIT_ORDER_MSBFIRST);
    io.bcm2835_spi_setDataMode(io.BCM2835_SPI_MODE0);
    #io.bcm2835_spi_setClockDivider(io.BCM2835_SPI_CLOCK_DIVIDER_65536);
    #SPI_CLOCK_DIVIDER_128=128: about 2.0MHz
    io.bcm2835_spi_setClockDivider(io.BCM2835_SPI_CLOCK_DIVIDER_128);
    io.bcm2835_spi_chipSelect(io.BCM2835_SPI_CS0);
    io.bcm2835_spi_setChipSelectPolarity(io.BCM2835_SPI_CS0, io.LOW);
    
    print('SPI configured')

def test_spi():
    send_data = 0x10
    read_data = io.bcm2835_spi_transfer(send_data)
    print("Sent to SPI: 0x{0:2X}. Read back from SPI: 0x{1:2X}.".format(send_data, read_data))
    if not send_data == read_data:
      print("Do you have the loopback from MOSI to MISO connected?")
      
def spi_send_receive(data_to_send, number_of_bytes=None):
    if not number_of_bytes:
        number_of_bytes = len(data_to_send)    
    #print('Sending {0} bytes of data: {1}'.format(number_of_bytes, list_to_string(data_to_send)))
    receive_data =ctypes.create_string_buffer(bytes(data_to_send[:number_of_bytes]), number_of_bytes)
    
    #Wait until there is not activity on the bus
    io.bcm2835_gpio_fsel(pin_sclk, dir_input)
    count = 0
    sclk_state = io.bcm2835_gpio_lev(pin_sclk)
    while(True):
        last_sclk_state = sclk_state
        sclk_state = io.bcm2835_gpio_lev(pin_sclk)
        if last_sclk_state == sclk_state:
            count += 1
        else:
            count = 0
        if count == 50:
            break
        
    io.bcm2835_spi_begin()
    io.bcm2835_spi_transfern(receive_data, number_of_bytes)
    io.bcm2835_spi_end()
    
    receive_data = list(receive_data)
    receive_data = [int.from_bytes(x, byteorder='big') for x in receive_data]
    return receive_data

def reboot():
    tx_data = [0x10, 0x20]
    spi_send_receive(tx_data)
    print('Reboot command sent')

def reboot_bootloader_mode():
    tx_data = [0x10, 0x21]
    spi_send_receive(tx_data)
    print('Reboot in bootloader mode command sent')

def reboot_normal_mode():
    tx_data = [0x10, 0x22]
    spi_send_receive(tx_data)
    print('Reboot in normal mode command sent')

def jump_to_main_program():
    tx_data = [0x10, 0x23]
    spi_send_receive(tx_data)
    print('Jump to main program command sent')

def turn_counter_clockwise():
    tx_data = [0x10, 0x3C]
    spi_send_receive(tx_data)
    print('Encoder turned counter-clockwise')

def turn_clockwise():
    tx_data = [0x10, 0x3D]
    spi_send_receive(tx_data)
    print('Encoder turned clockwise')

def press_pushbutton():
    tx_data = [0x10, 0x3E]
    spi_send_receive(tx_data)
    print('Pushbutton pressed')

def get_file_info(file_number):
    tx_data = [0x80, (file_number & 0b00111111)]
    spi_send_receive(tx_data)
    tx_data = [0x10]
    received_data = spi_send_receive(tx_data, 37)
    if not received_data[:3] == [0x80, 0xC1, 0x25]:
        print('Signature of data package is incorrect:', received_data[:3])
    print(received_data)
    print('File number: {0}'.format(received_data[3]))
    print('Return code: {0}'.format(received_data[4]))
    file_name = ''.join([chr(x) for x in received_data[5:13] if not x==0x20])
    extention = ''.join([chr(x) for x in received_data[13:16] if not x==0x20])
    print('File name: {0}.{1}'.format(file_name, extention))
    file_size = 2**24*received_data[36] + 2**16*received_data[35] + 2**8*received_data[34] + received_data[33]
    print('File size: {0} bytes'.format(file_size))
    
def list_files():
    for file_number in range(1, 63):
        tx_data = [0x80, (file_number & 0b00111111)]
        spi_send_receive(tx_data)
        tx_data = [0x10]
        received_data = spi_send_receive(tx_data, 37)
        if received_data[4] == 0:
            file_name = ''.join([chr(x) for x in received_data[5:13] if not x==0x20])
            extention = ''.join([chr(x) for x in received_data[13:16] if not x==0x20])
            file_size = 2**24*received_data[36] + 2**16*received_data[35] + 2**8*received_data[34] + received_data[33]
            print('File #{0}: {1}.{2}, {3} bytes'.format(file_number, file_name, extention, file_size))



def test_spi_communication(buffer_size, trials):
    print('\nTesting SPI Communication')
    print('-------------------------')
    fails = 0
    tx_data = [0x20]
    for i in range(buffer_size-1):
        tx_data.append(random.randint(0,255))
    spi_send_receive(tx_data)
    delay_ms(50)
    for t in range(trials):
        last_tx_data = tx_data
        tx_data = [0x20]
        for i in range(buffer_size-1):
            tx_data.append(random.randint(0,255))
        rx_data = spi_send_receive(tx_data)
        if(rx_data==last_tx_data):
            print(t, 'Status: Success')
        else:
            print(t, 'Status: FAILED')
            print('TX:', last_tx_data)
            print('RX:', rx_data)
            fails += 1
        delay_ms(50)  
    print('Statistics: {0} trials, {1} successful, {2} failed\n'.format(trials, trials-fails, fails))
    
        
def finish():
    io.bcm2835_spi_end() 
    io.bcm2835_close()
    
def get_status():
    print('\nStatus Information')
    print('------------------')
    send_data = [0x10]
    spi_send_receive(send_data)
    received_data = spi_send_receive(send_data, 44)
    if not received_data[:3] == [0x10, 0xC1, 0x25]:
        print('Signature of data package is incorrect:', received_data[:3])
    print('Flash is busy:', bool(received_data[3]))
    print('Bootloader version: {0}.{1}.{2}'.format(received_data[4], received_data[5], received_data[6]))
    print('User interface status: {0}'.format(received_data[7]))
    print('Encoder count: {0}'.format(received_data[8]))
    print('Push button count: {0}'.format(received_data[9]))
    print('Time slot: {0}'.format(received_data[10]))
    print('Done: {0}'.format(bool(received_data[11])))
    print('Bootloader mode: {0}'.format(received_data[12]))
    print('Display mode {0}\n'.format(received_data[13]))
    
def get_bootloader_details():
    print('\nBootloader Details')
    print('------------------')
    send_data = [0x13]
    spi_send_receive(send_data)
    received_data = spi_send_receive(send_data, 64)
    if not received_data[:3] == [0x13, 0xC1, 0x25]:
        print('Signature of data package is incorrect:', received_data[:3])
    file_size = 2**24*received_data[3] + 2**16*received_data[4] + 2**8*received_data[5] + received_data[6]
    print('File size: {0}'.format(file_size))
    entries = 2**8*received_data[7] + received_data[8]
    print('Current record: {0}'.format(entries))
    total_entries = 2**8*received_data[9] + received_data[10]
    print('Total number of records: {0}'.format(total_entries))
    print('Error: {0}'.format(received_data[11]))
    pages = 2**8*received_data[12] + received_data[13]
    print('Flash pages written: {0}'.format(pages))
    pages = 2**8*received_data[12] + received_data[13]
    print('Flash pages written: {0}'.format(pages))
    data_length = 2**8*received_data[14] + received_data[15]
    print('Length of last record: {0}'.format(data_length))
    address = 2**8*received_data[16] + received_data[17]
    print('Address of last record: {0}'.format(data_length))
    print('Record type of last record: {0}'.format(received_data[18]))
    print('Checksum of last record: {0}'.format(received_data[19]))
    print('Checksum check of last record: {0}'.format(received_data[20]))
    if data_length>43:
        data_length = 43
    print('Data or last record: {0}'.format(received_data[21:21+data_length]))
    
def read_display():
    send_data = [0x11]
    spi_send_receive(send_data)
    send_data = [0x12]
    delay_ms(20)
    received_data_1 = spi_send_receive(send_data, 44)
    if not received_data_1[:3] == [0x11, 0xC1, 0x25]:
        print('Signature of first data package is incorrect:', received_data_1[:3])
    delay_ms(20)
    received_data_2 = spi_send_receive(send_data, 44)
    if not received_data_2[:3] == [0x12, 0xC1, 0x25]:
        print('Signature of second data package is incorrect:', received_data_2[:3])    
    line_1 = ''.join([chr(x) for x in received_data_1[3:23]])
    line_2 = ''.join([chr(x) for x in received_data_1[23:43]])
    line_3 = ''.join([chr(x) for x in received_data_2[3:23]])
    line_4 = ''.join([chr(x) for x in received_data_2[23:43]])
    print('\n+----------------------+')
    print('| ' + line_1 + ' |')
    print('| ' + line_2 + ' |')
    print('| ' + line_3 + ' |')
    print('| ' + line_4 + ' |')
    print('+----------------------+\n')

def resize_file(file_number, new_file_size):
    #0x50: Resize file. Parameters: uint8_t FileNumber, uint32_t newFileSize, 0x4CEA
    size = [new_file_size>>24, (new_file_size>>16)&0xFF, (new_file_size>>8)&0xFF, new_file_size&0xFF]
    tx_data = [0x10, 0x50, file_number] + size + [0x4C, 0xEA]
    spi_send_receive(tx_data)
    print('File number {0} resized to {1} bytes'.format(file_number, new_file_size))

def delete_file(file_number):
    #0x51: Delete file. Parameters: uint8_t FileNumber, 0x66A0
    tx_data = [0x10, 0x51, file_number, 0x66, 0xA0]
    spi_send_receive(tx_data)
    print('File {0} deleted'.format(file_number))

def create_file(file_name, extention, file_size):
    #0x52: Create file. Parameters: char[8] FileName, char[3] FileExtention, uint32_t FileSize, 0xBD4F
    size = [file_size>>24, (file_size>>16)&0xFF, (file_size>>8)&0xFF, file_size&0xFF]
    tx_data = [0x10, 0x52]
    tx_data += [ord(c.upper()) for c in file_name[:8]]
    while len(tx_data) < 10:
        tx_data.append(ord(' '))
    tx_data += [ord(c.upper()) for c in extention[:3]]
    while len(tx_data) < 13:
        tx_data.append(ord(' '))
    tx_data += size
    tx_data += [0xBD, 0x4F]
    spi_send_receive(tx_data)
    print('File {0}.{1} created ({2} bytes)'.format(file_name, extention, file_size))

def rename_file(file_number, file_name, extention):
    #0x53: Rename file. Parameters: uint8_t FileNumber, char[8] NewFileName, char[3] NewFileExtention, 0x7E18
    tx_data = [0x10, 0x53, file_number]
    tx_data += [ord(c.upper()) for c in file_name[:8]]
    while len(tx_data) < 11:
        tx_data.append(ord(' '))
    tx_data += [ord(c.upper()) for c in extention[:3]]
    while len(tx_data) < 14:
        tx_data.append(ord(' '))
    tx_data += [0x7E, 0x18]
    spi_send_receive(tx_data)
    print('File number {0} renamed to {1}.{2}'.format(file_number, file_name, extention))

def append_to_file(file_number, data):
    #0x54: Append to file. Parameters: uint8_t FileNumber, uint8_t NumberOfBytes, 0xFE4B, DATA
    data = data[:58]
    number_of_bytes = len(data)
    tx_data = [0x10, 0x54, file_number, number_of_bytes, 0xFE, 0x4B]
    tx_data += [ord(c) for c in data]
    spi_send_receive(tx_data)
    print('{0} bytes appended to file number {1}'.format(number_of_bytes, file_number))
     
def modify_file(file_number, start_byte, data):
    #0x55: Modify file. Parameters: uint8_t FileNumber, uint32_t StartByte, uint8_t NumerOfBytes, 0x0F9B, DATA
    data = data[:54]
    number_of_bytes = len(data)
    start_bytes = [start_byte>>24, (start_byte>>16)&0xFF, (start_byte>>8)&0xFF, start_byte&0xFF]
    tx_data = [0x10, 0x55, file_number] + start_bytes + [number_of_bytes, 0x0F, 0x9B]
    tx_data += [ord(c) for c in data]
    spi_send_receive(tx_data)
    print('Modified file number {0} starting from byte {1}'.format(file_number, start_byte))

def format_drive():
    tx_data = [0x10, 0x56, 0xDA, 0x22]
    spi_send_receive(tx_data)
    print('Drive formated')

def write_file(local_file_name, remote_file_name):
    file_name, extention = remote_file_name.split('.')
    file_name = file_name[:8].upper()
    extention = extention[:3].upper()
    with open(local_file_name, 'r') as f:
        char_array = []
        for line in f:
            char_array += [c for c in line]
        size = len(char_array)
    #Create file of correct size
    create_file(file_name, extention, size)
    #Get number of that file
    delay_ms(50)
    file_number = find_file(remote_file_name)
    if file_number == 0:
        print('File not found')
        return
    #Modify content of file
    position = 0
    number_of_sectors = (size+511) // 512 
    for sector in range(number_of_sectors):
        for modify in range(9):
            offset = 57 * modify
            length = min(57, size-position, 512-offset)
            if length > 0:
                modify_buffer(offset, char_array[position:position+length])
                delay_ms(20)
                position += length
        buffer_to_file_sector(file_number, sector)
        delay_ms(50)

def print_file(local_file_name, remote_file_name):
    file_name, extention = remote_file_name.split('.')
    file_name = file_name[:8].upper()
    extention = extention[:3].upper()
    with open(local_file_name, 'r') as f:
        lines = [[c for c in x] for x in f]
        print(lines)
        """
        char_array = []
        for line in f:
            char_array += [c for c in line]
            #char_array += ['\r', '\n']
            #char_array += ['\n']
        size = len(char_array)
        """

def find_file(file_name):
    #0x81: Find file. Parameter: char[8] FileName, char[3] FileExtention
    name, extention = file_name.split('.')
    tx_data = [0x81]
    tx_data += [ord(c.upper()) for c in name[:8]]
    while len(tx_data) < 9:
        tx_data.append(ord(' '))
    tx_data += [ord(c.upper()) for c in extention[:3]]
    while len(tx_data) < 12:
        tx_data.append(ord(' '))
    received_data = spi_send_receive(tx_data)
    #print('TX:', tx_data)
    #print('RX:', received_data)
    #delay_ms(5)
    tx_data = [0x20]
    received_data = spi_send_receive(tx_data, 4)
    #print('RX:', received_data)
    if not received_data[:3] == [0x81, 0xC1, 0x25]:
        print('Signature of first data package is incorrect:', received_data[:3])
        return 0
    else:
        print('File number of file {0}: {1}'.format(file_name, received_data[3]))
        return received_data[3]

def file_sector_to_buffer(file_number, sector_number):
    #0x57: Read file sector to buffer. Parameters: uint8_t file_number, uint16_t sector, 0x1B35
    sector_numbers = [(sector_number>>8)&0xFF, sector_number&0xFF]
    tx_data = [0x10, 0x57, file_number] + sector_numbers + [0x1B, 0x35]
    spi_send_receive(tx_data)
    print('Sector {0} of file {1} copied to buffer'.format(sector_number, file_number))

def buffer_to_file_sector(file_number, sector_number):
    #0x58: Write buffer to file sector. Parameters: uint8_t file_number, uint16_t sector, 0x6A6D
    sector_numbers = [(sector_number>>8)&0xFF, sector_number&0xFF]
    tx_data = [0x10, 0x58, file_number] + sector_numbers + [0x6A, 0x6D]
    spi_send_receive(tx_data)
    print('Buffer copied to sector {0} of file {1}'.format(sector_number, file_number))

def modify_buffer(start_byte, data):
    #0x59: Modify buffer. Parameters: uint16_t StartByte, uint8_t NumerOfBytes, 0xE230, DATA
    data = data[:57]
    start_bytes = [(start_byte>>8)&0xFF, start_byte&0xFF]
    number_of_bytes = len(data)
    tx_data = [0x10, 0x59] + start_bytes + [number_of_bytes, 0xE2, 0x30]
    tx_data += [ord(c) for c in data]
    spi_send_receive(tx_data)
    print('Buffer modified starting from byte {0}'.format(start_byte))


init()
spi_init()
#test_spi_communication(64,100)
#get_status()
#read_display()
#press_pushbutton()
#get_bootloader_details()
#reboot()
#reboot_bootloader_mode()
#reboot_normal_mode()

#format_drive()
delay_ms(200)

#get_file_info(1);
#list_files()
delay_ms(50)
#delete_file(1)
#create_file('larger', 'txt', 766000)
#create_file('test', 'txt', 0)
delay_ms(50)
file_txt = find_file('file.txt')
test_txt = find_file('test.txt')
#modify_file(file_txt, 0, 'Just modified')
delay_ms(50)
#file_sector_to_buffer(test_txt, 0)
delay_ms(50)
#modify_buffer(2, 'Welcome to buffered file access')
delay_ms(50)
#buffer_to_file_sector(test_txt, 0)
#delete_file(filenbr)
#resize_file(filenbr, 0);
#rename_file(zero, 'BETA', 'HEX')
#append_to_file(filenbr, 'This is just a test')
#modify_file(filenbr, 5, 'This is more than just a stupid test')
#resize_file(zero, 5999);
#delay_ms(500)

#delay_ms(100)
"""
fw_hex = find_file('firmware.hex')
if not fw_hex == 255:
    delete_file(fw_hex)
"""

write_file('BootloaderTest.hex', 'bootldr.hex')
#write_file('SolarCharger_RevE.hex', 'firmware.hex')

#list_files()
#test_spi_communication(64,100)
#spi_send_receive([1,2,,4,5,6,7,8])
#finish()

#Configure pin 24 as input
#io.bcm2835_gpio_fsel(io.RPI_GPIO_P1_24, io.BCM2835_GPIO_FSEL_INPT)

#Configure pin 24 as output
#io.bcm2835_gpio_fsel(io.RPI_GPIO_P1_24, io.BCM2835_GPIO_FSEL_OUTP)

#Set output
#io.bcm2835_gpio_write(io.RPI_GPIO_P1_24, io.HIGH)
#io.bcm2835_gpio_write(io.RPI_GPIO_P1_24, io.LOW)

#Set pullup / pulldown resistors
#io.bcm2835_gpio_set_pud(io.RPI_GPIO_P1_24, io.BCM2835_GPIO_PUD_UP)
#io.bcm2835_gpio_set_pud(io.RPI_GPIO_P1_24, io.BCM2835_GPIO_PUD_DOWN)
#io.bcm2835_gpio_set_pud(io.RPI_GPIO_P1_24, io.BCM2835_GPIO_PUD_OFF)