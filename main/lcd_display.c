#include "lcd_display.h"
#include <rom/ets_sys.h>
#include "I2C_lib.h"


#define PCF_ADDRESS 0x27

#define CMD_INIT_FUNCTION_SET   0x02

#define FN_SET_4_BIT        (1 << 5)
#define FN_SET_DATA_LENGTH  (1 << 4)
#define FN_SET_DISP_LINES   (1 << 3)
#define FN_SET_FONT         (1 << 2)

#define DISP_CTRL               (1 << 3)
#define DISP_CTRL_ON            (1 << 2)
#define DISP_CTRL_CURSOR_ON     (1 << 1)
#define DISP_CTRL_CURSOR_BLINK  (1 << 0)

#define CLEAR_DISPLAY   (1 << 0)

// HD44780U
// PCF8574A

typedef enum
{
    HD_RS = 0x01,          // P0
    HD_RW = (0x01 << 1),   // P1
    HD_E = (0x01 << 2),    // P2
    HD_BT = (0x01 << 3),   // P3
    HD_DB4 = (0x01 << 4),   // P4
    HD_DB5 = (0x01 << 5),   // P5
    HD_DB6 = (0x01 << 6),   // P6
    HD_DB7 = (0x01 << 7)    // P7
} HD_bitmap;

void write_i2c(uint8_t data)
{
    uint8_t value = data | HD_BT;
    i2c_write(PCF_ADDRESS, &value, 1);
}

// Blocks
void pulse_enable(uint8_t data) {
    write_i2c(data | HD_E) ;
    ets_delay_us(1);

    write_i2c(data & ~HD_E);
    ets_delay_us(50);
} 

void write_command_nibble(const uint8_t data)
{
    write_i2c(data);
    pulse_enable(data);
}

void send_command(uint8_t data, uint8_t mode)
{
    write_command_nibble((data & 0xF0) | mode);
    write_command_nibble(((data << 4) & 0xF0) | mode);
}

void function_set_4_bit()
{
    send_command(FN_SET_4_BIT, 0);
}

void display_control()
{
    send_command(DISP_CTRL | DISP_CTRL_ON | DISP_CTRL_CURSOR_ON, 0);
}

void clear_display()
{
    send_command(CLEAR_DISPLAY, 0);
}

// bool check_busy_flag()
// {

// }

// Blocks
void lcd_init()
{
    ets_delay_us(50 * 1000);
    uint8_t blv = 0x08;
    i2c_write(PCF_ADDRESS, &blv, 1);
    ets_delay_us(1000 * 1000);
    write_command_nibble(CMD_INIT_FUNCTION_SET << 4);
    ets_delay_us(4100);
    write_command_nibble(CMD_INIT_FUNCTION_SET << 4);
    ets_delay_us(100);
    write_command_nibble(CMD_INIT_FUNCTION_SET << 4);

    write_command_nibble(0x02 << 4);

    function_set_4_bit();
    display_control();
    clear_display();
    ets_delay_us(2000);

    send_command(0x04 | 0x02, 0);
    send_command(0x02, 0);
}

void lcd_clear_display()
{

}