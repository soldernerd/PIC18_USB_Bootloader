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
    io.bcm2835_spi_setClockDivider(io.BCM2835_SPI_CLOCK_DIVIDER_256);
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
    receive_data =ctypes.create_string_buffer(bytes(data_to_send), number_of_bytes)
    io.bcm2835_spi_transfern(receive_data, number_of_bytes)
    receive_data = list(receive_data)
    receive_data = [int.from_bytes(x, byteorder='big') for x in receive_data]
    return receive_data

def test_spi_communication(buffer_size, trials):
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
        print(t)
        print('Sent:    ', list_to_string(last_tx_data))
        print('Received:', list_to_string(rx_data))
        if(rx_data==last_tx_data):
            print('Status: Success')
        else:
            print('Status: FAILED')
            fails += 1
        delay_ms(50)
        
    print('Statistics: {0} trials, {1} successful, {2} failed'.format(trials, trials-fails, fails))
    
        
def finish():
    io.bcm2835_spi_end() 
    io.bcm2835_close()
    
send_data = [0xDE, 0x19, 0x34, 0x77, 0x11, 0xFF, 0x33, 0x3D]

init()
spi_init()
#test_spi()
#receive_data = spi_send_receive(send_data)
#print(list_to_string(receive_data))
test_spi_communication(64,100)
#spi_send_receive([1,2,,4,5,6,7,8])
finish()

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