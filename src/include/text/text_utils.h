#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#define COLOR_DEFAULT 0x0F

// Basic text output functions
void clear(unsigned char color);
void putchar(char c, unsigned char color);
void print(const char* str, unsigned char color);
void print_hex(uint64_t num, unsigned char color);
void print_dec(uint64_t num, unsigned char color);

// Scrolling functions
void scroll_screen_up(int lines);
void scroll_screen_down(int lines);
void enable_scrolling(bool enable);
bool is_scrolling_enabled(void);
int get_scroll_offset(void);
void reset_scroll(void);

// Cursor functions
void set_cursor_position(int row, int col);
void get_cursor_position(int* row, int* col);

#endif
