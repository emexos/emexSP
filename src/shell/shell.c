#include "../include/text/text_utils.h"
#include "../drivers/keyboard/keyboard.h"
#include "../include/text/string_utils.h"
#include "../include/memory/memory.h"
#include <stdbool.h>

#define COMMAND_BUFFER_SIZE 256
#define MAX_COMMAND_HISTORY 50

// Command history structure
typedef struct {
    char commands[MAX_COMMAND_HISTORY][COMMAND_BUFFER_SIZE];
    int count;
    int current_index;
} CommandHistory;

static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;
static int cursor_pos = 0; // Position of cursor in command buffer
static CommandHistory history = {0};
static int history_browse_index = -1;
static char temp_command[COMMAND_BUFFER_SIZE]; // Store current command when browsing history

// Function declarations
static void process_command(const char* command);
static void command_help(void);
static void command_clear(void);
static void command_echo(const char* args);
static void command_keytest(void);
static void command_memory(void);
static void command_history_cmd(void);
static void add_to_history(const char* command);
static void handle_tab_completion(void);
static void handle_up_arrow(void);
static void handle_down_arrow(void);
static void handle_left_arrow(void);
static void handle_right_arrow(void);
static void handle_delete_key(void);
static void redraw_command_line(void);
static void clear_current_line(void);

void shell() {
    print("emexOS3 Shell v2.0\n", 0x0E);
    print("Type 'help' for available commands\n", 0x07);
    print("Use UP/DOWN arrows for command history, TAB for completion\n\n", 0x07);

    while (true) {
        print("> ", 0x0F);

        // Read command
        buffer_pos = 0;
        cursor_pos = 0;
        command_buffer[0] = '\0';
        history_browse_index = -1;

        while (true) {
            if (keyboard_has_key()) {
                char key = keyboard_get_key();

                if (key == '\n') {
                    // Enter pressed - execute command
                    putchar('\n', COLOR_DEFAULT);
                    command_buffer[buffer_pos] = '\0';

                    if (buffer_pos > 0) {
                        add_to_history(command_buffer);
                        process_command(command_buffer);
                    }
                    break;

                } else if (key == '\b') {
                    // Backspace pressed
                    if (cursor_pos > 0 && buffer_pos > 0) {
                        // Move characters after cursor back by one
                        for (int i = cursor_pos - 1; i < buffer_pos - 1; i++) {
                            command_buffer[i] = command_buffer[i + 1];
                        }
                        buffer_pos--;
                        cursor_pos--;
                        command_buffer[buffer_pos] = '\0';
                        redraw_command_line();
                    }

                } else if (key == KEY_DELETE_CHAR) {
                    // Delete key pressed
                    handle_delete_key();

                } else if (key == '\t') {
                    // Tab completion
                    handle_tab_completion();

                } else if (key == 27) {
                    // Escape key - clear current line
                    clear_current_line();

                } else if (key == KEY_UP_ARROW) {
                    handle_up_arrow();

                } else if (key == KEY_DOWN_ARROW) {
                    handle_down_arrow();

                } else if (key == KEY_LEFT_ARROW) {
                    handle_left_arrow();

                } else if (key == KEY_RIGHT_ARROW) {
                    handle_right_arrow();

                } else if (key >= 32 && key <= 126) {
                    // Printable character
                    if (buffer_pos < COMMAND_BUFFER_SIZE - 1) {
                        // Make room for new character
                        for (int i = buffer_pos; i > cursor_pos; i--) {
                            command_buffer[i] = command_buffer[i - 1];
                        }
                        command_buffer[cursor_pos] = key;
                        buffer_pos++;
                        cursor_pos++;
                        command_buffer[buffer_pos] = '\0';
                        redraw_command_line();
                    }
                }
            }

            // Small delay to prevent busy waiting
            for (volatile int i = 0; i < 1000; i++);
        }
    }
}

static void add_to_history(const char* command) {
    // Don't add empty commands or duplicate consecutive commands
    if (str_length(command) == 0) {
        return;
    }

    if (history.count > 0 && str_equals(command, history.commands[history.count - 1])) {
        return;
    }

    if (history.count < MAX_COMMAND_HISTORY) {
        str_copy(history.commands[history.count], command, COMMAND_BUFFER_SIZE);
        history.count++;
    } else {
        // Shift all commands up
        for (int i = 0; i < MAX_COMMAND_HISTORY - 1; i++) {
            str_copy(history.commands[i], history.commands[i + 1], COMMAND_BUFFER_SIZE);
        }
        str_copy(history.commands[MAX_COMMAND_HISTORY - 1], command, COMMAND_BUFFER_SIZE);
    }
}

static void handle_tab_completion(void) {
    // Simple tab completion - cycle through history that starts with current input
    static int tab_index = -1;
    static char original_input[COMMAND_BUFFER_SIZE];

    if (tab_index == -1) {
        // First tab press - save original input
        str_copy(original_input, command_buffer, COMMAND_BUFFER_SIZE);
        tab_index = history.count - 1;
    }

    // Find next matching command in history
    int start_index = tab_index;
    do {
        if (tab_index >= 0 && str_starts_with(history.commands[tab_index], original_input)) {
            // Found a match
            str_copy(command_buffer, history.commands[tab_index], COMMAND_BUFFER_SIZE);
            buffer_pos = str_length(command_buffer);
            cursor_pos = buffer_pos;
            redraw_command_line();
            tab_index--;
            return;
        }
        tab_index--;
        if (tab_index < 0) {
            tab_index = history.count - 1;
        }
    } while (tab_index != start_index);

    // No match found or cycled through all - reset to original
    str_copy(command_buffer, original_input, COMMAND_BUFFER_SIZE);
    buffer_pos = str_length(command_buffer);
    cursor_pos = buffer_pos;
    redraw_command_line();
    tab_index = -1;
}

static void handle_up_arrow(void) {
    if (history.count == 0) {
        return;
    }

    if (history_browse_index == -1) {
        // First time browsing - save current command
        str_copy(temp_command, command_buffer, COMMAND_BUFFER_SIZE);
        history_browse_index = history.count - 1;
    } else if (history_browse_index > 0) {
        history_browse_index--;
    }

    str_copy(command_buffer, history.commands[history_browse_index], COMMAND_BUFFER_SIZE);
    buffer_pos = str_length(command_buffer);
    cursor_pos = buffer_pos;
    redraw_command_line();
}

static void handle_down_arrow(void) {
    if (history_browse_index == -1) {
        return;
    }

    if (history_browse_index < history.count - 1) {
        history_browse_index++;
        str_copy(command_buffer, history.commands[history_browse_index], COMMAND_BUFFER_SIZE);
    } else {
        // Back to original command
        str_copy(command_buffer, temp_command, COMMAND_BUFFER_SIZE);
        history_browse_index = -1;
    }

    buffer_pos = str_length(command_buffer);
    cursor_pos = buffer_pos;
    redraw_command_line();
}

static void handle_left_arrow(void) {
    if (cursor_pos > 0) {
        cursor_pos--;
        // Move cursor visually (simple implementation)
        print("\b", COLOR_DEFAULT);
    }
}

static void handle_right_arrow(void) {
    if (cursor_pos < buffer_pos) {
        print(&command_buffer[cursor_pos], COLOR_DEFAULT);
        cursor_pos++;
    }
}

static void handle_delete_key(void) {
    if (cursor_pos < buffer_pos) {
        // Move characters after cursor back by one
        for (int i = cursor_pos; i < buffer_pos - 1; i++) {
            command_buffer[i] = command_buffer[i + 1];
        }
        buffer_pos--;
        command_buffer[buffer_pos] = '\0';
        redraw_command_line();
    }
}

static void redraw_command_line(void) {
    // Save cursor position
    int saved_row, saved_col;
    get_cursor_position(&saved_row, &saved_col);

    // Move to beginning of line and clear it
    set_cursor_position(saved_row, 0);
    for (int i = 0; i < 80; i++) {
        putchar(' ', COLOR_DEFAULT);
    }

    // Redraw prompt and command
    set_cursor_position(saved_row, 0);
    print("> ", 0x0F);
    print(command_buffer, COLOR_DEFAULT);

    // Position cursor correctly
    set_cursor_position(saved_row, 2 + cursor_pos);
}

static void clear_current_line(void) {
    while (buffer_pos > 0) {
        print("\b \b", COLOR_DEFAULT);
        buffer_pos--;
    }
    cursor_pos = 0;
    command_buffer[0] = '\0';
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
    else if (str_equals(command, "memory")) {
        command_memory();
    }
    else if (str_equals(command, "history")) {
        command_history_cmd();
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
    print("  memory   - Show memory information\n", 0x07);
    print("  history  - Show command history\n", 0x07);
    print("\nNavigation:\n", 0x0E);
    print("  UP/DOWN  - Browse command history\n", 0x07);
    print("  LEFT/RIGHT - Move cursor in command line\n", 0x07);
    print("  TAB      - Command completion from history\n", 0x07);
    print("  DEL      - Delete character at cursor\n", 0x07);
    print("  ESC      - Clear current line\n", 0x07);
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

static void command_memory(void) {
    if (is_memory_initialized()) {
        memory_stats_t stats = get_memory_stats();
        print("Memory Statistics:\n", 0x0E);
        print("  Total Heap:  ", COLOR_DEFAULT);
        print_dec(stats.total_heap, 0x0A);
        print(" bytes\n", COLOR_DEFAULT);
        print("  Allocated:   ", COLOR_DEFAULT);
        print_dec(stats.total_allocated, 0x0C);
        print(" bytes\n", COLOR_DEFAULT);
        print("  Free:        ", COLOR_DEFAULT);
        print_dec(stats.total_free, 0x0A);
        print(" bytes\n", COLOR_DEFAULT);
        print("  Allocations: ", COLOR_DEFAULT);
        print_dec(stats.allocation_count, 0x0B);
        print("\n", COLOR_DEFAULT);
    } else {
        print("Memory manager not initialized\n", 0x0C);
    }
}

static void command_history_cmd(void) {
    if (history.count == 0) {
        print("No command history available\n", 0x07);
        return;
    }

    print("Command History:\n", 0x0E);
    for (int i = 0; i < history.count; i++) {
        print("  ", COLOR_DEFAULT);
        print_dec(i + 1, 0x0B);
        print(": ", COLOR_DEFAULT);
        print(history.commands[i], 0x0A);
        print("\n", COLOR_DEFAULT);
    }
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
            } else if (key == KEY_UP_ARROW) {
                print("UP", 0x0A);
            } else if (key == KEY_DOWN_ARROW) {
                print("DOWN", 0x0A);
            } else if (key == KEY_LEFT_ARROW) {
                print("LEFT", 0x0A);
            } else if (key == KEY_RIGHT_ARROW) {
                print("RIGHT", 0x0A);
            } else if (key == KEY_DELETE_CHAR) {
                print("DEL", 0x0A);
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
