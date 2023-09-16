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

#define READ_BUSY_FLAG  (1 << 7)

#define RETURN_HOME (1 << 1)

#define SET_DDRAM_ADDR  (1 << 7)

#define CLEAR_DISPLAY   (1 << 0)

#define ROW_OFFSET (1 << 6)

static const char *TAG = "lcd_display";


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

uint8_t backlight_value = 0x00;

void write_i2c(uint8_t data)
{
    uint8_t value = data | backlight_value;
    ESP_ERROR_CHECK(i2c_write(PCF_ADDRESS, &value, 1));
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
    send_command(FN_SET_4_BIT | FN_SET_DISP_LINES, 0);
}

void display_control()
{
    send_command(DISP_CTRL | DISP_CTRL_ON | DISP_CTRL_CURSOR_ON | DISP_CTRL_CURSOR_BLINK, 0);
}

void lcd_clear_display()
{
    send_command(CLEAR_DISPLAY, 0);
}

void home()
{
    send_command(RETURN_HOME, 0);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    send_command(SET_DDRAM_ADDR | col | (row == 1 ? ROW_OFFSET : 0), 0);
}

void lcd_print(const char *sequence)
{
    home();
    for (uint8_t i = 0; i < 32; i++) {
        if (sequence[i] != '\0') {
            if (i == 16) {
                send_command(SET_DDRAM_ADDR | ROW_OFFSET, 0);
            }
            send_command(sequence[i], HD_RS);
        } else {
            break;
        }
    }
}

void lcd_print_position(const char *sequence, uint8_t row, uint8_t col)
{
    lcd_set_cursor(row, col);
    uint8_t start = col + (row * 16);
    for (uint8_t i = start; i < 32; i++) {
        if (sequence[i - start] != '\0') {
            if (i == 16 && start != 16) {
                send_command(SET_DDRAM_ADDR | ROW_OFFSET, 0);
            }
            send_command(sequence[i - start], HD_RS);
        } else {
            break;
        }
    }
}

uint8_t get_busy_flag()
{
    send_command(READ_BUSY_FLAG, HD_RW);
    uint8_t read_byte_1, read_byte_2;
    ESP_ERROR_CHECK(i2c_read(PCF_ADDRESS, &read_byte_1, 1));
    ESP_ERROR_CHECK(i2c_read(PCF_ADDRESS, &read_byte_2, 1));

    return read_byte_1 & HD_DB7;
}

// Blocks
void lcd_init()
{
    ets_delay_us(50 * 1000);
    // uint8_t blv = 0x08;
    // i2c_write(PCF_ADDRESS, &blv, 1);
    ets_delay_us(1000 * 1000);
    write_command_nibble(0x03 << 4);
    ets_delay_us(4100);
    write_command_nibble(0x03 << 4);
    ets_delay_us(100);
    write_command_nibble(0x03 << 4);

    write_command_nibble(0x02 << 4);

    function_set_4_bit();
    display_control();
    lcd_clear_display();
    ets_delay_us(2000);

    send_command(0x04 | 0x02, 0);
    send_command(0x02, 0);
    backlight_value = HD_BT;
    write_i2c(0);

    // lcd_print("I have a gun in my backpack.");
    // lcd_print_position("fonarfonarfonar", 0, 3);
    
}
