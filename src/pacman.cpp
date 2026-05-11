#include "pacman.h"
#include "input.h"
#include "shared.h"
#include "fps.h"
#include "achievements.h" // ДОБАВЛЕНО
#include <math.h>

// Размеры спрайтов
const int PACMAN_SIZE = 12;
const int GHOST_SIZE = 12;
const int DOT_SIZE = 1;
const int POWER_DOT_SIZE = 4;

// Константы лабиринта
const int MAZE_WIDTH = 27;
const int MAZE_HEIGHT = 15;
const float CELL_SIZE = 11.9f;

// Состояния игры
enum PacmanState {
    PLAYING,
    GAME_OVER,
    LEVEL_COMPLETE
};

// Направления
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

// Структура для тайлера лабиринта
struct Tile {
    bool wall;
    bool dot;
    bool powerDot;
    bool eaten;
};

// Структура привидения
struct Ghost {
    float x, y;
    Direction dir;
    int targetX, targetY;
    bool frightened;
    bool eaten;
    unsigned long frightenedStart;
    uint16_t color;
    float speed;
    unsigned long lastMoveTime; // Добавлено для контроля скорости движения
};

// Главная структура игры
struct PacmanGame {
    // Пакман
    float pacmanX, pacmanY;
    Direction pacmanDir;
    Direction nextDir;
    bool mouthOpen;
    unsigned long lastMouthTime;
    
    // Привидения
    Ghost ghosts[4];
    
    // Лабиринт
    Tile maze[MAZE_HEIGHT][MAZE_WIDTH];
    
    // Игровые параметры
    int score;
    int lives;
    int dotsEaten;
    int totalDots;
    PacmanState state;
    unsigned long lastUpdateTime;
    const unsigned long UPDATE_INTERVAL = 16;
    
    // Спрайты
    TFT_eSprite* pacmanSprite;
    TFT_eSprite* ghostSprite;
    
    // Флаги для достижений
    bool isFirstGame;
    bool ateGhostThisLevel;
    bool noDeathsThisLevel;
    
} pacmanGame;

const uint8_t mazeLayout[MAZE_HEIGHT][MAZE_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1},
    {1,3,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,1,1,1,2,2,3},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,2,1,1},
    {1,2,2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,1},
    {1,1,1,1,2,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,2,1,1},
    {0,0,0,0,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,0,0},
    {1,1,1,1,2,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,2,1,1},
    {1,2,2,2,2,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,2,2,1},
    {1,2,1,1,2,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,2,1,1},
    {1,2,1,1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1,1},
    {1,2,2,2,2,1,2,2,2,1,1,1,1,1,1,1,1,1,1,1,2,2,2,1,2,2,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Функция инициализации лабиринта
void initMaze() {
    pacmanGame.totalDots = 0;
    pacmanGame.dotsEaten = 0;
    
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            pacmanGame.maze[y][x].wall = (mazeLayout[y][x] == 1);
            pacmanGame.maze[y][x].dot = (mazeLayout[y][x] == 2);
            pacmanGame.maze[y][x].powerDot = (mazeLayout[y][x] == 3);
            pacmanGame.maze[y][x].eaten = false;
            
            if (pacmanGame.maze[y][x].dot || pacmanGame.maze[y][x].powerDot) {
                pacmanGame.totalDots++;
            }
        }
    }
}

// Функция проверки столкновения со стеной
bool canMoveTo(int gridX, int gridY, Direction dir) {
    if (gridX < 0 || gridX >= MAZE_WIDTH || gridY < 0 || gridY >= MAZE_HEIGHT) {
        return true; // Разрешаем движение через телепорты
    }
    
    return !pacmanGame.maze[gridY][gridX].wall;
}

// Конвертация координат в тайлы
void worldToGrid(float worldX, float worldY, int& gridX, int& gridY) {
    gridX = (int)(worldX / CELL_SIZE);
    gridY = (int)(worldY / CELL_SIZE);
}

// Конвертация тайлов в координаты
void gridToWorld(int gridX, int gridY, float& worldX, float& worldY) {
    worldX = gridX * CELL_SIZE + CELL_SIZE / 2;
    worldY = gridY * CELL_SIZE + CELL_SIZE / 2;
}

// Ограничение движения в сетке
void alignToGrid(float& x, float& y, Direction dir) {
    int gridX, gridY;
    worldToGrid(x, y, gridX, gridY);
    
    float targetX = gridX * CELL_SIZE + CELL_SIZE / 2;
    float targetY = gridY * CELL_SIZE + CELL_SIZE / 2;
    
    float speed = 2.0f;
    
    switch (dir) {
        case LEFT:
        case RIGHT:
            y = targetY;
            if (fabs(x - targetX) < speed) x = targetX;
            break;
        case UP:
        case DOWN:
            x = targetX;
            if (fabs(y - targetY) < speed) y = targetY;
            break;
    }
}

// Инициализация привидений
void initGhosts() {
    // Цвета привидений
    uint16_t ghostColors[4] = {TFT_RED, TFT_PINK, TFT_CYAN, TFT_ORANGE};
    float ghostSpeeds[4] = {1.0f, 0.9f, 0.8f, 0.7f}; // Уменьшены скорости
    
    // Начальные позиции привидений
    int startPositions[4][2] = {
        {9, 7},  // Красный
        {8, 7},  // Розовый
        {9, 8},  // Голубой
        {10, 7}  // Оранжевый
    };
    
    for (int i = 0; i < 4; i++) {
        pacmanGame.ghosts[i].color = ghostColors[i];
        pacmanGame.ghosts[i].speed = ghostSpeeds[i];
        pacmanGame.ghosts[i].frightened = false;
        pacmanGame.ghosts[i].eaten = false;
        pacmanGame.ghosts[i].frightenedStart = 0;
        pacmanGame.ghosts[i].lastMoveTime = millis(); // Инициализация таймера
        
        gridToWorld(startPositions[i][0], startPositions[i][1], 
                    pacmanGame.ghosts[i].x, pacmanGame.ghosts[i].y);
        
        // Начальные направления
        pacmanGame.ghosts[i].dir = (Direction)(i % 4);
    }
}

// AI для привидений (простой алгоритм погони)
void updateGhostAI(Ghost& ghost, int ghostIndex) {
    int ghostGridX, ghostGridY;
    worldToGrid(ghost.x, ghost.y, ghostGridX, ghostGridY);
    
    int pacmanGridX, pacmanGridY;
    worldToGrid(pacmanGame.pacmanX, pacmanGame.pacmanY, pacmanGridX, pacmanGridY);
    
    // Разные стратегии для каждого привидения
    switch (ghostIndex) {
        case 0: // Красный - напрямую к Пакману
            ghost.targetX = pacmanGridX;
            ghost.targetY = pacmanGridY;
            break;
        case 1: // Розовый - на 4 клетки впереди Пакмана
            ghost.targetX = pacmanGridX;
            ghost.targetY = pacmanGridY;
            if (pacmanGame.pacmanDir == UP) ghost.targetY -= 4;
            else if (pacmanGame.pacmanDir == DOWN) ghost.targetY += 4;
            else if (pacmanGame.pacmanDir == LEFT) ghost.targetX -= 4;
            else if (pacmanGame.pacmanDir == RIGHT) ghost.targetX += 4;
            break;
        case 2: // Голубой - отражение от вектора между красным и Пакманом
            {
                int redGhostX, redGhostY;
                worldToGrid(pacmanGame.ghosts[0].x, pacmanGame.ghosts[0].y, redGhostX, redGhostY);
                
                int dx = pacmanGridX - redGhostX;
                int dy = pacmanGridY - redGhostY;
                ghost.targetX = pacmanGridX + dx;
                ghost.targetY = pacmanGridY + dy;
            }
            break;
        case 3: // Оранжевый - случайная цель, когда далеко от Пакмана
            {
                float distance = sqrt(pow(ghost.x - pacmanGame.pacmanX, 2) + 
                                    pow(ghost.y - pacmanGame.pacmanY, 2));
                
                if (distance > CELL_SIZE * 8) {
                    ghost.targetX = pacmanGridX;
                    ghost.targetY = pacmanGridY;
                } else {
                    // Случайная цель в лабиринте
                    ghost.targetX = random(MAZE_WIDTH);
                    ghost.targetY = random(MAZE_HEIGHT);
                }
            }
            break;
    }
    
    // Если испуган - убегаем от Пакмана
    if (ghost.frightened && !ghost.eaten) {
        ghost.targetX = random(MAZE_WIDTH);
        ghost.targetY = random(MAZE_HEIGHT);
    }
    
    // Если съеден - возвращаемся в дом
    if (ghost.eaten) {
        ghost.targetX = 9;
        ghost.targetY = 7;
        
        // Если достиг дома - возрождаемся
        if (ghostGridX == 9 && ghostGridY == 7) {
            ghost.eaten = false;
            ghost.frightened = false;
        }
    }
}

// Движение привидения
void moveGhost(Ghost& ghost, int ghostIndex) {
    unsigned long currentTime = millis();
    
    // Проверяем, прошло ли достаточно времени для движения привидения
    if (currentTime - ghost.lastMoveTime < 50) { // Двигаемся каждые 100 мс
        return;
    }
    
    ghost.lastMoveTime = currentTime;
    
    updateGhostAI(ghost, ghostIndex);
    
    int gridX, gridY;
    worldToGrid(ghost.x, ghost.y, gridX, gridY);
    
    // Выбор направления движения
    Direction bestDir = ghost.dir;
    float bestDistance = 9999.0f;
    
    // Проверяем возможные направления
    Direction directions[4] = {UP, DOWN, LEFT, RIGHT};
    
    for (int i = 0; i < 4; i++) {
        Direction testDir = directions[i];
        
        // Нельзя разворачиваться на 180 градусов (кроме испуганных)
        if (!ghost.frightened && !ghost.eaten) {
            if ((ghost.dir == UP && testDir == DOWN) ||
                (ghost.dir == DOWN && testDir == UP) ||
                (ghost.dir == LEFT && testDir == RIGHT) ||
                (ghost.dir == RIGHT && testDir == LEFT)) {
                continue;
            }
        }
        
        int nextGridX = gridX;
        int nextGridY = gridY;
        
        switch (testDir) {
            case UP: nextGridY--; break;
            case DOWN: nextGridY++; break;
            case LEFT: nextGridX--; break;
            case RIGHT: nextGridX++; break;
        }
        
        // Телепорты по бокам
        if (nextGridX < 0) nextGridX = MAZE_WIDTH - 1;
        else if (nextGridX >= MAZE_WIDTH) nextGridX = 0;
        
        // Проверяем можно ли двигаться в этом направлении
        if (canMoveTo(nextGridX, nextGridY, testDir)) {
            // Вычисляем расстояние до цели
            float nextWorldX, nextWorldY;
            gridToWorld(nextGridX, nextGridY, nextWorldX, nextWorldY);
            
            float distance = sqrt(pow(nextWorldX - ghost.targetX * CELL_SIZE, 2) +
                                pow(nextWorldY - ghost.targetY * CELL_SIZE, 2));
            
            if (distance < bestDistance) {
                bestDistance = distance;
                bestDir = testDir;
            }
        }
    }
    
    ghost.dir = bestDir;
    
    // Движение
    float speed = ghost.speed;
    if (ghost.frightened) speed *= 0.5f;
    if (ghost.eaten) speed *= 1.5f;
    
    switch (ghost.dir) {
        case UP: ghost.y -= speed; break;
        case DOWN: ghost.y += speed; break;
        case LEFT: ghost.x -= speed; break;
        case RIGHT: ghost.x += speed; break;
    }
    
    // Обработка телепортов
    if (ghost.x < -CELL_SIZE/2) ghost.x = MAZE_WIDTH * CELL_SIZE - CELL_SIZE/2;
    else if (ghost.x > MAZE_WIDTH * CELL_SIZE - CELL_SIZE/2) ghost.x = -CELL_SIZE/2;
    
    // Ограничение в пределах лабиринта по вертикали
    if (ghost.y < 0) ghost.y = 0;
    if (ghost.y > MAZE_HEIGHT * CELL_SIZE) ghost.y = MAZE_HEIGHT * CELL_SIZE;
}

// Проверка столкновений
void checkCollisions() {
    int pacmanGridX, pacmanGridY;
    worldToGrid(pacmanGame.pacmanX, pacmanGame.pacmanY, pacmanGridX, pacmanGridY);
    
    // Проверка съедения точек
    if (!pacmanGame.maze[pacmanGridY][pacmanGridX].eaten) {
        if (pacmanGame.maze[pacmanGridY][pacmanGridX].dot) {
            pacmanGame.score += 10;
            pacmanGame.dotsEaten++;
            pacmanGame.maze[pacmanGridY][pacmanGridX].eaten = true;
        } else if (pacmanGame.maze[pacmanGridY][pacmanGridX].powerDot) {
            pacmanGame.score += 50;
            pacmanGame.dotsEaten++;
            pacmanGame.maze[pacmanGridY][pacmanGridX].eaten = true;
            
            // Делаем привидений испуганными
            for (int i = 0; i < 4; i++) {
                if (!pacmanGame.ghosts[i].eaten) {
                    pacmanGame.ghosts[i].frightened = true;
                    pacmanGame.ghosts[i].frightenedStart = millis();
                }
            }
        }
    }
    
    // Проверка столкновений с привидениями
    for (int i = 0; i < 4; i++) {
        float dx = pacmanGame.pacmanX - pacmanGame.ghosts[i].x;
        float dy = pacmanGame.pacmanY - pacmanGame.ghosts[i].y;
        float distance = sqrt(dx*dx + dy*dy);
        
        if (distance < PACMAN_SIZE/2 + GHOST_SIZE/2) {
            if (pacmanGame.ghosts[i].frightened && !pacmanGame.ghosts[i].eaten) {
                // Съедаем привидение
                pacmanGame.ghosts[i].eaten = true;
                pacmanGame.ghosts[i].frightened = false;
                pacmanGame.score += 200;
                
                // Отмечаем для достижения
                pacmanGame.ateGhostThisLevel = true;
            } else if (!pacmanGame.ghosts[i].eaten) {
                // Потеря жизни
                pacmanGame.lives--;
                pacmanGame.noDeathsThisLevel = false; // Уже не без смертей
                if (pacmanGame.lives <= 0) {
                    pacmanGame.state = GAME_OVER;
                } else {
                    // Респавн
                    pacmanGame.pacmanX = 9 * CELL_SIZE + CELL_SIZE/2;
                    pacmanGame.pacmanY = 13 * CELL_SIZE + CELL_SIZE/2;
                    pacmanGame.pacmanDir = LEFT;
                    pacmanGame.nextDir = NONE;
                    
                    // Сброс привидений
                    initGhosts();
                }
            }
        }
    }
    
    // Проверка завершения уровня
    if (pacmanGame.dotsEaten >= pacmanGame.totalDots) {
        pacmanGame.state = LEVEL_COMPLETE;
        
        // Проверяем достижения после завершения уровня
        check_pacman_achievements(
            pacmanGame.score, 
            pacmanGame.isFirstGame, 
            pacmanGame.ateGhostThisLevel, 
            true, // clearedLevel
            pacmanGame.noDeathsThisLevel
        );
        pacmanGame.isFirstGame = false;
    }
}

// Основная функция инициализации
void pacman_init() {
    // Инициализация Пакмана
    gridToWorld(9, 13, pacmanGame.pacmanX, pacmanGame.pacmanY);
    pacmanGame.pacmanDir = LEFT;
    pacmanGame.nextDir = NONE;
    pacmanGame.mouthOpen = true;
    pacmanGame.lastMouthTime = millis();
    
    // Инициализация игры
    pacmanGame.score = 0;
    pacmanGame.lives = 3;
    pacmanGame.state = PLAYING;
    pacmanGame.lastUpdateTime = millis();
    pacmanGame.isFirstGame = true;
    pacmanGame.ateGhostThisLevel = false;
    pacmanGame.noDeathsThisLevel = true; // Начинаем с предположения, что пройдем без смертей
    
    // Создание спрайтов
    pacmanGame.pacmanSprite = new TFT_eSprite(&screen);
    pacmanGame.ghostSprite = new TFT_eSprite(&screen);
    
    pacmanGame.pacmanSprite->createSprite(PACMAN_SIZE, PACMAN_SIZE);
    pacmanGame.ghostSprite->createSprite(GHOST_SIZE, GHOST_SIZE);
    
    // ДОБАВЛЕНО: Установка setSwapBytes для спрайтов
    pacmanGame.pacmanSprite->setSwapBytes(true);
    pacmanGame.ghostSprite->setSwapBytes(true);
    
    // Инициализация лабиринта и привидений
    initMaze();
    initGhosts();
}

// Основная функция обновления
void pacman_update() {
    if (pacmanGame.state != PLAYING) {
        if(joy2.button != PRESSED) {
          lastBut = false;
        }
        
        if(joy1.button == PRESSED){
          FPSrend = !FPSrend;
        }
        if(!lastBut && joy2.button == PRESSED){
          if (gameState == PACMAN) {
            gameState = MENU;
            gameChanged = false;
            lastBut = true;
          }
        }
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Пропускаем обновление если не прошло достаточно времени
    if (currentTime - pacmanGame.lastUpdateTime < pacmanGame.UPDATE_INTERVAL) {
        return;
    }
    
    float timeFactor = (currentTime - pacmanGame.lastUpdateTime) / (float)pacmanGame.UPDATE_INTERVAL;
    if (timeFactor > 2.0f) timeFactor = 2.0f;
    
    pacmanGame.lastUpdateTime = currentTime;
    
    // Управление джойстиком 1
    int joyX = joy1.x - 2048; // Центрируем значение
    int joyY = joy1.y - 2048;
    
    // Определяем желаемое направление
    Direction desiredDir = NONE;
    
    if (abs(joyX) > abs(joyY)) {
        if (joyX > 500) desiredDir = RIGHT;
        else if (joyX < -500) desiredDir = LEFT;
    } else {
        if (joyY > 500) desiredDir = UP;
        else if (joyY < -500) desiredDir = DOWN;
    }
    
    if (desiredDir != NONE) {
        pacmanGame.nextDir = desiredDir;
    }
    
    // Пробуем изменить направление
    if (pacmanGame.nextDir != NONE) {
        int gridX, gridY;
        worldToGrid(pacmanGame.pacmanX, pacmanGame.pacmanY, gridX, gridY);
        
        int nextGridX = gridX;
        int nextGridY = gridY;
        
        switch (pacmanGame.nextDir) {
            case UP: nextGridY--; break;
            case DOWN: nextGridY++; break;
            case LEFT: nextGridX--; break;
            case RIGHT: nextGridX++; break;
        }
        
        // Проверяем можно ли двигаться в желаемом направлении
        if (canMoveTo(nextGridX, nextGridY, pacmanGame.nextDir)) {
            pacmanGame.pacmanDir = pacmanGame.nextDir;
            pacmanGame.nextDir = NONE;
        }
    }
    
    // Анимация рта
    if (currentTime - pacmanGame.lastMouthTime > 100) {
        pacmanGame.mouthOpen = !pacmanGame.mouthOpen;
        pacmanGame.lastMouthTime = currentTime;
    }
    
    // Движение Пакмана
    float speed = 2.0f;
    
    // Проверяем можно ли двигаться в текущем направлении
    int currentGridX, currentGridY;
    worldToGrid(pacmanGame.pacmanX, pacmanGame.pacmanY, currentGridX, currentGridY);
    
    int nextGridX = currentGridX;
    int nextGridY = currentGridY;
    
    switch (pacmanGame.pacmanDir) {
        case UP: nextGridY--; break;
        case DOWN: nextGridY++; break;
        case LEFT: nextGridX--; break;
        case RIGHT: nextGridX++; break;
    }
    
    // Телепорты по бокам
    if (nextGridX < 0) nextGridX = MAZE_WIDTH - 1;
    else if (nextGridX >= MAZE_WIDTH) nextGridX = 0;
    
    if (canMoveTo(nextGridX, nextGridY, pacmanGame.pacmanDir)) {
        switch (pacmanGame.pacmanDir) {
            case UP: pacmanGame.pacmanY -= speed * timeFactor; break;
            case DOWN: pacmanGame.pacmanY += speed * timeFactor; break;
            case LEFT: pacmanGame.pacmanX -= speed * timeFactor; break;
            case RIGHT: pacmanGame.pacmanX += speed * timeFactor; break;
        }
    } else {
        // Выравниваем по сетке если уперлись в стену
        alignToGrid(pacmanGame.pacmanX, pacmanGame.pacmanY, pacmanGame.pacmanDir);
    }
    
    // Обработка телепортов
    if (pacmanGame.pacmanX < -CELL_SIZE/2) pacmanGame.pacmanX = MAZE_WIDTH * CELL_SIZE - CELL_SIZE/2;
    else if (pacmanGame.pacmanX > MAZE_WIDTH * CELL_SIZE - CELL_SIZE/2) pacmanGame.pacmanX = -CELL_SIZE/2;
    
    // Обновление привидений
    for (int i = 0; i < 4; i++) {
        // Таймер испуга (10 секунд)
        if (pacmanGame.ghosts[i].frightened && 
            currentTime - pacmanGame.ghosts[i].frightenedStart > 10000) {
            pacmanGame.ghosts[i].frightened = false;
        }
        
        moveGhost(pacmanGame.ghosts[i], i);
    }
    
    // Проверка столкновений
    checkCollisions();
    
    // Проверяем достижение "первая точка"
    if (pacmanGame.score > 0 && pacmanGame.isFirstGame) {
        check_pacman_achievements(
            pacmanGame.score, 
            pacmanGame.isFirstGame, 
            false, // ateGhost
            false, // clearedLevel
            false  // noDeaths
        );
    }
    
  if(joy2.button != PRESSED) {
    lastBut = false;
  }
  
  if(joy1.button == PRESSED){
    FPSrend = !FPSrend;
  }
  if(!lastBut && joy2.button == PRESSED){
    if (gameState == PACMAN) {
      gameState = MENU;
      gameChanged = false;
      lastBut = true;
    }
  }
}

// Функция отрисовки Пакмана
void drawPacman(int x, int y, Direction dir, bool mouthOpen) {
    pacmanGame.pacmanSprite->fillSprite(TFT_BLACK);
    
    int centerX = PACMAN_SIZE / 2;
    int centerY = PACMAN_SIZE / 2;
    int radius = PACMAN_SIZE / 2;
    
    // Цвет Пакмана
    uint16_t color = TFT_YELLOW;
    
    // Угол рта в зависимости от направления и состояния
    int startAngle = 0;
    int endAngle = 360;
    
    if (mouthOpen) {
        switch (dir) {
            case RIGHT:
                startAngle = 30;
                endAngle = 330;
                break;
            case LEFT:
                startAngle = 210;
                endAngle = 510;
                break;
            case UP:
                startAngle = 120;
                endAngle = 420;
                break;
            case DOWN:
                startAngle = 300;
                endAngle = 600;
                break;
        }
    }
    
    // Рисуем Пакмана как заливку
    for (int r = radius; r > 0; r--) {
        pacmanGame.pacmanSprite->fillSmoothCircle(centerX, centerY, r, color);
    }
    
    // Вырезаем рот
    if (mouthOpen) {
        pacmanGame.pacmanSprite->fillTriangle(
            centerX, centerY,
            centerX + radius * cos(startAngle * PI / 180.0),
            centerY + radius * sin(startAngle * PI / 180.0),
            centerX + radius * cos((startAngle + 60) * PI / 180.0),
            centerY + radius * sin((startAngle + 60) * PI / 180.0),
            TFT_BLACK
        );
    }
    
    pacmanGame.pacmanSprite->pushToSprite(&screen, x - centerX, y - centerY, TFT_BLACK);
}

// Функция отрисовки привидения
void drawGhost(int x, int y, uint16_t color, bool frightened, bool eaten) {
    pacmanGame.ghostSprite->fillSprite(TFT_BLACK);
    
    int centerX = GHOST_SIZE / 2;
    int centerY = GHOST_SIZE / 2;
    int radius = GHOST_SIZE / 2;

    // Цвет привидения
    uint16_t ghostColor = color;
    if (frightened) ghostColor = TFT_BLUE;
    if (eaten) ghostColor = TFT_WHITE;
    
    // Тело привидения (полукруг сверху)
    pacmanGame.ghostSprite->fillSmoothRoundRect(0, radius/2, GHOST_SIZE, radius, radius/2, ghostColor);
    
    // Голова (полукруг)
    pacmanGame.ghostSprite->fillSmoothCircle(centerX, centerY - 1, radius, ghostColor);
    
    // Ноги (волны)
    for (int i = 0; i < 3; i++) {
        int legX = i * (GHOST_SIZE / 3);
        pacmanGame.ghostSprite->fillCircle(legX + radius/3, GHOST_SIZE - 2, 2, ghostColor);
    }
    
    // Глаза
    if (!eaten) {
        pacmanGame.ghostSprite->fillCircle(centerX - 2, centerY - 1, 2, TFT_WHITE);
        pacmanGame.ghostSprite->fillCircle(centerX + 2, centerY - 1, 2, TFT_WHITE);
        
        pacmanGame.ghostSprite->fillCircle(centerX - 2, centerY - 1, 1, TFT_BLUE);
        pacmanGame.ghostSprite->fillCircle(centerX + 2, centerY - 1, 1, TFT_BLUE);
    }
    
    pacmanGame.ghostSprite->pushToSprite(&screen, x - centerX, y - centerY, TFT_BLACK);
}

// Функция отрисовки лабиринта
void drawMaze() {
    // Фон
    screen.fillRect(0, 0, MAZE_WIDTH * CELL_SIZE, MAZE_HEIGHT * CELL_SIZE, TFT_BLACK);
    
    // Стены
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (pacmanGame.maze[y][x].wall) {
                screen.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, TFT_BLUE);
                // Делаем стены более детальными
                screen.drawRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, TFT_NAVY);
            }
        }
    }
    
    // Точки
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (!pacmanGame.maze[y][x].wall && !pacmanGame.maze[y][x].eaten) {
                int centerX = x * CELL_SIZE + CELL_SIZE / 2;
                int centerY = y * CELL_SIZE + CELL_SIZE / 2;
                
                if (pacmanGame.maze[y][x].powerDot) {
                    // Супер-точки
                    screen.fillSmoothCircle(centerX, centerY, POWER_DOT_SIZE, TFT_YELLOW);
                    screen.fillSmoothCircle(centerX, centerY, POWER_DOT_SIZE - 2, TFT_ORANGE);
                } else if (pacmanGame.maze[y][x].dot) {
                    // Обычные точки
                    screen.fillSmoothCircle(centerX, centerY, DOT_SIZE, TFT_YELLOW);
                }
            }
        }
    }
}

// Основная функция рендеринга
void pacman_render(TFT_eSPI* tft) {
    screen.fillSprite(TFT_BLACK);
    
    // Отрисовка лабиринта
    drawMaze();
    
    // Отрисовка привидений
    for (int i = 0; i < 4; i++) {
        drawGhost(pacmanGame.ghosts[i].x, pacmanGame.ghosts[i].y,
                 pacmanGame.ghosts[i].color,
                 pacmanGame.ghosts[i].frightened,
                 pacmanGame.ghosts[i].eaten);
    }
    
    // Отрисовка Пакмана
    drawPacman(pacmanGame.pacmanX, pacmanGame.pacmanY, 
              pacmanGame.pacmanDir, pacmanGame.mouthOpen);
    
    // Панель с информацией (помещаем ее справа от лабиринта или снизу)
    int mazeWidth = MAZE_WIDTH * CELL_SIZE;
    int mazeHeight = MAZE_HEIGHT * CELL_SIZE;
    
    // Проверяем, хватает ли места снизу
    if (mazeHeight + 40 <= 230) {
        // Вариант A: Панель снизу (если лабиринт не слишком высокий)
        int infoY = mazeHeight;
        int infoHeight = 230 - mazeHeight;
        
        // Фон панели
        screen.fillRect(0, infoY, 240, infoHeight, TFT_BLACK);
        
        // Счет
        screen.setTextColor(TFT_WHITE, TFT_BLACK);
        screen.setTextSize(1);
        screen.setCursor(10, infoY + 5);
        screen.print("SCORE: ");
        screen.print(pacmanGame.score);
        
        // Жизни
        screen.setCursor(10, infoY + 20);
        screen.print("LIVES: ");
        for (int i = 0; i < pacmanGame.lives; i++) {
            drawPacman(60 + i * 15, infoY + 15, RIGHT, true);
        }
        
        // Сообщения о состоянии игры
        if (pacmanGame.state == GAME_OVER) {
            screen.setTextSize(1); // Уменьшаем размер текста
            screen.setTextColor(TFT_RED, TFT_BLACK);
            screen.setCursor(50, infoY + 40);
            screen.print("GAME OVER");
            
            screen.setTextSize(1);
            screen.setTextColor(TFT_WHITE, TFT_BLACK);
            screen.setCursor(30, infoY + 55);
            screen.print("Press JOY2 for Menu");
        } else if (pacmanGame.state == LEVEL_COMPLETE) {
            screen.setTextSize(1); // Уменьшаем размер текста
            screen.setTextColor(TFT_GREEN, TFT_BLACK);
            screen.setCursor(40, infoY + 40);
            screen.print("LEVEL CLEAR!");
            
            screen.setTextSize(1);
            screen.setTextColor(TFT_WHITE, TFT_BLACK);
            screen.setCursor(30, infoY + 55);
            screen.print("Press JOY2 for Menu");
        }
        
        // FPS
        if (FPSrend) {
            fps.drawToSprite(&screen, 180, infoY + 5);
        }
    } else {
        // Вариант B: Панель справа (если лабиринт занимает всю высоту)
        int infoX = mazeWidth;
        int infoWidth = 240 - mazeWidth;
        
        if (infoWidth > 60) { // Убедимся, что есть место для панели
            // Фон панели
            screen.fillRect(infoX, 0, infoWidth, 230, TFT_BLACK);
            
            // Вертикальная панель информации
            screen.setTextColor(TFT_WHITE, TFT_BLACK);
            screen.setTextSize(1);
            
            // Счет
            screen.setCursor(infoX + 5, 10);
            screen.print("SCORE:");
            screen.setCursor(infoX + 5, 25);
            screen.print(pacmanGame.score);
            
            // Жизни
            screen.setCursor(infoX + 5, 45);
            screen.print("LIVES:");
            for (int i = 0; i < pacmanGame.lives; i++) {
                drawPacman(infoX + 20 + i * 12, 60, RIGHT, true);
            }
            
            // FPS
            if (FPSrend) {
                fps.drawToSprite(&screen, infoX + 5, 90);
            } 
        }
    }
    // Отрисовка уведомлений о достижениях (всегда поверх всего)
    achievements_render();
    // Выводим на экран
    screen.pushSprite(0, 0);
}