#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// PS/2 Keyboard ports
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Status register bits
#define KEYBOARD_STATUS_OUTPUT_BUFFER_FULL 0x01
#define KEYBOARD_STATUS_INPUT_BUFFER_FULL  0x02

// Special keys
#define KEY_ESCAPE     0x01
#define KEY_BACKSPACE  0x0E
#define KEY_TAB        0x0F
#define KEY_ENTER      0x1C
#define KEY_CTRL       0x1D
#define KEY_LSHIFT     0x2A
#define KEY_RSHIFT     0x36
#define KEY_ALT        0x38
#define KEY_SPACE      0x39
#define KEY_CAPS       0x3A

// Function keys
#define KEY_F1         0x3B
#define KEY_F2         0x3C
#define KEY_F3         0x3D
#define KEY_F4         0x3E
#define KEY_F5         0x3F
#define KEY_F6         0x40
#define KEY_F7         0x41
#define KEY_F8         0x42
#define KEY_F9         0x43
#define KEY_F10        0x44
#define KEY_F11        0x57
#define KEY_F12        0x58

// Special characters
#define KEY_DELETE_CHAR   0x7F  // DEL character
#define KEY_UP_ARROW      0x80  // Custom codes for arrow keys
#define KEY_DOWN_ARROW    0x81
#define KEY_LEFT_ARROW    0x82
#define KEY_RIGHT_ARROW   0x83

#define KEYBOARD_BUFFER_SIZE 256

typedef struct {
    char buffer[KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
    int count;
} KeyboardBuffer;

typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
} KeyboardState;



void keyboard_init(void);
void keyboard_handler(void);
bool keyboard_has_key(void);
char keyboard_get_key(void);
void keyboard_flush_buffer(void);

#endif
