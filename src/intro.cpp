#include "intro.h"
#include <math.h>
#include "shared.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sprite_loader.h"

// Параметры куба
#define CUBE_SIZE 28.0f
#define ROTATION_SPEED 0.05f
#define INTRO_DURATION 4000  // 5 секунд анимации
#define SPLASH_DURATION 5000 // 3 секунды на логотип

// Вершины куба (8 точек)
Point3D cubeVertices[8] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    {CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    {CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
    {CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
    {CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
    {-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}
};

// Грани куба (12 ребер)
int cubeEdges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Задняя грань
    {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Передняя грань
    {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Соединяющие ребра
};

// Параметры камеры
#define CAMERA_DISTANCE 200.0f
#define SCREEN_CENTER_X 160
#define SCREEN_CENTER_Y 120

// Углы вращения
float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;

// Время старта интро
unsigned long introStartTime = 0;

// Флаг активности интро
bool introActive = false;

// Новые переменные для логотипов
static IntroState currentState = INTRO_STATE_SPLASH_1;
static unsigned long stateStartTime = 0;
static RawSprite logo1 = {nullptr, 0, 0, false, 0};
static RawSprite logo2 = {nullptr, 0, 0, false, 0};
static bool spritesLoaded = false;

// =========== СИСТЕМА ЗАГРУЗКИ ===========
static std::vector<LoadingFunction> loadingTasks;
static volatile int completedTasks = 0;
static volatile bool loadingStarted = false;
static volatile bool loadingComplete = false;
static TaskHandle_t loadingTaskHandle = NULL;

// =========== ПЛАВНЫЙ ПРОГРЕСС-БАР ===========
static float currentProgress = 0.0f;        // Текущее значение прогресса для плавной анимации (0-100)
static float targetProgress = 0.0f;          // Целевое значение прогресса
static const float PROGRESS_SMOOTHING = 0.10f; // Коэффициент сглаживания (0.05 - очень медленно, 0.3 - быстрее)

// Прототипы функций
void render_3d_animation(TFT_eSprite* screen);
void render_logo_with_pushImage(TFT_eSprite* screen, RawSprite* logo);

// Функция для загрузки логотипов
bool load_logo_sprites() {
    Serial.println("Loading logo sprites...");
    
    // Загружаем первый логотип 320x240
    logo1 = loadRawSprite("/salt_lamp_logo.raw", 320, 240);
    if (!logo1.isValid) {
        Serial.println("Failed to load /salt_lamp_logo.raw");
        return false;
    }
    Serial.printf("Logo 1 loaded: %dx%d, dataSize: %d bytes\n", logo1.width, logo1.height, logo1.dataSize);
    
    // Загружаем второй логотип 180x180
    logo2 = loadRawSprite("/kvantorium.raw", 180, 180);
    if (!logo2.isValid) {
        Serial.println("Failed to load /kvantorium.raw");
        freeRawSprite(&logo1);
        return false;
    }
    Serial.printf("Logo 2 loaded: %dx%d, dataSize: %d bytes\n", logo2.width, logo2.height, logo2.dataSize);
    
    spritesLoaded = true;
    return true;
}

// Освобождение памяти логотипов
void free_logo_sprites() {
    if (logo1.isValid) {
        freeRawSprite(&logo1);
        Serial.println("Logo 1 memory freed");
    }
    if (logo2.isValid) {
        freeRawSprite(&logo2);
        Serial.println("Logo 2 memory freed");
    }
    spritesLoaded = false;
}

// Функция загрузки на ядре 1
void backgroundLoadingTask(void* parameter) {
    Serial.println("Background loading started on Core 1");
    Serial.printf("Total tasks: %d\n", loadingTasks.size());
    
    for (int i = 0; i < loadingTasks.size(); i++) {
        Serial.printf("Executing task %d/%d...\n", i+1, loadingTasks.size());
        bool success = loadingTasks[i]();
        if (success) {
            completedTasks++;
            // Обновляем целевой прогресс
            targetProgress = (completedTasks * 100.0f) / loadingTasks.size();
            Serial.printf("Task %d completed. Target progress: %.1f%%\n", i+1, targetProgress);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    loadingComplete = true;
    targetProgress = 100.0f;
    Serial.println("Background loading completed on Core 1");
    
    vTaskDelete(NULL);
}

// Добавить задачу загрузки
void add_loading_task(LoadingFunction func) {
    loadingTasks.push_back(func);
    Serial.printf("Added loading task. Total: %d\n", loadingTasks.size());
}

// Запустить фоновую загрузку
void start_background_loading() {
    if (loadingStarted) return;
    if (loadingTasks.empty()) {
        Serial.println("Warning: No loading tasks added!");
        return;
    }
    
    completedTasks = 0;
    loadingStarted = true;
    loadingComplete = false;
    currentProgress = 0.0f;
    targetProgress = 0.0f;
    
    Serial.printf("Starting background loading with %d tasks on Core 1\n", loadingTasks.size());
    
    xTaskCreatePinnedToCore(
        backgroundLoadingTask,
        "BgLoader",
        8192,
        NULL,
        1,
        &loadingTaskHandle,
        1  // Ядро 1
    );
}

// Получить целевой прогресс (для отладки)
int get_target_progress() {
    return (int)targetProgress;
}

// Проверить завершение
bool is_loading_complete() {
    return loadingComplete;
}

// =========== 3D ФУНКЦИИ ===========

void rotatePoint(Point3D* point, float ax, float ay, float az) {
    // Вращение вокруг оси X
    float y1 = point->y * cos(ax) - point->z * sin(ax);
    float z1 = point->y * sin(ax) + point->z * cos(ax);
    point->y = y1;
    point->z = z1;
    
    // Вращение вокруг оси Y
    float x2 = point->x * cos(ay) + point->z * sin(ay);
    float z2 = -point->x * sin(ay) + point->z * cos(ay);
    point->x = x2;
    point->z = z2;
    
    // Вращение вокруг оси Z
    float x3 = point->x * cos(az) - point->y * sin(az);
    float y3 = point->x * sin(az) + point->y * cos(az);
    point->x = x3;
    point->y = y3;
}

Point2D projectPoint(Point3D point) {
    // Фактор перспективы
    float factor = CAMERA_DISTANCE / (CAMERA_DISTANCE + point.z);
    
    Point2D result;
    result.x = SCREEN_CENTER_X + (int)(point.x * factor);
    result.y = SCREEN_CENTER_Y + (int)(point.y * factor);
    
    return result;
}

// =========== ФУНКЦИИ ОТРИСОВКИ ЛОГОТИПОВ ===========

void render_logo_with_pushImage(TFT_eSprite* screen, RawSprite* logo) {
    if (!logo->isValid || !logo->data) {
        Serial.println("ERROR: Invalid logo sprite for pushImage");
        return;
    }
    
    // Вычисляем позицию для центрирования
    int x = (320 - logo->width) / 2;
    int y = (240 - logo->height) / 2;
    
    Serial.printf("Rendering logo at (%d, %d), size %dx%d\n", x, y, logo->width, logo->height);
    
    // Используем pushImage для быстрой отрисовки
    screen->pushImage(x, y, logo->width, logo->height, logo->data);
    
    // Добавляем эффект затухания в конце
    unsigned long elapsed = millis() - stateStartTime;
    float fadeProgress = (float)elapsed / SPLASH_DURATION;
    
    // Плавное затухание в последние 0.5 секунды
    if (fadeProgress > 0.83f) { // 2.5 секунды из 3
        float alpha = 1.0f - (fadeProgress - 0.83f) / 0.17f;
        if (alpha < 0) alpha = 0;
        
        // Для эффекта затухания рисуем полупрозрачный черный слой
        if (alpha < 0.7f) {
            screen->fillRect(0, 0, 320, 240, TFT_BLACK);
        }
    }
}

// =========== ФУНКЦИЯ РЕНДЕРА 3D АНИМАЦИИ ===========

void render_3d_animation(TFT_eSprite* screen) {
    // Очищаем спрайт
    screen->fillSprite(TFT_BLACK);
    
    // Массивы для проекций вершин
    Point2D projectedVertices[8];
    
    // Проекция всех вершин
    for (int i = 0; i < 8; i++) {
        Point3D vertex = cubeVertices[i];
        
        // Применяем вращение
        Point3D rotatedVertex = vertex;
        rotatePoint(&rotatedVertex, angleX, angleY, angleZ);
        
        // Проекция в 2D
        projectedVertices[i] = projectPoint(rotatedVertex);
    }
    
    // Рисуем ребра куба
    for (int i = 0; i < 12; i++) {
        int v1 = cubeEdges[i][0];
        int v2 = cubeEdges[i][1];
        
        Point2D p1 = projectedVertices[v1];
        Point2D p2 = projectedVertices[v2];
        
        // Рисуем линию
        screen->drawLine(p1.x, p1.y, p2.x, p2.y, rgb(92, 207, 112));
        
        // Рисуем вершины (точки)
        screen->fillSmoothCircle(p1.x, p1.y, 2, rgb(92, 207, 112));
        screen->fillSmoothCircle(p2.x, p2.y, 2, rgb(92, 207, 112));
    }
    
    // Добавляем текст
    screen->setTextColor(rgb(3, 170, 31), TFT_BLACK);
    screen->setTextSize(2);
    screen->drawCentreString("Salty Console", 160, 20, 2);
    
    screen->setTextColor(rgb(3, 170, 31), TFT_BLACK);
    screen->setTextSize(1);
    screen->drawCentreString("by SALT LAMP", 160, 55, 1);
    
    screen->setTextColor(rgb(5, 109, 22), TFT_BLACK);
    screen->setTextSize(1);
    screen->drawCentreString("Loading...", 160, 180, 1);
    
    screen->setTextColor(rgb(25, 151, 113), TFT_BLACK);
    screen->setTextSize(1);
    screen->drawString("Version 2.5.14, but this is just the beginning...", 0, 230, 1);
    
    // Прогресс-бар
    int barWidth = 200;
    int barHeight = 8;
    int barX = (320 - barWidth) / 2;
    int barY = 200;
    
    screen->drawRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);
    
    int fillWidth = (int)(barWidth * currentProgress / 100.0f);
    if (fillWidth > 0) {
        for (int i = 0; i < fillWidth; i++) {
            int ratio = (i * 255) / barWidth;
            uint16_t color = screen->color565(
                0, 
                (ratio * 200) / 255, 
                255 - (ratio * 100) / 255
            );
            screen->drawFastVLine(barX + i, barY + 1, barHeight - 2, color);
        }
    }
    
    // Звезды
    static int stars[50][3];
    static bool starsInitialized = false;
    
    if (!starsInitialized) {
        for (int i = 0; i < 50; i++) {
            stars[i][0] = random(320);
            stars[i][1] = random(240);
            stars[i][2] = random(1, 4);
        }
        starsInitialized = true;
    }
    
    for (int i = 0; i < 50; i++) {
        stars[i][0] -= stars[i][2];
        if (stars[i][0] < 0) {
            stars[i][0] = 320;
            stars[i][1] = random(240);
        }
        
        uint16_t starColor;
        switch(stars[i][2]) {
            case 1: starColor = TFT_DARKGREY; break;
            case 2: starColor = TFT_LIGHTGREY; break;
            case 3: starColor = TFT_WHITE; break;
            default: starColor = TFT_WHITE;
        }
        
        screen->drawPixel(stars[i][0], stars[i][1], starColor);
    }
}

// =========== ФУНКЦИИ ИНТРО ===========

void intro_init(TFT_eSPI* tft) {
    introStartTime = millis();
    introActive = true;
    
    // Устанавливаем начальное состояние
    currentState = INTRO_STATE_SPLASH_1;
    stateStartTime = millis();
    
    // Загружаем логотипы
    if (!spritesLoaded) {
        load_logo_sprites();
    }
    
    // Сброс углов вращения
    angleX = 0.0f;
    angleY = 0.0f;
    angleZ = 0.0f;
    
    // Сброс прогресса
    currentProgress = 0.0f;
    targetProgress = 0.0f;
    
    // Настройка дисплея
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    Serial.println("Intro initialized");
}

bool intro_update(TFT_eSPI* tft, TFT_eSprite* screen) {
    if (!introActive) return false;
    
    unsigned long now = millis();
    unsigned long elapsedInState = now - stateStartTime;
    
    // Обновление состояния
    switch (currentState) {
        case INTRO_STATE_SPLASH_1:
            // Первый логотип показывается 3 секунды
            if (elapsedInState >= SPLASH_DURATION) {
                currentState = INTRO_STATE_SPLASH_2;
                stateStartTime = now;
                Serial.println("Switching to logo 2");
            }
            break;
            
        case INTRO_STATE_SPLASH_2:
            // Второй логотип показывается 3 секунды
            if (elapsedInState >= SPLASH_DURATION) {
                currentState = INTRO_STATE_ANIMATION;
                stateStartTime = now;
                Serial.println("Starting 3D animation");
                // Освобождаем память логотипов, они больше не нужны
                free_logo_sprites();
            }
            break;
            
        case INTRO_STATE_ANIMATION: {
            // Анимация куба длится INTRO_DURATION
            if (elapsedInState >= INTRO_DURATION) {
                currentState = INTRO_STATE_COMPLETE;
                introActive = false;
                Serial.println("Intro complete");
                return false;
            }
            
            // Обновление углов вращения
            float progress = (float)elapsedInState / INTRO_DURATION;
            
            // Плавное изменение скоростей вращения
            angleX += ROTATION_SPEED * (1.0f + sin(progress * PI) * 0.5f);
            angleY += ROTATION_SPEED * (0.8f + cos(progress * PI * 2) * 0.3f);
            angleZ += ROTATION_SPEED * (0.6f + sin(progress * PI * 3) * 0.2f);
            
            // Плавное обновление прогресс-бара
            if (currentProgress < targetProgress) {
                currentProgress += (targetProgress - currentProgress) * PROGRESS_SMOOTHING;
                if (targetProgress - currentProgress < 0.1f) {
                    currentProgress = targetProgress;
                }
            }
            break;
        }
            
        case INTRO_STATE_COMPLETE:
            return false;
    }
    
    return true;
}

void intro_render(TFT_eSprite* screen) {
    if (!introActive) return;
    
    switch (currentState) {
        case INTRO_STATE_SPLASH_1:
            // Показываем первый логотип с помощью pushImage
            if (logo1.isValid && logo1.data) {
                render_logo_with_pushImage(screen, &logo1);
            } else {
                // Fallback если логотип не загружен
                screen->fillSprite(TFT_BLACK); 
                screen->setTextColor(TFT_WHITE, TFT_BLACK);
                screen->setTextSize(2);
                screen->drawCentreString("Loading Logo 1...", 160, 110, 2);
                Serial.println("Logo 1 not valid, showing fallback text");
            }
            break;
            
        case INTRO_STATE_SPLASH_2:
            // Показываем второй логотип с помощью pushImage
            if (logo2.isValid && logo2.data) {
                render_logo_with_pushImage(screen, &logo2);
            } else {
                // Fallback если логотип не загружен
                screen->fillSprite(TFT_BLACK);
                screen->setTextColor(TFT_WHITE, TFT_BLACK);
                screen->setTextSize(2);
                screen->drawCentreString("Loading Logo 2...", 160, 110, 2);
                Serial.println("Logo 2 not valid, showing fallback text");
            }
            break;
            
        case INTRO_STATE_ANIMATION:
            // Рисуем 3D анимацию куба
            render_3d_animation(screen);
            break;
            
        default:
            screen->fillSprite(TFT_BLACK);
            break;
    }
}