#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "shared.h"

// Типы достижений
enum AchievementType {
    ACH_PONG_FIRST_GAME,     // Первая игра в Pong
    ACH_PONG_FIRST_WIN,      // Первая победа в Pong
    ACH_PONG_MASTER,         // Набрать 10 очков в Pong
    ACH_PONG_PERFECT,        // Выиграть со счетом 10-0
    
    ACH_PACMAN_FIRST_GAME,   // Первая игра в Pacman
    ACH_PACMAN_FIRST_DOT,    // Съесть первую точку
    ACH_PACMAN_GHOST_EATER,  // Съесть привидение
    ACH_PACMAN_CLEAR_LEVEL,  // Пройти уровень
    ACH_PACMAN_NO_DEATHS,    // Пройти уровень без потерь жизни
    
    ACH_COLLECTOR_BRONZE,    // Получить 5 достижений
    ACH_COLLECTOR_SILVER,    // Получить 10 достижений
    ACH_COLLECTOR_GOLD,      // Получить все достижения
    
    ACH_TOTAL_COUNT          // Общее количество достижений
};

// Типы уведомлений
enum NotificationType {
    NOTIFICATION_ACHIEVEMENT,  // Достижение
    NOTIFICATION_CUSTOM        // Пользовательское уведомление
};

// Состояние достижения
struct Achievement {
    AchievementType id;
    const char* name;
    const char* description;
    bool unlocked;
    unsigned long unlockTime; 
    uint16_t iconColor;
};

// Универсальная структура уведомления
struct Notification {
    NotificationType type;
    
    // Для достижений
    AchievementType achievementId;
    
    // Для пользовательских уведомлений
    char customTitle[32];
    char customMessage[64];
    uint16_t customColor;
    
    unsigned long startTime;
};

// Состояние уведомления для рендера
struct NotificationRenderState {
    bool active;
    Notification notification;
    unsigned long startTime;
    float positionY;
};

// Глобальные переменные
extern Achievement achievements[ACH_TOTAL_COUNT];
extern NotificationRenderState currentNotification;

// Основные функции (НОВЫЕ)
void notifications_init();
void notifications_reset();
void notifications_update();
void notifications_render();

// Функции для обратной совместимости (СТАРЫЕ ИМЕНА)
void achievements_init();
void achievements_reset();
void achievements_update();
void achievements_render();

// Функции для достижений
void unlock_achievement(AchievementType id);
void check_pong_achievements(int score1, int score2, bool isFirstGame);
void check_pacman_achievements(int score, bool isFirstGame, bool ateGhost, bool clearedLevel, bool noDeaths);

// Универсальная функция для показа уведомлений
void show_notification(const char* title, const char* message, uint16_t color = TFT_WHITE);
void show_notification(const String& title, const String& message, uint16_t color = TFT_WHITE);

// Функции для экрана достижений
void achievements_screen_update();
void achievements_screen_render();

#endif