#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdint.h>

#define COLOR_DEFAULT 0x0F

void clear(unsigned char color);
void putchar(char c, unsigned char color);
void print(const char* str, unsigned char color);
void print_hex(uint64_t num, unsigned char color);
void print_dec(uint64_t num, unsigned char color);

// Cursor management functions
int get_cursor_row(void);
int get_cursor_col(void);
void set_cursor_position(int row, int col);

// VGA cursor control
void disable_cursor(void);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void update_cursor(int row, int col);

#endif
