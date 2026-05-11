#ifndef SEABATTLE_H
#define SEABATTLE_H

#include <TFT_eSPI.h>
#include "shared.h"

// Типы кораблей
enum ShipType {
    SHIP_SMALL = 0,     // Маленький (1 палуба)
    SHIP_MEDIUM = 1,    // Средний (2 палубы) 
    SHIP_LARGE = 2      // Большой (3 палубы)
};

// Структура корабля
struct Ship {
    float x;            // Позиция X
    float y;            // Позиция Y
    ShipType type;      // Тип корабля
    float speed;        // Скорость движения
    bool active;        // Активен ли корабль
    bool movingRight;   // Направление движения
    int health;         // Здоровье (сколько палуб)
    int hitTimer;       // Таймер анимации попадания
    bool isHit;         // Попадание в корабль
};

// Структура прицела
struct Crosshair {
    float x;            // Позиция X
    float y;            // Позиция Y
    bool firing;        // Анимация выстрела
    int fireTimer;      // Таймер выстрела
    int reloadTimer;    // Таймер перезарядки
};

// Структура выстрела
struct Shot {
    float x;
    float y;
    float speed;
    bool active;
};

// Размеры кораблей
#define SHIP_SMALL_WIDTH    24
#define SHIP_SMALL_HEIGHT   8
#define SHIP_MEDIUM_WIDTH   36
#define SHIP_MEDIUM_HEIGHT  12
#define SHIP_LARGE_WIDTH    48
#define SHIP_LARGE_HEIGHT   16

#define CROSSHAIR_SIZE      16
#define SHOT_SPEED          300.0f
#define MAX_SHIPS           8
#define MAX_SHOTS           3

// Игровые константы
#define SCORE_HIT_SMALL     50
#define SCORE_HIT_MEDIUM    75
#define SCORE_HIT_LARGE     100
#define SCORE_SINK_SMALL    100
#define SCORE_SINK_MEDIUM   200
#define SCORE_SINK_LARGE    300

// Функции
void seabattle_init();
void seabattle_update();
void seabattle_render(TFT_eSPI* tft);

// Внутренние функции
void seabattle_update_ships(float deltaTime);
void seabattle_update_crosshair(float deltaTime);
void seabattle_update_shots(float deltaTime);
void seabattle_check_collisions();
void seabattle_spawn_ship();
void seabattle_fire();
Ship* seabattle_get_ship_at_position(int x, int y);


// Глобальные переменные игры
extern Ship ships[MAX_SHIPS];
extern Crosshair crosshair;
extern Shot shots[MAX_SHOTS];
extern int score;
extern int lives;
extern int level;
extern int shipsSunk;
extern bool gameOver;
extern unsigned long lastSpawnTime;
extern unsigned long lastFrameTimeSeabattle;

#endif