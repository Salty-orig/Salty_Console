#ifndef PACMAN_H
#define PACMAN_H

#include <TFT_eSPI.h>

void pacman_init();
void pacman_update();
void pacman_render(TFT_eSPI* tft);

#endif