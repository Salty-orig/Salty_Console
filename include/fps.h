// fps.h
#ifndef FPS_H
#define FPS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class FPS {
private: 
    unsigned long frameCount = 0;
    unsigned long lastFpsTime = 0; 
    float fps = 0;
    unsigned long lastFrameTime = 0;
    float frameTime = 0;
    
public:
    // Начать отсчет кадра
    void beginFrame();
    
    // Закончить отсчет кадра
    void endFrame();
    
    // Получить текущий FPS
    float getFPS() const { return fps; }
    
    // Получить время кадра
    float getFrameTime() const { return frameTime; }
    
    // Нарисовать FPS в указанный спрайт
    void drawToSprite(TFT_eSprite* sprite, int x, int y);
};

// Глобальный экземпляр
extern FPS fps;

#endif