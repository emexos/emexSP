#include "../include/text/text_utils.h"
#include "../drivers/keyboard/keyboard.h"
#include "../include/text/string_utils.h"
#include "../include/memory/memory.h"
#include "../include/boot.h"
#include <stdbool.h>

#define COMMAND_BUFFER_SIZE 256

static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;
static int prompt_start_row = 0;
static int prompt_start_col = 0;

static void process_command(const char* command);
static void command_help(void);
static void command_clear(void);
static void command_echo(const char* args);
static void command_keytest(void);
static void command_meminfo(void);
static void command_memtest(void);

void shell() {
    print("emexOS3 beta ", 0x0E);
    print("Type help\n", 0x07);

    // Initialize memory manager
    memory_init();


    // Enable cursor for shell input (nice blinking cursor)
    enable_cursor(14, 15);

    while (true) {
        print("> ", 0x0F);

        // Remember where the prompt starts
        prompt_start_row = get_cursor_row();
        prompt_start_col = get_cursor_col();

        // Update hardware cursor position
        update_cursor(prompt_start_row, prompt_start_col);

        // Read command
        buffer_pos = 0;
        command_buffer[0] = '\0';

        while (true) {
            if (keyboard_has_key()) {
                char key = keyboard_get_key();

                if (key == '\n') {
                    // Enter pressed - execute command
                    putchar('\n', COLOR_DEFAULT);
                    command_buffer[buffer_pos] = '\0';

                    if (buffer_pos > 0) {
                        process_command(command_buffer);
                    }
                    break;

                } else if (key == '\b') {
                    // Backspace pressed
                    if (buffer_pos > 0) {
                        buffer_pos--;
                        command_buffer[buffer_pos] = '\0';

                        // Handle cursor position properly
                        int current_row = get_cursor_row();
                        int current_col = get_cursor_col();

                        if (current_col > 0) {
                            // Simple case: move cursor back and clear character
                            putchar('\b', COLOR_DEFAULT);
                            update_cursor(get_cursor_row(), get_cursor_col());
                        } else {
                            // Handle wrapping to previous line
                            if (current_row > prompt_start_row) {
                                set_cursor_position(current_row - 1, 79);
                                putchar(' ', COLOR_DEFAULT);
                                set_cursor_position(current_row - 1, 79);
                            }
                        }
                    }

                } else if (key == '\t') {
                    // Tab completion could be implemented here
                    continue;

                } else if (key == 27) {
                    // Escape key - clear current line
                    // Go back to prompt start and clear everything after it
                    set_cursor_position(prompt_start_row, prompt_start_col);

                    // Clear the rest of the line(s)
                    while (buffer_pos > 0) {
                        putchar(' ', COLOR_DEFAULT);
                        buffer_pos--;
                    }

                    // Reset cursor to prompt start
                    set_cursor_position(prompt_start_row, prompt_start_col);
                    buffer_pos = 0;
                    command_buffer[0] = '\0';

                } else if (key >= 32 && key <= 126) {
                    // Printable character
                    if (buffer_pos < COMMAND_BUFFER_SIZE - 1) {
                        command_buffer[buffer_pos] = key;
                        buffer_pos++;
                        putchar(key, COLOR_DEFAULT);
                        // Update cursor position after typing
                        update_cursor(get_cursor_row(), get_cursor_col());
                    }
                }
            }

            // Small delay to prevent busy waiting
            for (volatile int i = 0; i < 1000; i++);
        }
    }
}

static void process_command(const char* command) {
    if (str_equals(command, "help")) {
        command_help();
    }
    else if (str_equals(command, "clear")) {
        command_clear();
    }
    else if (str_starts_with(command, "echo ")) {
        command_echo(command + 5); // Skip "echo "
    }
    else if (str_equals(command, "keytest")) {
        command_keytest();
    }
    else if (str_equals(command, "meminfo")) {
        command_meminfo();
    }
    else if (str_equals(command, "memtest")) {
        command_memtest();
    }
    else if (str_equals(command, "")) {
        // Empty command, do nothing
    }
    else {
        print("Unknown command: ", 0x0C);
        print(command, 0x0C);
        print("\nType 'help' for available commands\n", 0x07);
    }
}

static void command_help(void) {
    print("Available commands:\n", 0x0E);
    print("  help     - Show this help message\n", 0x07);
    print("  clear    - Clear the screen\n", 0x07);
    print("  echo     - Echo text to screen\n", 0x07);
    print("  keytest  - Test keyboard input\n", 0x07);
    print("  meminfo  - Show memory information\n", 0x07);
    print("  memtest  - Run memory allocation test\n", 0x07);
    print("\n", COLOR_DEFAULT);
}

static void command_clear(void) {
    clear(COLOR_DEFAULT);
}

static void command_echo(const char* args) {
    if (*args == '\0') {
        print("Usage: echo <text>\n", 0x0C);
        return;
    }

    print(args, 0x0A);
    print("\n", COLOR_DEFAULT);
}

static void command_keytest(void) {
    print("Keyboard Test Mode - Press keys to see their codes\n", 0x0E);
    print("Press ESC to exit\n\n", 0x07);

    // Disable cursor during keytest to avoid confusion
    disable_cursor();

    while (true) {
        if (keyboard_has_key()) {
            char key = keyboard_get_key();

            if (key == 27) { // ESC
                print("\nExiting keyboard test mode\n", 0x0E);
                // Re-enable cursor when returning to shell
                enable_cursor(14, 15);
                break;
            }

            print("Key: '", COLOR_DEFAULT);
            if (key >= 32 && key <= 126) {
                putchar(key, 0x0A);
            } else {
                print("?", 0x0C);
            }
            print("' ASCII: ", COLOR_DEFAULT);
            print_dec((uint64_t)key, 0x0B);
            print(" (0x", COLOR_DEFAULT);
            print_hex((uint64_t)key, 0x0B);
            print(")\n", COLOR_DEFAULT);
        }

        // Small delay
        for (volatile int i = 0; i < 10000; i++);
    }
}

static void command_meminfo(void) {
    print("Memory Information:\n", 0x0E);
    print("==================\n", 0x0E);

    print("Heap Usage:     ", COLOR_DEFAULT);
    print_dec(get_heap_usage(), 0x0B);
    print(" bytes\n", COLOR_DEFAULT);

    print("Free Memory:    ", COLOR_DEFAULT);
    print_dec(get_free_memory(), 0x0A);
    print(" bytes\n", COLOR_DEFAULT);

    print("Total Allocated:", COLOR_DEFAULT);
    print_dec(get_total_allocated(), 0x0D);
    print(" bytes\n", COLOR_DEFAULT);

    print("Total Freed:    ", COLOR_DEFAULT);
    print_dec(get_total_freed(), 0x09);
    print(" bytes\n", COLOR_DEFAULT);

    uint32_t heap_size = 0x400000; // 4MB
    uint32_t used_percent = (get_heap_usage() * 100) / heap_size;

    print("Heap Usage:     ", COLOR_DEFAULT);
    print_dec(used_percent, 0x0C);
    print("%\n", COLOR_DEFAULT);

    print("\n", COLOR_DEFAULT);
}

static void command_memtest(void) {
    print("Starting memory test...\n", 0x0E);

    if (memory_test()) {
        print("Memory test completed successfully!\n", 0x0A);
    } else {
        print("Memory test failed!\n", 0x0C);
    }

    print("\n", COLOR_DEFAULT);
}
