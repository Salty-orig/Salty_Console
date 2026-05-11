#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include <TFT_eSPI.h>
#include <SD.h>
#include <Arduino.h>

struct RawSprite {
    uint16_t* data;
    int width;
    int height;
    bool isValid;
    size_t dataSize;  // Добавляем размер для отладки
};

// Загрузка RAW файла (автоопределение размера)
RawSprite loadRawSprite(const char* filename);

// Загрузка RAW файла с явным указанием размеров
RawSprite loadRawSprite(const char* filename, int width, int height);

// Освобождение памяти
void freeRawSprite(RawSprite* sprite);

// Функция для дампа первых нескольких пикселей (отладка)
void dumpSpritePixels(RawSprite* sprite, int count);

#endif