#include "voltage.h"
#include "input.h"

VoltageDisplay voltageDisplay;

void VoltageDisplay::update() {
    unsigned long currentTime = millis();
    
    // Обновляем значение напряжения только раз в UPDATE_INTERVAL миллисекунд
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        voltage = 1212.0; // Заглушка для примера
        lastUpdateTime = currentTime;
    }
}

void VoltageDisplay::drawToSprite(TFT_eSprite* sprite, int x, int y) {
    if (!sprite) return;
    
    // Сохраняем текущие настройки
    uint8_t oldSize = sprite->textsize;
    uint16_t oldColor = sprite->textcolor;
    uint16_t oldBgColor = sprite->textbgcolor;
    
    // Определяем цвет в зависимости от напряжения
    uint16_t textColor;
    if (voltage < 11.0) {
        textColor = TFT_RED;        // Низкое напряжение - красный
    } else if (voltage < 12.0) {
        textColor = TFT_ORANGE;     // Среднее напряжение - оранжевый
    } else if (voltage < 14.0) {
        textColor = TFT_GREEN;      // Нормальное напряжение - зеленый
    } else {
        textColor = TFT_YELLOW;     // Высокое напряжение - желтый
    }
    
    // Устанавливаем настройки для отображения напряжения
    sprite->setTextColor(textColor, TFT_BLACK);
    sprite->setTextSize(1);
    
    // Рисуем напряжение
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.1fV", voltage);
    sprite->drawString(buffer, x, y);
    
    // Восстанавливаем настройки
    sprite->setTextSize(oldSize);
    sprite->setTextColor(oldColor, oldBgColor);
}