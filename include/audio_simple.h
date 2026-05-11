#ifndef AUDIO_SIMPLE_H
#define AUDIO_SIMPLE_H

#include <VS1053.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <TFT_eSPI.h>
#include "shared.h"

// Пины VS1053
#define VS1053_CS 5 
#define VS1053_DCS 21 
#define VS1053_DREQ 22
#define SDREADER_CS 26

// SPI пины
#define SPI_CLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

void audio_init();
void audio_play(const char* path);
void audio_play(const char* path, bool loop);
void audio_stop();
void audio_pause();
void audio_resume();
void audio_set_loop(bool loop);
bool audio_is_playing();
bool audio_is_paused();
const char* audio_current_track();
void audio_next_track();
void audio_prev_track();

// Функции для работы с BMP
bool drawBmpFromSD(const char* filename, int16_t x, int16_t y);
bool drawBmpToSprite(TFT_eSprite* sprite, const char* filename, int16_t x, int16_t y);

#endif