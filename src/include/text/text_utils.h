#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdint.h>

#define COLOR_DEFAULT 0x0F

void clear(unsigned char color);
void putchar(char c, unsigned char color);
void print(const char* str, unsigned char color);
void print_hex(uint64_t num, unsigned char color);
void print_dec(uint64_t num, unsigned char color);

#endif
