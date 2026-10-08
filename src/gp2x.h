#ifndef GP2X_H
#define GP2X_H

// GP2X joystick button numbers (kept for the original GP2X code paths; the
// Android port never opens a joystick, so these events never arrive).
#define GP2X_BUTTON_UP              (0)
#define GP2X_BUTTON_DOWN            (4)
#define GP2X_BUTTON_LEFT            (2)
#define GP2X_BUTTON_RIGHT           (6)
#define GP2X_BUTTON_UPLEFT          (1)
#define GP2X_BUTTON_UPRIGHT         (7)
#define GP2X_BUTTON_DOWNLEFT        (3)
#define GP2X_BUTTON_DOWNRIGHT       (5)
#define GP2X_BUTTON_CLICK           (18)
#define GP2X_BUTTON_A               (12)
#define GP2X_BUTTON_B               (13)
#define GP2X_BUTTON_Y               (14)
#define GP2X_BUTTON_X               (15)
#define GP2X_BUTTON_L               (11)
#define GP2X_BUTTON_R               (10)
#define GP2X_BUTTON_START           (8)
#define GP2X_BUTTON_SELECT          (9)
#define GP2X_BUTTON_VOLUP           (16)
#define GP2X_BUTTON_VOLDOWN         (17)

// Dingoo buttons. On the Dingoo these were SDL 1.2 key codes. In this port
// they are private key codes: port.cpp translates touch controls, keyboards
// and gamepads into these, so the original game logic stays untouched.
#define DINGOO_BUTTON_UP            0x7F000001
#define DINGOO_BUTTON_DOWN          0x7F000002
#define DINGOO_BUTTON_RIGHT         0x7F000003
#define DINGOO_BUTTON_LEFT          0x7F000004
#define DINGOO_BUTTON_R             0x7F000005
#define DINGOO_BUTTON_L             0x7F000006
#define DINGOO_BUTTON_A             0x7F000007
#define DINGOO_BUTTON_B             0x7F000008
#define DINGOO_BUTTON_X             0x7F000009
#define DINGOO_BUTTON_Y             0x7F00000A
#define DINGOO_BUTTON_SELECT        0x7F00000B
#define DINGOO_BUTTON_START         0x7F00000C

#endif
