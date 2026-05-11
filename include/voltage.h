#ifndef VOLTAGE_H
#define VOLTAGE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class VoltageDisplay {
private:
    float voltage = 0.0;
    unsigned long lastUpdateTime = 0;
    const unsigned long UPDATE_INTERVAL = 500; // Обновление каждые 500 мс
    
public:
    // Обновить значение напряжения
    void update();
    
    // Получить текущее напряжение
    float getVoltage() const { return voltage; }
    
    // Нарисовать напряжение в указанный спрайт
    void drawToSprite(TFT_eSprite* sprite, int x, int y);
};

// Глобальный экземпляр
extern VoltageDisplay voltageDisplay;

#endif