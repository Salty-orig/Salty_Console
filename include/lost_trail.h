#ifndef LOST_TRAIL_H
#define LOST_TRAIL_H

#include <TFT_eSPI.h>
#include "shared.h"
#include "sprite_loader.h"

// Размер тайла в пикселях
const int TILE_SIZE = 16;

// Типы тайлов (для визуального редактирования)
enum TileType {
    TILE_EMPTY = 0,        // Пустота
    TILE_GROUND = 1,       // Земля/платформа
    TILE_LADDER = 2,       // Лестница
    TILE_SPIKE = 3,        // Шипы
    TILE_PORTAL = 4,       // Портал на следующий уровень
    TILE_COIN = 5,         // Монетка/бонус
    TILE_SPAWN_PLAYER = 6, // Точка появления игрока
    TILE_SPAWN_SLIME = 7,  // Точка появления слизня
    TILE_SPAWN_SKELETON = 8, // Точка появления скелета
    TILE_SPAWN_BAT = 9,    // Точка появления летучей мыши
    TILE_SPAWN_ARCHER = 10, // Точка появления лучника
    TILE_CHECKPOINT = 11,  // Чекпоинт
};

// Состояния игры
enum TrailState {
    TRAIL_MENU,
    TRAIL_GAME,
    TRAIL_GAME_OVER,
    TRAIL_VICTORY,
    TRAIL_LEVEL_COMPLETE,
    TRAIL_LOADING
};

// Направления
enum Direction {
    DIR_NONE,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
};

// Типы врагов
enum EnemyType {
    ENEMY_SLIME,
    ENEMY_SKELETON,
    ENEMY_BAT,
    ENEMY_ARCHER
};

// Состояние игрока
struct Player {
    float x, y;           // Позиция
    float vx, vy;         // Скорость
    float width, height;  // Размеры
    bool onGround;        // На земле?
    bool onLadder;        // На лестнице?
    bool facingRight;     // Куда смотрит
    int health;           // Здоровье
    int maxHealth;        // Макс здоровье
    int attackTimer;      // Таймер атаки (ближний бой)
    int shootTimer;       // Таймер перезарядки выстрела
    int invincibleTimer;  // Неуязвимость после получения урона
    bool canDoubleJump;   // Можно ли сделать двойной прыжок
    int ammo;             // Патроны
    int maxAmmo;          // Макс патронов
    int coins;            // Собранные монеты
    int checkpointX;      // Последний чекпоинт
    int checkpointY;
    int animFrame;        // Кадр анимации
    int walkCycle;        // Цикл ходьбы
};

// Структура для врага
struct Enemy {
    float x, y;
    float vx, vy;
    float width, height;
    EnemyType type;
    int health;
    int maxHealth;
    int attackTimer;
    int patrolDir;        // Направление патрулирования
    float patrolLeft;     // Левая граница патруля
    float patrolRight;    // Правая граница патруля
    bool active;
    bool facingRight;
    int damageCooldown;   // Задержка после получения урона
    int shootTimer;       // Таймер стрельбы (для лучников)
    int animFrame;        // Кадр анимации
};

// Платформа/блок (для рантайма)
struct Platform {
    float x, y;
    float width, height;
    int type;             // Тип тайла
    bool isSolid;
    bool isLadder;
    bool isSpike;
    bool isPortal;
    bool isCoin;
    bool isCheckpoint;
};

// Снаряд
struct Projectile {
    float x, y;
    float vx, vy;
    float width, height;
    int damage;
    bool fromPlayer;
    bool active;
    int lifeTime;
    int bounceCount;
};

// Монета/бонус
struct Coin {
    float x, y;
    bool active;
    int animFrame;
};

// Структура уровня
struct Level {
    int width;            // Ширина в тайлах
    int height;           // Высота в тайлах
    const uint8_t* tiles; // Указатель на массив тайлов
    int startX;           // Стартовая позиция X (в пикселях)
    int startY;           // Стартовая позиция Y (в пикселях)
    int requiredCoins;    // Сколько монет нужно для портала
    int levelNumber;      // Номер уровня
};

// Структура для хранения всех спрайтов
struct TrailSprites {
    RawSprite playerIdle;
    RawSprite playerWalk[4];
    RawSprite playerJump;
    RawSprite playerAttack;
    RawSprite playerHurt;
    
    RawSprite slime[2];
    RawSprite skeleton[2];
    RawSprite bat[2];
    RawSprite archer[2];
    
    RawSprite ground;
    RawSprite ladder;
    RawSprite spike;
    RawSprite portal;
    RawSprite coin[4];
    RawSprite checkpoint;
    
    RawSprite background;
    RawSprite arrow;
};

// Основные функции
void lost_trail_init(TFT_eSPI* tft);
void lost_trail_update();
void lost_trail_render(TFT_eSPI* tft);
void loadLevel(int levelNum);
void loadTrailSprites();  // Новая функция загрузки спрайтов

extern TrailSprites trailSprites;  // Глобальная переменная со спрайтами

#endif