#include "../include/text/text_utils.h"
#include "../drivers/keyboard/keyboard.h"
#include "../include/text/string_utils.h"
#include <stdbool.h>

#define COMMAND_BUFFER_SIZE 256

static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;

static void process_command(const char* command);
static void command_help(void);
static void command_clear(void);
static void command_echo(const char* args);
static void command_keytest(void);

void shell() {
    print("emexOS3 Shell v1.0\n", 0x0E);
    print("Type 'help' for available commands\n\n", 0x07);

    while (true) {
        print("> ", 0x0F);

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
                        // Move cursor back, print space, move back again
                        // This is a simple way to erase the character
                        print("\b \b", COLOR_DEFAULT);
                    }

                } else if (key == '\t') {
                    // Tab completion could be implemented here
                    continue;

                } else if (key == 27) {
                    // Escape key - clear current line
                    while (buffer_pos > 0) {
                        print("\b \b", COLOR_DEFAULT);
                        buffer_pos--;
                    }
                    command_buffer[0] = '\0';

                } else if (key >= 32 && key <= 126) {
                    // Printable character
                    if (buffer_pos < COMMAND_BUFFER_SIZE - 1) {
                        command_buffer[buffer_pos] = key;
                        buffer_pos++;
                        putchar(key, COLOR_DEFAULT);
                    }
                }
            }

            // Small delay to prevent busy waiting
            for (volatile int i = 0; i < 1000; i++);
        }
    }
}

// Forward declarations
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

    while (true) {
        if (keyboard_has_key()) {
            char key = keyboard_get_key();

            if (key == 27) { // ESC
                print("\nExiting keyboard test mode\n", 0x0E);
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
