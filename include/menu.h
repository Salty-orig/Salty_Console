#ifndef MENU_H
#define MENU_H

#include "display.h" 
#include <TFT_eSPI.h>
#include "doom.h"
#include "voltage.h"  // Добавлено
#include "sprite_loader.h"

extern int x;
// Объявляем спрайты для меню
extern TFT_eSprite menuBgSprite;
extern TFT_eSprite menuItemSprite;
extern TFT_eSprite menuHighlightSprite;

extern TFT_eSprite fpsSprite;

// Снежинки
extern bool snowEnabled;
extern float SNOW_SPEED; // Глобальная переменная скорости
extern float SNOW_SIZE;  // Глобальная переменная размера

// QR код
extern bool qrScreenActive;
extern RawSprite qrSprite;

void menu_init(TFT_eSPI* tft);
void menu_update();
void menu_render(TFT_eSPI* tft);
void loadQRSprite();
void showQRScreen();

#endif