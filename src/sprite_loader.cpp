#include "sprite_loader.h"

RawSprite loadRawSprite(const char* filename) {
    RawSprite result = {nullptr, 0, 0, false, 0};
    
    Serial.println("\n=== RAW Sprite Loader Debug ===");
    Serial.printf("Opening file: %s\n", filename);
    
    File file = SD.open(filename);
    if (!file) {
        Serial.println("ERROR: Cannot open file");
        return result;
    }
    
    Serial.printf("File size: %d bytes\n", file.size());
    
    // Определяем размеры по размеру файла (предполагаем 320x240)
    int width = 320;
    int height = 240;
    
    // Проверяем что размер файла соответствует 320x240 (153600 байт)
    size_t expectedSize = width * height * sizeof(uint16_t);
    
    if (file.size() != expectedSize) {
        Serial.printf("WARNING: File size %d doesn't match 320x240 (%d bytes)\n", 
                     file.size(), expectedSize);
        
        // Пробуем угадать размеры из размера файла
        int pixels = file.size() / 2;
        width = 320;  // предполагаем ширину 320
        height = pixels / width;
        
        Serial.printf("Guessing: %dx%d (%d pixels)\n", width, height, pixels);
    }
    
    Serial.printf("Using dimensions: %dx%d\n", width, height);
    
    // Выделяем память
    size_t pixelCount = width * height;
    size_t bufferSize = pixelCount * sizeof(uint16_t);
    
    Serial.printf("Allocating %d bytes\n", bufferSize);
    
    uint16_t* buffer = (uint16_t*)ps_malloc(bufferSize);
    if (!buffer) {
        buffer = (uint16_t*)malloc(bufferSize);
        if (!buffer) {
            Serial.println("ERROR: Out of memory!");
            file.close();
            return result;
        }
        Serial.println("Using regular RAM");
    } else {
        Serial.println("Using PSRAM");
    }
    
    // Читаем все данные сразу
    size_t bytesRead = file.read((uint8_t*)buffer, bufferSize);
    file.close();
    
    Serial.printf("Read %d bytes\n", bytesRead);
    
    if (bytesRead != bufferSize) {
        Serial.printf("WARNING: Read %d/%d bytes\n", bytesRead, bufferSize);
    }
    
    // Проверяем первые несколько пикселей
    Serial.println("First 16 pixels (hex):");
    for (int i = 0; i < 16 && i < pixelCount; i++) {
        Serial.printf("  [%d]: 0x%04X", i, buffer[i]);
        if ((i + 1) % 4 == 0) Serial.println();
    }
    Serial.println();
    
    // Проверяем, не нужно ли swap
    Serial.println("Checking byte order...");
    uint16_t testPixel = buffer[0];
    uint16_t swapped = (testPixel >> 8) | (testPixel << 8);
    Serial.printf("First pixel: 0x%04X, swapped: 0x%04X\n", testPixel, swapped);
    
    result.data = buffer;
    result.width = width;
    result.height = height;
    result.isValid = true;
    result.dataSize = bufferSize;
    
    Serial.println("=== Load complete ===\n");
    return result;
}

// Альтернативная функция с явным указанием размеров
RawSprite loadRawSprite(const char* filename, int width, int height) {
    RawSprite result = {nullptr, width, height, false, 0};
    
    Serial.printf("Loading RAW sprite with known size: %dx%d\n", width, height);
    
    File file = SD.open(filename);
    if (!file) {
        Serial.println("ERROR: Cannot open file");
        return result;
    }
     
    size_t expectedSize = width * height * sizeof(uint16_t);
    size_t fileSize = file.size();
    
    Serial.printf("File size: %d bytes, expected: %d bytes\n", fileSize, expectedSize);
    
    if (fileSize < expectedSize) {
        Serial.println("ERROR: File too small");
        file.close();
        return result;
    }
    
    size_t bufferSize = expectedSize;
    uint16_t* buffer = (uint16_t*)ps_malloc(bufferSize);
    if (!buffer) {
        buffer = (uint16_t*)malloc(bufferSize);
        if (!buffer) {
            Serial.println("ERROR: Out of memory!");
            file.close();
            return result;
        }
    }
    
    size_t bytesRead = file.read((uint8_t*)buffer, bufferSize);
    file.close();
    
    if (bytesRead != bufferSize) {
        Serial.printf("WARNING: Read %d/%d bytes\n", bytesRead, bufferSize);
    }
    
    result.data = buffer;
    result.isValid = true;
    result.dataSize = bufferSize;
    
    return result;
}

void freeRawSprite(RawSprite* sprite) {
    if (sprite && sprite->data) {
        free(sprite->data);
        sprite->data = nullptr;
        sprite->isValid = false;
    }
}