#ifndef PONG_H
#define PONG_H

#include "display.h"
#include "shared.h"

void pong_init();
void pong_update();
void pong_render(TFT_eSPI* tft);
void pong_menu_init();
void pong_settings_init();

#endif