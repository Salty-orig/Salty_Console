// doom.cpp - ФИНАЛЬНАЯ ВЕРСИЯ С КРУТЫМИ ВРАГАМИ И ВЫХОДОМ В МЕНЮ
#include "doom.h"
#include "input.h"
#include "shared.h"
#include <math.h>
#include "fps.h"

// КОНФИГУРАЦИЯ
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define HALF_WIDTH 160
#define HALF_HEIGHT 120
#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define WALL_HEIGHT 48
#define FOV 60.0f
#define MAX_DEPTH 20.0f

// Карта
const uint8_t game_map[MAP_WIDTH * MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

// Типы врагов
enum EnemyType {
    ENEMY_IMP,
    ENEMY_DEMON,
    ENEMY_SKULL
};

struct Player {
    float x, y;
    float angle;
    float move_speed;
    float rot_speed;
};

struct DoomGame {
    Player player;
    DoomGameState state;
    int health;
    int ammo;
    bool map_visible;
    unsigned long last_update;
    int score;
    int kill_count;
};

static DoomGame game;

const uint16_t wall_colors[] = {
    TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA
};

// Сущности
enum EntityType {
    ENTITY_NONE,
    ENTITY_ENEMY,
    ENTITY_BULLET,
    ENTITY_HEALTH_PACK,
    ENTITY_AMMO_PACK
};

struct Entity {
    EntityType type;
    EnemyType enemy_type;
    
    float x, y;
    float vx, vy;
    
    int hp;
    int max_hp;
    bool active;
    
    bool aggro;
    float trigger_radius;
    float attack_cooldown;
    unsigned long last_attack_time;
    float move_speed;
    uint16_t color;
    
    int animation_frame;
    unsigned long last_animation_time;
};

#define MAX_ENTITIES 32
Entity entities[MAX_ENTITIES];

// Функция спавна врага
Entity* spawnEnemy(EnemyType type, float x, float y) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) {
            entities[i].type = ENTITY_ENEMY;
            entities[i].enemy_type = type;
            entities[i].x = x;
            entities[i].y = y;
            entities[i].active = true;
            entities[i].aggro = false;
            entities[i].last_attack_time = 0;
            entities[i].animation_frame = 0;
            entities[i].last_animation_time = 0;
            
            switch(type) {
                case ENEMY_IMP:
                    entities[i].hp = 50;
                    entities[i].max_hp = 50;
                    entities[i].trigger_radius = 4.0f;
                    entities[i].move_speed = 0.025f;
                    entities[i].color = TFT_RED;
                    entities[i].attack_cooldown = 800;
                    break;
                case ENEMY_DEMON:
                    entities[i].hp = 150;
                    entities[i].max_hp = 150;
                    entities[i].trigger_radius = 3.5f;
                    entities[i].move_speed = 0.012f;
                    entities[i].color = TFT_MAGENTA;
                    entities[i].attack_cooldown = 1200;
                    break;
                case ENEMY_SKULL:
                    entities[i].hp = 30;
                    entities[i].max_hp = 30;
                    entities[i].trigger_radius = 5.5f;
                    entities[i].move_speed = 0.045f;
                    entities[i].color = TFT_CYAN;
                    entities[i].attack_cooldown = 600;
                    break;
            }
            return &entities[i];
        }
    }
    return NULL;
}

void spawnBonus(EntityType type, float x, float y) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) {
            entities[i].type = type;
            entities[i].x = x;
            entities[i].y = y;
            entities[i].active = true;
            break;
        }
    }
}

Entity* spawnEntity(EntityType type, float x, float y) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) {
            entities[i].type = type;
            entities[i].x = x;
            entities[i].y = y;
            entities[i].vx = 0;
            entities[i].vy = 0;
            entities[i].active = true;
            return &entities[i];
        }
    }
    return NULL;
}

bool is_wall(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return true;
    return game_map[y * MAP_WIDTH + x] > 0;
}

// Проверка видимости
bool has_line_of_sight(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrt(dx*dx + dy*dy);
    
    if (distance < 0.1f) return true;
    
    float step = 0.1f;
    int steps = (int)(distance / step);
    
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float check_x = x1 + dx * t;
        float check_y = y1 + dy * t;
        
        if (is_wall((int)check_x, (int)check_y)) {
            return false;
        }
    }
    return true;
}

uint16_t get_wall_color(int wall_type, bool side, float distance) {
    if (wall_type <= 0 || wall_type > 6) return TFT_WHITE;
    uint16_t color = wall_colors[wall_type - 1];
    if (distance > 8.0f) {
        if (distance > 12.0f) return TFT_BLACK;
        return color;
    }
    return color;
}

void cast_single_column(int column, float ray_angle) {
    float ray_x = game.player.x;
    float ray_y = game.player.y;
    
    float ray_dir_x = cos(ray_angle);
    float ray_dir_y = sin(ray_angle);
    
    int map_x = (int)ray_x;
    int map_y = (int)ray_y;
    
    float delta_dist_x = fabs(1.0f / ray_dir_x);
    float delta_dist_y = fabs(1.0f / ray_dir_y);
    
    float side_dist_x, side_dist_y;
    int step_x, step_y;
    bool hit = false;
    bool side = 0;
    
    if (ray_dir_x < 0) {
        step_x = -1;
        side_dist_x = (ray_x - map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (map_x + 1.0f - ray_x) * delta_dist_x;
    }
    
    if (ray_dir_y < 0) {
        step_y = -1;
        side_dist_y = (ray_y - map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (map_y + 1.0f - ray_y) * delta_dist_y;
    }
    
    while (!hit) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            map_x += step_x;
            side = 0;
        } else {
            side_dist_y += delta_dist_y;
            map_y += step_y;
            side = 1;
        }
        
        if (is_wall(map_x, map_y)) {
            hit = true;
        }
    }
    
    float perp_wall_dist;
    if (side == 0)
        perp_wall_dist = side_dist_x - delta_dist_x;
    else
        perp_wall_dist = side_dist_y - delta_dist_y;
    
    if (perp_wall_dist <= 0) perp_wall_dist = 0.1f;
    
    int line_height = (int)(WALL_HEIGHT / perp_wall_dist);
    if (line_height > SCREEN_HEIGHT) line_height = SCREEN_HEIGHT;
    
    int draw_start = -line_height / 2 + SCREEN_HEIGHT / 2;
    if (draw_start < 0) draw_start = 0;
    int draw_end = line_height / 2 + SCREEN_HEIGHT / 2;
    if (draw_end >= SCREEN_HEIGHT) draw_end = SCREEN_HEIGHT - 1;
    
    int wall_type = game_map[map_y * MAP_WIDTH + map_x];
    uint16_t color = get_wall_color(wall_type, side, perp_wall_dist);
    
    if (draw_end > draw_start) {
        screen.drawFastVLine(column * 2, draw_start, draw_end - draw_start, color);
        screen.drawFastVLine(column * 2 + 1, draw_start, draw_end - draw_start, color);
    }
    
    uint16_t floor_color = TFT_DARKGREEN;
    uint16_t ceiling_color = TFT_DARKGREY;
    
    if (draw_start > 0) {
        screen.drawFastVLine(column * 2, 0, draw_start, ceiling_color);
        screen.drawFastVLine(column * 2 + 1, 0, draw_start, ceiling_color);
    }
    
    if (draw_end < SCREEN_HEIGHT) {
        screen.drawFastVLine(column * 2, draw_end, SCREEN_HEIGHT - draw_end, floor_color);
        screen.drawFastVLine(column * 2 + 1, draw_end, SCREEN_HEIGHT - draw_end, floor_color);
    }
}

void doom_init() {
    game.player.x = 1.5f;
    game.player.y = 1.5f;
    game.player.angle = 0.0f;
    game.player.move_speed = 0.08f;
    game.player.rot_speed = 0.06f;
    
    game.state = DOOM_PLAYING;
    game.health = 100;
    game.ammo = 50;
    game.map_visible = false;
    game.score = 0;
    game.kill_count = 0;
    game.last_update = millis();

    for (int i = 0; i < MAX_ENTITIES; i++)
        entities[i].active = false;

    // Враги
    spawnEnemy(ENEMY_IMP, 3.5, 3.5);
    spawnEnemy(ENEMY_IMP, 8.5, 4.5);
    spawnEnemy(ENEMY_IMP, 5.5, 10.5);
    spawnEnemy(ENEMY_IMP, 12.5, 12.5);
    spawnEnemy(ENEMY_IMP, 2.5, 13.5);
    spawnEnemy(ENEMY_DEMON, 6.5, 7.5);
    spawnEnemy(ENEMY_DEMON, 10.5, 3.5);
    spawnEnemy(ENEMY_DEMON, 3.5, 9.5);
    spawnEnemy(ENEMY_SKULL, 8.5, 8.5);
    spawnEnemy(ENEMY_SKULL, 13.5, 2.5);
    spawnEnemy(ENEMY_SKULL, 7.5, 13.5);
    
    // Бонусы
    spawnBonus(ENTITY_HEALTH_PACK, 4.5, 4.5);
    spawnBonus(ENTITY_AMMO_PACK, 11.5, 11.5);
    spawnBonus(ENTITY_HEALTH_PACK, 9.5, 1.5);
}

void doom_update() {
    if (game.state != DOOM_PLAYING && game.state != DOOM_GAME_OVER) return;
    
    unsigned long current_time = millis();
    
    // Обработка Game Over - выход в меню
    if (game.state == DOOM_GAME_OVER) {
        // По кнопке B (joy2) выходим в меню
        if (joy2.button == PRESSED) {
            gameState = MENU;
            gameChanged = false;
        }
        return;
    }
    
    // Обновление сущностей
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) continue;

        switch (entities[i].type) {
            case ENTITY_ENEMY: {
                float dx = game.player.x - entities[i].x;
                float dy = game.player.y - entities[i].y;
                float dist = sqrt(dx*dx + dy*dy);
                
                // ТРИГГЕР - враг активируется при приближении
                if (!entities[i].aggro && dist < entities[i].trigger_radius) {
                    entities[i].aggro = true;
                }
                
                // Анимация
                if (current_time - entities[i].last_animation_time > 150) {
                    entities[i].animation_frame = (entities[i].animation_frame + 1) % 6;
                    entities[i].last_animation_time = current_time;
                }
                
                // ДВИЖЕНИЕ только если агроген
                if (entities[i].aggro) {
                    // Патрулирование если не видит игрока
                    if (dist < 6.0f && has_line_of_sight(entities[i].x, entities[i].y, game.player.x, game.player.y)) {
                        // Бежит к игроку
                        if (dist > 0.8f) {
                            float move_x = (dx / dist) * entities[i].move_speed;
                            float move_y = (dy / dist) * entities[i].move_speed;
                            
                            float new_x = entities[i].x + move_x;
                            float new_y = entities[i].y + move_y;
                            
                            if (!is_wall((int)new_x, (int)entities[i].y))
                                entities[i].x = new_x;
                            if (!is_wall((int)entities[i].x, (int)new_y))
                                entities[i].y = new_y;
                        } else {
                            // Атака
                            if (current_time - entities[i].last_attack_time > entities[i].attack_cooldown) {
                                int damage = (entities[i].enemy_type == ENEMY_DEMON) ? 20 : 
                                            (entities[i].enemy_type == ENEMY_IMP) ? 10 : 15;
                                game.health -= damage;
                                entities[i].last_attack_time = current_time;
                                if (game.health < 0) game.health = 0;
                            }
                        }
                    }
                }
            } break;
            
            case ENTITY_BULLET: {
                entities[i].x += entities[i].vx;
                entities[i].y += entities[i].vy;
                
                if (is_wall((int)entities[i].x, (int)entities[i].y)) {
                    entities[i].active = false;
                    break;
                }
                
                for (int j = 0; j < MAX_ENTITIES; j++) {
                    if (entities[j].type == ENTITY_ENEMY && entities[j].active) {
                        float dx = entities[j].x - entities[i].x;
                        float dy = entities[j].y - entities[i].y;
                        
                        if (sqrt(dx*dx + dy*dy) < 0.6f) {
                            entities[j].hp -= 35;
                            entities[i].active = false;
                            
                            if (entities[j].hp <= 0) {
                                int points = (entities[j].enemy_type == ENEMY_DEMON) ? 200 :
                                            (entities[j].enemy_type == ENEMY_IMP) ? 100 : 150;
                                game.score += points;
                                game.kill_count++;
                                
                                if (random(100) < 30) {
                                    if (random(2) == 0)
                                        spawnBonus(ENTITY_HEALTH_PACK, entities[j].x, entities[j].y);
                                    else
                                        spawnBonus(ENTITY_AMMO_PACK, entities[j].x, entities[j].y);
                                }
                                entities[j].active = false;
                            }
                            break;
                        }
                    }
                }
            } break;
            
            case ENTITY_HEALTH_PACK: {
                float dx = game.player.x - entities[i].x;
                float dy = game.player.y - entities[i].y;
                if (sqrt(dx*dx + dy*dy) < 0.8f) {
                    game.health = min(100, game.health + 25);
                    entities[i].active = false;
                }
            } break;
            
            case ENTITY_AMMO_PACK: {
                float dx = game.player.x - entities[i].x;
                float dy = game.player.y - entities[i].y;
                if (sqrt(dx*dx + dy*dy) < 0.8f) {
                    game.ammo += 20;
                    entities[i].active = false;
                }
            } break;
        }
    }
    
    // Game Over проверка
    if (game.health <= 0) {
        game.state = DOOM_GAME_OVER;
        return;
    }
    
    // Управление
    static unsigned long last_input_time = 0;
    if (current_time - last_input_time < 30) return;
    last_input_time = current_time;
    
    float delta_time = 1.0f;
    float move_speed = game.player.move_speed * delta_time;
    float rot_speed = game.player.rot_speed * delta_time;
    
    // Движение
    if (joy1.y > 3000) {
        float new_x = game.player.x + cos(game.player.angle) * move_speed;
        float new_y = game.player.y + sin(game.player.angle) * move_speed;
        if (!is_wall((int)new_x, (int)game.player.y)) game.player.x = new_x;
        if (!is_wall((int)game.player.x, (int)new_y)) game.player.y = new_y;
    }
    else if (joy1.y < 1000) {
        float new_x = game.player.x - cos(game.player.angle) * move_speed;
        float new_y = game.player.y - sin(game.player.angle) * move_speed;
        if (!is_wall((int)new_x, (int)game.player.y)) game.player.x = new_x;
        if (!is_wall((int)game.player.x, (int)new_y)) game.player.y = new_y;
    }
    
    // Поворот
    if (joy2.x < 1000) {
        game.player.angle -= rot_speed;
        if (game.player.angle < 0) game.player.angle += 2 * M_PI;
    }
    else if (joy2.x > 3000) {
        game.player.angle += rot_speed;
        if (game.player.angle > 2 * M_PI) game.player.angle -= 2 * M_PI;
    }
    
    // Стрейф
    if (joy1.x > 3000) {
        float new_x = game.player.x - sin(game.player.angle) * move_speed * 0.7f;
        float new_y = game.player.y + cos(game.player.angle) * move_speed * 0.7f;
        if (!is_wall((int)new_x, (int)game.player.y)) game.player.x = new_x;
        if (!is_wall((int)game.player.x, (int)new_y)) game.player.y = new_y;
    }
    else if (joy1.x < 1000) {
        float new_x = game.player.x + sin(game.player.angle) * move_speed * 0.7f;
        float new_y = game.player.y - cos(game.player.angle) * move_speed * 0.7f;
        if (!is_wall((int)new_x, (int)game.player.y)) game.player.x = new_x;
        if (!is_wall((int)game.player.x, (int)new_y)) game.player.y = new_y;
    }
    
    // Стрельба
    static bool last_fire = false;
    if (btn_rt.state == PRESSED && !last_fire && game.ammo > 0) {
        game.ammo--;
        Entity* b = spawnEntity(ENTITY_BULLET, game.player.x, game.player.y);
        if (b) {
            b->vx = cos(game.player.angle) * 0.35f;
            b->vy = sin(game.player.angle) * 0.35f;
        }
    }
    last_fire = (btn_rt.state == PRESSED);
    
    // Карта
    static bool last_map = false;
    if (joy1.button == PRESSED && !last_map) {
        game.map_visible = !game.map_visible;
    }
    last_map = (joy1.button == PRESSED);
    
    // Выход в меню во время игры по кнопке B (joy2)
    static bool last_exit = false;
    if (btn_lt.state == PRESSED && !last_exit) {
        gameState = MENU;
        gameChanged = true;
        last_exit = true;
    }
    if (btn_lt.state != PRESSED) last_exit = false;
    
    // FPS переключение
    if(joy1.button == PRESSED && joy2.button == PRESSED){
        FPSrend = !FPSrend;
    }
}

// Функция рисования врага с формой
void drawEnemy(int x, int y, int size, Entity* enemy) {
    uint16_t bodyColor, eyeColor;
    
    // Выбор цветов в зависимости от типа
    switch(enemy->enemy_type) {
        case ENEMY_IMP:
            bodyColor = enemy->aggro ? TFT_ORANGE : TFT_MAROON;
            eyeColor = TFT_YELLOW;
            break;
        case ENEMY_DEMON:
            bodyColor = enemy->aggro ? TFT_MAGENTA : TFT_PURPLE;
            eyeColor = TFT_RED;
            break;
        case ENEMY_SKULL:
            bodyColor = enemy->aggro ? TFT_CYAN : TFT_NAVY;
            eyeColor = TFT_WHITE;
            break;
    }
    
    // Тело (круглое или квадратное в зависимости от анимации)
    if (enemy->animation_frame % 2 == 0) {
        screen.fillRoundRect(x, y, size, size, size/4, bodyColor);
    } else {
        screen.fillRect(x, y, size, size, bodyColor);
    }
    
    // Глаза (зависят от агро)
    int eyeSize = size / 5;
    if (eyeSize < 2) eyeSize = 2;
    
    if (enemy->aggro) {
        // Злые глаза - красные с белыми зрачками
        screen.fillCircle(x + size/3, y + size/3, eyeSize, TFT_RED);
        screen.fillCircle(x + size*2/3, y + size/3, eyeSize, TFT_RED);
        screen.fillCircle(x + size/3 + eyeSize/3, y + size/3 - eyeSize/3, eyeSize/2, TFT_WHITE);
        screen.fillCircle(x + size*2/3 + eyeSize/3, y + size/3 - eyeSize/3, eyeSize/2, TFT_WHITE);
        
        // Брови (злые)
        screen.drawLine(x + size/3 - eyeSize, y + size/3 - eyeSize, 
                        x + size/3 + eyeSize, y + size/3 - eyeSize*2, TFT_BLACK);
        screen.drawLine(x + size*2/3 + eyeSize, y + size/3 - eyeSize,
                        x + size*2/3 - eyeSize, y + size/3 - eyeSize*2, TFT_BLACK);
    } else {
        // Пассивные глаза
        screen.fillCircle(x + size/3, y + size/3, eyeSize, eyeColor);
        screen.fillCircle(x + size*2/3, y + size/3, eyeSize, eyeColor);
        screen.fillCircle(x + size/3 + eyeSize/3, y + size/3 - eyeSize/3, eyeSize/3, TFT_BLACK);
        screen.fillCircle(x + size*2/3 + eyeSize/3, y + size/3 - eyeSize/3, eyeSize/3, TFT_BLACK);
    }
    
    // Рот
    if (enemy->aggro) {
        screen.drawLine(x + size/2 - size/4, y + size*2/3, 
                        x + size/2 + size/4, y + size*2/3, TFT_BLACK);
    }
    
    // Полоска здоровья
    int health_percent = (enemy->hp * 100) / enemy->max_hp;
    screen.fillRect(x, y - 5, (size * health_percent) / 100, 3, TFT_GREEN);
    screen.drawRect(x, y - 5, size, 3, TFT_RED);
    
    // Эффект пульсации при агро
    if (enemy->aggro && (enemy->animation_frame % 2 == 0)) {
        screen.drawRect(x-1, y-1, size+2, size+2, TFT_RED);
    }
}

// Рендеринг сущностей
void renderEntities() {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) continue;
        
        if (entities[i].type == ENTITY_ENEMY) {
            float dx = entities[i].x - game.player.x;
            float dy = entities[i].y - game.player.y;
            float dist = sqrt(dx*dx + dy*dy);
            
            if (dist > 8.0f) continue;
            
            if (!has_line_of_sight(game.player.x, game.player.y, entities[i].x, entities[i].y)) {
                continue;
            }
            
            float angle = atan2(dy, dx) - game.player.angle;
            while (angle > M_PI) angle -= 2*M_PI;
            while (angle < -M_PI) angle += 2*M_PI;
            
            if (fabs(angle) < (FOV * 0.5 * M_PI / 180.0)) {
                int screenX = (int)((angle / (FOV * M_PI / 180.0) + 0.5) * 320);
                if (screenX < 0 || screenX >= 320) continue;
                
                // Размер врага зависит от расстояния
                int size = (int)(140 / dist);
                if (size < 12) size = 12;
                if (size > 70) size = 70;
                
                int x = screenX - size/2;
                int y = 120 - size/2;
                
                drawEnemy(x, y, size, &entities[i]);
            }
        }
        else if (entities[i].type == ENTITY_BULLET) {
            float dx = entities[i].x - game.player.x;
            float dy = entities[i].y - game.player.y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist > 10.0f) continue;
            
            if (!has_line_of_sight(game.player.x, game.player.y, entities[i].x, entities[i].y)) {
                continue;
            }
            
            float angle = atan2(dy, dx) - game.player.angle;
            while (angle > M_PI) angle -= 2*M_PI;
            while (angle < -M_PI) angle += 2*M_PI;
            
            if (fabs(angle) < (FOV * 0.5 * M_PI / 180.0)) {
                int screenX = (int)((angle / (FOV * M_PI / 180.0) + 0.5) * 320);
                if (screenX >= 0 && screenX < 320) {
                    screen.fillCircle(screenX, 120, 3, TFT_YELLOW);
                }
            }
        }
        else if (entities[i].type == ENTITY_HEALTH_PACK || entities[i].type == ENTITY_AMMO_PACK) {
            float dx = entities[i].x - game.player.x;
            float dy = entities[i].y - game.player.y;
            float dist = sqrt(dx*dx + dy*dy);
            if (dist > 6.0f) continue;
            
            if (!has_line_of_sight(game.player.x, game.player.y, entities[i].x, entities[i].y)) {
                continue;
            }
            
            float angle = atan2(dy, dx) - game.player.angle;
            while (angle > M_PI) angle -= 2*M_PI;
            while (angle < -M_PI) angle += 2*M_PI;
            
            if (fabs(angle) < (FOV * 0.5 * M_PI / 180.0)) {
                int screenX = (int)((angle / (FOV * M_PI / 180.0) + 0.5) * 320);
                if (screenX >= 0 && screenX < 320) {
                    if (entities[i].type == ENTITY_HEALTH_PACK) {
                        screen.fillCircle(screenX, 120, 6, TFT_GREEN);
                        screen.setCursor(screenX-2, 118);
                        screen.setTextColor(TFT_WHITE);
                        screen.print("+");
                    } else {
                        screen.fillRect(screenX-3, 117, 6, 6, TFT_YELLOW);
                        screen.setCursor(screenX-2, 118);
                        screen.setTextColor(TFT_BLACK);
                        screen.print("A");
                    }
                }
            }
        }
    }
}

void render_hud() {
    screen.setTextColor(TFT_GREEN, TFT_BLACK);
    screen.setTextSize(1);
    
    screen.setCursor(5, 220);
    screen.printf("HP:%d", game.health);
    
    screen.setCursor(80, 220);
    screen.printf("AMMO:%d", game.ammo);
    
    screen.setCursor(160, 220);
    screen.printf("SCORE:%d", game.score);
    
    screen.setCursor(240, 220);
    screen.printf("KILLS:%d", game.kill_count);
    
    // Прицел
    screen.drawFastHLine(158, 119, 5, rgb(255, 0, 0));
    screen.drawFastVLine(160, 117, 5, rgb(255, 0, 0));
    screen.drawCircle(160, 120, 8, rgb(255, 0, 0));
    
    // Миникарта
    if (game.map_visible) {
        int map_size = 70;
        int map_x = SCREEN_WIDTH - map_size - 5;
        int map_y = 5;
        
        screen.fillRect(map_x, map_y, map_size, map_size, TFT_BLACK);
        screen.drawRect(map_x, map_y, map_size, map_size, TFT_WHITE);
        
        int cell_size = map_size / MAP_WIDTH;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (game_map[y * MAP_WIDTH + x] > 0) {
                    screen.fillRect(map_x + x * cell_size, map_y + y * cell_size, 
                                   cell_size, cell_size, TFT_WHITE);
                }
            }
        }
        
        // Враги на миникарте
        for (int i = 0; i < MAX_ENTITIES; i++) {
            if (entities[i].type == ENTITY_ENEMY && entities[i].active) {
                int ex = map_x + (int)(entities[i].x * cell_size);
                int ey = map_y + (int)(entities[i].y * cell_size);
                screen.fillCircle(ex, ey, 2, TFT_RED);
            }
        }
        
        // Игрок
        int player_x = map_x + (int)(game.player.x * cell_size);
        int player_y = map_y + (int)(game.player.y * cell_size);
        screen.fillCircle(player_x, player_y, 2, TFT_GREEN);
        
        int dir_x = player_x + cos(game.player.angle) * 8;
        int dir_y = player_y + sin(game.player.angle) * 8;
        screen.drawLine(player_x, player_y, dir_x, dir_y, TFT_RED);
    }
}

void doom_render(TFT_eSPI* tft) {
    screen.fillSprite(TFT_BLACK);
    
    if (game.state == DOOM_GAME_OVER) {
        // Экран Game Over
        screen.fillSprite(TFT_BLACK);
        screen.setTextColor(TFT_RED, TFT_BLACK);
        screen.setTextSize(3);
        screen.setCursor(70, 80);
        screen.print("GAME OVER");
        screen.setTextSize(1);
        screen.setTextColor(TFT_YELLOW, TFT_BLACK);
        screen.setCursor(80, 120);
        screen.printf("SCORE: %d", game.score);
        screen.setCursor(80, 140);
        screen.printf("KILLS: %d", game.kill_count);
        screen.setTextColor(TFT_GREEN, TFT_BLACK);
        screen.setCursor(50, 180);
        screen.print("PRESS B TO EXIT");
        screen.pushSprite(0, 0);
        return;
    }
    
    // Ray casting
    float fov_rad = FOV * (M_PI / 180.0f);
    float ray_angle_start = game.player.angle - fov_rad / 2;
    float ray_angle_step = fov_rad / HALF_WIDTH;
    
    for (int x = 0; x < HALF_WIDTH; x++) {
        float ray_angle = ray_angle_start + ray_angle_step * x;
        cast_single_column(x, ray_angle);
    }
    
    renderEntities();
    render_hud();
    
    if (game.state == DOOM_PAUSED) {
        screen.setTextColor(TFT_YELLOW, TFT_BLACK);
        screen.setTextSize(2);
        screen.setCursor(120, 100);
        screen.print("PAUSED");
    }
    
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.setTextSize(1);
    screen.setCursor(5, 5);
    screen.print("DOOM");
    
    if(FPSrend){
        fps.drawToSprite(&screen, 260, 0);
    }
    
    screen.pushSprite(0, 0);
}

DoomGameState doom_get_state() {
    return game.state;
}

void doom_reset() {
    doom_init();
}