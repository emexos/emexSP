#include "../../include/text/text_utils.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGAMEMORY ((volatile unsigned short*)0xB8000)

static int cursor_row = 0;
static int cursor_col = 0;

void clear(unsigned char color)
{
    unsigned short blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        VGAMEMORY[i] = blank;
    }
    cursor_row = 0;
    cursor_col = 0;
}

void putchar(char c, unsigned char color)
{
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;
        return;
    }

    VGAMEMORY[cursor_row * VGA_WIDTH + cursor_col] = (color << 8) | c;

    cursor_col++;
    if (cursor_col >= VGA_WIDTH)
    {
        cursor_col = 0;
        cursor_row++;
    }

    if (cursor_row >= VGA_HEIGHT)
    {
        cursor_row = 0;
    }
}

void print(const char* str, unsigned char color)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        putchar(str[i], color);
    }
}

void print_hex(uint64_t num, unsigned char color)
{
    const char* hex_chars = "0123456789ABCDEF";
    char buffer[17];
    buffer[16] = '\0';

    int pos = 15;
    while (pos >= 0)
    {
        buffer[pos] = hex_chars[num & 0xF];
        num >>= 4;
        pos--;
        if (num == 0 && pos >= 0)
            break;
    }

    putchar('0', color);
    putchar('x', color);
    print(&buffer[pos + 1], color);
}

void print_dec(uint64_t num, unsigned char color)
{
    if (num == 0)
    {
        putchar('0', color);
        return;
    }

    char buffer[21];
    int pos = 20;
    buffer[pos] = '\0';

    while (num > 0)
    {
        buffer[--pos] = '0' + (num % 10);
        num /= 10;
    }

    print(&buffer[pos], color);
}
