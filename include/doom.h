// doom.h
#ifndef DOOM_H
#define DOOM_H

#include <TFT_eSPI.h>

enum DoomGameState {
    DOOM_PLAYING,
    DOOM_PAUSED,
    DOOM_GAME_OVER
};





void doom_init();
void doom_update();
void doom_render(TFT_eSPI* tft);
DoomGameState doom_get_state();
void doom_reset();

#endif