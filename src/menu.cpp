#include "menu.h"
#include "audio_simple.h"
#include "input.h"
#include "shared.h"
#include "fps.h" 
#include "voltage.h"
#include "pong.h"
#include "pacman.h"
#include "achievements.h"
#include "doom.h"
#include "seabattle.h"
#include "lost_trail.h"
#include "sprite_loader.h"


// Глобальные переменные меню
GameState gameState = MENU;
extern TFT_eSPI tft;

// QR код
bool qrScreenActive = false;
RawSprite qrSprite = {nullptr, 0, 0, false, 0};

bool gameChanged = false;
int x; 
int menuSelection = 0;
// ИЗМЕНЕНО: сократил названия, чтобы поместились
const char* menuItems[] = {"Pong", "Pac-Man", "Achieve", "DOOM", "Sea battle", "Lost Trail", "Settings"};
const int menuItemCount = 7; 

extern unsigned long lastInputTime;
bool BTN = false;
bool lastBut = false;

bool VOLTAGErend = true;
bool OFF, exactlyOff, exactlyOffYes, offSelect, sleeping;

// Глобальная переменная для управления звуком
bool audioEnabled = true;  // По умолчанию звук включен

// Снежинки
bool snowEnabled = false;
const int MAX_SNOWFLAKES = 60;
float SNOW_SPEED = 0.9f;
float SNOW_SIZE = 3.0f;

enum SnowflakeType {
    SNOW_DOT,
    SNOW_CROSS,
    SNOW_PLUS
};

struct Snowflake {
    float x;
    float y;
    float speed;
    uint8_t type;
    uint8_t brightness;
    float swing;
    uint8_t layer;
};
Snowflake snowflakes[MAX_SNOWFLAKES];

const unsigned long BTN_DEB = 50;

static bool joyBtnPrevState = false;
static bool btnAPrevState = false;
static uint32_t joyBtnDebTimer = 0;
static uint32_t btnADebTimer = 0;
static bool joyBtnProcessed = false;
static bool btnAProcessed = false;

// =========== НАСТРОЙКИ ===========
int settingsSelection = 0;

// ИЗМЕНЕНО: добавил пункт для звука
const char* settingsItems[] = {
    "Snow: OFF",
    "FPS: OFF", 
    "Audio: ON",
    "QR Code",
    "Back"
};
const int settingsItemCount = 5;

float settingsHighlightProgress[5] = {0, 0, 0, 0, 0};
float settingsGlowProgress[5] = {0, 0, 0, 0, 0};

// =========== АНИМАЦИИ МЕНЮ ===========
float menuHighlightProgress[7] = {0, 0, 0, 0, 0, 0};
float menuGlowProgress[7] = {0, 0, 0, 0, 0, 0};
unsigned long lastMenuAnimTime = 0;
const float ANIM_SPEED = 0.20f;

// Цвета
const uint16_t COLOR_NORMAL = rgb(8, 83, 12);
const uint16_t COLOR_HIGHLIGHT = rgb(4, 194, 14);
const uint16_t COLOR_GLOW_BASE = rgb(6, 138, 13);

// Функция для получения цвета подсветки
uint16_t getGlowColor(float progress) {
    if (progress <= 0) return COLOR_NORMAL;
    
    uint8_t r1 = 8, g1 = 83, b1 = 12;
    uint8_t r2 = 6, g2 = 138, b2 = 13;
    
    uint8_t r = r1 + (r2 - r1) * progress;
    uint8_t g = g1 + (g2 - g1) * progress;
    uint8_t b = b1 + (b2 - b1) * progress;
    
    return rgb(r, g, b);
}

uint16_t getSettingsGlowColor(float progress) {
    if (progress <= 0) return COLOR_NORMAL;
    
    uint8_t r1 = 8, g1 = 83, b1 = 12;
    uint8_t r2 = 6, g2 = 138, b2 = 13;
    
    uint8_t r = r1 + (r2 - r1) * progress;
    uint8_t g = g1 + (g2 - g1) * progress;
    uint8_t b = b1 + (b2 - b1) * progress;
    
    return rgb(r, g, b);
}

void initSnowflakes() {
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        snowflakes[i].x = random(0, 320);
        snowflakes[i].y = random(-100, 240);
        snowflakes[i].layer = random(0, 3);
        
        float baseSpeed;
        switch(snowflakes[i].layer) {
            case 0:
                baseSpeed = 0.6f + random(0, 70) / 300.0f;
                break;
            case 1:
                baseSpeed = 0.4f + random(0, 60) / 300.0f;
                break;
            case 2:
                baseSpeed = 0.2f + random(0, 50) / 300.0f;
                break;
            default:
                baseSpeed = 0.3f + random(0, 100) / 300.0f;
        }
        
        snowflakes[i].speed = baseSpeed;
        snowflakes[i].type = random(0, 3);
        snowflakes[i].brightness = 100 + snowflakes[i].layer * 30 + random(0, 50); 
        snowflakes[i].swing = random(0, 314) / 100.0f;
    }
}

uint16_t getSnowColor(uint8_t brightness) {
    if (brightness > 220) return TFT_WHITE;
    if (brightness > 180) return 0xDEFB;
    if (brightness > 140) return 0xCE79;
    if (brightness > 100) return 0xBDF7;
    return 0x9CF3;
}

void snow_update() {
    if (!snowEnabled) return;
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        snowflakes[i].y += snowflakes[i].speed * SNOW_SPEED;
        
        float swingAmplitude;
        switch(snowflakes[i].layer) {
            case 0: swingAmplitude = 0.5f; break;
            case 1: swingAmplitude = 0.3f; break;
            case 2: swingAmplitude = 0.2f; break;
            default: swingAmplitude = 0.3f;
        }
        
        snowflakes[i].swing += 0.02f;
        snowflakes[i].x += sin(snowflakes[i].swing) * swingAmplitude;
        
        if (snowflakes[i].y > 260) {
            snowflakes[i].x = random(0, 320);
            snowflakes[i].y = random(-100, -20);
            
            uint8_t currentLayer = snowflakes[i].layer;
            
            float baseSpeed;
            switch(currentLayer) {
                case 0:
                    baseSpeed = 0.6f + random(0, 70) / 300.0f;
                    snowflakes[i].brightness = 180 + random(0, 76);
                    break;
                case 1:
                    baseSpeed = 0.4f + random(0, 60) / 300.0f;
                    snowflakes[i].brightness = 130 + random(0, 70);
                    break;
                case 2:
                    baseSpeed = 0.2f + random(0, 50) / 300.0f;
                    snowflakes[i].brightness = 100 + random(0, 60);
                    break;
                default:
                    baseSpeed = 0.3f + random(0, 100) / 300.0f;
            }
            
            snowflakes[i].speed = baseSpeed;
            snowflakes[i].type = random(0, 3);
            snowflakes[i].swing = random(0, 314) / 100.0f;
        }
        
        if (snowflakes[i].x < -20) snowflakes[i].x = 320;
        if (snowflakes[i].x > 340) snowflakes[i].x = 0;
    }
}

void drawSnowflake(TFT_eSprite* sprite, int x, int y, uint8_t type, uint8_t brightness, float size, uint8_t layer) {
    uint16_t color = getSnowColor(brightness);
    int s = (int)size;
    
    float layerSizeMod = 1.0f;
    switch(layer) {
        case 0: layerSizeMod = 1.5f; break;
        case 1: layerSizeMod = 1.0f; break;
        case 2: layerSizeMod = 0.7f; break;
    }
    
    s = (int)(s * layerSizeMod);
    if (s < 1) s = 1;
    
    if (x < 0 || x >= 320 || y < 0 || y >= 240) return;
    
    switch(type) {
        case SNOW_DOT:
            if (s <= 1) {
                sprite->drawPixel(x, y, color);
            } else if (s == 2) {
                sprite->fillCircle(x, y, 1, color);
                if (x > 0) sprite->drawPixel(x-1, y, color);
                if (x < 319) sprite->drawPixel(x+1, y, color);
                if (y > 0) sprite->drawPixel(x, y-1, color);
                if (y < 239) sprite->drawPixel(x, y+1, color);
            } else {
                sprite->fillSmoothCircle(x, y, s, color);
            }
            break;
            
        case SNOW_CROSS:
            for (int dy = -s; dy <= s; dy++) {
                int drawY = y + dy;
                if (drawY >= 0 && drawY < 240) {
                    sprite->drawPixel(x, drawY, color);
                }
            }
            for (int dx = -s; dx <= s; dx++) {
                int drawX = x + dx;
                if (drawX >= 0 && drawX < 320) {
                    sprite->drawPixel(drawX, y, color);
                }
            }
            if (layer == 0) {
                for (int d = -s; d <= s; d++) {
                    int drawX1 = x + d;
                    int drawY1 = y + d;
                    if (drawX1 >= 0 && drawX1 < 320 && drawY1 >= 0 && drawY1 < 240) {
                        sprite->drawPixel(drawX1, drawY1, color);
                    }
                    
                    int drawX2 = x + d;
                    int drawY2 = y - d;
                    if (drawX2 >= 0 && drawX2 < 320 && drawY2 >= 0 && drawY2 < 240) {
                        sprite->drawPixel(drawX2, drawY2, color);
                    }
                }
            }
            break;
            
        case SNOW_PLUS:
            for (int dy = -s; dy <= s; dy++) {
                int drawY = y + dy;
                if (drawY >= 0 && drawY < 240) {
                    sprite->drawPixel(x, drawY, color);
                }
            }
            for (int dx = -s; dx <= s; dx++) {
                int drawX = x + dx;
                if (drawX >= 0 && drawX < 320) {
                    sprite->drawPixel(drawX, y, color);
                }
            }
            break;
    }
}

void snow_render(TFT_eSprite* sprite) {
    if (!snowEnabled) return;
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        if (snowflakes[i].layer == 2) {
            drawSnowflake(sprite, 
                         (int)snowflakes[i].x, 
                         (int)snowflakes[i].y, 
                         snowflakes[i].type, 
                         snowflakes[i].brightness,
                         SNOW_SIZE,
                         snowflakes[i].layer);
        }
    }
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        if (snowflakes[i].layer == 1) {
            drawSnowflake(sprite, 
                         (int)snowflakes[i].x, 
                         (int)snowflakes[i].y, 
                         snowflakes[i].type, 
                         snowflakes[i].brightness,
                         SNOW_SIZE,
                         snowflakes[i].layer);
        }
    }
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        if (snowflakes[i].layer == 0) {
            drawSnowflake(sprite, 
                         (int)snowflakes[i].x, 
                         (int)snowflakes[i].y, 
                         snowflakes[i].type, 
                         snowflakes[i].brightness,
                         SNOW_SIZE,
                         snowflakes[i].layer);
        }
    }
}

void menu_init(TFT_eSPI* tft) {
    settingsSelection = 0;
    CHANGES_BTN = true;
    BTN = false;
    
    joyBtnPrevState = false;
    btnAPrevState = false;
    joyBtnDebTimer = 0;
    btnADebTimer = 0;
    joyBtnProcessed = false;
    btnAProcessed = false;
    
    menuBgSprite.createSprite(320, 240);
    menuItemSprite.createSprite(160, 50);
    menuHighlightSprite.createSprite(160, 50);
    screen.createSprite(320, 240);
    fpsSprite.createSprite(60, 20);
    
    initSnowflakes();

    menuHighlightSprite.setSwapBytes(false);
    menuBgSprite.setSwapBytes(false);
    screen.setSwapBytes(false);

    menuItemSprite.setTextColor(TFT_BLUE, TFT_DARKCYAN, true);
    menuItemSprite.setTextSize(2);
    
    menuHighlightSprite.setTextColor(TFT_DARKCYAN, TFT_BLUE, true);
    menuHighlightSprite.setTextSize(2);
    // screen.setFreeFont(&FreeSerif9pt7b);  
}

void settings_init() {
    settingsSelection = 0;
    
    // Обновляем текст пунктов настроек
    if (snowEnabled) settingsItems[0] = "Snow: ON";
    else settingsItems[0] = "Snow: OFF";
    
    if (FPSrend) settingsItems[1] = "FPS: ON";
    else settingsItems[1] = "FPS: OFF";
    
    // Новый пункт для звука
    if (audioEnabled) settingsItems[2] = "Audio: ON";
    else settingsItems[2] = "Audio: OFF";
    
    settingsItems[3] = "QR Code";
    settingsItems[4] = "Back";
    
    // Сбрасываем анимации
    for (int i = 0; i < settingsItemCount; i++) {
        settingsHighlightProgress[i] = 0;
        settingsGlowProgress[i] = 0;
    }
    
    CHANGES_BTN = true;
}

// Функция для загрузки QR кода
void loadQRSprite() {
    if (!qrSprite.isValid) {
        qrSprite = loadRawSprite("/git.raw", 82, 82);
        if (!qrSprite.isValid) {
            Serial.println("Failed to load QR code");
        }
    }
}

// Функция для отображения QR экрана
void showQRScreen() {
    if (!qrSprite.isValid) {
        loadQRSprite();
    }
    
    screen.fillSprite(rgb(0, 40, 8));
    
    // Заголовок
    screen.setTextColor(rgb(8, 83, 12));
    screen.setTextSize(2);
    screen.setTextDatum(TL_DATUM);
    screen.drawString("GitHub QR", 110, 10);
    
    // Рисуем рамку вокруг QR
    screen.drawRoundRect(119, 60, 82, 82, 5, rgb(8, 83, 12));
    screen.drawRoundRect(118, 59, 84, 84, 5, rgb(8, 83, 12));
    
    // Рисуем QR код если загружен
    if (qrSprite.isValid && qrSprite.data) {
        // Используем pushImage для отображения спрайта
        screen.pushImage(120, 60, qrSprite.width, qrSprite.height, qrSprite.data);
    } else {
        screen.setTextColor(rgb(141, 28, 0));
        screen.setTextSize(1);
        screen.drawString("QR not found", 135, 95);
    }
    
    // Подсказка
    screen.setTextSize(1);
    screen.setTextColor(rgb(8, 83, 12));
    screen.drawString("Press any button to exit", 95, 170);
    
    // Рисуем снежинки если включены
    if (snowEnabled) {
        snow_render(&screen);
    }
    
    screen.pushSprite(0, 0);
}


void menu_update() {
    unsigned long currentTime = millis();
    
    voltageDisplay.update();

    // ===== ОБРАБОТКА QR ЭКРАНА =====
    if (qrScreenActive) {
        // Любая кнопка выходит из QR экрана
        if (joy2.button == PRESSED || joy1.button == PRESSED || btn_lt.state == PRESSED) {
            qrScreenActive = false;
            CHANGES_BTN = true;
            // Ждем отпускания кнопки
            delay(200);
        }
        return; // Выходим из update, не обрабатываем остальное
    }
    
    // ===== ОБНОВЛЕНИЕ АНИМАЦИЙ И СНЕГА (КАЖДЫЙ КАДР) =====
    // Это должно работать ВСЕГДА, независимо от gameState и JOY_DELAY
    
    // Обновляем снежинки (каждые 16мс ~60 FPS) - ДЛЯ ЛЮБОГО СОСТОЯНИЯ
    static unsigned long lastSnowUpdate = 0;
    if (currentTime - lastSnowUpdate >= 16) {
        if (snowEnabled) {
            snow_update();
            CHANGES_BTN = true; // Перерисовываем если снег включен
        }
        lastSnowUpdate = currentTime;
    }
    
    // Обновляем анимации меню (для MENU)
    if (gameState == MENU) {
        bool needRedraw = false;
        for (int i = 0; i < menuItemCount; i++) {
            // Прогресс выделения
            if (i == menuSelection) {
                if (menuHighlightProgress[i] < 1.0f) {
                    menuHighlightProgress[i] += ANIM_SPEED;
                    if (menuHighlightProgress[i] > 1.0f) menuHighlightProgress[i] = 1.0f;
                    needRedraw = true;
                }
            } else {
                if (menuHighlightProgress[i] > 0) {
                    menuHighlightProgress[i] -= ANIM_SPEED * 0.7f;
                    if (menuHighlightProgress[i] < 0) menuHighlightProgress[i] = 0;
                    needRedraw = true;
                }
            }
            
            // Прогресс подсветки соседей
            if (abs(i - menuSelection) == 1) {
                if (menuGlowProgress[i] < 0.5f) {
                    menuGlowProgress[i] += ANIM_SPEED * 0.8f;
                    if (menuGlowProgress[i] > 0.5f) menuGlowProgress[i] = 0.5f;
                    needRedraw = true;
                }
            } else {
                if (menuGlowProgress[i] > 0) {
                    menuGlowProgress[i] -= ANIM_SPEED * 0.8f;
                    if (menuGlowProgress[i] < 0) menuGlowProgress[i] = 0;
                    needRedraw = true;
                }
            }
        }
        if (needRedraw) CHANGES_BTN = true;
    }
    
    // Обновляем анимации для настроек (для SETTINGS)
    else if (gameState == SETTINGS) {
        bool needRedraw = false;
        for (int i = 0; i < settingsItemCount; i++) {
            // Прогресс выделения для настроек
            if (i == settingsSelection) {
                if (settingsHighlightProgress[i] < 1.0f) {
                    settingsHighlightProgress[i] += ANIM_SPEED;
                    if (settingsHighlightProgress[i] > 1.0f) settingsHighlightProgress[i] = 1.0f;
                    needRedraw = true;
                }
            } else {
                if (settingsHighlightProgress[i] > 0) {
                    settingsHighlightProgress[i] -= ANIM_SPEED * 0.7f;
                    if (settingsHighlightProgress[i] < 0) settingsHighlightProgress[i] = 0;
                    needRedraw = true;
                }
            }
            
            // Прогресс подсветки соседей для настроек
            if (abs(i - settingsSelection) == 1) {
                if (settingsGlowProgress[i] < 0.5f) {
                    settingsGlowProgress[i] += ANIM_SPEED * 0.8f;
                    if (settingsGlowProgress[i] > 0.5f) settingsGlowProgress[i] = 0.5f;
                    needRedraw = true;
                }
            } else {
                if (settingsGlowProgress[i] > 0) {
                    settingsGlowProgress[i] -= ANIM_SPEED * 0.8f;
                    if (settingsGlowProgress[i] < 0) settingsGlowProgress[i] = 0;
                    needRedraw = true;
                }
            }
        }
        if (needRedraw) CHANGES_BTN = true;
    }
    
    // ===== ОБРАБОТКА ВВОДА (С ЗАДЕРЖКОЙ) =====
    static unsigned long lastJoyTime = 0;
    const unsigned long JOY_DELAY = 200;
    
    // Навигация только если прошло достаточно времени
    if (currentTime - lastJoyTime >= JOY_DELAY) {
        
        // МЕНЮ
        if(gameState == MENU) {
            if(joy2.y > 3000 && !OFF) {
                menuSelection = (menuSelection - 1 + menuItemCount) % menuItemCount;
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
            else if(joy2.y < 1000 && !OFF) {
                menuSelection = (menuSelection + 1) % menuItemCount;
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
            if(joy2.x > 3000 && !exactlyOff) {                
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
                OFF = true;
            }
            else if(joy2.x < 1000 && !exactlyOff) {               
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
                OFF = false;
                exactlyOff = false;
            }
            if(joy2.x > 3000 && exactlyOff) {                
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                offSelect = true;
            }
            else if(joy2.x < 1000 && exactlyOff) {               
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                offSelect = false;
            }

            if(!lastBut && joy2.button == PRESSED && OFF && !exactlyOff) {
                exactlyOff = true;
                lastBut = true;
                lastJoyTime = currentTime;
            }

            if(!lastBut && joy2.button == PRESSED && OFF && exactlyOff && !offSelect) {
                exactlyOff = false;
                OFF = false;
                lastBut = true;
                lastJoyTime = currentTime;
            }

            if(!lastBut && joy2.button == PRESSED && OFF && exactlyOff && offSelect) {                
                lastBut = true;
                lastJoyTime = currentTime;
                sleeping = true;
                while(joy2.button == PRESSED){
                    input_update();
                    delay(100);
                }                
                esp_light_sleep_start();
                exactlyOff = false;
                OFF = false;
                offSelect = false;
                input_update();
                delay(1000);
                while(joy2.button == PRESSED){
                    input_update();
                    delay(100);
                }
                delay(1000);
                sleeping = false;  
            }

            // Обработка нажатия кнопки
            if(joy2.button != PRESSED) {
                lastBut = false;
            }

            if (!lastBut && joy2.button == PRESSED && !OFF) {        
                if(menuSelection == 0) {
                    gameState = PONG;
                    gameChanged = true;
                    pong_menu_init();
                } else if(menuSelection == 1) {
                    gameState = PACMAN;
                    gameChanged = true;
                    pacman_init();  
                } else if(menuSelection == 2) {
                    gameState = ACHIEVEMENTS_SCREEN;
                    gameChanged = true;
                    CHANGES_BTN = true;
                } else if(menuSelection == 3) {
                    gameState = DOOM;
                    gameChanged = true;
                    doom_init();
                } else if(menuSelection == 4) {
                    gameState = SEABATTLE;
                    gameChanged = true;
                    audio_play("/sound/sea.mp3");
                    seabattle_init();
                } else if(menuSelection == 5) {
                    gameState = LOST_TRAIL;
                    gameChanged = true;
                    lost_trail_init(&tft);
                }else if(menuSelection == 6) { 
                    gameState = SETTINGS;
                    gameChanged = false;
                    settings_init();
                }

                CHANGES_BTN = true;
                lastBut = true;
                lastJoyTime = currentTime;
            }
        }
        
        // НАСТРОЙКИ
        else if(gameState == SETTINGS) {
            if(joy2.y > 3000) {
                settingsSelection = (settingsSelection - 1 + settingsItemCount) % settingsItemCount;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
            else if(joy2.y < 1000) {
                settingsSelection = (settingsSelection + 1) % settingsItemCount;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
            if(joy2.x > 3000) {                
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
                OFF = true;
            }
            else if(joy2.x < 1000) {               
                lastInputTime = currentTime;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
                OFF = false;
            }

            static bool lastButSettings = false;
            if(joy2.button != PRESSED) {
                lastButSettings = false;
            }

            if (!lastButSettings && joy2.button == PRESSED) {
                switch(settingsSelection) {
                    case 0:
                        snowEnabled = !snowEnabled;
                        settingsItems[0] = snowEnabled ? "Snow: ON" : "Snow: OFF";
                        break;
                        
                    case 1:
                        FPSrend = !FPSrend;
                        settingsItems[1] = FPSrend ? "FPS: ON" : "FPS: OFF";
                        break;
                        
                    case 2:  // Новый пункт для звука
                        audioEnabled = !audioEnabled;
                        settingsItems[2] = audioEnabled ? "Audio: ON" : "Audio: OFF";
                        // Останавливаем воспроизведение при выключении звука
                        if (!audioEnabled) {
                            audio_stop();
                            Serial.println("Audio disabled");
                        } else {
                            Serial.println("Audio enabled");
                        }
                        break;
                        
                    case 3:  // QR код
                        qrScreenActive = true;
                        loadQRSprite();
                        CHANGES_BTN = true;
                        break;
                        
                    case 4:  // Back
                        gameState = MENU;
                        menuSelection = 5;
                        CHANGES_BTN = true;
                        break;
                }
                
                CHANGES_BTN = true;
                lastButSettings = true;
                lastJoyTime = currentTime;
            }
        }
    }

    // Общие кнопки (без задержки)
    if(joy1.button == PRESSED){
        FPSrend = !FPSrend;
        CHANGES_BTN = true;  
    }
    
    static unsigned long lastVoltageToggle = 0;
    if(btn_lt.state == JUST_PRESSED && currentTime - lastVoltageToggle > 200) {
        VOLTAGErend = !VOLTAGErend;
        CHANGES_BTN = true;
        lastVoltageToggle = currentTime;
    }
}

void drawRobot(TFT_eSprite* sprite, int x, int y) {
    uint16_t robotMainColor = rgb(8, 83, 12);
    uint16_t robotAccentColor = rgb(4, 194, 14);
    uint16_t robotDarkColor = rgb(0, 40, 8);
    bool eyesSelect = true;
    int yEyes = 0;
    
    sprite->fillRoundRect(x, y, 100, 100, 3, robotMainColor);    
    sprite->fillRect(x + 7, y + 7, 40, 40, robotDarkColor);    
    sprite->fillRect(x + 54, y + 7, 40, 40, robotDarkColor);    
    sprite->fillRect(x + 7, y + 59, 87, 30, robotDarkColor);
    sprite->drawFastVLine(x + 24, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 25, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 41, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 42, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 58, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 59, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 75, y + 59, 30, robotMainColor);
    sprite->drawFastVLine(x + 76, y + 59, 30, robotMainColor); 
    
    if(gameState == MENU){
        yEyes = map(menuSelection, 0, 5, 0, 26);
    } else if(gameState == SETTINGS){
        yEyes = map(settingsSelection, 0, 3, 0, 26);
    } else {
        yEyes = 13;  // Значение по умолчанию
    }
    
    if(eyesSelect){
        sprite->fillRect(x + 16, y + 11 + yEyes, 5, 5, robotMainColor); // глаз 1
        sprite->fillRect(x + 60, y + 11 + yEyes, 5, 5, robotMainColor); // глаз 2
    }else{
        sprite->fillRect(x + 26, y, 4, 4, robotMainColor); // глаз 1
        sprite->fillRect(x + 73, y, 4, 4, robotMainColor); // глаз 2
    }
}

void drawRobiComment(TFT_eSprite* sprite, int x, int y, const char* text) {
    sprite->drawSmoothRoundRect(x, y, 1, 0, 100, 42, rgb(8, 83, 12), rgb(0, 40, 8));
    sprite->setTextColor(rgb(8, 83, 12), rgb(0, 40, 8));
    sprite->setTextSize(1);
    sprite->drawString("Robi:", x + 40, y + 4);
    sprite->drawFastHLine(x, y + 13, 100, rgb(8, 83, 12));
    sprite->drawFastHLine(x, y + 14, 100, rgb(8, 83, 12));
    sprite->drawString(text, x + 5, y + 20);
}

void drawOffButton(TFT_eSprite* sprite, int x, int y, bool select, bool exactlyOff) {
    int size = 1;
    x = x + 16;
    y = y + 16;
    sprite->fillSmoothCircle(x, y, 16 *size, select ? rgb(4, 194, 14) : rgb(8, 83, 12));
    sprite->drawSmoothArc(x, y, 12 *size, 10, 220, 140, rgb(0, 40, 8),  select ? rgb(4, 194, 14) : rgb(8, 83, 12), true);
    sprite->drawWideLine(x, y, x, y - 12 *size, 3 *size, rgb(0, 40, 8), select ? rgb(4, 194, 14) : rgb(8, 83, 12));
    if(exactlyOff){
        screen.setTextSize(2);
        sprite->fillRect(105, 80, 85, 55, rgb(144, 153, 18));
        sprite->setTextColor(rgb(0, 40, 8));
        sprite->setCursor(110, 100);            
        sprite->print("YES");
        sprite->setCursor(160, 100);
        sprite->print("NO");
        if(offSelect){
            sprite->setCursor(110, 100);
            sprite->setTextColor(rgb(0, 255, 51));
            sprite->print("YES");
        }else{
            sprite->setTextColor(rgb(255, 0, 0));
            sprite->setCursor(160, 100);
            sprite->print("NO");
        }
    }
}

void drawBattery(TFT_eSprite* sprite, int x, int y, int level) {
    uint16_t color;
    sprite->drawSmoothRoundRect(x, y, 3, 1, 49, 25, rgb(8, 83, 12), rgb(0, 40, 8));
    sprite->fillSmoothRoundRect(x + 52, y + 5, 3, 16, 2, rgb(8, 83, 12), rgb(0, 40, 8));
    if(level >= 0 && level <= 15){
        color = rgb(141, 28, 0);
    }
    if(level >= 15 && level <= 33){
        color = rgb(141, 68, 0);
    }
    if(level >= 33 && level <= 100){
        color = rgb(8, 83, 12);
    }
    if(level >= 0){
        int bar = level > 33 ? 12 : map(level, 0, 33, 0, 12);
        sprite->fillSmoothRoundRect(x + 5, y + 5, bar, 16, 2, color, rgb(0, 40, 8));
    } 
    if(level >= 33){
        int bar = level > 66 ? 12 : map(level, 34, 66, 0, 12);
        sprite->fillSmoothRoundRect(x + 19, y + 5, bar, 16, 2, color, rgb(0, 40, 8));
    } 
    if(level >= 66){
        int bar = level > 100 ? 12 : map(level, 67, 100, 0, 12);
        sprite->fillSmoothRoundRect(x + 33, y + 5, bar, 16, 2, color, rgb(0, 40, 8));
    }
}


void menu_render(TFT_eSPI* tft) {
  // Если активен QR экран - показываем его
  if (qrScreenActive) {
    showQRScreen();
    return;
  }
  if(sleeping){
    screen.fillSprite(rgb(0, 0, 0));
    screen.pushSprite(0, 0);
    return;
  }else
    // =========== Прорисовка фона =========== menuSprite.isValid
    if (false) {
        // Рисуем фон из спрайта
        //screen.pushImage(0, 0, 320, 240, image);
        screen.pushImage(0, 0, menuSprite.width, menuSprite.height, menuSprite.data);
    } else {
        // Рисуем обычный фон
        screen.fillSprite(rgb(0, 40, 8));
    }
    //screen.fillSprite(rgb(0, 40, 8));
    screen.drawRoundRect(1,1,318,238,8,rgb(8, 83, 12));
    screen.drawRoundRect(2,2,316,236,7,rgb(8, 83, 12));
    screen.drawFastHLine(1, 30, 318, rgb(8, 83, 12));
    screen.drawFastHLine(1, 31, 318, rgb(8, 83, 12));
    screen.setTextColor(rgb(8, 83, 12));
    screen.setTextSize(3);
    screen.setTextDatum(TL_DATUM);
    
    if (gameState == MENU) {
        screen.drawString("MENU", 124, 7);
    } else if (gameState == SETTINGS) {
        screen.drawString("SETTINGS", 100, 7);
    }
    
    // =========== Рисуем декоративного робота ===========
    
    int robotX, robotY;
    robotX = 200;
    robotY = 85;


    
    drawRobot(&screen, robotX, robotY);
    if(exactlyOff){
        drawRobiComment(&screen, robotX, robotY + 105, "exactly?");
    }else{
        drawRobiComment(&screen, robotX, robotY + 105, "Hello world!");
    }
    

    // =========== Рисуем уровень заряда ===========
    drawBattery(&screen, 245, 40, 43); 
    
    drawOffButton(&screen, 200, 40, OFF, exactlyOff);

    // =========== Рисуем пункты ===========
    int yPos = 39;  // ИЗМЕНЕНО: сдвинул выше, чтобы поместилось 6 пунктов
    int xPos = 18;
    int spacing = 30; // ИЗМЕНЕНО: уменьшил расстояние между пунктами
    
    if (gameState == MENU) {
        for(int i = 0; i < menuItemCount; i++) {
            screen.setTextSize(2);
            
            int xOffset = 0;
            if (i == menuSelection) {
                xOffset = (int)(3 * menuHighlightProgress[i]);
            }
            
            uint16_t textColor;
            
            if (i == menuSelection) {
                if (menuHighlightProgress[i] >= 0.99f) {
                    textColor = COLOR_HIGHLIGHT;
                } else {
                    uint8_t r1 = 8, g1 = 83, b1 = 12;
                    uint8_t r2 = 4, g2 = 194, b2 = 14;
                    
                    uint8_t r = r1 + (r2 - r1) * menuHighlightProgress[i];
                    uint8_t g = g1 + (g2 - g1) * menuHighlightProgress[i];
                    uint8_t b = b1 + (b2 - b1) * menuHighlightProgress[i];
                    
                    textColor = rgb(r, g, b);
                }
            } else {
                textColor = getGlowColor(menuGlowProgress[i]);
            }
            
            screen.setTextColor(textColor);
            screen.drawString(menuItems[i], xPos + xOffset, yPos + i * spacing);
        }
    }
    
    else if (gameState == SETTINGS) {
        for(int i = 0; i < settingsItemCount; i++) {
            screen.setTextSize(2);
            
            int xOffset = 0;
            if (i == settingsSelection) {
                xOffset = (int)(3 * settingsHighlightProgress[i]);
            }
            
            uint16_t textColor;
            
            if (i == settingsSelection) {
                if (settingsHighlightProgress[i] >= 0.99f) {
                    textColor = COLOR_HIGHLIGHT;
                } else {
                    uint8_t r1 = 8, g1 = 83, b1 = 12;
                    uint8_t r2 = 4, g2 = 194, b2 = 14;
                    
                    uint8_t r = r1 + (r2 - r1) * settingsHighlightProgress[i];
                    uint8_t g = g1 + (g2 - g1) * settingsHighlightProgress[i]; 
                    uint8_t b = b1 + (b2 - b1) * settingsHighlightProgress[i];
                    
                    textColor = rgb(r, g, b);
                }
            } else {
                textColor = getSettingsGlowColor(settingsGlowProgress[i]);
            }
            
            screen.setTextColor(textColor);
            screen.drawString(settingsItems[i], xPos + xOffset, yPos + i * spacing);
        }
    }
    
    // Рисуем снежинки
    if (snowEnabled) {
        snow_render(&screen);
    }
    
    // Рисуем FPS и напряжение
    if(FPSrend){
        fps.drawToSprite(&screen, 220 - 60, 0);
        voltageDisplay.drawToSprite(&screen, 260, 0);
    }

    achievements_render();

    screen.pushSprite(0, 0);
}