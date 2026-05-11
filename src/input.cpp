#include "input.h"
#include <Arduino.h>
#include <Preferences.h>

// Определение глобальных переменных
Joystick joy1 = {2048, 2048, RELEASED, 0};
Joystick joy2 = {2048, 2048, RELEASED, 0};
DigitalButton btn_up = {RELEASED, 0};
DigitalButton btn_down = {RELEASED, 0};
DigitalButton btn_left = {RELEASED, 0};
DigitalButton btn_right = {RELEASED, 0};
DigitalButton btn_lt = {RELEASED, 0};
DigitalButton btn_rt = {RELEASED, 0};
unsigned long lastInputTime = 0;
const unsigned long MENU_COOLDOWN = 200; 
bool CHANGES_BTN = true; 

// Константы для фильтрации джойстиков
const int JOY_DEADZONE1 = 500;          // Мертвая зона для фильтрации шума 
const int JOY_SMOOTHING_SAMPLES = 15;  // Количество выборок для усреднения
 
// Буферы для фильтрации джойстиков
int joy1_x_buffer[JOY_SMOOTHING_SAMPLES] = {0};
int joy1_y_buffer[JOY_SMOOTHING_SAMPLES] = {0};
int joy2_x_buffer[JOY_SMOOTHING_SAMPLES] = {0};
int joy2_y_buffer[JOY_SMOOTHING_SAMPLES] = {0};
int buffer_index = 0;

// Функция антидребезга для кнопок
void debounce(ButtonState& state, bool rawState, unsigned long& lastChange) {
    unsigned long currentTime = millis();
    
    switch(state) {
        case RELEASED:
            if(rawState) {
                state = JUST_PRESSED; // НИКОГДА НЕ ИСПОЛЬЗУЙ !!!
                lastChange = currentTime;
            }
            break;
            
        case JUST_PRESSED:
            if(currentTime - lastChange >= DEBOUNCE_DELAY) {
                state = PRESSED; // ИСПОЛЬЗУЙ ВМЕСТО JUST_PRESSED !!!
            } else if(!rawState) {
                state = RELEASED;
            }
            break;
            
        case PRESSED:
            if(!rawState) {
                state = JUST_RELEASED;
                lastChange = currentTime;
            }
            break;
            
        case JUST_RELEASED:
            if(currentTime - lastChange >= DEBOUNCE_DELAY) {
                state = RELEASED;
            } else if(rawState) {
                state = PRESSED;
            }
            break;
    }
}

// Фильтрация значений джойстика
int filterJoystickValue(int buffer[], int newValue) {
    // Обновляем буфер
    buffer[buffer_index] = newValue;
    
    // Вычисляем среднее
    long sum = 0;
    for(int i = 0; i < JOY_SMOOTHING_SAMPLES; i++) {
        sum += buffer[i];
    }
    int average = sum / JOY_SMOOTHING_SAMPLES;
    
    // Применяем мертвую зону
    if(abs(newValue - average) < JOY_DEADZONE1) {
        return average;  // Возвращаем усредненное значение, если в пределах мертвой зоны
    }
    
    return newValue;  // Возвращаем реальное значение при значительном движении
}

void input_init() {
    // Настройка ADC для более стабильного чтения (если поддерживается платой)
    analogReadResolution(12);  // Для плат с 12-битным ADC
    
    // Инициализация аналоговых входов (джойстики)
    pinMode(JOY1_BTN, INPUT_PULLUP);
    pinMode(JOY2_BTN, INPUT_PULLUP);
    
    // Инициализация цифровых кнопок
    pinMode(BTN_A, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    pinMode(JOY1_X, INPUT);
    pinMode(JOY1_Y, INPUT);
    pinMode(JOY2_X, INPUT);
    pinMode(JOY2_Y, INPUT);
    //pinMode(VOLTAGE_SENSOR, INPUT);
    
    // Инициализация буферов фильтрации
    int initial1_x = analogRead(JOY1_X);
    int initial1_y = analogRead(JOY1_Y);
    int initial2_x = analogRead(JOY2_X);
    int initial2_y = analogRead(JOY2_Y);
    
    for(int i = 0; i < JOY_SMOOTHING_SAMPLES; i++) {
        joy1_x_buffer[i] = initial1_x;
        joy1_y_buffer[i] = initial1_y;
        joy2_x_buffer[i] = initial2_x;
        joy2_y_buffer[i] = initial2_y;
    }
    
    //adcAttachPin(VOLTAGE_SENSOR);  // привязываем пин к АЦП
}

void input_update() {
    unsigned long currentTime = millis();
    
    // Обновление индекса буфера
    buffer_index = (buffer_index + 1) % JOY_SMOOTHING_SAMPLES;
    
    // Чтение и фильтрация джойстика 1
    int raw1_x = analogRead(JOY1_Y);
    int raw1_y = analogRead(JOY1_X);
    
    joy1.x = filterJoystickValue(joy1_x_buffer, raw1_x);
    joy1.y = filterJoystickValue(joy1_y_buffer, raw1_y);
    
    // Маппинг с правильными диапазонами
    // joy1.x = map(joy1.x, 0, 4095, 0, 300);  // 12-бит = 0-4095
    // joy1.y = map(joy1.y, 0, 4095, 0, 220);
    
    // Включи антидребезг для кнопки джойстика!
    debounce(joy1.button, !digitalRead(JOY1_BTN), joy1.lastChange);
    
    // Чтение и фильтрация джойстика 2
    int raw2_x = analogRead(JOY2_Y);
    int raw2_y = analogRead(JOY2_X);
    
    joy2.x = filterJoystickValue(joy2_x_buffer, raw2_x);
    joy2.y = filterJoystickValue(joy2_y_buffer, raw2_y);
    
    // Маппинг с правильными диапазонами
    // joy2.x = map(joy2.x, 0, 4095, 0, 320);  // 10-бит = 0-1023
    // joy2.y = map(joy2.y, 0, 4095, 0, 240);
    
    // Включи антидребезг для кнопки джойстика!
    debounce(joy2.button, !digitalRead(JOY2_BTN), joy2.lastChange);
    
    // Обновление цифровых кнопок
    debounce(btn_lt.state, !digitalRead(BTN_A), btn_lt.lastChange);
    debounce(btn_rt.state, !digitalRead(BTN_B), btn_rt.lastChange);
    
    // Обновление времени последнего ввода
    if(joy1.button == JUST_PRESSED || joy1.button == JUST_RELEASED || 
       joy2.button == JUST_PRESSED || joy2.button == JUST_RELEASED ||
       btn_lt.state == JUST_PRESSED || btn_rt.state == JUST_PRESSED) {
        lastInputTime = currentTime;
    }
}

// Функция измерения напряжения аккумулятора
float read_battery_voltage() {
    //int rawValue = analogReadMilliVolts(VOLTAGE_SENSOR);
    
    // Конвертируем ADC значение в напряжение на делителе (Vout)
    // float vout = analogReadMilliVolts(VOLTAGE_SENSOR);
    
    // Коэффициент делителя: R1=100K, R2=10K
    // Vin = Vout * (R1 + R2) / R2 = Vout * (100 + 10) / 10 = Vout * 11
    //float vin = vout * 11.0;
    
    return 12.0; // Заглушка для примера
}

// Функция вычисления заряда в процентах от 0 до 100
// Напряжение: 4.1V = 100%, 3.1V = 0%
int get_battery_percent() {
    // float voltage = read_battery_voltage();
    
    // // Ограничиваем напряжение диапазоном 3.1V - 4.1V
    // if (voltage >= 4.1) {
    //     return 100;
    // }
    // if (voltage <= 3.1) {
    //     return 0;
    // }
    
    // // Линейная интерполяция
    // // Процент = (voltage - 3.1) / (4.1 - 3.1) * 100
    // int percent = (int)((voltage - 3.1) / 1.0 * 100);
    
    return 100; // Заглушка для примера
}
