#include "shared.h"

// Определение глобальной переменной для цвета спиннера
static int spinnerId = startColorAnimation(rgb(0, 117, 63), rgb(0, 255, 13), 750, true);
uint16_t currentColorSpinner = getColor(spinnerId);

// Массив для хранения активных анимаций
static Animation animations[MAX_ANIMATIONS];
static int nextAnimId = 0;

// Функция для поиска свободного слота анимации
static int findFreeAnimationSlot() {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) {
            return i;
        }
    }
    // Если нет свободных слотов, перезаписываем самую старую анимацию
    static int oldestAnim = 0;
    oldestAnim = (oldestAnim + 1) % MAX_ANIMATIONS;
    animations[oldestAnim].active = false;
    return oldestAnim;
}

// Функция для поиска анимации по ID
static int findAnimationById(int animId) {
    if (animId >= 0 && animId < MAX_ANIMATIONS && animations[animId].active) {
        return animId;
    }
    return -1;
}

// Функция плавного движения с ускорением/замедлением (ease-in-out)
static float easeInOut(float t) {
    // Кубическая функция плавности: t^2 * (3 - 2t)
    if (t < 0) return 0;
    if (t > 1) return 1;
    return t * t * (3.0f - 2.0f * t);
}

void updateAnimations() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!animations[i].active) continue;
        
        // Вычисляем прогресс анимации
        float progress = (float)(currentTime - animations[i].startTime) / animations[i].duration;
        
        if (progress >= 1.0f) {
            // Анимация завершена
            animations[i].currentX = animations[i].targetX;
            animations[i].currentY = animations[i].targetY;
            animations[i].active = false;
        } else {
            // Применяем функцию плавности
            float easedProgress = easeInOut(progress);
            
            // Интерполируем позицию
            animations[i].currentX = animations[i].startX + 
                (animations[i].targetX - animations[i].startX) * easedProgress;
            animations[i].currentY = animations[i].startY + 
                (animations[i].targetY - animations[i].startY) * easedProgress;
        }
    }
}

float animate(int lastX, int lastY, int newX, int newY, int time, int& outX, int& outY, int animId) {
    unsigned long currentTime = millis();
    
    // Если указан конкретный ID, пытаемся найти эту анимацию
    int slot = -1;
    if (animId >= 0) {
        slot = findAnimationById(animId);
    }
    
    // Если анимация не найдена или не указан ID, создаем новую
    if (slot == -1) {
        slot = findFreeAnimationSlot();
        
        // Заполняем данные новой анимации
        animations[slot].active = true;
        animations[slot].startX = lastX;
        animations[slot].startY = lastY;
        animations[slot].targetX = newX;
        animations[slot].targetY = newY;
        animations[slot].currentX = lastX;
        animations[slot].currentY = lastY;
        animations[slot].startTime = currentTime;
        animations[slot].duration = time;
        
        // Сохраняем ID для возврата
        if (animId == -1) {
            animId = slot;
        }
    } else {
        // Если анимация уже существует, но цели изменились, обновляем
        if (animations[slot].targetX != newX || animations[slot].targetY != newY) {
            animations[slot].startX = animations[slot].currentX;
            animations[slot].startY = animations[slot].currentY;
            animations[slot].targetX = newX;
            animations[slot].targetY = newY;
            animations[slot].startTime = currentTime;
            animations[slot].duration = time;
        }
    }
    
    // Возвращаем текущие координаты
    outX = (int)animations[slot].currentX;
    outY = (int)animations[slot].currentY;
    
    return animId; // Возвращаем ID анимации для последующего использования
}

// Массив для хранения активных цветовых анимаций
static ColorAnimation colorAnimations[MAX_COLOR_ANIMATIONS];
static int nextColorAnimId = 0;

// Функция для поиска свободного слота цветовой анимации
static int findFreeColorAnimationSlot() {
    for (int i = 0; i < MAX_COLOR_ANIMATIONS; i++) {
        if (!colorAnimations[i].active) {
            return i;
        }
    }
    // Если нет свободных слотов, перезаписываем самую старую
    static int oldestAnim = 0;
    oldestAnim = (oldestAnim + 1) % MAX_COLOR_ANIMATIONS;
    colorAnimations[oldestAnim].active = false;
    return oldestAnim;
}

// Функция для плавной интерполяции между двумя 16-битными цветами
static uint16_t interpolateColor(uint16_t color1, uint16_t color2, float progress) {
    // Извлекаем компоненты RGB из 16-битных цветов
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    // Интерполяция
    uint8_t r = r1 + (r2 - r1) * progress;
    uint8_t g = g1 + (g2 - g1) * progress;
    uint8_t b = b1 + (b2 - b1) * progress;
    
    // Собираем обратно в 16-битный цвет
    return (r << 11) | (g << 5) | b;
}

// Преобразование HSV в RGB (для радуги)
static uint16_t hsvToRgb(uint8_t hue) {
    // hue: 0-255 (полный круг)
    float h = (hue / 255.0f) * 360.0f;
    float s = 1.0f;
    float v = 1.0f;
    
    float c = v * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    uint8_t red = (uint8_t)((r + m) * 255);
    uint8_t green = (uint8_t)((g + m) * 255);
    uint8_t blue = (uint8_t)((b + m) * 255);
    
    return rgb(red, green, blue);
}

void updateColorAnimations() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < MAX_COLOR_ANIMATIONS; i++) {
        if (!colorAnimations[i].active) continue;
        
        if (colorAnimations[i].mode == 1) { // Режим радуги
            unsigned long elapsed = currentTime - colorAnimations[i].startTime;
            
            if (colorAnimations[i].loop) {
                // Бесконечная радуга
                float progress = fmod((float)elapsed / colorAnimations[i].duration, 1.0f);
                colorAnimations[i].rainbowHue = (uint8_t)(progress * 255);
                colorAnimations[i].currentColor = hsvToRgb(colorAnimations[i].rainbowHue);
            } else {
                // Одноразовая анимация
                if (elapsed >= colorAnimations[i].duration) {
                    colorAnimations[i].active = false;
                } else {
                    float progress = (float)elapsed / colorAnimations[i].duration;
                    colorAnimations[i].rainbowHue = (uint8_t)(progress * 255);
                    colorAnimations[i].currentColor = hsvToRgb(colorAnimations[i].rainbowHue);
                }
            }
        } else { // Обычный режим перехода между двумя цветами
            unsigned long elapsed = currentTime - colorAnimations[i].startTime;
            float progress = (float)elapsed / colorAnimations[i].duration;
            
            if (progress >= 1.0f) {
                if (colorAnimations[i].loop) {
                    if (colorAnimations[i].reverse) {
                        // Меняем цвета местами
                        uint16_t temp = colorAnimations[i].startColor;
                        colorAnimations[i].startColor = colorAnimations[i].targetColor;
                        colorAnimations[i].targetColor = temp;
                        colorAnimations[i].startTime = currentTime;
                        progress = 0;
                    } else {
                        colorAnimations[i].startTime = currentTime;
                        progress = 0;
                    }
                } else {
                    colorAnimations[i].active = false;
                    colorAnimations[i].currentColor = colorAnimations[i].targetColor;
                    continue;
                }
            }
            
            float easedProgress = easeInOut(progress);
            colorAnimations[i].currentColor = interpolateColor(
                colorAnimations[i].startColor, 
                colorAnimations[i].targetColor, 
                easedProgress
            );
        }
    }
}

// Запуск анимации между двумя цветами
int startColorAnimation(uint16_t color1, uint16_t color2, int durationMs, bool loop) {
    int slot = findFreeColorAnimationSlot();
    
    colorAnimations[slot].active = true;
    colorAnimations[slot].startColor = color1;
    colorAnimations[slot].targetColor = color2;
    colorAnimations[slot].currentColor = color1;
    colorAnimations[slot].startTime = millis();
    colorAnimations[slot].duration = durationMs;
    colorAnimations[slot].loop = loop;
    colorAnimations[slot].reverse = true;  // По умолчанию реверсируем для плавного туда-обратно
    colorAnimations[slot].mode = 0;
    colorAnimations[slot].rainbowHue = 0;
    
    return slot;
}

// Запуск радужной анимации
int startRainbowAnimation(int durationMs, bool loop) {
    int slot = findFreeColorAnimationSlot();
    
    colorAnimations[slot].active = true;
    colorAnimations[slot].startTime = millis();
    colorAnimations[slot].duration = durationMs;
    colorAnimations[slot].loop = loop;
    colorAnimations[slot].mode = 1;
    colorAnimations[slot].rainbowHue = 0;
    colorAnimations[slot].currentColor = hsvToRgb(0);
    
    return slot;
}

// Получение текущего цвета по ID анимации
uint16_t getColor(int animId) {
    if (animId >= 0 && animId < MAX_COLOR_ANIMATIONS && colorAnimations[animId].active) {
        return colorAnimations[animId].currentColor;
    }
    // Если анимация не найдена, возвращаем цвет по умолчанию
    return rgb(8, 83, 12);
}

// Остановка цветовой анимации
void stopColorAnimation(int animId) {
    if (animId >= 0 && animId < MAX_COLOR_ANIMATIONS) {
        colorAnimations[animId].active = false;
    }
}

// =========== АНИМАЦИЯ КРУТИЛКИ (SPINNER) ===========
static SpinnerAnimation spinners[MAX_SPINNERS];

void startSpinner(int x, int y, int radius, int thickness, int durationMs) {
    for (int i = 0; i < MAX_SPINNERS; i++) {
        if (!spinners[i].active) {
            spinners[i].active = true;
            spinners[i].x = x;
            spinners[i].y = y;
            spinners[i].radius = radius;
            spinners[i].thickness = thickness;
            spinners[i].startTime = millis();
            spinners[i].duration = durationMs;
            spinners[i].angle = 0;
            return;
        }
    }
}

void stopSpinner(int index) {
    if (index >= 0 && index < MAX_SPINNERS) {
        spinners[index].active = false;
    }
}

void stopAllSpinners() {
    for (int i = 0; i < MAX_SPINNERS; i++) {
        spinners[i].active = false;
    }
}

void updateSpinners() {
    unsigned long currentTime = millis();
    for (int i = 0; i < MAX_SPINNERS; i++) {
        if (spinners[i].active) {
            // Угол вращается от 0 до 360 градусов
            float progress = fmod((float)(currentTime - spinners[i].startTime) / spinners[i].duration, 1.0f);
            spinners[i].angle = progress * 360.0f;
        }
    }
}

void drawSpinner(int index, TFT_eSprite& sprite) {
    if (index < 0 || index >= MAX_SPINNERS || !spinners[index].active) return;
    
    SpinnerAnimation& s = spinners[index];
    
    // Нормализуем угол в диапазоне 0-360
    float normalizedAngle = fmod(s.angle, 360.0f);
    if (normalizedAngle < 0) normalizedAngle += 360.0f;
    
    // Рисуем одну половинчатую дугу (180 градусов) с переливающимся цветом
    float startAngle = normalizedAngle;
    float endAngle = startAngle + 180.0f;
    
    // Если дуга переходит через 0 градусов, рисуем две части
    if (endAngle > 360.0f) {
        // Первая часть: от startAngle до 360
        uint8_t hue = (uint8_t)((millis() / 8) % 256);
        uint16_t color = hsvToRgb(hue);
        sprite.drawSmoothArc(s.x, s.y, s.radius, s.radius - s.thickness, 
                             startAngle, 360.0f, currentColorSpinner, TFT_BLACK, true);
        
        // Вторая часть: от 0 до endAngle - 360
        endAngle -= 360.0f;
        sprite.drawSmoothArc(s.x, s.y, s.radius, s.radius - s.thickness, 
                             0.0f, endAngle, currentColorSpinner, TFT_BLACK, true);
    } else {
        // Обычная отрисовка
        uint8_t hue = (uint8_t)((millis() / 8) % 256);
        uint16_t color = hsvToRgb(hue);
        sprite.drawSmoothArc(s.x, s.y, s.radius, s.radius - s.thickness, 
                             startAngle, endAngle, currentColorSpinner, TFT_BLACK, true);
    }
}

// =========== АНИМАЦИЯ ПРОГРЕСС-БАРА С ВОЛНОЙ ===========
static WaveProgressBar progressBars[MAX_PROGRESS_BARS];

int createProgressBar(int x, int y, int width, int height, bool showPercent) {
    for (int i = 0; i < MAX_PROGRESS_BARS; i++) {
        if (!progressBars[i].active) {
            progressBars[i].active = true;
            progressBars[i].x = x;
            progressBars[i].y = y;
            progressBars[i].width = width;
            progressBars[i].height = height;
            progressBars[i].progress = 0;
            progressBars[i].lastWaveTime = millis();
            progressBars[i].waveOffset = 0;
            progressBars[i].showPercent = showPercent;
            return i;
        }
    }
    return -1;
}

void updateProgressBar(int index, float progress) {
    if (index >= 0 && index < MAX_PROGRESS_BARS && progressBars[index].active) {
        progressBars[index].progress = constrain(progress, 0.0f, 1.0f);
        progressBars[index].lastWaveTime = millis();
    }
}

void drawProgressBar(int index, TFT_eSprite& sprite) {
    if (index < 0 || index >= MAX_PROGRESS_BARS || !progressBars[index].active) return;
    
    WaveProgressBar& pb = progressBars[index];
    
    // Обновляем смещение волны (движение вправо) - медленнее для длинных волн
    pb.waveOffset += 0.08f;  // Меньше скорость
    if (pb.waveOffset > 200) pb.waveOffset -= 200;
    
    // Рисуем фон (темная рамка)
    // sprite.drawRect(pb.x, pb.y, pb.width, pb.height, rgb(80, 80, 80));
    // sprite.fillRect(pb.x + 1, pb.y + 1, pb.width - 2, pb.height - 2, rgb(30, 30, 30));
    
    // Вычисляем ширину заполненной части
    int filledWidth = (int)((pb.width - 2) * pb.progress);
    
    if (filledWidth > 0) {
        int barHeight = pb.height - 2;
        int barY = pb.y + 1;
        
        // Рисуем сплошную заливку с волновым эффектом (без полосок)
        for (int px = 0; px < filledWidth; px++) {
            int barX = pb.x + 1 + px;
            
            // Волновой эффект: длинные волны (меньше частота)
            float waveValue = sin((px * 0.05f) + pb.waveOffset) * 0.5f + 0.5f;
            
            // Только два цвета: зеленый и светло-зеленый
            uint8_t g = (uint8_t)(100 + waveValue * 155);  // От 100 до 255
            uint8_t r = (uint8_t)(20 + waveValue * 50);    // Немного красного для теплого оттенка
            uint8_t b = (uint8_t)(20 + waveValue * 50);    // Немного синего
            
            sprite.drawFastVLine(barX, barY, barHeight, rgb(r, g, b));
        }
    }
    
    // Отображаем процент текстом
    if (pb.showPercent) {
        sprite.setTextColor(rgb(255, 255, 255));
        sprite.setTextSize(1);
        char percentText[8];
        sprintf(percentText, "%d%%", (int)(pb.progress * 100));
        int textWidth = strlen(percentText) * 6;
        sprite.setCursor(pb.x + (pb.width - textWidth) / 2, pb.y + (pb.height - 8) / 2);
        sprite.print(percentText);
    }
}

void removeProgressBar(int index) {
    if (index >= 0 && index < MAX_PROGRESS_BARS) {
        progressBars[index].active = false;
    }
}

void removeAllProgressBars() {
    for (int i = 0; i < MAX_PROGRESS_BARS; i++) {
        progressBars[i].active = false;
    }
}

// Функция для обновления цвета спиннера
void updateSpinnerColor() {
    currentColorSpinner = getColor(spinnerId);
}