#include "../../include/text/text_utils.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGAMEMORY ((volatile unsigned short*)0xB8000)

// VGA Cursor control ports
#define VGA_CRTC_INDEX_PORT 0x3D4
#define VGA_CRTC_DATA_PORT  0x3D5

static int cursor_row = 0;
static int cursor_col = 0;

// Port I/O functions
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Disable the blinking VGA cursor
void disable_cursor(void) {
    outb(VGA_CRTC_INDEX_PORT, 0x0A);  // Cursor Start Register
    outb(VGA_CRTC_DATA_PORT, 0x20);   // Set bit 5 to disable cursor
}

// Enable the VGA cursor
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(VGA_CRTC_INDEX_PORT, 0x0A);
    outb(VGA_CRTC_DATA_PORT, (inb(VGA_CRTC_DATA_PORT) & 0xC0) | cursor_start);

    outb(VGA_CRTC_INDEX_PORT, 0x0B);
    outb(VGA_CRTC_DATA_PORT, (inb(VGA_CRTC_DATA_PORT) & 0xE0) | cursor_end);
}

// Update VGA cursor position
void update_cursor(int row, int col) {
    uint16_t pos = row * VGA_WIDTH + col;

    outb(VGA_CRTC_INDEX_PORT, 0x0F);  // Low byte
    outb(VGA_CRTC_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CRTC_INDEX_PORT, 0x0E);  // High byte
    outb(VGA_CRTC_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

void clear(unsigned char color)
{
    unsigned short blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        VGAMEMORY[i] = blank;
    }
    cursor_row = 0;
    cursor_col = 0;

    // Disable cursor initially (shell will enable it when needed)
    disable_cursor();
}

static void scroll_up(void)
{
    // Move all lines up by one
    for (int row = 0; row < VGA_HEIGHT - 1; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            VGAMEMORY[row * VGA_WIDTH + col] = VGAMEMORY[(row + 1) * VGA_WIDTH + col];
        }
    }

    // Clear the last line
    unsigned short blank = (COLOR_DEFAULT << 8) | ' ';
    for (int col = 0; col < VGA_WIDTH; col++) {
        VGAMEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;
    }

    cursor_row = VGA_HEIGHT - 1;
}

void putchar(char c, unsigned char color)
{
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= VGA_HEIGHT) {
            scroll_up();
        }
        return;
    }

    if (c == '\b')
    {
        // Handle backspace
        if (cursor_col > 0) {
            cursor_col--;
            VGAMEMORY[cursor_row * VGA_WIDTH + cursor_col] = (color << 8) | ' ';
        }
        return;
    }

    VGAMEMORY[cursor_row * VGA_WIDTH + cursor_col] = (color << 8) | c;

    cursor_col++;
    if (cursor_col >= VGA_WIDTH)
    {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= VGA_HEIGHT) {
            scroll_up();
        }
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

// New functions for cursor management
int get_cursor_row(void) {
    return cursor_row;
}

int get_cursor_col(void) {
    return cursor_col;
}

void set_cursor_position(int row, int col) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        cursor_row = row;
        cursor_col = col;
        // Update hardware cursor position
        update_cursor(row, col);
    }
}
