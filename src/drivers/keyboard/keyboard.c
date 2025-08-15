#include "keyboard.h"

// Port I/O functions - these need to be implemented or included
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Global keyboard state
static KeyboardBuffer kb_buffer = {0};
static KeyboardState kb_state = {0};
static uint8_t keyboard_read_data(void);
static uint8_t keyboard_read_status(void);

// US QWERTY scancode to ASCII conversion table
static const char scancode_to_char[] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`', 0,   '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// Shifted characters
static const char scancode_to_char_shift[] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void keyboard_init(void) {
    // Initialize buffer
    kb_buffer.head = 0;
    kb_buffer.tail = 0;
    kb_buffer.count = 0;

    // Initialize state
    kb_state.shift_pressed = false;
    kb_state.ctrl_pressed = false;
    kb_state.alt_pressed = false;
    kb_state.caps_lock = false;

    // Flush any pending keyboard data
    while (keyboard_read_status() & KEYBOARD_STATUS_OUTPUT_BUFFER_FULL) {
        keyboard_read_data();
    }
}

static uint8_t keyboard_read_data(void) {
    return inb(KEYBOARD_DATA_PORT);
}

static uint8_t keyboard_read_status(void) {
    return inb(KEYBOARD_STATUS_PORT);
}

static char scancode_to_ascii(uint8_t scancode, bool shift, bool caps) {
    if (scancode >= sizeof(scancode_to_char)) {
        return 0;
    }

    char c;
    if (shift) {
        c = scancode_to_char_shift[scancode];
    } else {
        c = scancode_to_char[scancode];
    }

    // Handle caps lock for letters
    if (caps && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (caps && c >= 'A' && c <= 'Z' && !shift) {
        // If caps is on but shift is not pressed, keep uppercase
    } else if (caps && c >= 'A' && c <= 'Z' && shift) {
        // If caps is on and shift is pressed, make lowercase
        c = c - 'A' + 'a';
    }

    return c;
}

void keyboard_handler(void) {
    uint8_t status = keyboard_read_status();

    if (!(status & KEYBOARD_STATUS_OUTPUT_BUFFER_FULL)) {
        return;
    }

    uint8_t scancode = keyboard_read_data();

    // Handle extended keys (0xE0 prefix)
    static bool extended_key = false;
    if (scancode == 0xE0) {
        extended_key = true;
        return;
    }

    // Handle key releases (bit 7 set)
    if (scancode & 0x80) {
        scancode &= 0x7F; // Remove release bit

        if (extended_key) {
            extended_key = false;
            return;
        }

        switch (scancode) {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                kb_state.shift_pressed = false;
                break;
            case KEY_CTRL:
                kb_state.ctrl_pressed = false;
                break;
            case KEY_ALT:
                kb_state.alt_pressed = false;
                break;
        }
        return;
    }

    // Handle key presses
    if (extended_key) {
        extended_key = false;

        // Handle extended keys
        switch (scancode) {
            case 0x53: // Delete key
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = KEY_DELETE_CHAR;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
                break;
            case 0x48: // Up arrow
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = KEY_UP_ARROW;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
                break;
            case 0x50: // Down arrow
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = KEY_DOWN_ARROW;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
                break;
            case 0x4B: // Left arrow
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = KEY_LEFT_ARROW;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
                break;
            case 0x4D: // Right arrow
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = KEY_RIGHT_ARROW;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
                break;
        }
        return;
    }

    switch (scancode) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            kb_state.shift_pressed = true;
            break;
        case KEY_CTRL:
            kb_state.ctrl_pressed = true;
            break;
        case KEY_ALT:
            kb_state.alt_pressed = true;
            break;
        case KEY_CAPS:
            kb_state.caps_lock = !kb_state.caps_lock;
            break;
        default: {
            // Convert scancode to ASCII
            char ascii = scancode_to_ascii(scancode, kb_state.shift_pressed, kb_state.caps_lock);

            // Handle special keys
            if (scancode == KEY_ENTER) {
                ascii = '\n';
            } else if (scancode == KEY_BACKSPACE) {
                ascii = '\b';
            } else if (scancode == KEY_TAB) {
                ascii = '\t';
            } else if (scancode == KEY_ESCAPE) {
                ascii = 27; // ESC character
            }

            // Add to buffer if it's a printable character or special key
            if (ascii != 0) {
                if (kb_buffer.count < KEYBOARD_BUFFER_SIZE - 1) {
                    kb_buffer.buffer[kb_buffer.head] = ascii;
                    kb_buffer.head = (kb_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    kb_buffer.count++;
                }
            }
            break;
        }
    }
}

bool keyboard_has_key(void) {
    // Check if there's data available first
    keyboard_handler();
    return kb_buffer.count > 0;
}

char keyboard_get_key(void) {
    if (!keyboard_has_key()) {
        return 0;
    }

    char key = kb_buffer.buffer[kb_buffer.tail];
    kb_buffer.tail = (kb_buffer.tail + 1) % KEYBOARD_BUFFER_SIZE;
    kb_buffer.count--;

    return key;
}

void keyboard_flush_buffer(void) {
    kb_buffer.head = 0;
    kb_buffer.tail = 0;
    kb_buffer.count = 0;
}
