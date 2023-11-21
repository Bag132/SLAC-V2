#include "lcd_display.h"
#include <rom/ets_sys.h>
#include "I2C_lib.h"
#include "DS1307.h"

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


// HD44780U https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
// PCF8574A https://nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf

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
    send_command(DISP_CTRL | DISP_CTRL_ON, 0);
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

void lcd_display_time(const ds_time *time) 
{
    char time_str[16];
    char ampm[3];
    ampm[2] = '\0';
    uint8_t hours;
    if (time->hours < 12) {
        ampm[0] = 'a';
        ampm[1] = 'm';

        if (time->hours == 0) {
            hours = 12;
        } else {
            hours = time->hours;
        }
    } else if (time->hours >= 12){
        ampm[0] = 'p';
        ampm[1] = 'm';
        if (time->hours != 12) {
            hours = time->hours - 12;
        } else {
            hours = time->hours;
        }
    }

    char *day_str = "  ";
    switch (time->day_of_week) {
        case 1:
            day_str = "Sun";
            break;
        case 2:
            day_str = "Mon";
            break;
        case 3:
            day_str = "Tue";
            break;
        case 4:
            day_str = "Wed";
            break;
        case 5:
            day_str = "Thu";
            break;
        case 6:
            day_str = "Fri";
            break;
        case 7:
            day_str = "Sat";
    }

    sprintf(time_str, "%s %02d:%02d%s", day_str, hours, time->minutes, ampm);
    lcd_print_position(time_str, 0, 0);
}

void lcd_display_alarm_time()
{
    FILE *time_file = fopen("/spiffs/alarm_time.txt", "r");
    if (time_file == NULL) {
        ESP_LOGE(TAG, "Couldn't open alarm_time.txt");
        return;
    }

    char time_str[20];
    time_str[5] = '\0';

    int n = fread(time_str, sizeof(char), 5, time_file);
    fclose(time_file);

    if (n != 5) {
        ESP_LOGE(TAG, "Invalid time in alarm_time.txt '%s'", time_str);
        return;
    } else {
        uint8_t hours_tens = time_str[0] - 48;
        uint8_t hours = (time_str[1] - 48) + (10 * hours_tens);
        uint8_t minutes_tens = time_str[3] - 48;
        uint8_t minutes = (time_str[4] - 48) + (10 * minutes_tens);

        char ampm[3];
        ampm[2] = '\0';
        if (hours < 12) {
            ampm[0] = 'a';
            ampm[1] = 'm';

            if (hours == 0) {
                hours = 12;
            }
        } else if (hours >= 12){
            ampm[0] = 'p';
            ampm[1] = 'm';
            if (hours != 12) {
                hours = hours - 12;
            }
        }

        sprintf(time_str, "Alarm: %02d:%02d%s", hours, minutes, ampm);
        lcd_print_position(time_str, 1, 0);
    }
}

static uint8_t get_busy_flag()
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
}
