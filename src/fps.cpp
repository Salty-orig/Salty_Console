// fps.cpp
#include "fps.h"

FPS fps;

void FPS::beginFrame() {
    lastFrameTime = micros();
}
 
void FPS::endFrame() {
    unsigned long currentTime = micros();
    frameTime = (currentTime - lastFrameTime) / 1000.0f; // мс
    
    frameCount++;
    unsigned long now = millis(); 
    
    if (now - lastFpsTime >= 1000) {
        fps = (frameCount * 1000.0f) / (now - lastFpsTime);
        frameCount = 0;
        lastFpsTime = now;
    }
}

void FPS::drawToSprite(TFT_eSprite* sprite, int x, int y) {
    if (!sprite) return;
    
    // Сохраняем текущие настройки
    uint8_t oldSize = sprite->textsize;
    uint16_t oldColor = sprite->textcolor;
    uint16_t oldBgColor = sprite->textbgcolor;
    
    // Устанавливаем настройки для FPS
    sprite->setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    sprite->setTextSize(1);
    
    // Рисуем FPS
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "FPS:%.1f %.1fms", fps, frameTime);
    sprite->drawString(buffer, x, y);
    
    // Восстанавливаем настройки
    sprite->setTextSize(oldSize);
    sprite->setTextColor(oldColor, oldBgColor);
}