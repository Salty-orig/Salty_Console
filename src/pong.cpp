#include "pong.h"
//#include "audio_simple.h"
#include "input.h"
#include "shared.h"
#include "fps.h"
#include "achievements.h"
#include <math.h>

PongState pongState = PONG_MENU;  

// Константы скорости
const float BALL_BASE_SPEED = 1.9f;
const float PADDLE_SPEED = 3.0f;


// Размеры хитбоксов
const float BALL_RADIUS = 8.0f;
const float PADDLE_WIDTH = 10.0f;
const float PADDLE_HEIGHT = 40.0f;

// Флаги для отслеживания первой игры
static bool isFirstPongGame = true;

// Пункты меню Pong
const char* pongMenuItems[] = {"Start Game", "Settings", "Back to Main Menu"};
const int pongMenuItemCount = 3;
static int pongMenuSelection = 0;

// Пункты настроек Pong
const char* pongSettingsItems[] = {"Game Mode: 2 Players", "Bot Difficulty: Normal", "Ball Speed: Normal", "Paddle Size: Normal", "Back"};
const int pongSettingsItemCount = 5;
static int pongSettingsSelection = 0;

// Режимы игры
enum GameMode {
  MODE_TWO_PLAYERS,
  MODE_VS_BOT
};

// Сложность бота
enum BotDifficulty {
  DIFFICULTY_EASY,
  DIFFICULTY_NORMAL,
  DIFFICULTY_HARD
};

int rainbowId = startColorAnimation(rgb(255, 1, 200), rgb(255, 38, 0), 750, true);
int ballAnimId = startColorAnimation(rgb(255, 238, 1), rgb(255, 174, 0), 2500, true);
int ballAnimMaxSpeedId = startRainbowAnimation(1200, true);

// Настройки игры
static struct PongSettings {
  float ballSpeedMultiplier = 1.0f;
  float paddleHeightMultiplier = 1.0f;
  GameMode gameMode = MODE_TWO_PLAYERS;
  BotDifficulty botDifficulty = DIFFICULTY_NORMAL;
  
  // Текстовые представления
  const char* getBallSpeedText() {
    if (ballSpeedMultiplier < 0.8f) return "Ball Speed: Slow";
    if (ballSpeedMultiplier > 1.2f) return "Ball Speed: Fast";
    return "Ball Speed: Normal";
  }
  
  const char* getPaddleSizeText() {
    if (paddleHeightMultiplier < 0.8f) return "Paddle Size: Small";
    if (paddleHeightMultiplier > 1.2f) return "Paddle Size: Large";
    return "Paddle Size: Normal";
  }
  
  const char* getGameModeText() {
    if (gameMode == MODE_TWO_PLAYERS) return "Game Mode: 2 Players";
    return "Game Mode: VS Bot";
  }
  
  const char* getBotDifficultyText() {
    switch(botDifficulty) {
      case DIFFICULTY_EASY: return "Bot Difficulty: Easy";
      case DIFFICULTY_NORMAL: return "Bot Difficulty: Normal";
      case DIFFICULTY_HARD: return "Bot Difficulty: Hard";
      default: return "Bot Difficulty: Normal";
    }
  }
} pongSettings;

// Анимации для меню
static float pongMenuHighlightProgress[3] = {0, 0, 0};
static float pongMenuGlowProgress[3] = {0, 0, 0};
static float pongSettingsHighlightProgress[5] = {0, 0, 0, 0, 0};
static float pongSettingsGlowProgress[5] = {0, 0, 0, 0, 0};
static unsigned long lastPongMenuAnimTime = 0;
const float PONG_ANIM_SPEED = 0.20f;

struct PongGame {
  float ballX = 160.0f;
  float ballY = 120.0f;
  float ballSpeedX = 0.0f;
  float ballSpeedY = 0.0f;
  float player1Y = 100.0f;
  float player2Y = 100.0f;
  int score1 = 0;
  int score2 = 0;
  
  // Таймер для независимого от FPS движения
  unsigned long lastUpdateTime = 0;
  const unsigned long UPDATE_INTERVAL = 16; // ~60 FPS в мс
  
  // Для бота: предсказание движения мяча
  float targetY = 100.0f;
  float botVelocity = 0.0f;
} pongGame;

void pong_init() {
  

  pongGame.ballX = 160.0f;
  pongGame.ballY = 120.0f;
  
  // Начальная скорость с случайным направлением
  int directionX = random(-2, 2) ? 1 : -1;
  int directionY = random(-2, 2) ? 1 : -1;
  
  // Начальная скорость с учетом множителя из настроек
  float baseSpeed = BALL_BASE_SPEED * pongSettings.ballSpeedMultiplier;
  pongGame.ballSpeedX = baseSpeed * directionX;
  pongGame.ballSpeedY = baseSpeed * directionY;
  
  pongGame.lastUpdateTime = millis();
  pongGame.botVelocity = 0.0f;
  pongGame.player2Y = 100.0f; // Сбрасываем позицию бота
  
}

void pong_menu_init() {
  pongState = PONG_MENU;
  pongMenuSelection = 0;
  
  // Сбрасываем анимации
  for (int i = 0; i < pongMenuItemCount; i++) {
    pongMenuHighlightProgress[i] = 0;
    pongMenuGlowProgress[i] = 0;
  }
  pongGame.score1 = 0;
  pongGame.score2 = 0;
}

void pong_settings_init() {
  pongState = PONG_SETTINGS;
  pongSettingsSelection = 0;
  
  // Сбрасываем анимации настроек
  for (int i = 0; i < pongSettingsItemCount; i++) {
    pongSettingsHighlightProgress[i] = 0;
    pongSettingsGlowProgress[i] = 0;
  }
}

// Функция проверки столкновения круга с прямоугольником
bool check_circle_rect_collision(float circleX, float circleY, float radius,
                                 float rectX, float rectY, float rectWidth, float rectHeight) {
  // Находим ближайшую точку прямоугольника к центру круга
  float closestX = circleX;
  float closestY = circleY;
  
  if (circleX < rectX) {
    closestX = rectX;
  } else if (circleX > rectX + rectWidth) {
    closestX = rectX + rectWidth;
  }
  
  if (circleY < rectY) {
    closestY = rectY;
  } else if (circleY > rectY + rectHeight) {
    closestY = rectY + rectHeight;
  }
  
  float distanceX = circleX - closestX;
  float distanceY = circleY - closestY;
  float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
  
  return distanceSquared <= (radius * radius);
}

// Функция управления ботом - вызывается каждый кадр
void update_bot(float timeFactor) {
  if (pongSettings.gameMode != MODE_VS_BOT) return;
  
  float effectivePaddleHeight = PADDLE_HEIGHT * pongSettings.paddleHeightMultiplier;
  float maxPaddleY = 240 - effectivePaddleHeight;
  
  // Параметры сложности
  float predictionAccuracy = 1.0f;
  float maxSpeedMultiplier = 1.0f;
  float errorChance = 0.0f;
  float errorMagnitude = 0.0f;
  
  switch(pongSettings.botDifficulty) {
    case DIFFICULTY_EASY:
      predictionAccuracy = 0.65f;
      maxSpeedMultiplier = 0.7f;
      errorChance = 0.32f;
      errorMagnitude = 45.0f;
      break;
      
    case DIFFICULTY_NORMAL:
      predictionAccuracy = 0.85f;
      maxSpeedMultiplier = 0.92f;
      errorChance = 0.15f;
      errorMagnitude = 25.0f;
      break;
      
    case DIFFICULTY_HARD:
      predictionAccuracy = 0.99f;
      maxSpeedMultiplier = 1.18f;
      errorChance = 0.001f;
      errorMagnitude = 0.5f;
      break;
  }
  
  // Предсказываем позицию мяча
  float targetY = pongGame.ballY;
  
  if (pongGame.ballSpeedX > 0) {
    // Мяч летит в сторону бота
    float distanceToBot = 300 - pongGame.ballX;
    if (distanceToBot > 0 && pongGame.ballSpeedX > 0) {
      float timeToReach = distanceToBot / pongGame.ballSpeedX;
      float predictedY = pongGame.ballY + pongGame.ballSpeedY * timeToReach;
      
      // Количество учитываемых отскоков
      int maxBounces = 1;
      if (predictionAccuracy > 0.9f) maxBounces = 3;
      else if (predictionAccuracy > 0.8f) maxBounces = 2;
      
      // Предсказываем отскоки
      for (int i = 0; i < maxBounces; i++) {
        if (predictedY < BALL_RADIUS) {
          predictedY = 2 * BALL_RADIUS - predictedY;
        } else if (predictedY > 240 - BALL_RADIUS) {
          predictedY = 2 * (240 - BALL_RADIUS) - predictedY;
        }
      }
      
      // Применяем точность предсказания
      targetY = pongGame.ballY * (1 - predictionAccuracy) + predictedY * predictionAccuracy;
    }
  }
  
  // Добавляем случайные ошибки
  if (random(100) < errorChance * 100) {
    targetY += random(-errorMagnitude, errorMagnitude);
  }
  
  // Ограничиваем целевую позицию
  targetY = constrain(targetY, 0, maxPaddleY);
  
  pongGame.targetY = targetY;
  
  // Плавное движение к цели
  float diff = targetY - pongGame.player2Y;
  
  // Максимальная скорость бота
  float maxBotSpeed = PADDLE_SPEED * maxSpeedMultiplier;
  
  // Двигаем бота к цели с плавностью
  if (abs(diff) > 2.0f) {
    // Чем дальше цель, тем быстрее движение
    float speed = diff * 0.25f;
    speed = constrain(speed, -maxBotSpeed, maxBotSpeed);
    
    // Применяем инерцию для плавности
    pongGame.botVelocity = pongGame.botVelocity * 0.92f + speed * 0.08f;
    pongGame.botVelocity = constrain(pongGame.botVelocity, -maxBotSpeed, maxBotSpeed);
    
    pongGame.player2Y += pongGame.botVelocity * timeFactor;
  } else {
    // Если близко к цели, замедляемся
    pongGame.botVelocity *= 0.95f;
    pongGame.player2Y += pongGame.botVelocity * timeFactor;
  }
  
  // Ограничиваем позицию
  pongGame.player2Y = constrain(pongGame.player2Y, 0, maxPaddleY);
}

void pong_update() {
  unsigned long currentTime = millis();
  
  // Обновление анимаций меню
  if (pongState == PONG_MENU || pongState == PONG_SETTINGS) {
    bool needRedraw = false;
    int itemCount = (pongState == PONG_MENU) ? pongMenuItemCount : pongSettingsItemCount;
    int selection = (pongState == PONG_MENU) ? pongMenuSelection : pongSettingsSelection;
    float* highlightProgress = (pongState == PONG_MENU) ? pongMenuHighlightProgress : pongSettingsHighlightProgress;
    float* glowProgress = (pongState == PONG_MENU) ? pongMenuGlowProgress : pongSettingsGlowProgress;
    
    for (int i = 0; i < itemCount; i++) {
      // Прогресс выделения
      if (i == selection) {
        if (highlightProgress[i] < 1.0f) {
          highlightProgress[i] += PONG_ANIM_SPEED;
          if (highlightProgress[i] > 1.0f) highlightProgress[i] = 1.0f;
          needRedraw = true;
        }
      } else {
        if (highlightProgress[i] > 0) {
          highlightProgress[i] -= PONG_ANIM_SPEED * 0.7f;
          if (highlightProgress[i] < 0) highlightProgress[i] = 0;
          needRedraw = true;
        }
      }
      
      // Прогресс подсветки соседей
      if (abs(i - selection) == 1) {
        if (glowProgress[i] < 0.5f) {
          glowProgress[i] += PONG_ANIM_SPEED * 0.8f;
          if (glowProgress[i] > 0.5f) glowProgress[i] = 0.5f;
          needRedraw = true;
        }
      } else {
        if (glowProgress[i] > 0) {
          glowProgress[i] -= PONG_ANIM_SPEED * 0.8f;
          if (glowProgress[i] < 0) glowProgress[i] = 0;
          needRedraw = true;
        }
      }
    }
    if (needRedraw) CHANGES_BTN = true;
  }
  
  // Обработка ввода с задержкой
  static unsigned long lastPongJoyTime = 0;
  const unsigned long JOY_DELAY = 200;
  
  if (currentTime - lastPongJoyTime >= JOY_DELAY) {
    
    // МЕНЮ PONG
    if (pongState == PONG_MENU) {
      if (joy2.y > 3000) { // Вверх
        pongMenuSelection = (pongMenuSelection - 1 + pongMenuItemCount) % pongMenuItemCount;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      } else if (joy2.y < 1000) { // Вниз
        pongMenuSelection = (pongMenuSelection + 1) % pongMenuItemCount;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      }
      
      static bool lastButPongMenu = false;
      if (joy2.button != PRESSED) {
        lastButPongMenu = false;
      }
      
      if (!lastButPongMenu && joy2.button == PRESSED) {
        switch(pongMenuSelection) {
          case 0: // Start Game
            pongState = PONG_GAME;
            pong_init();
            break;
          case 1: // Settings
            pong_settings_init();
            break;
          case 2: // Back to Main Menu
            gameState = MENU;
            gameChanged = false;
            isFirstPongGame = true;
            break;
        }
        lastButPongMenu = true;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      }
    }
    
    // НАСТРОЙКИ PONG
    else if (pongState == PONG_SETTINGS) {
      if (joy2.y > 3000) { // Вверх
        pongSettingsSelection = (pongSettingsSelection - 1 + pongSettingsItemCount) % pongSettingsItemCount;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      } else if (joy2.y < 1000) { // Вниз
        pongSettingsSelection = (pongSettingsSelection + 1) % pongSettingsItemCount;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      }
      
      static bool lastButPongSettings = false;
      if (joy2.button != PRESSED) {
        lastButPongSettings = false;
      }
      
      if (!lastButPongSettings && joy2.button == PRESSED) {
        switch(pongSettingsSelection) {
          case 0: // Game Mode
            if (pongSettings.gameMode == MODE_TWO_PLAYERS) {
              pongSettings.gameMode = MODE_VS_BOT;
            } else {
              pongSettings.gameMode = MODE_TWO_PLAYERS;
            }
            break;
            
          case 1: // Bot Difficulty
            switch(pongSettings.botDifficulty) {
              case DIFFICULTY_EASY:
                pongSettings.botDifficulty = DIFFICULTY_NORMAL;
                break;
              case DIFFICULTY_NORMAL:
                pongSettings.botDifficulty = DIFFICULTY_HARD;
                break;
              case DIFFICULTY_HARD:
                pongSettings.botDifficulty = DIFFICULTY_EASY;
                break;
            }
            break;
            
          case 2: // Ball Speed
            if (pongSettings.ballSpeedMultiplier < 0.8f) {
              pongSettings.ballSpeedMultiplier = 1.0f;
            } else if (pongSettings.ballSpeedMultiplier < 1.1f) {
              pongSettings.ballSpeedMultiplier = 1.3f;
            } else {
              pongSettings.ballSpeedMultiplier = 0.7f;
            }
            break;
            
          case 3: // Paddle Size
            if (pongSettings.paddleHeightMultiplier < 0.8f) {
              pongSettings.paddleHeightMultiplier = 1.0f;
            } else if (pongSettings.paddleHeightMultiplier < 1.1f) {
              pongSettings.paddleHeightMultiplier = 1.3f;
            } else {
              pongSettings.paddleHeightMultiplier = 0.7f;
            }
            break;
            
          case 4: // Back
            pong_menu_init();
            break;
        }
        lastButPongSettings = true;
        lastPongJoyTime = currentTime;
        CHANGES_BTN = true;
      }
    }
  }
  
  // ИГРОВОЙ ПРОЦЕСС
  if (pongState == PONG_GAME) {
    
    // Пропускаем обновление если не прошло достаточно времени
    if (currentTime - pongGame.lastUpdateTime < pongGame.UPDATE_INTERVAL) {
      return;
    }
    
    // Вычисляем множитель для компенсации задержек
    float timeFactor = (currentTime - pongGame.lastUpdateTime) / (float)pongGame.UPDATE_INTERVAL;
    pongGame.lastUpdateTime = currentTime;
    
    if (timeFactor > 2.0f) timeFactor = 2.0f;
    
    // Управление ракетками
    const int UP_ZONE = 1000;
    const int DOWN_ZONE = 3000;
    
    // Левая ракетка (игрок 1)
    int joy1Pos = joy1.y;
    float paddle1Speed = 0.0f;
    
    if (joy1Pos < UP_ZONE) {
      paddle1Speed = 1.0f;
    } else if (joy1Pos > DOWN_ZONE) {
      paddle1Speed = -1.0f;
    }
    
    pongGame.player1Y += paddle1Speed * PADDLE_SPEED * timeFactor;
    
    // Правая ракетка (игрок 2 или бот)
    if (pongSettings.gameMode == MODE_TWO_PLAYERS) {
      int joy2Pos = joy2.y;
      float paddle2Speed = 0.0f;
      
      if (joy2Pos < UP_ZONE) {
        paddle2Speed = 1.0f;
      } else if (joy2Pos > DOWN_ZONE) {
        paddle2Speed = -1.0f;
      }
      
      pongGame.player2Y += paddle2Speed * PADDLE_SPEED * timeFactor;
    } else {
      // Режим VS Bot - обновляем бота каждый кадр
      update_bot(timeFactor);
    }
    
    // Ограничение движения ракеток с учетом размера
    float effectivePaddleHeight = PADDLE_HEIGHT * pongSettings.paddleHeightMultiplier;
    float maxPaddleY = 240 - effectivePaddleHeight;
    
    pongGame.player1Y = constrain(pongGame.player1Y, 0, maxPaddleY);
    pongGame.player2Y = constrain(pongGame.player2Y, 0, maxPaddleY);
    
    // Движение мяча
    pongGame.ballX += pongGame.ballSpeedX * timeFactor;
    pongGame.ballY += pongGame.ballSpeedY * timeFactor;
    
    // Отскоки от стен
    if (pongGame.ballY <= BALL_RADIUS) {
      pongGame.ballY = BALL_RADIUS;
      pongGame.ballSpeedY = fabs(pongGame.ballSpeedY);
    }
    if (pongGame.ballY >= 240 - BALL_RADIUS) {
      pongGame.ballY = 240 - BALL_RADIUS;
      pongGame.ballSpeedY = -fabs(pongGame.ballSpeedY);
    }
    
    // Столкновения с ракетками
    // Левая ракетка
    if (check_circle_rect_collision(pongGame.ballX, pongGame.ballY, BALL_RADIUS,
                                    10, pongGame.player1Y, PADDLE_WIDTH, effectivePaddleHeight)) {
                                    
      pongGame.ballX = 10 + PADDLE_WIDTH + BALL_RADIUS;
      pongGame.ballSpeedX = fabs(pongGame.ballSpeedX);
      
      float relativeHitPos = (pongGame.ballY - (pongGame.player1Y + effectivePaddleHeight/2)) / (effectivePaddleHeight/2);
      pongGame.ballSpeedY += relativeHitPos * 0.7f;
      
      pongGame.ballSpeedX *= 1.05f;
      pongGame.ballSpeedY *= 1.05f;
    }
    
    // Правая ракетка
    if (check_circle_rect_collision(pongGame.ballX, pongGame.ballY, BALL_RADIUS,
                                    300, pongGame.player2Y, PADDLE_WIDTH, effectivePaddleHeight)) {
                                    
      pongGame.ballX = 300 - BALL_RADIUS;
      pongGame.ballSpeedX = -fabs(pongGame.ballSpeedX);
      
      float relativeHitPos = (pongGame.ballY - (pongGame.player2Y + effectivePaddleHeight/2)) / (effectivePaddleHeight/2);
      pongGame.ballSpeedY += relativeHitPos * 0.7f;
      
      pongGame.ballSpeedX *= 1.05f;
      pongGame.ballSpeedY *= 1.05f;
    }
    
    // Ограничение скорости мяча    
    float maxSpeed = 7.0f * pongSettings.ballSpeedMultiplier;
    pongGame.ballSpeedY = constrain(pongGame.ballSpeedY, -maxSpeed, maxSpeed);
    pongGame.ballSpeedX = constrain(pongGame.ballSpeedX, -maxSpeed, maxSpeed);
    
    // Забитие гола
    if (pongGame.ballX < -BALL_RADIUS) {
      pongGame.score2++;
      check_pong_achievements(pongGame.score1, pongGame.score2, isFirstPongGame);
      isFirstPongGame = false;
      pong_init();
    } else if (pongGame.ballX > 320 + BALL_RADIUS) {
      pongGame.score1++;
      check_pong_achievements(pongGame.score1, pongGame.score2, isFirstPongGame);
      isFirstPongGame = false;
      pong_init();
    }
    
    // Возврат в меню по кнопке
    static bool lastButGame = false;
    if (joy2.button != PRESSED) {
      lastButGame = false;
    }
    
    if (!lastButGame && joy2.button == PRESSED) {
      pong_menu_init();
      lastButGame = true;
    }
  }
}

void pong_render(TFT_eSPI* tft) {

  uint16_t currentColor = getColor(rainbowId);
  uint16_t currentColorBall = getColor(ballAnimId);
  uint16_t currentColorBallSpeed = getColor(ballAnimMaxSpeedId);
 
  if (pongState == PONG_GAME) {
    // ИГРОВОЙ ЭКРАН
    screen.fillSprite(rgb(0, 80, 47));
    // Центральная линия пунктиром
    for (int y = 0; y < 240; y += 20) {
      screen.drawFastVLine(160, y, 10, TFT_WHITE);
    }
    
    // Ракетки с учетом размера из настроек
    float effectivePaddleHeight = PADDLE_HEIGHT * pongSettings.paddleHeightMultiplier;
    screen.fillSmoothRoundRect(10, (int)pongGame.player1Y, 10, (int)effectivePaddleHeight, 5, TFT_WHITE);
    if (pongSettings.gameMode == MODE_VS_BOT) {
      switch(pongSettings.botDifficulty) {
        case DIFFICULTY_EASY:
          screen.fillSmoothRoundRect(300, (int)pongGame.player2Y, 10, (int)effectivePaddleHeight, 5, TFT_WHITE);
          screen.fillSmoothRoundRect(302, (int)pongGame.player2Y + 2, 6, (int)effectivePaddleHeight - 4, 3, rgb(21, 255, 0));
        break;
          
        case DIFFICULTY_NORMAL:
          screen.fillSmoothRoundRect(300, (int)pongGame.player2Y, 10, (int)effectivePaddleHeight, 5, TFT_WHITE);
          screen.fillSmoothRoundRect(302, (int)pongGame.player2Y + 2, 6, (int)effectivePaddleHeight - 4, 3, rgb(255, 166, 0));
        break;
          
        case DIFFICULTY_HARD:
          screen.fillSmoothRoundRect(300, (int)pongGame.player2Y, 10, (int)effectivePaddleHeight, 5, TFT_WHITE);
          screen.fillSmoothRoundRect(302, (int)pongGame.player2Y + 2, 6, (int)effectivePaddleHeight - 4, 3, currentColor);
        break;
      }
    }else if (pongSettings.gameMode == MODE_TWO_PLAYERS) {
      screen.fillSmoothRoundRect(300, (int)pongGame.player2Y, 10, (int)effectivePaddleHeight, 5, TFT_WHITE);
    }
    
    // Счет
    screen.setTextColor(rgb(160, 252, 236), rgb(0, 80, 47));
    screen.setTextSize(3);
    
    String score1Str = String(pongGame.score1);
    int score1Width = score1Str.length() * 18;
    screen.setCursor(80 - score1Width/2, 10);
    screen.print(pongGame.score1);
    
    screen.setCursor(153, 10);
    screen.print(":");
    
    String score2Str = String(pongGame.score2);
    int score2Width = score2Str.length() * 18;
    screen.setCursor(240 - score2Width/2, 10);
    screen.print(pongGame.score2);
    
    // Границы поля
    screen.drawFastHLine(0, 0, 320, rgb(160, 252, 236));
    screen.drawFastHLine(0, 239, 320, rgb(160, 252, 236));
    screen.drawFastVLine(0, 1, 240, rgb(160, 252, 236));
    screen.drawFastVLine(319, 1, 240, rgb(160, 252, 236));
    
    // Мяч
    float currentSpeed = sqrt(pongGame.ballSpeedX * pongGame.ballSpeedX + pongGame.ballSpeedY * pongGame.ballSpeedY);
    uint16_t ballColor = (currentSpeed > 6.0f) ? currentColorBallSpeed : currentColorBall;
    screen.fillSmoothCircle((int)pongGame.ballX, (int)pongGame.ballY, (int)BALL_RADIUS, ballColor);
    
    // Подсказка для выхода и режим игры
    screen.setTextColor(TFT_WHITE, rgb(0, 80, 47));
    screen.setTextSize(1);
    screen.setCursor(10, 220);
    
    if (pongSettings.gameMode == MODE_VS_BOT) {
      // Отображаем сложность бота
      String difficultyText;
      switch(pongSettings.botDifficulty) {
        case DIFFICULTY_EASY: difficultyText = "Easy"; break;
        case DIFFICULTY_NORMAL: difficultyText = "Normal"; break;
        case DIFFICULTY_HARD: difficultyText = "HARD"; break;
      }
      screen.print("VS Bot [" + difficultyText + "] | B: Menu");
    } else {
      screen.print("2 Players | B: Menu");
    }
    
  }
  
  else if (pongState == PONG_MENU) {
    // ГЛАВНОЕ МЕНЮ PONG
    screen.fillSprite(rgb(0, 122, 71));

    screen.setTextColor(rgb(160, 252, 236), rgb(0, 122, 71));
    screen.setTextSize(3);
    screen.setCursor(100, 30);
    screen.print("PONG");
    
    screen.drawFastHLine(80, 60, 160, rgb(160, 252, 236));
    
    // Пункты меню
    int yPos = 90;
    int xPos = 80;
    int spacing = 35;
    
    for(int i = 0; i < pongMenuItemCount; i++) {
      screen.setTextSize(2);
      
      int xOffset = (i == pongMenuSelection) ? (int)(3 * pongMenuHighlightProgress[i]) : 0;
      
      uint16_t textColor;
      if (i == pongMenuSelection) {
        if (pongMenuHighlightProgress[i] >= 0.99f) {
          textColor = rgb(4, 194, 14);
        } else {
          uint8_t r = 8 + (4 - 8) * pongMenuHighlightProgress[i];
          uint8_t g = 83 + (194 - 83) * pongMenuHighlightProgress[i];
          uint8_t b = 12 + (14 - 12) * pongMenuHighlightProgress[i];
          textColor = rgb(r, g, b);
        }
      } else {
        float glow = pongMenuGlowProgress[i];
        uint8_t r = 8 + (6 - 8) * glow;
        uint8_t g = 83 + (138 - 83) * glow;
        uint8_t b = 12 + (13 - 12) * glow;
        textColor = rgb(r, g, b);
      }
      
      screen.setTextColor(textColor, rgb(0, 122, 71));
      screen.drawString(pongMenuItems[i], xPos + xOffset, yPos + i * spacing);
    }
    
    // Индикатор управления
    screen.setTextColor(rgb(160, 252, 236), rgb(0, 122, 71));
    screen.setTextSize(1);
    screen.setCursor(10, 220);
    screen.print("Joy: Up/Down  B: Select");
  }
  
  else if (pongState == PONG_SETTINGS) {

    screen.fillSprite(rgb(0, 122, 71));
    // ЭКРАН НАСТРОЕК
    screen.setTextColor(rgb(160, 252, 236), rgb(0, 122, 71));
    screen.setTextSize(2);
    screen.setCursor(90, 20);
    screen.print("SETTINGS");
    
    screen.drawFastHLine(80, 40, 160, rgb(160, 252, 236));
    
    // Обновляем тексты настроек
    pongSettingsItems[0] = pongSettings.getGameModeText();
    pongSettingsItems[1] = pongSettings.getBotDifficultyText();
    pongSettingsItems[2] = pongSettings.getBallSpeedText();
    pongSettingsItems[3] = pongSettings.getPaddleSizeText();
    
    // Пункты настроек
    int yPos = 55;
    int xPos = 35;
    int spacing = 35;
    
    for(int i = 0; i < pongSettingsItemCount; i++) {
      screen.setTextSize(2);
      
      int xOffset = (i == pongSettingsSelection) ? (int)(3 * pongSettingsHighlightProgress[i]) : 0;
      
      uint16_t textColor;
      if (i == pongSettingsSelection) {
        if (pongSettingsHighlightProgress[i] >= 0.99f) {
          textColor = rgb(4, 194, 14);
        } else {
          uint8_t r = 8 + (4 - 8) * pongSettingsHighlightProgress[i];
          uint8_t g = 83 + (194 - 83) * pongSettingsHighlightProgress[i];
          uint8_t b = 12 + (14 - 12) * pongSettingsHighlightProgress[i];
          textColor = rgb(r, g, b);
        }
      } else {
        float glow = pongSettingsGlowProgress[i];
        uint8_t r = 8 + (6 - 8) * glow;
        uint8_t g = 83 + (138 - 83) * glow;
        uint8_t b = 12 + (13 - 12) * glow;
        textColor = rgb(r, g, b);
      }
      
      screen.setTextColor(textColor, rgb(0, 122, 71));
      screen.drawString(pongSettingsItems[i], xPos + xOffset, yPos + i * spacing);
    }
    
    // Добавляем версию в углу
    screen.setTextColor(rgb(160, 252, 236), rgb(0, 122, 71));
    screen.setTextSize(1);
    screen.setCursor(5, 230);
    screen.print("PONG: Enhanced Edition v2.14");

  }
  
  // Вывод FPS
  if (FPSrend) {
    fps.drawToSprite(&screen, 220, 0);
  }
  
  // Отрисовка уведомлений о достижениях
  achievements_render();
  
  screen.pushSprite(0, 0);
}