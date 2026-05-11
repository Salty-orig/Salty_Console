#include "lost_trail.h"
#include "lost_trail_levels.h"
#include "input.h"
#include "shared.h"
#include "fps.h"
#include "achievements.h"
#include <math.h>

extern TFT_eSPI tft;



// =========== КОНСТАНТЫ ===========
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;

// Размеры игрока
const float PLAYER_WIDTH = 16.0f;
const float PLAYER_HEIGHT = 24.0f;

// =========== ПРОСТАЯ ФИЗИКА ===========
const float PLAYER_SPEED = 4.5f;              // Скорость ходьбы
const float PLAYER_JUMP_POWER = -15.0f;         // Сила прыжка
const float GRAVITY = 3.5f;                    // Гравитация
const float MAX_FALL_SPEED = 18.0f;              // Максимальная скорость падения

// =========== ПЕРЕМЕННЫЕ СКОРОСТИ АТАКИ ===========
const float MELEE_ATTACK_DURATION = 8;
const float INVINCIBLE_DURATION = 60;
const float SHOOT_COOLDOWN = 20;

// =========== ПЕРЕМЕННЫЕ СКОРОСТИ ВРАГОВ ===========
const float SLIME_SPEED = 1.0f;
const float SKELETON_SPEED = 1.5f;
const float BAT_SPEED = 2.2f;
const float ARCHER_SPEED = 1.0f;
const float ENEMY_GRAVITY = 0.4f;
const float ENEMY_TERMINAL_VELOCITY = 6.0f;

// =========== ПЕРЕМЕННЫЕ СКОРОСТИ СНАРЯДОВ ===========
const float ARROW_SPEED = 5.5f;
const float ARROW_GRAVITY = 0.1f;
const float ARROW_WIDTH = 8;
const float ARROW_HEIGHT = 3;
const int ARROW_LIFETIME = 120;
const int ARROW_DAMAGE = 2;

// =========== ПЕРЕМЕННЫЕ СКОРОСТИ ОТБРАСЫВАНИЯ ===========
const float DAMAGE_KNOCKBACK_X = 5.0f;
const float DAMAGE_KNOCKBACK_Y = -5.0f;
const float SPIKE_KNOCKBACK_X = 4.0f;
const float SPIKE_KNOCKBACK_Y = -6.0f;
const float ENEMY_KNOCKBACK_X = 3.0f;
const float ENEMY_KNOCKBACK_Y = -4.0f;

// =========== ПЕРЕМЕННЫЕ СКОРОСТИ АНИМАЦИИ ===========
const int ANIM_SPEED_PLAYER_WALK = 2;      // Скорость анимации ходьбы игрока
const int ANIM_SPEED_ENEMY_SLIME = 20;     // Скорость анимации слизня
const int ANIM_SPEED_ENEMY_SKELETON = 15;  // Скорость анимации скелета
const int ANIM_SPEED_ENEMY_BAT = 12;       // Скорость анимации летучей мыши
const int ANIM_SPEED_ENEMY_ARCHER = 18;    // Скорость анимации лучника
const int ANIM_SPEED_COIN = 10;            // Скорость анимации монет

// =========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===========
static TrailState trailState = TRAIL_MENU;
static Player player;
static Enemy enemies[50];
static int enemyCount = 0;
static Platform platforms[200];
static int platformCount = 0;
static Coin coins[50];
static int coinCount = 0;
static Projectile projectiles[50];
static int projectileCount = 0;

// Спрайты
TrailSprites trailSprites;
static bool spritesLoaded = false;

// Уровни
static int currentLevel = 0;
static int totalCoinsCollected = 0;
static int portalX = 0;
static int portalY = 0;
static bool portalActive = false;

// Камера
static float cameraX = 0;
static float cameraY = 0;

// Счет и прогресс
static int killCount = 0;
static int totalEnemies = 0;

// Меню
static int menuSelection = 0;
static const char* menuItems[] = {"Start Game", "Controls", "Back to Main"};
static const int menuItemCount = 3;

// Анимации меню
static float menuHighlightProgress[3] = {0, 0, 0};
static float menuGlowProgress[3] = {0, 0, 0};
const float MENU_ANIM_SPEED = 0.20f;

// Ввод
static bool jumpPressed = false;
static bool attackPressed = false;
static bool shootPressed = false;
static bool lastJumpState = false;
static bool lastAttackState = false;
static bool lastShootState = false;
static float moveX = 0;

// Двойной прыжок
static bool doubleJumpUsed = false;

// Функция для правильного отображения цвета (swap bytes если нужно)
inline uint16_t correctColor(uint16_t color) {
    // Если цвет черный (прозрачный) - возвращаем как есть
    if (color == 0x0000) return 0x0000;
    
    // Меняем местами байты для правильного отображения
    // RGB565 формат: старший байт - RRRR RGGG, младший - GGGB BBBB 
    return ((color >> 8) & 0xFF) | ((color & 0xFF) << 8);
}

// Функция для отрисовки спрайта попиксельно с правильными цветами
void drawSpritePixelByPixel(int x, int y, RawSprite* sprite, bool mirror = false, uint16_t transparentColor = 0x0000) {
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

// Функция для получения текущего спрайта игрока (без учета мигания)
RawSprite* getCurrentPlayerSprite() {
    if (player.attackTimer > 0) {
        return &trailSprites.playerAttack;
    } else if (!player.onGround) {
        return &trailSprites.playerJump;
    } else if (abs(player.vx) > 0.01f) {
        int frame = (player.walkCycle / ANIM_SPEED_PLAYER_WALK) % 4;
        return &trailSprites.playerWalk[frame];
    } else {
        return &trailSprites.playerIdle;
    }
}

// Вспомогательная функция для проверки существования файла
bool fileExists(const char* filename) {
    File file = SD.open(filename);
    if (file) {
        file.close();
        return true;
    }
    return false;
}

// Вспомогательная функция для проверки столкновения двух прямоугольников
bool checkRectCollision(float x1, float y1, float w1, float h1, 
                        float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
            x1 + w1 > x2 &&
            y1 < y2 + h2 &&
            y1 + h1 > y2);
}

// =========== ПРОВЕРКА НАХОЖДЕНИЯ НА ПЛАТФОРМЕ ===========
bool isOnPlatform(float x, float y, float w, float h) {
    // Создаём небольшой отступ для проверки под ногами
    float feetY = y + h;
    float checkOffset = 3.0f; // Проверяем на 3 пикселя ниже
    
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        if (!p.isSolid) continue;
        
        // Проверяем, находится ли игрок прямо над платформой
        // и касается ли он её
        if (x + w > p.x && x < p.x + p.width) { // По горизонтали перекрываются
            if (feetY >= p.y && feetY <= p.y + checkOffset) { // Ноги касаются платформы
                return true;
            }
        }
    }
    return false;
}

// =========== ПРОВЕРКА СТОЛКНОВЕНИЯ СО СТЕНАМИ ===========
bool wouldCollide(float x, float y, float w, float h) {
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        if (!p.isSolid) continue;
        
        if (checkRectCollision(x, y, w, h, p.x, p.y, p.width, p.height)) {
            return true;
        }
    }
    return false;
}

// =========== ПРОВЕРКА НА ЛЕСТНИЦЕ ===========
bool isOnLadder(float x, float y, float w, float h) {
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        if (!p.isLadder) continue;
        
        if (checkRectCollision(x, y, w, h, p.x, p.y, p.width, p.height)) {
            return true;
        }
    }
    return false;
}

// =========== ИСПРАВЛЕННАЯ ПРОВЕРКА НА ШИПАХ ===========
bool isOnSpike(float x, float y, float w, float h) {
    // Увеличиваем область проверки для шипов
    float checkY = y + h + 4; // Проверяем чуть выше ног (чтобы точно поймать касание)
    
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        if (!p.isSpike) continue;
        
        // Проверяем пересечение с шипами с небольшим запасом
        if (x + w > p.x && x < p.x + p.width) { // По горизонтали перекрываются
            if (checkY >= p.y && checkY <= p.y + p.height) { // Ноги касаются шипов
                return true;
            }
            // Также проверяем полное перекрытие (если игрок провалился в шипы)
            if (y + h > p.y && y < p.y + p.height) {
                return true;
            }
        }
    }
    return false;
}

// Функция для безопасной загрузки спрайта
auto loadSprite = [](const char* filename, int width, int height) -> RawSprite {
    if (fileExists(filename)) {
        return loadRawSprite(filename, width, height);
    } else {
        Serial.printf("Sprite not found: %s\n", filename);
        RawSprite empty = {nullptr, width, height, false, 0};
        return empty;
    }
};

// =========== ЗАГРУЗКА СПРАЙТОВ ===========
void loadTrailSprites() {
    spritesLoaded = true;
    
    Serial.println("Loading Trail sprites...");
    
    // Спрайты игрока
    trailSprites.playerIdle = loadSprite("/trail/player_idle.raw", 16, 24);
    for (int i = 0; i < 4; i++) {
        char filename[32];
        sprintf(filename, "/trail/player_walk%d.raw", i);
        trailSprites.playerWalk[i] = loadSprite(filename, 16, 24);
    }
    trailSprites.playerJump = loadSprite("/trail/player_jump.raw", 16, 24);
    trailSprites.playerAttack = loadSprite("/trail/player_attack.raw", 16, 24);
    trailSprites.playerHurt = loadSprite("/trail/player_hurt.raw", 16, 24);
    
    // Обновляем прогресс
    if (loadingProgressBarId != -1) updateProgressBar(loadingProgressBarId, 0.15f);

    // Спрайты врагов
    for (int i = 0; i < 2; i++) {
        char filename[32];
        sprintf(filename, "/trail/slime%d.raw", i);
        trailSprites.slime[i] = loadSprite(filename, 18, 18);
        
        sprintf(filename, "/trail/skeleton%d.raw", i);
        trailSprites.skeleton[i] = loadSprite(filename, 16, 24);
        
        sprintf(filename, "/trail/bat%d.raw", i);
        trailSprites.bat[i] = loadSprite(filename, 16, 16);
        
        sprintf(filename, "/trail/archer%d.raw", i);
        trailSprites.archer[i] = loadSprite(filename, 16, 24);
    }
    
    if (loadingProgressBarId != -1) updateProgressBar(loadingProgressBarId, 0.35f);
    
    // Спрайты блоков
    trailSprites.ground = loadSprite("/trail/ground.raw", 16, 16);
    trailSprites.ladder = loadSprite("/trail/ladder.raw", 16, 16);
    trailSprites.spike = loadSprite("/trail/spike.raw", 16, 16);
    trailSprites.portal = loadSprite("/trail/portal.raw", 16, 16);
    trailSprites.checkpoint = loadSprite("/trail/checkpoint.raw", 16, 16);
    
    if (loadingProgressBarId != -1) updateProgressBar(loadingProgressBarId, 0.55f);
    
    // Спрайты монет (анимация)
    for (int i = 0; i < 4; i++) {
        char filename[32];
        sprintf(filename, "/trail/coin%d.raw", i);
        trailSprites.coin[i] = loadSprite(filename, 12, 12);
    }
    
    if (loadingProgressBarId != -1) updateProgressBar(loadingProgressBarId, 0.75f);
    
    // Фон и стрела
    trailSprites.background = loadSprite("/trail/background.raw", 320, 240);
    trailSprites.arrow = loadSprite("/trail/arrow.raw", 8, 3);
    
    if (loadingProgressBarId != -1) updateProgressBar(loadingProgressBarId, 0.9f);

    Serial.println("Trail sprites loading complete!");
}

// =========== ЗАГРУЗКА УРОВНЯ ИЗ МАССИВА ===========
void loadLevel(int levelNum) {
    if (!spritesLoaded) {
        loadTrailSprites();
    }
    
    if (levelNum >= TOTAL_LEVELS) {
        trailState = TRAIL_VICTORY;
        return;
    }
    
    Level& level = LEVELS[levelNum];
    currentLevel = levelNum;
    
    // Очищаем массивы
    platformCount = 0;
    enemyCount = 0;
    coinCount = 0;
    projectileCount = 0;
    totalEnemies = 0;
    portalActive = false;
    totalCoinsCollected = 0;
    
    // Проходим по всем тайлам
    for (int ty = 0; ty < level.height; ty++) {
        for (int tx = 0; tx < level.width; tx++) {
            int tileIndex = ty * level.width + tx;
            uint8_t tile = level.tiles[tileIndex];
            
            float x = tx * TILE_SIZE;
            float y = ty * TILE_SIZE;
            
            switch (tile) {
                case TILE_GROUND:
                    platforms[platformCount++] = {
                        x, y, TILE_SIZE, TILE_SIZE,
                        TILE_GROUND, true, false, false, false, false, false
                    };
                    break;
                    
                case TILE_LADDER:
                    platforms[platformCount++] = {
                        x, y, TILE_SIZE, TILE_SIZE,
                        TILE_LADDER, false, true, false, false, false, false
                    };
                    break;
                    
                case TILE_SPIKE:
                    platforms[platformCount++] = {
                        x, y, TILE_SIZE, TILE_SIZE,
                        TILE_SPIKE, true, false, true, false, false, false
                    };
                    break;
                    
                case TILE_PORTAL:
                    portalX = x;
                    portalY = y;
                    platforms[platformCount++] = {
                        x, y, TILE_SIZE, TILE_SIZE,
                        TILE_PORTAL, false, false, false, true, false, false
                    };
                    break;
                    
                case TILE_COIN:
                    coins[coinCount++] = {x + TILE_SIZE/2, y + TILE_SIZE/2, true, 0};
                    break;
                    
                case TILE_SPAWN_PLAYER:
                    player.x = x;
                    player.y = y;
                    player.checkpointX = x;
                    player.checkpointY = y;
                    break;
                    
                case TILE_SPAWN_SLIME:
                    enemies[enemyCount++] = {
                        x, y - 5, 0, 0, 18, 18, ENEMY_SLIME, 3, 3, 0, 1,
                        x - 50, x + 100, true, true, 0, 0, 0
                    };
                    totalEnemies++;
                    break;
                    
                case TILE_SPAWN_SKELETON:
                    enemies[enemyCount++] = {
                        x, y - 8, 0, 0, 16, 24, ENEMY_SKELETON, 5, 5, 0, -1,
                        x - 80, x + 80, true, false, 0, 0, 0
                    };
                    totalEnemies++;
                    break;
                    
                case TILE_SPAWN_BAT:
                    enemies[enemyCount++] = {
                        x, y, 0, 0, 16, 16, ENEMY_BAT, 2, 2, 0, 1,
                        x - 100, x + 100, true, true, 0, 0, 0
                    };
                    totalEnemies++;
                    break;
                    
                case TILE_SPAWN_ARCHER:
                    enemies[enemyCount++] = {
                        x, y - 8, 0, 0, 16, 24, ENEMY_ARCHER, 4, 4, 0, -1,
                        x - 60, x + 60, true, false, 0, 0, 0
                    };
                    totalEnemies++;
                    break;
                    
                case TILE_CHECKPOINT:
                    platforms[platformCount++] = {
                        x, y, TILE_SIZE, TILE_SIZE,
                        TILE_CHECKPOINT, false, false, false, false, false, true
                    };
                    break;
            }
        }
    }
    
    player.health = player.maxHealth;
    player.animFrame = 0;
    player.walkCycle = 0;
    player.onGround = false;
    player.vy = 0;
    player.vx = 0;
    doubleJumpUsed = false;
    
    killCount = 0;
    
    
    
    Serial.print("Level loaded: ");
    Serial.println(levelNum + 1);
}

// =========== ИНИЦИАЛИЗАЦИЯ ===========
void lost_trail_init(TFT_eSPI* tft) {
    trailState = TRAIL_LOADING;
    Serial.println("Lost Trail init started");

    screen.fillSprite(TFT_BLACK);
    screen.setTextColor(rgb(3, 170, 31), TFT_BLACK);
    screen.setTextSize(2);
    screen.drawCentreString("Loading...", 160, 20, 2);
    
    // Создаём спиннер
    startSpinner(160, 120, 8, 4, 800);
    
    // Создаём волновой прогресс-бар
    loadingProgressBarId = createProgressBar(0, 235, 320, 5, false);
    updateProgressBar(loadingProgressBarId, 0.0f);
    
    // Инициализация игрока
    player.width = PLAYER_WIDTH;
    player.height = PLAYER_HEIGHT;
    player.onGround = false;
    player.onLadder = false;
    player.facingRight = true;
    player.health = 5;
    player.maxHealth = 5;
    player.attackTimer = 0;
    player.shootTimer = 0;
    player.invincibleTimer = 0;
    player.canDoubleJump = true;
    player.ammo = 15;
    player.maxAmmo = 30;
    player.coins = 0;
    player.animFrame = 0;
    player.walkCycle = 0;
    
    doubleJumpUsed = false;
    
    // Обновляем прогресс (10%)
    updateProgressBar(loadingProgressBarId, 0.1f);
    
    // Загружаем спрайты если еще не загружены
    if (!spritesLoaded) {
        loadTrailSprites();
    }
    
    // Обновляем прогресс (50%)
    updateProgressBar(loadingProgressBarId, 0.5f);
    
    // Загружаем первый уровень
    loadLevel(0);
    
    cameraX = 0;
    cameraY = 0;
    
    // Обновляем прогресс (100%)
    updateProgressBar(loadingProgressBarId, 1.0f);
    
    delay(300); // Даём время увидеть 100%
    
    // Удаляем спиннер и прогресс-бар
    stopAllSpinners();
    removeProgressBar(loadingProgressBarId);
    loadingProgressBarId = -1;
    
    Serial.println("Lost Trail init completed");
    trailState = TRAIL_MENU;
}
void lost_trail_menu_init() {
    trailState = TRAIL_MENU;
    menuSelection = 0;
    
    for (int i = 0; i < menuItemCount; i++) {
        menuHighlightProgress[i] = 0;
        menuGlowProgress[i] = 0;
    }
}

// =========== ИСПРАВЛЕННАЯ ОБРАБОТКА ДВИЖЕНИЯ С ДВОЙНЫМ ПРЫЖКОМ ===========
void updatePlayerMovement() {
    // Запоминаем старую позицию
    float oldX = player.x;
    float oldY = player.y;
    
    // ГОРИЗОНТАЛЬНОЕ ДВИЖЕНИЕ - БЕЗ ИНЕРЦИИ
    player.vx = 0; // Сбрасываем горизонтальную скорость каждый кадр!
    
    static int walkCooldown = 0; // Задержка перед сбросом анимации ходьбы
    
    if (joy1.x < 800) {
        player.vx = -PLAYER_SPEED;
        player.facingRight = false;
        player.walkCycle++;
        walkCooldown = 5; // Задержка в 5 кадров перед сбросом
    } else if (joy1.x > 3200) {
        player.vx = PLAYER_SPEED;
        player.facingRight = true;
        player.walkCycle++;
        walkCooldown = 5;
    } else {
        if (walkCooldown > 0) {
            walkCooldown--;
        } else {
            player.walkCycle = 0;
        }
    }
    
    // Применяем горизонтальное движение с проверкой столкновений
    if (player.vx != 0) {
        float newX = player.x + player.vx;
        if (!wouldCollide(newX, player.y, player.width, player.height)) {
            player.x = newX;
        }
    }
    
    // ВЕРТИКАЛЬНОЕ ДВИЖЕНИЕ (ГРАВИТАЦИЯ)
    // Гравитация применяется только если не на лестнице
    if (!player.onLadder) {
        player.vy += GRAVITY;
        if (player.vy > MAX_FALL_SPEED) player.vy = MAX_FALL_SPEED;
    } else {
        // На лестнице вертикальная скорость сбрасывается
        player.vy = 0;
    }
    
    // Применяем вертикальное движение
    if (player.vy != 0) {
        float newY = player.y + player.vy;
        
        // Проверяем столкновение
        if (!wouldCollide(player.x, newY, player.width, player.height)) {
            player.y = newY;
        } else {
            // При столкновении
            if (player.vy > 0) { // Падение вниз
                // Пытаемся приземлиться точно на платформу
                bool landed = false;
                for (int i = 0; i < platformCount; i++) {
                    Platform& p = platforms[i];
                    if (!p.isSolid) continue;
                    
                    // Если игрок находится над платформой
                    if (player.x + player.width > p.x && player.x < p.x + p.width) {
                        // Приземляемся на платформу
                        float landY = p.y - player.height;
                        if (landY >= oldY && landY <= player.y + player.vy) {
                            player.y = landY;
                            player.vy = 0;
                            landed = true;
                            doubleJumpUsed = false; // Сбрасываем двойной прыжок при приземлении
                            break;
                        }
                    }
                }
                if (!landed) {
                    player.y = oldY; // Возвращаемся к старой позиции
                    player.vy = 0;
                }
            } else { // Прыжок вверх - ударился головой
                player.vy = 0;
            }
        }
    }
    
    // ПРОВЕРКА НА ЗЕМЛЕ - ДОЛЖНА БЫТЬ ПОСЛЕ ПРИМЕНЕНИЯ ДВИЖЕНИЯ
    bool wasOnGround = player.onGround;
    player.onGround = isOnPlatform(player.x, player.y, player.width, player.height);
    
    // Если только что приземлились, сбрасываем двойной прыжок
    if (!wasOnGround && player.onGround) {
        doubleJumpUsed = false;
    }
    
    // ПРОВЕРКА НА ЛЕСТНИЦЕ
    player.onLadder = isOnLadder(player.x, player.y, player.width, player.height);
    
    // ПРЫЖОК И ДВОЙНОЙ ПРЫЖОК
    jumpPressed = (joy1.button == PRESSED);
    
    if (jumpPressed && !lastJumpState) {
        if (player.onGround) {
            // Обычный прыжок с земли
            player.vy = PLAYER_JUMP_POWER;
            player.onGround = false;
            doubleJumpUsed = false;
            Serial.println("Jump!"); // Для отладки
        } 
        else if (!doubleJumpUsed && player.canDoubleJump) {
            // Двойной прыжок в воздухе
            player.vy = PLAYER_JUMP_POWER * 0.8f; // Чуть слабее обычного
            doubleJumpUsed = true;
            Serial.println("Double jump!"); // Для отладки
        }
    }
    lastJumpState = jumpPressed;
    
    // ДВИЖЕНИЕ ПО ЛЕСТНИЦЕ
    if (player.onLadder) {
        if (joy1.y > 320) {
            float newY = player.y - PLAYER_SPEED;
            if (!wouldCollide(player.x, newY, player.width, player.height)) {
                player.y = newY;
            }
        } else if (joy1.y < 8000) {
            float newY = player.y + PLAYER_SPEED;
            if (!wouldCollide(player.x, newY, player.width, player.height)) {
                player.y = newY;
            }
        }
    }
}

// =========== СБОР МОНЕТ ===========
void collectCoins() {
    for (int i = 0; i < coinCount; i++) {
        Coin& c = coins[i];
        if (!c.active) continue;
        
        if (checkRectCollision(player.x, player.y, player.width, player.height,
                               c.x - 6, c.y - 6, 12, 12)) {
            c.active = false;
            player.coins++;
            totalCoinsCollected++;
            player.ammo = min(player.ammo + 2, player.maxAmmo);
            
            Level& level = LEVELS[currentLevel];
            if (totalCoinsCollected >= level.requiredCoins) {
                portalActive = true;
            }
        }
    }
}

// =========== ИСПРАВЛЕННАЯ ПРОВЕРКА ШИПОВ ===========
void checkSpikes() {
    // Проверяем каждый кадр, даже если есть неуязвимость
    if (isOnSpike(player.x, player.y, player.width, player.height)) {
        // Урон только если нет неуязвимости
        if (player.invincibleTimer <= 0) {
            player.health -= 2;
            player.invincibleTimer = INVINCIBLE_DURATION;
            player.vy = SPIKE_KNOCKBACK_Y; // Отбрасываем вверх
            
            Serial.println("Spike damage!"); // Для отладки
        }
    }
}

// =========== ПРОВЕРКА ЧЕКПОИНТОВ ===========
void checkCheckpoints() {
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        if (!p.isCheckpoint) continue;
        
        if (checkRectCollision(player.x, player.y, player.width, player.height,
                               p.x, p.y, p.width, p.height)) {
            player.checkpointX = p.x;
            player.checkpointY = p.y - player.height;
            player.health = player.maxHealth;
        }
    }
}

// =========== ПРОВЕРКА ПОРТАЛА ===========
void checkPortal() {
    if (!portalActive) return;
    
    if (checkRectCollision(player.x, player.y, player.width, player.height,
                           portalX, portalY, TILE_SIZE, TILE_SIZE)) {
        loadLevel(currentLevel + 1);
    }
}

// =========== СИСТЕМА СНАРЯДОВ ===========
void spawnProjectile(float x, float y, float vx, float vy, int damage, bool fromPlayer) {
    if (projectileCount >= 50) return;
    
    projectiles[projectileCount].x = x;
    projectiles[projectileCount].y = y;
    projectiles[projectileCount].vx = vx;
    projectiles[projectileCount].vy = vy;
    projectiles[projectileCount].width = ARROW_WIDTH;
    projectiles[projectileCount].height = ARROW_HEIGHT;
    projectiles[projectileCount].damage = damage;
    projectiles[projectileCount].fromPlayer = fromPlayer;
    projectiles[projectileCount].active = true;
    projectiles[projectileCount].lifeTime = ARROW_LIFETIME;
    projectiles[projectileCount].bounceCount = 1;
    
    projectileCount++;
}

void updateProjectiles() {
    for (int i = 0; i < projectileCount; i++) {
        if (!projectiles[i].active) continue;
        
        Projectile& p = projectiles[i];
        
        p.x += p.vx;
        p.y += p.vy;
        
        if (!p.fromPlayer) {
            p.vy += ARROW_GRAVITY;
        }
        
        p.lifeTime--;
        if (p.lifeTime <= 0) {
            p.active = false;
            continue;
        }
        
        // Проверка столкновения со стенами
        if (wouldCollide(p.x, p.y, p.width, p.height)) {
            p.active = false;
            continue;
        }
        
        // Попадание во врагов
        if (p.fromPlayer) {
            for (int j = 0; j < enemyCount; j++) {
                Enemy& e = enemies[j];
                if (!e.active) continue;
                
                if (checkRectCollision(p.x, p.y, p.width, p.height,
                                       e.x, e.y, e.width, e.height)) {
                    e.health -= p.damage;
                    p.active = false;
                    
                    if (e.health <= 0) {
                        e.active = false;
                        killCount++;
                    }
                    break;
                }
            }
        }
        // Попадание в игрока
        else {
            if (checkRectCollision(p.x, p.y, p.width, p.height,
                                   player.x, player.y, player.width, player.height)) {
                if (player.invincibleTimer <= 0) {
                    player.health -= p.damage;
                    player.invincibleTimer = INVINCIBLE_DURATION;
                    player.vy = DAMAGE_KNOCKBACK_Y;
                }
                p.active = false;
            }
        }
    }
    
    // Удаляем неактивные снаряды
    int newCount = 0;
    for (int i = 0; i < projectileCount; i++) {
        if (projectiles[i].active) {
            if (i != newCount) {
                projectiles[newCount] = projectiles[i];
            }
            newCount++;
        }
    }
    projectileCount = newCount;
}

// =========== АТАКА ИГРОКА ===========
void playerMeleeAttack() {
    if (player.attackTimer > 0) {
        float attackX = player.facingRight ? player.x + player.width : player.x - 20;
        float attackY = player.y;
        float attackW = 25;
        float attackH = player.height;
        
        for (int i = 0; i < enemyCount; i++) {
            Enemy& e = enemies[i];
            if (!e.active) continue;
            
            if (checkRectCollision(attackX, attackY, attackW, attackH,
                                  e.x, e.y, e.width, e.height)) {
                e.health -= 2;
                
                if (e.health <= 0) {
                    e.active = false;
                    killCount++;
                } else {
                    e.damageCooldown = 20;
                }
            }
        }
    }
}

void playerShoot() {
    if (player.shootTimer <= 0 && player.ammo > 0) {
        float spawnX = player.facingRight ? player.x + player.width : player.x - ARROW_WIDTH;
        float spawnY = player.y + player.height / 2 - ARROW_HEIGHT / 2;
        float arrowVx = player.facingRight ? ARROW_SPEED : -ARROW_SPEED;
        
        spawnProjectile(spawnX, spawnY, arrowVx, 0, 2, true);
        
        player.ammo--;
        player.shootTimer = SHOOT_COOLDOWN;
    }
}

// =========== УПРОЩЕННАЯ ОБРАБОТКА ВРАГОВ ===========
void updateEnemies() {
    for (int i = 0; i < enemyCount; i++) {
        Enemy& e = enemies[i];
        if (!e.active) continue;
        
        e.animFrame++;
        
        if (e.damageCooldown > 0) e.damageCooldown--;
        if (e.shootTimer > 0) e.shootTimer--;
        
        // Гравитация для наземных врагов
        if (e.type != ENEMY_BAT) {
            e.vy += ENEMY_GRAVITY;
            if (e.vy > ENEMY_TERMINAL_VELOCITY) e.vy = ENEMY_TERMINAL_VELOCITY;
        }
        
        // Движение врагов
        float oldX = e.x;
        
        switch (e.type) {
            case ENEMY_SLIME:
                e.vx = e.patrolDir * SLIME_SPEED;
                break;
                
            case ENEMY_SKELETON: {
                float dx = player.x - e.x;
                if (abs(dx) < 200) {
                    e.patrolDir = (dx > 0) ? 1 : -1;
                    e.facingRight = (dx > 0);
                }
                e.vx = e.patrolDir * SKELETON_SPEED;
                break;
            }
            
            case ENEMY_BAT: {
                float dx = player.x - e.x;
                float dy = player.y - e.y;
                float dist = sqrt(dx*dx + dy*dy);
                
                if (dist < 150) {
                    float angle = atan2(dy, dx);
                    e.vx = cos(angle) * BAT_SPEED;
                    e.vy = sin(angle) * BAT_SPEED * 0.7f;
                    e.facingRight = (dx > 0);
                } else {
                    e.vx = e.patrolDir * BAT_SPEED * 0.5f;
                    
                    if (random(0, 30) == 0) {
                        e.vy = -2;
                    }
                }
                break;
            }
            
            case ENEMY_ARCHER: {
                float dx = player.x - e.x;
                e.facingRight = (dx > 0);
                
                if (e.shootTimer <= 0 && abs(dx) < 300) {
                    float arrowVx = e.facingRight ? ARROW_SPEED * 0.8f : -ARROW_SPEED * 0.8f;
                    spawnProjectile(
                        e.facingRight ? e.x + e.width : e.x - ARROW_WIDTH,
                        e.y + e.height/2,
                        arrowVx, -0.5f, 1, false);
                    e.shootTimer = 60;
                }
                
                e.vx = e.patrolDir * ARCHER_SPEED * 0.3f;
                break;
            }
        }
        
        // Применяем движение для наземных врагов
        if (e.type != ENEMY_BAT) {
            // Горизонтальное движение
            float newX = e.x + e.vx;
            if (!wouldCollide(newX, e.y, e.width, e.height)) {
                e.x = newX;
            } else {
                e.patrolDir *= -1;
                e.facingRight = (e.patrolDir > 0);
            }
            
            // Вертикальное движение
            float newY = e.y + e.vy;
            if (!wouldCollide(e.x, newY, e.width, e.height)) {
                e.y = newY;
            } else {
                if (e.vy > 0) { // Приземление
                    e.vy = 0;
                } else {
                    e.vy = 0;
                }
            }
            
            // Границы патруля
            if (e.x < e.patrolLeft) {
                e.x = e.patrolLeft;
                e.patrolDir = 1;
                e.facingRight = true;
            } else if (e.x + e.width > e.patrolRight) {
                e.x = e.patrolRight - e.width;
                e.patrolDir = -1;
                e.facingRight = false;
            }
        } else {
            // Движение летучих мышей
            e.x += e.vx;
            e.y += e.vy;
            
            if (e.y < 30) e.y = 30;
            if (e.y > LEVELS[currentLevel].height * TILE_SIZE - 50) {
                e.y = LEVELS[currentLevel].height * TILE_SIZE - 50;
                e.vy = 0;
            }
        }
        
        // Урон игроку
        if (e.damageCooldown <= 0) {
            if (checkRectCollision(player.x, player.y, player.width, player.height,
                                  e.x, e.y, e.width, e.height)) {
                if (player.invincibleTimer <= 0) {
                    player.health--;
                    player.invincibleTimer = INVINCIBLE_DURATION;
                    player.vy = DAMAGE_KNOCKBACK_Y;
                    
                    e.damageCooldown = 30;
                }
            }
        }
    }
}

// =========== УПРОЩЕННОЕ ОБНОВЛЕНИЕ ===========
void lost_trail_update() {
    unsigned long currentTime = millis();
    
    if (trailState == TRAIL_MENU) {
        bool needRedraw = false;
        for (int i = 0; i < menuItemCount; i++) {
            if (i == menuSelection) {
                if (menuHighlightProgress[i] < 1.0f) {
                    menuHighlightProgress[i] += MENU_ANIM_SPEED;
                    if (menuHighlightProgress[i] > 1.0f) menuHighlightProgress[i] = 1.0f;
                    needRedraw = true;
                }
            } else {
                if (menuHighlightProgress[i] > 0) {
                    menuHighlightProgress[i] -= MENU_ANIM_SPEED * 0.7f;
                    if (menuHighlightProgress[i] < 0) menuHighlightProgress[i] = 0;
                    needRedraw = true;
                }
            }
        }
        if (needRedraw) CHANGES_BTN = true;
        
        static unsigned long lastJoyTime = 0;
        const unsigned long JOY_DELAY = 200;
        
        if (currentTime - lastJoyTime >= JOY_DELAY) {
            if (joy2.y > 3000) {
                menuSelection = (menuSelection - 1 + menuItemCount) % menuItemCount;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            } else if (joy2.y < 1000) {
                menuSelection = (menuSelection + 1) % menuItemCount;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
            
            static bool lastButMenu = false;
            if (joy2.button != PRESSED) {
                lastButMenu = false;
            }
            
            if (!lastButMenu && joy2.button == PRESSED) {
                switch(menuSelection) {
                    case 0:                       
                        lost_trail_init(&tft);      
                        trailState = TRAIL_GAME;                 
                        break;
                    case 1:
                        break;
                    case 2:
                        gameState = MENU;
                        gameChanged = false;
                        break;
                }
                lastButMenu = true;
                lastJoyTime = currentTime;
                CHANGES_BTN = true;
            }
        }
    }
    
    else if (trailState == TRAIL_GAME) {
        // Анимация
        player.animFrame++;
        
        // Чтение кнопок
        attackPressed = (btn_lt.state == PRESSED);
        shootPressed = (btn_rt.state == PRESSED);
        
        // ПРОСТОЕ ОБНОВЛЕНИЕ ДВИЖЕНИЯ
        updatePlayerMovement();
        
        // Атака
        if (attackPressed && !lastAttackState && player.attackTimer <= 0) {
            player.attackTimer = MELEE_ATTACK_DURATION;
        }
        lastAttackState = attackPressed;
        
        // Стрельба
        if (shootPressed && !lastShootState) {
            playerShoot();
        }
        lastShootState = shootPressed;
        
        // Обновление таймеров
        if (player.attackTimer > 0) {
            player.attackTimer--;
            playerMeleeAttack();
        }
        if (player.shootTimer > 0) player.shootTimer--;
        if (player.invincibleTimer > 0) player.invincibleTimer--;
        
        // Взаимодействия с окружением
        collectCoins();
        checkSpikes();
        checkCheckpoints();
        checkPortal();
        
        // Обновление врагов и снарядов
        updateEnemies();
        updateProjectiles();
        
        // Смерть и возрождение
        if (player.health <= 0 || player.y > LEVELS[currentLevel].height * TILE_SIZE + 100) {
            player.health = player.maxHealth;
            player.vy = 0;
            player.vx = 0;
            player.invincibleTimer = INVINCIBLE_DURATION;
            doubleJumpUsed = false;
            
            // Перезагружаем монеты и врагов
            loadLevel(currentLevel);
            
            // Возвращаемся на последний посещённый чекпоинт
            player.x = player.checkpointX;
            player.y = player.checkpointY;
        }
        
        // Камера
        float targetCameraX = player.x - SCREEN_WIDTH/2 + player.width/2;
        float targetCameraY = player.y - SCREEN_HEIGHT/2 + player.height/2;
        
        int levelWidth = LEVELS[currentLevel].width * TILE_SIZE;
        int levelHeight = LEVELS[currentLevel].height * TILE_SIZE;
        
        if (targetCameraX < 0) targetCameraX = 0;
        if (targetCameraX > levelWidth - SCREEN_WIDTH) targetCameraX = levelWidth - SCREEN_WIDTH;
        if (targetCameraY < 0) targetCameraY = 0;
        if (targetCameraY > levelHeight - SCREEN_HEIGHT) targetCameraY = levelHeight - SCREEN_HEIGHT;
        
        // Плавное следование камеры
        const float CAMERA_LERP_FACTOR = 0.22f; // Настраиваемый коэффициент плавности (0.05 - медленнее, 0.3 - быстрее)
        cameraX += (targetCameraX - cameraX) * CAMERA_LERP_FACTOR;
        cameraY += (targetCameraY - cameraY) * CAMERA_LERP_FACTOR;
        
        // Выход в меню
        static unsigned long backPressTime = 0;
        if (btn_lt.state == PRESSED) {
            if (backPressTime == 0) backPressTime = currentTime;
            else if (currentTime - backPressTime > 2500) {
                lost_trail_menu_init();
                backPressTime = 0;
            }
        } else {
            backPressTime = 0;
        }
    }
    
    else if (trailState == TRAIL_GAME_OVER || trailState == TRAIL_VICTORY) {
        if (joy2.button == PRESSED || joy1.button == PRESSED) {
            lost_trail_menu_init();
        }
    }
}

// =========== ОТРИСОВКА ===========
void drawPlayerSprite() {
    int screenX = (int)(player.x - cameraX);
    int screenY = (int)(player.y - cameraY);
    
    // Если игрок неуязвим, рисуем мигание между текущим спрайтом и спрайтом урона
    if (player.invincibleTimer > 0) {
        // Определяем, какой кадр мигания: четный/нечетный
        // Меняем спрайт каждые 3 кадра для плавного мигания
        int blinkFrame = (player.invincibleTimer / 3) % 2;
        
        if (blinkFrame == 0) {
            // Рисуем спрайт урона
            if (trailSprites.playerHurt.isValid) {
                drawSpritePixelByPixel(screenX, screenY, &trailSprites.playerHurt, !player.facingRight, 0x0000);
            } else {
                // Если спрайт урона не загружен, рисуем текущий спрайт
                RawSprite* currentSprite = getCurrentPlayerSprite();
                if (currentSprite && currentSprite->isValid) {
                    drawSpritePixelByPixel(screenX, screenY, currentSprite, !player.facingRight, 0x0000);
                }
            }
        } else {
            // Рисуем обычный спрайт
            RawSprite* currentSprite = getCurrentPlayerSprite();
            if (currentSprite && currentSprite->isValid) {
                drawSpritePixelByPixel(screenX, screenY, currentSprite, !player.facingRight, 0x0000);
            } else {
                screen.fillRect(screenX, screenY, player.width, player.height, rgb(100, 200, 255));
            }
        }
    } else {
        // Обычная отрисовка без мигания
        RawSprite* currentSprite = getCurrentPlayerSprite();
        if (currentSprite && currentSprite->isValid) {
            drawSpritePixelByPixel(screenX, screenY, currentSprite, !player.facingRight, 0x0000);
        } else {
            screen.fillRect(screenX, screenY, player.width, player.height, rgb(100, 200, 255));
            if (player.facingRight) {
                screen.fillRect(screenX + player.width - 4, screenY + 4, 2, 2, rgb(255, 255, 255));
                screen.fillRect(screenX + player.width - 8, screenY + 4, 2, 2, rgb(255, 255, 255));
            } else {
                screen.fillRect(screenX + 2, screenY + 4, 2, 2, rgb(255, 255, 255));
                screen.fillRect(screenX + 6, screenY + 4, 2, 2, rgb(255, 255, 255));
            }
        }
    }
    
    int healthWidth = (player.health * player.width) / player.maxHealth;
    screen.fillRect(screenX, screenY - 6, healthWidth, 3, rgb(255, 50, 50));
}

void drawEnemySprite(Enemy& e) {
    if (!e.active) return;
    
    int screenX = (int)(e.x - cameraX);
    int screenY = (int)(e.y - cameraY);
    
    if (screenX + e.width < 0 || screenX > SCREEN_WIDTH ||
        screenY + e.height < 0 || screenY > SCREEN_HEIGHT) {
        return;
    }
    
    RawSprite* sprite = nullptr;
    int frame = 0;
    
    switch (e.type) {
        case ENEMY_SLIME:
            frame = (e.animFrame / ANIM_SPEED_ENEMY_SLIME) % 2;
            sprite = &trailSprites.slime[frame];
            break;
        case ENEMY_SKELETON:
            frame = (e.animFrame / ANIM_SPEED_ENEMY_SKELETON) % 2;
            sprite = &trailSprites.skeleton[frame];
            break;
        case ENEMY_BAT:
            frame = (e.animFrame / ANIM_SPEED_ENEMY_BAT) % 2;
            sprite = &trailSprites.bat[frame];
            break;
        case ENEMY_ARCHER:
            frame = (e.animFrame / ANIM_SPEED_ENEMY_ARCHER) % 2;
            sprite = &trailSprites.archer[frame];
            break;
    }
    
    if (sprite && sprite->isValid) {
        drawSpritePixelByPixel(screenX, screenY, sprite, !e.facingRight, 0x0000);
    } else {
        uint16_t color;
        switch (e.type) {
            case ENEMY_SLIME: color = rgb(100, 255, 100); break;
            case ENEMY_SKELETON: color = rgb(220, 220, 220); break;
            case ENEMY_BAT: color = rgb(150, 100, 255); break;
            case ENEMY_ARCHER: color = rgb(200, 150, 100); break;
            default: color = rgb(255, 255, 255);
        }
        screen.fillRect(screenX, screenY, e.width, e.height, color);
    }
    
    int healthWidth = (e.health * e.width) / e.maxHealth;
    screen.fillRect(screenX, screenY - 4, healthWidth, 2, rgb(255, 50, 50));
}

void drawPlatformSprites() {
    for (int i = 0; i < platformCount; i++) {
        Platform& p = platforms[i];
        
        int screenX = (int)(p.x - cameraX);
        int screenY = (int)(p.y - cameraY);
        
        if (screenX + p.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + p.height < 0 || screenY > SCREEN_HEIGHT) {
            continue;
        }
        
        RawSprite* sprite = nullptr;
        
        switch (p.type) {
            case TILE_GROUND:
                sprite = &trailSprites.ground;
                break;
            case TILE_LADDER:
                sprite = &trailSprites.ladder;
                break;
            case TILE_SPIKE:
                sprite = &trailSprites.spike;
                break;
            case TILE_PORTAL:
                if (portalActive) {
                    sprite = &trailSprites.portal;
                }
                break;
            case TILE_CHECKPOINT:
                sprite = &trailSprites.checkpoint;
                break;
        }
        
        if (sprite && sprite->isValid) {
            drawSpritePixelByPixel(screenX, screenY, sprite, false, 0x0000);
        } else {
            // Рисуем примитивную графику для ВСЕХ типов, если спрайт не загружен
            switch (p.type) {
                case TILE_GROUND:
                    screen.fillRect(screenX, screenY, p.width, p.height, rgb(80, 160, 80));
                    screen.drawRect(screenX, screenY, p.width, p.height, rgb(40, 100, 40));
                    break;
                    
                case TILE_LADDER:
                    screen.fillRect(screenX, screenY, p.width, p.height, rgb(160, 120, 80));
                    for (int j = 0; j < 4; j++) {
                        screen.drawFastVLine(screenX + 4 + j * 3, screenY, p.height, rgb(100, 70, 40));
                    }
                    break;
                    
                case TILE_SPIKE:
                    // Рисуем шипы треугольниками
                    screen.fillTriangle(
                        screenX, screenY + p.height,
                        screenX + p.width/2, screenY,
                        screenX + p.width, screenY + p.height,
                        rgb(150, 150, 150)
                    );
                    screen.drawTriangle(
                        screenX, screenY + p.height,
                        screenX + p.width/2, screenY,
                        screenX + p.width, screenY + p.height,
                        rgb(80, 80, 80)
                    );
                    break;
                    
                case TILE_PORTAL:
                    if (portalActive) {
                        screen.fillCircle(screenX + p.width/2, screenY + p.height/2, p.width/3, rgb(255, 0, 255));
                        screen.drawCircle(screenX + p.width/2, screenY + p.height/2, p.width/3, rgb(200, 0, 200));
                    }
                    break;
                    
                case TILE_CHECKPOINT:
                    screen.fillRect(screenX, screenY, p.width, p.height, rgb(255, 215, 0));
                    screen.drawRect(screenX, screenY, p.width, p.height, rgb(200, 150, 0));
                    screen.fillRect(screenX + 4, screenY + 4, p.width - 8, p.height - 8, rgb(255, 255, 255));
                    break;
                    
                default:
                    if (p.isSolid) {
                        screen.fillRect(screenX, screenY, p.width, p.height, rgb(100, 100, 100));
                        screen.drawRect(screenX, screenY, p.width, p.height, rgb(50, 50, 50));
                    }
                    break;
            }
        }
    }
}

void drawCoinSprite(Coin& c) {
    if (!c.active) return;
    
    int screenX = (int)(c.x - cameraX) - 6;
    int screenY = (int)(c.y - cameraY) - 6;
    
    if (screenX < -12 || screenX > SCREEN_WIDTH || screenY < -12 || screenY > SCREEN_HEIGHT) {
        return;
    }
    
    c.animFrame++;
    int frame = (c.animFrame / ANIM_SPEED_COIN) % 4;
    
    if (trailSprites.coin[frame].isValid) {
        drawSpritePixelByPixel(screenX, screenY, &trailSprites.coin[frame], false, 0x0000);
    } else {
        int offset = (c.animFrame / 10) % 4 - 2;
        screen.fillCircle(screenX + 6, screenY + 6 + offset, 4, rgb(255, 215, 0));
    }
}

void drawProjectileSprite(Projectile& p) {
    int screenX = (int)(p.x - cameraX);
    int screenY = (int)(p.y - cameraY);
    
    if (screenX + p.width < 0 || screenX > SCREEN_WIDTH ||
        screenY + p.height < 0 || screenY > SCREEN_HEIGHT) {
        return;
    }
    
    if (trailSprites.arrow.isValid) {
        drawSpritePixelByPixel(screenX, screenY, &trailSprites.arrow, (p.vx < 0), 0x0000);
    } else {
        uint16_t color = p.fromPlayer ? rgb(255, 255, 100) : rgb(255, 100, 100);
        screen.fillRect(screenX, screenY, p.width, p.height, color);
    }
}

void lost_trail_render(TFT_eSPI* tft) {
    // Рисуем фон
    if (trailSprites.background.isValid) {
        screen.pushImage(0, 0, 320, 240, trailSprites.background.data);
    } else {
        screen.fillSprite(rgb(30, 40, 60));
    }
    
    if (trailState == TRAIL_MENU) {
        screen.setTextColor(rgb(200, 120, 50));
        screen.setTextSize(3);
        screen.setCursor(70, 40);
        screen.print("LOST TRAIL");
        
        screen.drawFastHLine(70, 70, 180, rgb(200, 120, 50));
        
        int yPos = 100;
        int xPos = 80;
        int spacing = 35;
        
        for(int i = 0; i < menuItemCount; i++) {
            screen.setTextSize(2);
            
            int xOffset = (i == menuSelection) ? (int)(3 * menuHighlightProgress[i]) : 0;
            
            uint16_t textColor;
            if (i == menuSelection) {
                if (menuHighlightProgress[i] >= 0.99f) {
                    textColor = rgb(187, 255, 132);
                } else {
                    uint8_t val = 150 + 105 * menuHighlightProgress[i];
                    textColor = rgb(val, val, val);
                }
            } else {
                float glow = menuGlowProgress[i];
                uint8_t val = 100 + 100 * glow;
                textColor = rgb(val, 150, val);
            }
            
            screen.setTextColor(textColor);
            screen.drawString(menuItems[i], xPos + xOffset, yPos + i * spacing);
        }
        
        screen.setTextColor(rgb(107, 107, 107), rgb(137, 176, 255));
        screen.setTextSize(1);
        screen.setCursor(10, 230);
        screen.print("B: Select  RT: Shoot  A: Attack");
    }
    
    else if (trailState == TRAIL_GAME) {
        drawPlatformSprites();
        
        for (int i = 0; i < projectileCount; i++) {
            drawProjectileSprite(projectiles[i]);
        }
        
        for (int i = 0; i < enemyCount; i++) {
            drawEnemySprite(enemies[i]);
        }
        
        for (int i = 0; i < coinCount; i++) {
            drawCoinSprite(coins[i]);
        }
        
        if (portalActive) {
            int screenX = (int)(portalX - cameraX);
            int screenY = (int)(portalY - cameraY);
            if (trailSprites.portal.isValid) {
                drawSpritePixelByPixel(screenX, screenY, &trailSprites.portal, false, 0x0000);
            }
        }
        
        drawPlayerSprite();
        
        // UI
        screen.setTextColor(rgb(255, 255, 255));
        screen.setTextSize(1);
        
        screen.setCursor(5, 5);
        screen.print("Lvl:");
        screen.print(currentLevel + 1);
        screen.print("/");
        screen.print(TOTAL_LEVELS);
        
        screen.setCursor(5, 18);
        screen.print("HP: ");
        for (int i = 0; i < player.health; i++) {
            screen.fillRect(35 + i * 8, 18, 6, 6, rgb(255, 50, 50));
        }
        
        screen.setCursor(5, 31);
        screen.print("Ammo:");
        screen.print(player.ammo);
        
        screen.setCursor(5, 44);
        screen.print("Coins:");
        screen.print(totalCoinsCollected);
        screen.print("/");
        screen.print(LEVELS[currentLevel].requiredCoins);
        
        screen.setCursor(200, 5);
        screen.print("Kills:");
        screen.print(killCount);
        
        screen.setCursor(200, 18);
        screen.print("A:Melee");
        screen.setCursor(200, 31);
        screen.print("B:Jump");
        screen.setCursor(200, 44);
        screen.print("RT:Shoot");
        
        screen.setCursor(5, 230);
        screen.print("Hold LT:Menu");
        
        if (!portalActive) {
            screen.setCursor(150, 230);
            screen.print("Find coins!");
        }
    }
    
    else if (trailState == TRAIL_GAME_OVER) {
        screen.fillSprite(rgb(0, 0, 0));
        screen.setTextColor(rgb(255, 50, 50), rgb(0, 0, 0));
        screen.setTextSize(3);
        screen.setCursor(80, 100);
        screen.print("GAME OVER");
        
        screen.setTextSize(1);
        screen.setCursor(100, 150);
        screen.print("Press any button");
    }
    
    else if (trailState == TRAIL_VICTORY) {
        screen.fillSprite(rgb(50, 50, 100));
        screen.setTextColor(rgb(255, 255, 100), rgb(50, 50, 100));
        screen.setTextSize(3);
        screen.setCursor(40, 80);
        screen.print("CONGRATS!");
        
        screen.setTextSize(2);
        screen.setCursor(60, 120);
        screen.print("You won!");
        
        screen.setTextSize(1);
        screen.setCursor(80, 160);
        screen.print("Press any button");
    }

    else if (trailState == TRAIL_LOADING) {
    screen.fillSprite(rgb(0, 0, 0));
    screen.setTextColor(rgb(200, 200, 255), rgb(0, 0, 0));   
    screen.setTextSize(2);
    screen.drawCentreString("Loading Lost Trail...", 160, 50, 2);
    
    // Обновляем и рисуем спиннер (крутилку)
    updateSpinners();
    drawSpinner(0, screen);  // предполагаем что индекс 0
    
    // Рисуем прогресс-бар
    if (loadingProgressBarId != -1) {
        drawProgressBar(loadingProgressBarId, screen);
    }

    }
    
    if (FPSrend) {
        fps.drawToSprite(&screen, 240, 0);
    }
    
    screen.pushSprite(0, 0);
}