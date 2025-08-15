#include "../../include/text/text_utils.h"
#include "../../include/memory/memory.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGAMEMORY ((volatile unsigned short*)0xB8000)

// Screen buffer for scrolling
#define SCROLL_BUFFER_LINES 200
static uint16_t scroll_buffer[SCROLL_BUFFER_LINES][VGA_WIDTH];
static int scroll_buffer_start = 0;
static int scroll_buffer_end = 0;
static int scroll_buffer_count = 0;
static int scroll_offset = 0; // 0 = showing current screen, positive = scrolled up

static int cursor_row = 0;
static int cursor_col = 0;
static bool scrolling_enabled = true;

// Internal functions
static void scroll_up_one_line(void);
static void save_line_to_buffer(int line);
static void restore_screen_from_buffer(void);
static void update_hardware_cursor(void);

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
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

    // Clear scroll buffer
    scroll_buffer_start = 0;
    scroll_buffer_end = 0;
    scroll_buffer_count = 0;
    scroll_offset = 0;
}

void putchar(char c, unsigned char color)
{
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;

        // Check if we need to scroll
        if (cursor_row >= VGA_HEIGHT)
        {
            if (scrolling_enabled)
            {
                scroll_up_one_line();
                cursor_row = VGA_HEIGHT - 1;
            }
            else
            {
                cursor_row = 0; // Wrap around if scrolling disabled
            }
        }

        update_hardware_cursor();
        return;
    }

    if (c == '\b')
    {
        // Backspace
        if (cursor_col > 0)
        {
            cursor_col--;
            VGAMEMORY[cursor_row * VGA_WIDTH + cursor_col] = (color << 8) | ' ';
        }
        update_hardware_cursor();
        return;
    }

    VGAMEMORY[cursor_row * VGA_WIDTH + cursor_col] = (color << 8) | c;

    cursor_col++;
    if (cursor_col >= VGA_WIDTH)
    {
        cursor_col = 0;
        cursor_row++;

        // Check if we need to scroll
        if (cursor_row >= VGA_HEIGHT)
        {
            if (scrolling_enabled)
            {
                scroll_up_one_line();
                cursor_row = VGA_HEIGHT - 1;
            }
            else
            {
                cursor_row = 0; // Wrap around if scrolling disabled
            }
        }
    }

    update_hardware_cursor();
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

static void scroll_up_one_line(void)
{
    // Save the top line to scroll buffer
    save_line_to_buffer(0);

    // Scroll all lines up by one
    for (int row = 0; row < VGA_HEIGHT - 1; row++)
    {
        for (int col = 0; col < VGA_WIDTH; col++)
        {
            VGAMEMORY[row * VGA_WIDTH + col] = VGAMEMORY[(row + 1) * VGA_WIDTH + col];
        }
    }

    // Clear the bottom line
    unsigned short blank = (COLOR_DEFAULT << 8) | ' ';
    for (int col = 0; col < VGA_WIDTH; col++)
    {
        VGAMEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;
    }

    // Reset scroll offset since we're showing current content
    scroll_offset = 0;
}

static void save_line_to_buffer(int line)
{
    if (line < 0 || line >= VGA_HEIGHT)
        return;

    // Copy line to scroll buffer
    for (int col = 0; col < VGA_WIDTH; col++)
    {
        scroll_buffer[scroll_buffer_end][col] = VGAMEMORY[line * VGA_WIDTH + col];
    }

    scroll_buffer_end = (scroll_buffer_end + 1) % SCROLL_BUFFER_LINES;

    if (scroll_buffer_count < SCROLL_BUFFER_LINES)
    {
        scroll_buffer_count++;
    }
    else
    {
        // Buffer is full, move start
        scroll_buffer_start = (scroll_buffer_start + 1) % SCROLL_BUFFER_LINES;
    }
}

static void restore_screen_from_buffer(void)
{
    if (scroll_offset == 0)
        return; // Already showing current screen

    // Calculate which lines to show
    int buffer_index = (scroll_buffer_end - scroll_offset + SCROLL_BUFFER_LINES) % SCROLL_BUFFER_LINES;

    for (int row = 0; row < VGA_HEIGHT; row++)
    {
        int source_index = (buffer_index + row) % SCROLL_BUFFER_LINES;

        if (source_index < scroll_buffer_count)
        {
            for (int col = 0; col < VGA_WIDTH; col++)
            {
                VGAMEMORY[row * VGA_WIDTH + col] = scroll_buffer[source_index][col];
            }
        }
        else
        {
            // Clear line if no data available
            unsigned short blank = (COLOR_DEFAULT << 8) | ' ';
            for (int col = 0; col < VGA_WIDTH; col++)
            {
                VGAMEMORY[row * VGA_WIDTH + col] = blank;
            }
        }
    }
}

static void update_hardware_cursor(void)
{
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;

    // Send low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    // Send high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void scroll_screen_up(int lines)
{
    if (lines <= 0 || scroll_buffer_count == 0)
        return;

    scroll_offset += lines;
    if (scroll_offset > scroll_buffer_count)
        scroll_offset = scroll_buffer_count;

    restore_screen_from_buffer();
}

void scroll_screen_down(int lines)
{
    if (lines <= 0)
        return;

    scroll_offset -= lines;
    if (scroll_offset < 0)
        scroll_offset = 0;

    if (scroll_offset == 0)
    {
        // Restore current screen
        // This is a bit complex - we need to rebuild the current screen
        // For now, just clear scroll offset
    }
    else
    {
        restore_screen_from_buffer();
    }
}

void set_cursor_position(int row, int col)
{
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH)
    {
        cursor_row = row;
        cursor_col = col;
        update_hardware_cursor();
    }
}

void get_cursor_position(int* row, int* col)
{
    if (row) *row = cursor_row;
    if (col) *col = cursor_col;
}

void enable_scrolling(bool enable)
{
    scrolling_enabled = enable;
}

bool is_scrolling_enabled(void)
{
    return scrolling_enabled;
}

int get_scroll_offset(void)
{
    return scroll_offset;
}

void reset_scroll(void)
{
    scroll_offset = 0;
    // Could restore current screen here if needed
}
