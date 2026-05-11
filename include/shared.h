#ifndef SHARED_H
#define SHARED_H

#include <TFT_eSPI.h>
#include "sprite_loader.h"
#include "intro.h"
#include "input.h"

// Просто объявляем переменную как extern
extern bool FPSrend;
extern int currentFPS;
extern TFT_eSprite screen;
extern RawSprite menuSprite;

#define TRANSPARENT_BLACK 0x0000 

enum GameState { MENU, PONG, PACMAN, ACHIEVEMENTS_SCREEN, DOOM, SEABATTLE, LOST_TRAIL, SETTINGS };
extern GameState gameState;
static GameState lastGameState = MENU;

enum PongState {
    PONG_MENU,
    PONG_GAME,
    PONG_PAUSE,
    PONG_SETTINGS
};

extern PongState pongState;

extern bool gameChanged, CHANGES_BTN, BTN, lastBut;

static int robotAnimId = 1;  // ID анимации для робота

// Добавляем переменные для delta time
extern float deltaTime;  // Время в секундах с последнего кадра
extern unsigned long lastFrameTime;  // Время последнего кадра в мс

// Глобальная переменная для управления звуком
extern bool audioEnabled;  // Глобальная переменная для включения/выключения звука

// Глобальная функция для создания цвета из RGB
inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

inline uint16_t rgba(uint8_t r, uint8_t g, uint8_t b, float a) {
    // Конвертируем RGB в 16-битный цвет
    uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    
    // Если прозрачность больше 0, помечаем цвет как "прозрачный" для спрайтов
    // Но сохраняем само значение цвета! Просто устанавливаем младший бит в 1
    if (a < 0.99f) {  // Если есть хоть какая-то прозрачность
        return color | 0x0001;  // Устанавливаем младший бит для обозначения прозрачности
    }
    
    return color;  // Полностью непрозрачный
}

// Дополнительные удобные макросы для часто используемых цветов
#define RGB(r,g,b) rgb(r,g,b)

// Примеры предустановленных цветов
#define CUSTOM_RED    rgb(241, 7, 7)
#define CUSTOM_GREEN  rgb(28, 135, 28)
#define CUSTOM_BLUE   rgb(28, 28, 135)
#define CUSTOM_ORANGE rgb(255, 165, 0)

struct Animation {
    bool active;
    float startX, startY;
    float targetX, targetY;
    float currentX, currentY;
    unsigned long startTime;
    unsigned long duration;
};

// Функция для создания и управления анимациями
float animate(int lastX, int lastY, int newX, int newY, int time, int& outX, int& outY, int animId = -1);

// Максимальное количество одновременных анимаций
#define MAX_ANIMATIONS 16

// Функция для обновления всех анимаций (вызывать в начале каждого кадра)
void updateAnimations();

// Структура для цветовой анимации
struct ColorAnimation {
    bool active;
    uint16_t startColor;
    uint16_t targetColor;
    uint16_t currentColor;
    unsigned long startTime;
    unsigned long duration;
    bool loop;           // Зациклить анимацию
    bool reverse;        // Обратное направление
    uint8_t mode;        // 0 = normal, 1 = rainbow
    uint8_t rainbowHue;  // Текущий оттенок для радуги
};

// Максимальное количество цветовых анимаций
#define MAX_COLOR_ANIMATIONS 16

// Функции для цветовых анимаций
int startColorAnimation(uint16_t color1, uint16_t color2, int durationMs, bool loop = false);
int startRainbowAnimation(int durationMs, bool loop = true);
uint16_t getColor(int animId);
void updateColorAnimations();
void stopColorAnimation(int animId);

// =========== АНИМАЦИЯ КРУТИЛКИ (SPINNER) ===========
struct SpinnerAnimation {
    bool active;
    int x, y;           // Центр
    int radius;         // Радиус
    int thickness;      // Толщина дуги
    unsigned long startTime;
    int duration;       // Длительность полного оборота в мс
    float angle;        // Текущий угол
};

#define MAX_SPINNERS 4

void startSpinner(int x, int y, int radius, int thickness, int durationMs = 1000);
void stopSpinner(int index);
void updateSpinners();
void drawSpinner(int index, TFT_eSprite& sprite);
void stopAllSpinners();

// =========== АНИМАЦИЯ ПРОГРЕСС-БАРА С ВОЛНОЙ ===========
struct WaveProgressBar {
    bool active;
    int x, y;           // Позиция левого верхнего угла
    int width, height;  // Размеры бара
    float progress;     // Прогресс от 0 до 1
    unsigned long lastWaveTime;
    float waveOffset;   // Смещение волны
    bool showPercent;   // Показывать процент
};

static int loadingProgressBarId = 1;

#define MAX_PROGRESS_BARS 4

int createProgressBar(int x, int y, int width, int height, bool showPercent = true);
void updateProgressBar(int index, float progress);
void drawProgressBar(int index, TFT_eSprite& sprite);
void removeProgressBar(int index);
void removeAllProgressBars();

// Функция для обновления цвета спиннера
void updateSpinnerColor();

#endif