#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <Preferences.h>

// Пины джойстиков
#define JOY1_X 25
#define JOY1_Y 33 
#define JOY1_BTN 4
#define JOY2_X 34
#define JOY2_Y 35
#define JOY2_BTN 32

// Пины кнопок
#define BTN_A 0 //!
#define BTN_B 2 //!

// Пин датчика напряжения
#define VOLTAGE_SENSOR 32  // просто не используем пока что...

// Конфигурация
#define JOY_DEADZONE 50
#define JOY_THRESHOLD 2000
#define DEBOUNCE_DELAY 15

// Состояния ввода
enum ButtonState {
    RELEASED = 0,
    PRESSED = 1,
    JUST_PRESSED = 2,
    JUST_RELEASED = 3
};

// Структуры управления
struct Joystick {
    int x;
    int y;
    ButtonState button;
    unsigned long lastChange;
};

struct DigitalButton {
    ButtonState state;
    unsigned long lastChange;
};

// Внешние переменные (должны быть определены в input.cpp)
extern Joystick joy1;
extern Joystick joy2;
extern DigitalButton btn_up;
extern DigitalButton btn_down;
extern DigitalButton btn_left;
extern DigitalButton btn_right;
extern DigitalButton btn_lt;
extern DigitalButton btn_rt;

// Переменные для меню (должны быть определены в input.cpp)
extern unsigned long lastInputTime;
extern const unsigned long MENU_COOLDOWN;
extern bool CHANGES_BTN;

// Функции
void input_init();
void input_update();
void debounce(ButtonState& state, bool rawState, unsigned long& lastChange);
float read_battery_voltage();  // Добавлена функция измерения напряжения
int get_battery_percent();  // Добавлена функция вычисления заряда

#endif