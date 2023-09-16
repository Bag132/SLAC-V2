#include "I2C_lib.h"

void lcd_init();

void lcd_clear_display();

void lcd_set_cursor(uint8_t row, uint8_t col);

void lcd_print(const char *sequence);

void lcd_print_position(const char *sequence, uint8_t row, uint8_t col);
