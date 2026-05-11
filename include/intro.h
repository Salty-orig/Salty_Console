#ifndef INTRO_H
#define INTRO_H

#include <TFT_eSPI.h>
#include <functional>
#include <vector>

// Структура для 3D точки
struct Point3D {
    float x, y, z;
};

// Структура для 2D точки (после проекции)
struct Point2D {
    int x, y;
};

// Тип функции загрузки
typedef std::function<bool()> LoadingFunction;

// Состояния интро
enum IntroState {
    INTRO_STATE_SPLASH_1,      // Первый логотип
    INTRO_STATE_SPLASH_2,      // Второй логотип
    INTRO_STATE_ANIMATION,     // 3D анимация
    INTRO_STATE_COMPLETE       // Завершено
};

extern bool introActive;

// Функции интро
void intro_init(TFT_eSPI* tft);
bool intro_update(TFT_eSPI* tft, TFT_eSprite* screen);
void intro_render(TFT_eSprite* screen);

// Функции для фоновой загрузки на ядре 1
void add_loading_task(LoadingFunction func);
void start_background_loading();
int get_loading_progress();
bool is_loading_complete();

#endif