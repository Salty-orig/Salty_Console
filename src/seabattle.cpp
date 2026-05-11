#include "seabattle.h"
#include "input.h"
#include "shared.h"
#include <TFT_eSPI.h>
#include "fps.h"
#include "sprite_loader.h"

// Глобальные переменные игры
Ship ships[MAX_SHIPS];
Crosshair crosshair;
Shot shots[MAX_SHOTS];
int score = 0;
int lives = 3;
int level = 1;
int shipsSunk = 0;
bool gameOver = false;
unsigned long lastSpawnTime = 0;
unsigned long lastFrameTimeSeabattle = 0;
bool initialHitPlayed = false;

// Структура для спрайтов
struct SeabattleSprites {
    RawSprite shipSmall;
    RawSprite shipMedium;
    RawSprite shipLarge;
    RawSprite explosion[4];  // Анимация взрыва - 4 кадра
    RawSprite crosshairSprite;
    RawSprite shotSprite;
    RawSprite background;    // Фон моря
    RawSprite heart;         // Спрайт сердечка
};

static SeabattleSprites sprites;
static bool spritesLoaded = false;
static int explosionFrame = 0;
static int seabattleLoadingSpinnerId = 2;

bool loadingSeabattleSprites = false;

// Функция для правильного отображения цвета (swap bytes если нужно)
static inline uint16_t correctColor(uint16_t color) {
    // Если цвет черный (прозрачный) - возвращаем как есть
    if (color == 0x0000) return 0x0000;
    
    // Меняем местами байты для правильного отображения
    // RGB565 формат: старший байт - RRRR RGGG, младший - GGGB BBBB
    return ((color >> 8) & 0xFF) | ((color & 0xFF) << 8);
}

// Функция для отрисовки спрайта попиксельно с правильными цветами
static void drawSpritePixelByPixel(int x, int y, RawSprite* sprite, bool mirror = false, uint16_t transparentColor = 0x0000) {
    if (!sprite || !sprite->isValid) return;
    
    if (mirror) {
        for (int sx = 0; sx < sprite->width; sx++) {
            for (int sy = 0; sy < sprite->height; sy++) {
                uint16_t pixel = sprite->data[sy * sprite->width + sx];
                uint16_t correctedPixel = correctColor(pixel);
                if (correctedPixel != transparentColor) {
                    screen.drawPixel(x + sprite->width - 1 - sx, y + sy, correctedPixel);
                }
            }
        }
    } else {
        for (int sx = 0; sx < sprite->width; sx++) {
            for (int sy = 0; sy < sprite->height; sy++) {
                uint16_t pixel = sprite->data[sy * sprite->width + sx];
                uint16_t correctedPixel = correctColor(pixel);
                if (correctedPixel != transparentColor) {
                    screen.drawPixel(x + sx, y + sy, correctedPixel);
                }
            }
        }
    }
}

// Вспомогательная функция для проверки существования файла
bool fileExistsSea(const char* filename) {
    File file = SD.open(filename);
    if (file) {
        file.close();
        return true;
    }
    return false;
}

// Загрузка спрайтов
void loadSeabattleSprites() {
    if (spritesLoaded) return;
    
    Serial.println("Loading Sea Battle sprites...");
    
    // Загружаем фон моря
    if (fileExistsSea("/seabattle/sea_background.raw")) {
        sprites.background = loadRawSprite("/seabattle/sea_background.raw", 320, 240);
    } else {
        sprites.background.isValid = false;
        Serial.println("sea_background.raw not found");
    }
    
    // Загружаем спрайты кораблей
    if (fileExistsSea("/seabattle/ship_small.raw")) {
        sprites.shipSmall = loadRawSprite("/seabattle/ship_small.raw", SHIP_SMALL_WIDTH, SHIP_SMALL_HEIGHT);
    } else {
        sprites.shipSmall.isValid = false;
        Serial.println("ship_small.raw not found");
    }
    
    if (fileExistsSea("/seabattle/ship_medium.raw")) {
        sprites.shipMedium = loadRawSprite("/seabattle/ship_medium.raw", SHIP_MEDIUM_WIDTH, SHIP_MEDIUM_HEIGHT);
    } else {
        sprites.shipMedium.isValid = false;
        Serial.println("ship_medium.raw not found");
    }
    
    if (fileExistsSea("/seabattle/ship_large.raw")) {
        sprites.shipLarge = loadRawSprite("/seabattle/ship_large.raw", SHIP_LARGE_WIDTH, SHIP_LARGE_HEIGHT);
    } else {
        sprites.shipLarge.isValid = false;
        Serial.println("ship_large.raw not found");
    }
    
    // Загружаем анимацию взрыва - 4 кадра
    for (int i = 0; i < 4; i++) {
        char filename[32];
        sprintf(filename, "/seabattle/explosion%d.raw", i);
        if (fileExistsSea(filename)) {
            sprites.explosion[i] = loadRawSprite(filename, 32, 32);
        } else {
            sprites.explosion[i].isValid = false;
        }
    }
    
    // Загружаем спрайт прицела
    if (fileExistsSea("/seabattle/crosshair.raw")) {
        sprites.crosshairSprite = loadRawSprite("/seabattle/crosshair.raw", CROSSHAIR_SIZE * 2, CROSSHAIR_SIZE * 2);
    } else {
        sprites.crosshairSprite.isValid = false;
    }
    
    // Загружаем спрайт снаряда
    if (fileExistsSea("/seabattle/shot.raw")) {
        sprites.shotSprite = loadRawSprite("/seabattle/shot.raw", 4, 8);
    } else {
        sprites.shotSprite.isValid = false;
    }
    
    // Загружаем спрайт сердечка
    if (fileExistsSea("/seabattle/heart.raw")) {
        sprites.heart = loadRawSprite("/seabattle/heart.raw", 16, 16);
    } else {
        sprites.heart.isValid = false;
        Serial.println("heart.raw not found");
    }
    
    spritesLoaded = true;
    Serial.println("Sea Battle sprites loaded!");
}

void seabattle_init() {
    Serial.println("Initializing Sea Battle...");
    loadingSeabattleSprites = true;
    startSpinner(160, 150, 8, 3, 800);
    // Загружаем спрайты
    loadSeabattleSprites();
    
    if (seabattleLoadingSpinnerId >= 0) {
        stopSpinner(seabattleLoadingSpinnerId);
        seabattleLoadingSpinnerId = -1;
    }
    
    // Инициализация кораблей
    for(int i = 0; i < MAX_SHIPS; i++) {
        ships[i].active = false;
        ships[i].isHit = false;
        ships[i].hitTimer = 0;
    }
    
    // Инициализация прицела
    crosshair.x = 160.0f;
    crosshair.y = 120.0f;
    crosshair.firing = false;
    crosshair.fireTimer = 0;
    crosshair.reloadTimer = 0;
    
    // Инициализация выстрелов
    for(int i = 0; i < MAX_SHOTS; i++) {
        shots[i].active = false;
    }
    
    // Сброс игровых параметров
    score = 0;
    lives = 3;
    level = 1;
    shipsSunk = 0;
    gameOver = false;
    
    lastSpawnTime = millis();
    lastFrameTimeSeabattle = millis();
    
    // Создаем начальные корабли
    for(int i = 0; i < 4; i++) {
        seabattle_spawn_ship();
        delay(100);
    }
    loadingSeabattleSprites = false;
    Serial.println("Sea Battle initialized!");
}

void seabattle_update() {
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastFrameTimeSeabattle) / 1000.0f;
    if (deltaTime > 0.033f) deltaTime = 0.033f;
    lastFrameTimeSeabattle = currentTime;
    
    if(gameOver) {
        if(joy2.button != PRESSED) {
            lastBut = false;
        }
        
        if(!lastBut && joy2.button == PRESSED){
            if (gameState == SEABATTLE) {
                gameState = MENU;
                gameChanged = false;
                lastBut = true;
            }
        }
        return;
    }
    
    seabattle_update_ships(deltaTime);
    seabattle_update_crosshair(deltaTime);
    seabattle_update_shots(deltaTime);
    seabattle_check_collisions();
    
    // Спавн новых кораблей
    int spawnDelay = max(500, 2000 - (level * 100));
    if(currentTime - lastSpawnTime > spawnDelay) {
        seabattle_spawn_ship();
        lastSpawnTime = currentTime;
    }
    
    // Проверка проигрыша
    for(int i = 0; i < MAX_SHIPS; i++) {
        if(ships[i].active && !ships[i].isHit) {
            int shipHeight;
            switch(ships[i].type) {
                case SHIP_SMALL: shipHeight = SHIP_SMALL_HEIGHT; break;
                case SHIP_MEDIUM: shipHeight = SHIP_MEDIUM_HEIGHT; break;
                case SHIP_LARGE: shipHeight = SHIP_LARGE_HEIGHT; break;
            }
            
            if(ships[i].y > 240 - shipHeight) {
                ships[i].active = false;
                lives--;
                
                if(lives <= 0) {
                    gameOver = true;
                }
            }
        }
    }
    
    // Повышение уровня
    if(shipsSunk >= level * 5) {
        level++;
    }
}

void seabattle_update_ships(float deltaTime) {
    for(int i = 0; i < MAX_SHIPS; i++) {
        if(!ships[i].active) continue;
        
        if(ships[i].isHit) {
            ships[i].hitTimer += 2;
            // Длительность взрыва для 4 кадров: 4 кадра * 15 = 60
if(ships[i].hitTimer > 32) {
                ships[i].active = false;
                int shipScore = 0;
                
                switch(ships[i].type) {
                    case SHIP_SMALL: shipScore = SCORE_SINK_SMALL; break;
                    case SHIP_MEDIUM: shipScore = SCORE_SINK_MEDIUM; break;
                    case SHIP_LARGE: shipScore = SCORE_SINK_LARGE; break;
                }
                
                score += shipScore;
                shipsSunk++;
            }
        } else {
            // Движение корабля
            float moveSpeed = ships[i].speed * deltaTime;
            if(ships[i].movingRight) {
                ships[i].x += moveSpeed;
                
                int shipWidth;
                switch(ships[i].type) {
                    case SHIP_SMALL: shipWidth = SHIP_SMALL_WIDTH; break;
                    case SHIP_MEDIUM: shipWidth = SHIP_MEDIUM_WIDTH; break;
                    case SHIP_LARGE: shipWidth = SHIP_LARGE_WIDTH; break;
                }
                
                if(ships[i].x > 320 - shipWidth) {
                    ships[i].movingRight = false;
                    ships[i].x = 320 - shipWidth;
                    ships[i].y += 8;
                }
            } else {
                ships[i].x -= moveSpeed;
                
                if(ships[i].x < 0) {
                    ships[i].movingRight = true;
                    ships[i].x = 0;
                    ships[i].y += 8;
                }
            }
            
            ships[i].y += 30.0f * deltaTime;
        }
    }
}

void seabattle_update_crosshair(float deltaTime) {
    float moveSpeed = 200.0f * deltaTime;
    
    if(joy1.x < 1000 && crosshair.x > CROSSHAIR_SIZE/2) {
        crosshair.x -= moveSpeed;
    }
    if(joy1.x > 3000 && crosshair.x < 320 - CROSSHAIR_SIZE/2) {
        crosshair.x += moveSpeed;
    }
    if(joy1.y < 1000 && crosshair.y < 240 - CROSSHAIR_SIZE/2) {
        crosshair.y += moveSpeed;
    }
    if(joy1.y > 3000 && crosshair.y > CROSSHAIR_SIZE/2) {
        crosshair.y -= moveSpeed;
    }
    
    crosshair.x = constrain(crosshair.x, CROSSHAIR_SIZE/2, 320 - CROSSHAIR_SIZE/2);
    crosshair.y = constrain(crosshair.y, CROSSHAIR_SIZE/2, 240 - CROSSHAIR_SIZE/2);
    
    if(crosshair.reloadTimer > 0) {
        crosshair.reloadTimer--;
    }
    
    if(btn_rt.state == PRESSED && crosshair.reloadTimer == 0) {
        seabattle_fire();
        crosshair.reloadTimer = 12;
    }
    
    if(crosshair.firing) {
        crosshair.fireTimer++;
        if(crosshair.fireTimer > 6) {
            crosshair.firing = false;
            crosshair.fireTimer = 0;
        }
    }

    if(joy2.button != PRESSED) {
        lastBut = false;
    }
    
    if(joy1.button == PRESSED){
        FPSrend = !FPSrend;
    }
    if(!lastBut && joy2.button == PRESSED){
        if (gameState == SEABATTLE) {
            gameState = MENU;
            gameChanged = false;
            lastBut = true;
        } 
    }
}

void seabattle_update_shots(float deltaTime) {
    for(int i = 0; i < MAX_SHOTS; i++) {
        if(shots[i].active) {
            shots[i].y -= shots[i].speed * deltaTime;
            
            if(shots[i].y < -10) {
                shots[i].active = false;
            }
        }
    }
}

void seabattle_check_collisions() {
    for(int i = 0; i < MAX_SHOTS; i++) {
        if(!shots[i].active) continue;
        
        for(int j = 0; j < MAX_SHIPS; j++) {
            if(!ships[j].active || ships[j].isHit) continue;
            
            int shipWidth, shipHeight;
            switch(ships[j].type) {
                case SHIP_SMALL:
                    shipWidth = SHIP_SMALL_WIDTH;
                    shipHeight = SHIP_SMALL_HEIGHT;
                    break;
                case SHIP_MEDIUM:
                    shipWidth = SHIP_MEDIUM_WIDTH;
                    shipHeight = SHIP_MEDIUM_HEIGHT;
                    break;
                case SHIP_LARGE:
                    shipWidth = SHIP_LARGE_WIDTH;
                    shipHeight = SHIP_LARGE_HEIGHT;
                    break;
            }
            
            if(shots[i].x + 2 >= ships[j].x && 
               shots[i].x - 2 <= ships[j].x + shipWidth &&
               shots[i].y + 2 >= ships[j].y && 
               shots[i].y - 2 <= ships[j].y + shipHeight) {
                
                ships[j].isHit = true;
                ships[j].hitTimer = 0;
                shots[i].active = false;
                
                switch(ships[j].type) {
                    case SHIP_SMALL:
                        score += SCORE_HIT_SMALL;
                        break;
                    case SHIP_MEDIUM:
                        score += SCORE_HIT_MEDIUM;
                        break;
                    case SHIP_LARGE:
                        score += SCORE_HIT_LARGE;
                        break;
                }
                
                break;
            }
        }
    }
}

void seabattle_spawn_ship() {
    for(int i = 0; i < MAX_SHIPS; i++) {
        if(!ships[i].active) {
            ships[i].type = static_cast<ShipType>(random(0, 3));
            ships[i].speed = 40.0f + (level * 8.0f);
            ships[i].active = true;
            ships[i].movingRight = random(0, 2) == 0;
            ships[i].isHit = false;
            ships[i].hitTimer = 0;
            
            int shipWidth;
            switch(ships[i].type) {
                case SHIP_SMALL: shipWidth = SHIP_SMALL_WIDTH; ships[i].health = 1; break;
                case SHIP_MEDIUM: shipWidth = SHIP_MEDIUM_WIDTH; ships[i].health = 2; break;
                case SHIP_LARGE: shipWidth = SHIP_LARGE_WIDTH; ships[i].health = 3; break;
            }
            
            ships[i].y = 56;
            if (ships[i].movingRight) {
                ships[i].x = -shipWidth;
            } else {
                ships[i].x = 320;
            }
            
            break;
        }
    }
}

void seabattle_fire() {
    for(int i = 0; i < MAX_SHOTS; i++) {
        if(!shots[i].active) {
            shots[i].x = crosshair.x;
            shots[i].y = crosshair.y;
            shots[i].speed = SHOT_SPEED;
            shots[i].active = true;
            
            crosshair.firing = true;
            crosshair.fireTimer = 0;
            break;
        }
    }
}

Ship* seabattle_get_ship_at_position(int x, int y) {
    for(int i = 0; i < MAX_SHIPS; i++) {
        if(!ships[i].active || ships[i].isHit) continue;
        
        int shipWidth, shipHeight;
        switch(ships[i].type) {
            case SHIP_SMALL:
                shipWidth = SHIP_SMALL_WIDTH;
                shipHeight = SHIP_SMALL_HEIGHT;
                break;
            case SHIP_MEDIUM:
                shipWidth = SHIP_MEDIUM_WIDTH;
                shipHeight = SHIP_MEDIUM_HEIGHT;
                break;
            case SHIP_LARGE:
                shipWidth = SHIP_LARGE_WIDTH;
                shipHeight = SHIP_LARGE_HEIGHT;
                break;
        }
        
        if(x >= ships[i].x && x <= ships[i].x + shipWidth &&
           y >= ships[i].y && y <= ships[i].y + shipHeight) {
            return &ships[i];
        }
    }
    return nullptr;
}

void drawShip(Ship& ship, TFT_eSPI* tft) {
    int shipWidth, shipHeight;
    RawSprite* sprite = nullptr;
    
    switch(ship.type) {
        case SHIP_SMALL:
            shipWidth = SHIP_SMALL_WIDTH;
            shipHeight = SHIP_SMALL_HEIGHT;
            if (sprites.shipSmall.isValid) sprite = &sprites.shipSmall;
            break;
        case SHIP_MEDIUM:
            shipWidth = SHIP_MEDIUM_WIDTH;
            shipHeight = SHIP_MEDIUM_HEIGHT;
            if (sprites.shipMedium.isValid) sprite = &sprites.shipMedium;
            break;
        case SHIP_LARGE:
            shipWidth = SHIP_LARGE_WIDTH;
            shipHeight = SHIP_LARGE_HEIGHT;
            if (sprites.shipLarge.isValid) sprite = &sprites.shipLarge;
            break;
    }
    
    if (sprite && sprite->isValid) {
        drawSpritePixelByPixel(ship.x, ship.y, sprite, false, 0x0000);
    } else {
        uint16_t shipColor;
        switch(ship.type) {
            case SHIP_SMALL: shipColor = TFT_DARKGREEN; break;
            case SHIP_MEDIUM: shipColor = TFT_BLUE; break;
            case SHIP_LARGE: shipColor = TFT_DARKGREY; break;
        }
        
        screen.fillRoundRect(ship.x, ship.y, shipWidth, shipHeight, 3, shipColor);
        
        switch(ship.type) {
            case SHIP_SMALL:
                screen.fillRect(ship.x + 4, ship.y - 2, shipWidth - 8, 4, TFT_DARKGREY);
                break;
            case SHIP_MEDIUM:
                screen.fillRect(ship.x + 6, ship.y - 2, 8, 4, TFT_DARKGREY);
                screen.fillRect(ship.x + shipWidth - 14, ship.y - 2, 8, 4, TFT_DARKGREY);
                break;
            case SHIP_LARGE:
                screen.fillRect(ship.x + 8, ship.y - 3, 10, 6, TFT_DARKGREY);
                screen.fillRect(ship.x + shipWidth/2 - 5, ship.y - 3, 10, 6, TFT_DARKGREY);
                screen.fillRect(ship.x + shipWidth - 18, ship.y - 3, 10, 6, TFT_DARKGREY);
                break;
        }
    }
}

void drawExplosion(Ship& ship) {
    // 4 кадра взрыва, каждый на 15 тиков
    int frame = ship.hitTimer / 8;
    if (frame > 3) frame = 3;
    
    int shipWidth, shipHeight;
    switch(ship.type) {
        case SHIP_SMALL: shipWidth = SHIP_SMALL_WIDTH; shipHeight = SHIP_SMALL_HEIGHT; break;
        case SHIP_MEDIUM: shipWidth = SHIP_MEDIUM_WIDTH; shipHeight = SHIP_MEDIUM_HEIGHT; break;
        case SHIP_LARGE: shipWidth = SHIP_LARGE_WIDTH; shipHeight = SHIP_LARGE_HEIGHT; break;
    }
    
    if (sprites.explosion[frame].isValid) {
        int exX = ship.x + shipWidth/2 - 16;
        int exY = ship.y + shipHeight/2 - 16;
        drawSpritePixelByPixel(exX, exY, &sprites.explosion[frame], false, 0x0000);
    } else {
        int explosionSize = ship.hitTimer / 4;
        uint16_t explosionColor;
        
        if(ship.hitTimer < 15) {
            explosionColor = TFT_YELLOW;
        } else if(ship.hitTimer < 30) {
            explosionColor = TFT_ORANGE;
        } else if(ship.hitTimer < 45) {
            explosionColor = TFT_RED;
        } else {
            explosionColor = rgb(128, 0, 0);
        }
        
        screen.fillCircle(ship.x + shipWidth/2, ship.y + shipHeight/2, explosionSize, explosionColor);
    }
}

void drawHearts() {
    int heartX = 290;
    int heartY = 25;
    
    for(int i = 0; i < lives; i++) {
        if (sprites.heart.isValid) {
            // Рисуем сердечко спрайтом
            drawSpritePixelByPixel(heartX - i * 20, heartY, &sprites.heart, false, 0x0000);
        } else {
            // Fallback отрисовка сердечка
            screen.fillTriangle(heartX - i * 20 + 2, heartY + 3,
                               heartX - i * 20 + 8, heartY + 12,
                               heartX - i * 20 + 14, heartY + 3,
                               TFT_RED);
            screen.fillTriangle(heartX - i * 20 + 8, heartY + 12,
                               heartX - i * 20 + 14, heartY + 3,
                               heartX - i * 20 + 2, heartY + 3,
                               TFT_RED);
            screen.fillRect(heartX - i * 20 + 4, heartY + 5, 8, 8, TFT_RED);
        }
    }
}

void seabattle_render(TFT_eSPI* tft) {
    if(loadingSeabattleSprites){
        screen.fillSprite(TFT_BLACK);
        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setTextSize(2);
        screen.drawString("Loading sprites...", 60, 110);
        
        if (seabattleLoadingSpinnerId >= 0) {
            drawSpinner(seabattleLoadingSpinnerId, screen);
        }
        
        screen.pushSprite(0, 0);
        return;
    }
    // Рисуем фон моря
    if (sprites.background.isValid) {
        screen.pushImage(0, 0, 320, 240, sprites.background.data);
    } else {
        screen.fillSprite(TFT_NAVY);
        
        // Сетка как fallback
        for(int x = 0; x < 320; x += 20) {
            screen.drawFastVLine(x, 0, 240, TFT_DARKGREY);
        }
        for(int y = 0; y < 240; y += 20) {
            screen.drawFastHLine(0, y, 320, TFT_DARKGREY);
        }
    }
    
    // Рисуем корабли и взрывы
    for(int i = 0; i < MAX_SHIPS; i++) {
        if(!ships[i].active) continue;
        
        if(ships[i].isHit) {
            drawExplosion(ships[i]);
        } else {
            drawShip(ships[i], tft);
        }
    }
    
    // Рисуем выстрелы
    for(int i = 0; i < MAX_SHOTS; i++) {
        if(shots[i].active) {
            if (sprites.shotSprite.isValid) {
                drawSpritePixelByPixel(shots[i].x - 2, shots[i].y - 4, &sprites.shotSprite, false, 0x0000);
            } else {
                screen.fillCircle(shots[i].x, shots[i].y, 2, TFT_YELLOW);
                screen.drawLine(shots[i].x, shots[i].y + 2, shots[i].x, shots[i].y + 8, TFT_WHITE);
            }
        }
    }
    
    // Анимация выстрела
    if(crosshair.firing) {
        int fireSize = crosshair.fireTimer;
        screen.fillCircle(crosshair.x, crosshair.y + 5, fireSize * 2, TFT_YELLOW);
    }
    
    // Рисуем прицел
    if (sprites.crosshairSprite.isValid) {
        drawSpritePixelByPixel(crosshair.x - 16, crosshair.y - 16, &sprites.crosshairSprite, false, 0x0000);
    } else {
        screen.drawLine(crosshair.x - 8, crosshair.y, crosshair.x + 8, crosshair.y, TFT_RED);
        screen.drawLine(crosshair.x, crosshair.y - 8, crosshair.x, crosshair.y + 8, TFT_RED);
        screen.drawSmoothCircle(crosshair.x, crosshair.y, 10, TFT_RED, TFT_TRANSPARENT);
    }
    
    // HUD
    screen.setTextColor(TFT_WHITE);
    screen.setTextSize(1);
    
    screen.drawString("SCORE", 10, 10);
    screen.setTextSize(2);
    screen.drawString(String(score), 10, 25);
    
    screen.setTextSize(1);
    screen.drawString("LIVES", 250, 10);
    
    // Рисуем сердечки здоровья
    drawHearts();
    
    screen.setTextSize(1);
    screen.drawString("LEVEL", 10, 210);
    screen.setTextSize(2);
    screen.drawString(String(level), 10, 225);
    
    screen.setTextSize(1);
    screen.drawString("SHIPS SUNK", 200, 210);
    screen.setTextSize(2);
    screen.drawString(String(shipsSunk), 220, 225);
    
    screen.setTextSize(1);
    screen.drawString("JOY2: MOVE & FIRE", 100, 220);
    
    if(gameOver) {
        screen.fillRect(60, 80, 200, 80, TFT_DARKGREY);
        screen.drawRect(60, 80, 200, 80, TFT_WHITE);
        
        screen.setTextColor(TFT_RED, TFT_DARKGREY);
        screen.setTextSize(3);
        screen.drawString("GAME OVER", 70, 95);
        
        screen.setTextColor(TFT_WHITE, TFT_DARKGREY);
        screen.setTextSize(2);
        screen.drawString("SCORE:", 90, 125);
        screen.drawString(String(score), 180, 125);
        
        screen.setTextSize(1);
        screen.drawString("PRESS A BUTTON", 95, 150);
    }
    
    if (FPSrend) {
        fps.drawToSprite(&screen, 220, 0);
    }
    screen.pushSprite(0, 0);
}