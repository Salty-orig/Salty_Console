#include "audio_simple.h"
#include <SPI.h>
#include <Arduino.h>

// Внешняя переменная для глобального управления звуком
extern bool audioEnabled;

static VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);
static File currentFile;
static bool isPlaying = false;
static bool isPaused = false;
static bool loopEnabled = false;                  // Новый флаг зацикливания
static char currentTrackPath[64] = "";            // Храним путь к текущему треку
static uint8_t buffer[512];
static TaskHandle_t audioTaskHandle = NULL;

void audioTaskFunction(void* param) {
    Serial.println("Audio task started");
    
    while (1) {
        if (isPlaying && !isPaused && currentFile && audioEnabled) {
            if (player.data_request()) {
                size_t bytesRead = currentFile.read(buffer, sizeof(buffer));
                
                if (bytesRead > 0) {
                    player.playChunk(buffer, bytesRead);
                    
                    if (bytesRead < sizeof(buffer)) {
                        // Конец файла
                        currentFile.close();
                        
                        if (loopEnabled) {
                            // Зацикливание - открываем файл заново
                            Serial.println("Looping track...");
                            currentFile = SD.open(currentTrackPath);
                            if (currentFile) {
                                player.startSong();
                                // Не меняем isPlaying, оставляем true
                            } else {
                                Serial.println("ERROR: Cannot reopen file for loop!");
                                isPlaying = false;
                            }
                        } else {
                            // Без зацикливания - останавливаем
                            isPlaying = false;
                            Serial.println("Playback finished");
                        }
                    }
                }
            }
        }
        
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void audio_init() {
    Serial.println("Initializing audio...");
    
    SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
    
    if (!SD.begin(SDREADER_CS)) {
        Serial.println("ERROR: SD card failed!");
        return;
    }
    
    player.begin();
    
    if (!player.isChipConnected()) {
        Serial.println("ERROR: VS1053 not connected!");
        return;
    }
    
    player.switchToMp3Mode();
    player.setVolume(0);
    
    xTaskCreatePinnedToCore(
        audioTaskFunction,
        "AudioTask",
        4096,
        NULL,
        5,
        &audioTaskHandle,
        0
    );
    
    Serial.println("Audio initialized");
}

// Основная функция play (без зацикливания по умолчанию)
void audio_play(const char* path) {
    player.setVolume(85);
    audio_play(path, false);  // По умолчанию без зацикливания
}

// Перегруженная функция с параметром зацикливания
void audio_play(const char* path, bool loop) {
    // Если звук отключен глобально, ничего не делаем
    if (!audioEnabled) {
        Serial.println("Audio is disabled globally");
        return;
    }
    
    // Останавливаем текущее воспроизведение если есть
    if (isPlaying) {
        player.stopSong();
        if (currentFile) {
            currentFile.close();
        }
        isPlaying = false;
    }
    
    // Сохраняем путь к треку
    strncpy(currentTrackPath, path, sizeof(currentTrackPath) - 1);
    currentTrackPath[sizeof(currentTrackPath) - 1] = '\0';
    
    // Устанавливаем режим зацикливания
    loopEnabled = loop;
    
    // Открываем новый файл
    currentFile = SD.open(path);
    if (currentFile) {
        isPlaying = true;
        isPaused = false;
        player.startSong();
        Serial.print("Playing: ");
        Serial.print(path);
        if (loop) {
            Serial.print(" (looping)");
        }
        Serial.println();
    } else {
        Serial.print("ERROR: Cannot open: ");
        Serial.println(path);
    }
}

void audio_stop() {
    if (isPlaying) {
        player.stopSong();
        if (currentFile) {
            currentFile.close();
        }
        isPlaying = false;
        isPaused = false;
        currentTrackPath[0] = '\0';  // Очищаем название трека
        Serial.println("Playback stopped");
    }
}

void audio_pause() {
    if (isPlaying && !isPaused && audioEnabled) {
        isPaused = true;
        player.setVolume(0);
        Serial.println("Playback paused");
    }
}

void audio_resume() {
    if (isPlaying && isPaused && audioEnabled) {
        isPaused = false;
        player.setVolume(80);
        Serial.println("Playback resumed");
    }
}

void audio_set_loop(bool loop) {
    loopEnabled = loop;
    Serial.print("Loop mode: ");
    Serial.println(loop ? "ON" : "OFF");
}

bool audio_is_playing() {
    return isPlaying;
}

bool audio_is_paused() {
    return isPaused;
}

const char* audio_current_track() {
    return currentTrackPath;
}

// Заглушки для будущего расширения (плейлисты)
void audio_next_track() {
    Serial.println("Next track - not implemented");
}

void audio_prev_track() {
    Serial.println("Previous track - not implemented");
}

// Функция для рисования BMP на экране напрямую
bool drawBmpFromSD(const char* filename, int16_t x, int16_t y) {
    File bmpFile = SD.open(filename);
    if (!bmpFile) {
        Serial.print("ERROR: Cannot open BMP file: ");
        Serial.println(filename);
        return false;
    }
    
    // Читаем заголовок BMP (54 байта)
    uint8_t bmpHeader[54];
    if (bmpFile.read(bmpHeader, 54) < 54) {
        Serial.println("ERROR: Invalid BMP header");
        bmpFile.close();
        return false;
    }
    
    // Проверяем сигнатуру BMP 
    if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
        Serial.println("ERROR: Not a BMP file");
        bmpFile.close();
        return false;
    }
    
    // Получаем информацию об изображении
    int32_t bmpWidth = *(int32_t*)&bmpHeader[18];
    int32_t bmpHeight = *(int32_t*)&bmpHeader[22];
    uint16_t bmpBPP = *(uint16_t*)&bmpHeader[28];  // Битов на пиксель
    
    Serial.printf("BMP: %dx%d, %d bpp\n", bmpWidth, bmpHeight, bmpBPP);
    
    // Поддерживаем только 24-битные BMP
    if (bmpBPP != 24) {
        Serial.println("ERROR: Only 24-bit BMP supported");
        bmpFile.close();
        return false;
    }
    
    // Получаем смещение данных изображения
    uint32_t dataOffset = *(uint32_t*)&bmpHeader[10];
    bmpFile.seek(dataOffset);
    
    // Выделяем буферы в куче
    uint8_t* lineBuffer = (uint8_t*)malloc(bmpWidth * 3);
    uint16_t* tftBuffer = (uint16_t*)malloc(bmpWidth * 2);
    
    if (!lineBuffer || !tftBuffer) {
        Serial.println("ERROR: Out of memory");
        if (lineBuffer) free(lineBuffer);
        if (tftBuffer) free(tftBuffer);
        bmpFile.close();
        return false;
    }
    
    // BMP хранятся снизу вверх, поэтому идем с последней строки
    for (int row = 0; row < bmpHeight; row++) {
        int yPos = y + bmpHeight - 1 - row;  // Переворачиваем
        
        // Пропускаем строки за пределами экрана
        if (yPos >= 0 && yPos < 240) {
            // Читаем строку
            bmpFile.read(lineBuffer, bmpWidth * 3);
            
            // Конвертируем RGB888 (24-bit) в RGB565 (16-bit)
            for (int col = 0; col < bmpWidth; col++) {
                int xPos = x + col;
                
                // Рисуем только пиксели в пределах экрана
                if (xPos >= 0 && xPos < 320) {
                    uint8_t r = lineBuffer[col * 3 + 2];  // Красный
                    uint8_t g = lineBuffer[col * 3 + 1];  // Зеленый
                    uint8_t b = lineBuffer[col * 3];      // Синий
                    
                    // Конвертация в RGB565
                    tftBuffer[col] = ( (r & 0xF8) << 8 ) | ( (g & 0xFC) << 3 ) | ( b >> 3 );
                }
            }
            
            // Выводим строку на экран через спрайт screen
            screen.pushImage(x, yPos, bmpWidth, 1, tftBuffer);
        } else {
            // Пропускаем строку если она вне экрана
            bmpFile.seek(bmpFile.position() + bmpWidth * 3);
        }
    }
    
    free(lineBuffer);
    free(tftBuffer);
    bmpFile.close();
    
    Serial.println("BMP drawn successfully");
    return true;
}

// Функция для рисования BMP в спрайт (для меню)
bool drawBmpToSprite(TFT_eSprite* sprite, const char* filename, int16_t x, int16_t y) {
    File bmpFile = SD.open(filename);
    if (!bmpFile) {
        Serial.print("ERROR: Cannot open BMP file: ");
        Serial.println(filename);
        return false;
    }
    
    // Читаем заголовок BMP
    uint8_t bmpHeader[54];
    if (bmpFile.read(bmpHeader, 54) < 54) {
        bmpFile.close();
        return false;
    }
    
    // Проверяем сигнатуру
    if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
        bmpFile.close();
        return false;
    }
    
    int32_t bmpWidth = *(int32_t*)&bmpHeader[18];
    int32_t bmpHeight = *(int32_t*)&bmpHeader[22];
    uint16_t bmpBPP = *(uint16_t*)&bmpHeader[28];
    
    if (bmpBPP != 24) {
        bmpFile.close();
        return false;
    }
    
    uint32_t dataOffset = *(uint32_t*)&bmpHeader[10];
    bmpFile.seek(dataOffset);
    
    // Буферы
    uint8_t* lineBuffer = (uint8_t*)malloc(bmpWidth * 3);
    uint16_t* tftBuffer = (uint16_t*)malloc(bmpWidth * 2);
    
    if (!lineBuffer || !tftBuffer) {
        if (lineBuffer) free(lineBuffer);
        if (tftBuffer) free(tftBuffer);
        bmpFile.close();
        return false;
    }
    
    // Рисуем в спрайт
    for (int row = 0; row < bmpHeight; row++) {
        int yPos = y + bmpHeight - 1 - row;
        
        bmpFile.read(lineBuffer, bmpWidth * 3);
        
        for (int col = 0; col < bmpWidth; col++) {
            uint8_t r = lineBuffer[col * 3 + 2];
            uint8_t g = lineBuffer[col * 3 + 1];
            uint8_t b = lineBuffer[col * 3];
            
            tftBuffer[col] = sprite->color565(r, g, b);
        }
        
        sprite->pushImage(x, yPos, bmpWidth, 1, tftBuffer);
    }
    
    free(lineBuffer);
    free(tftBuffer);
    bmpFile.close();
    
    return true;
}