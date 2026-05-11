#include "achievements.h"
#include "input.h"
#include "menu.h"
#include "shared.h"
#include <queue>
#include <cstring>

// Глобальные переменные
Achievement achievements[ACH_TOTAL_COUNT];

// Очередь для хранения уведомлений
static std::queue<Notification> notificationQueue;
NotificationRenderState currentNotification = {false, {NOTIFICATION_ACHIEVEMENT, ACH_PONG_FIRST_GAME, "", "", 0}, 0, 240.0f};

// Добавляем переменные для прокрутки
static int achievementsScrollOffset = 0;
static const int ACHIEVEMENTS_PER_SCREEN = 5;
static const int SCROLL_SPEED_SLOW = 200;
static const int SCROLL_SPEED_FAST = 200;
static unsigned long lastScrollTime = 0;
static bool fastScrollMode = false;

// Константы для анимации уведомлений
static const unsigned long NOTIFICATION_SHOW_TIME = 3000;
static const unsigned long NOTIFICATION_TRANSITION_TIME = 500;
static const unsigned long NOTIFICATION_BETWEEN_TIME = 300;

// Цвета в стиле меню
const uint16_t COLOR_ACH_BG = rgb(0, 40, 8);
const uint16_t COLOR_ACH_BORDER = rgb(8, 83, 12);
const uint16_t COLOR_ACH_TEXT_NORMAL = rgb(8, 83, 12);
const uint16_t COLOR_ACH_TEXT_HIGHLIGHT = rgb(4, 194, 14);
const uint16_t COLOR_ACH_ITEM_BG = rgb(8, 83, 12);
const uint16_t COLOR_ACH_ITEM_BG_DARK = rgb(0, 40, 8);

// Цвета для уведомлений
const uint16_t COLOR_NOTIFY_BG = rgb(20, 100, 20);
const uint16_t COLOR_NOTIFY_BORDER = rgb(30, 200, 30);
const uint16_t COLOR_NOTIFY_TEXT = TFT_WHITE;
const uint16_t COLOR_NOTIFY_DESC = TFT_LIGHTGREY;

// =========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===========

// Получить количество уведомлений в очереди
int get_notification_queue_size() {
    return notificationQueue.size();
}

// Показать следующее уведомление из очереди
void show_next_notification() {
    if (notificationQueue.empty()) {
        currentNotification.active = false;
        return;
    }
    
    Notification notif = notificationQueue.front();
    notificationQueue.pop();
    
    currentNotification.active = true;
    currentNotification.notification = notif;
    currentNotification.startTime = millis();
    currentNotification.positionY = 240.0f;
}

// =========== ФУНКЦИИ ИНИЦИАЛИЗАЦИИ ===========

// Инициализация системы уведомлений
void notifications_init() {
    // Инициализация всех достижений
    achievements[ACH_PONG_FIRST_GAME] = {ACH_PONG_FIRST_GAME, "First Pong", "Play your first Pong game", false, 0, TFT_CYAN};
    achievements[ACH_PONG_FIRST_WIN] = {ACH_PONG_FIRST_WIN, "Pong Champion", "Win your first Pong match", false, 0, TFT_GREEN};
    achievements[ACH_PONG_MASTER] = {ACH_PONG_MASTER, "Pong Master", "Score 10 points in Pong", false, 0, TFT_YELLOW};
    achievements[ACH_PONG_PERFECT] = {ACH_PONG_PERFECT, "Perfect Game", "Win Pong 10-0", false, 0, TFT_GOLD};
    
    achievements[ACH_PACMAN_FIRST_GAME] = {ACH_PACMAN_FIRST_GAME, "First Maze", "Play your first Pacman game", false, 0, TFT_ORANGE};
    achievements[ACH_PACMAN_FIRST_DOT] = {ACH_PACMAN_FIRST_DOT, "Hungry Pac", "Eat your first dot", false, 0, TFT_YELLOW};
    achievements[ACH_PACMAN_GHOST_EATER] = {ACH_PACMAN_GHOST_EATER, "Ghost Hunter", "Eat a frightened ghost", false, 0, TFT_BLUE};
    achievements[ACH_PACMAN_CLEAR_LEVEL] = {ACH_PACMAN_CLEAR_LEVEL, "Maze Master", "Clear a Pacman level", false, 0, TFT_GREEN};
    achievements[ACH_PACMAN_NO_DEATHS] = {ACH_PACMAN_NO_DEATHS, "Perfect Run", "Clear a level without losing lives", false, 0, TFT_PURPLE};
    
    achievements[ACH_COLLECTOR_BRONZE] = {ACH_COLLECTOR_BRONZE, "Bronze Collector", "Unlock 5 achievements", false, 0, TFT_BROWN};
    achievements[ACH_COLLECTOR_SILVER] = {ACH_COLLECTOR_SILVER, "Silver Collector", "Unlock 10 achievements", false, 0, TFT_SILVER};
    achievements[ACH_COLLECTOR_GOLD] = {ACH_COLLECTOR_GOLD, "Gold Collector", "Unlock all achievements", false, 0, TFT_GOLD};
    
    achievementsScrollOffset = 0;
    lastScrollTime = 0;
    fastScrollMode = false;
    
    // Очищаем очередь
    while (!notificationQueue.empty()) {
        notificationQueue.pop();
    }
}

// ДЛЯ ОБРАТНОЙ СОВМЕСТИМОСТИ - achievements_init
void achievements_init() {
    notifications_init();
}

// Сброс всех уведомлений
void notifications_reset() {
    for(int i = 0; i < ACH_TOTAL_COUNT; i++) {
        achievements[i].unlocked = false;
        achievements[i].unlockTime = 0;
    }
    currentNotification.active = false;
    achievementsScrollOffset = 0;
    lastScrollTime = 0;
    fastScrollMode = false;
    
    while (!notificationQueue.empty()) {
        notificationQueue.pop();
    }
}

// ДЛЯ ОБРАТНОЙ СОВМЕСТИМОСТИ - achievements_reset
void achievements_reset() {
    notifications_reset();
}

// =========== ФУНКЦИИ ПОКАЗА УВЕДОМЛЕНИЙ ===========

// Универсальная функция показа уведомления (C-style строки)
void show_notification(const char* title, const char* message, uint16_t color) {
    Notification notif;
    notif.type = NOTIFICATION_CUSTOM;
    notif.customColor = color;
    notif.startTime = millis();
    
    strncpy(notif.customTitle, title, 31);
    notif.customTitle[31] = '\0';
    
    strncpy(notif.customMessage, message, 63);
    notif.customMessage[63] = '\0';
    
    notificationQueue.push(notif);
    
    if (!currentNotification.active && !notificationQueue.empty()) {
        show_next_notification();
    }
}

// Универсальная функция показа уведомления (String)
void show_notification(const String& title, const String& message, uint16_t color) {
    show_notification(title.c_str(), message.c_str(), color);
}

// =========== ФУНКЦИИ ДОСТИЖЕНИЙ ===========

// Разблокировка достижения
void unlock_achievement(AchievementType id) {
    if(id >= ACH_TOTAL_COUNT || achievements[id].unlocked) return;
    
    achievements[id].unlocked = true;
    achievements[id].unlockTime = millis();
    
    // Создаем уведомление о достижении
    Notification notif;
    notif.type = NOTIFICATION_ACHIEVEMENT;
    notif.achievementId = id;
    notif.startTime = millis();
    
    notificationQueue.push(notif);
    
    if (!currentNotification.active && !notificationQueue.empty()) {
        show_next_notification();
    }
    
    // Проверяем коллекционные достижения
    int unlockedCount = 0;
    for(int i = 0; i < ACH_TOTAL_COUNT; i++) {
        if(achievements[i].unlocked) unlockedCount++;
    }
    
    if(unlockedCount >= 5 && !achievements[ACH_COLLECTOR_BRONZE].unlocked) {
        unlock_achievement(ACH_COLLECTOR_BRONZE);
    }
    if(unlockedCount >= 10 && !achievements[ACH_COLLECTOR_SILVER].unlocked) {
        unlock_achievement(ACH_COLLECTOR_SILVER);
    }
    if(unlockedCount >= ACH_TOTAL_COUNT - 1 && !achievements[ACH_COLLECTOR_GOLD].unlocked) {
        unlock_achievement(ACH_COLLECTOR_GOLD);
    }
}

// Проверка достижений для Pong
void check_pong_achievements(int score1, int score2, bool isFirstGame) {
    static bool pongFirstGameChecked = false;
    static bool pongFirstWinChecked = false;
    static bool pongMasterChecked = false;
    static bool pongPerfectChecked = false;
    
    if(isFirstGame && !pongFirstGameChecked) {
        unlock_achievement(ACH_PONG_FIRST_GAME);
        pongFirstGameChecked = true;
    }
    
    if(score1 > score2 && !pongFirstWinChecked) {
        unlock_achievement(ACH_PONG_FIRST_WIN);
        pongFirstWinChecked = true;
    }
    
    if((score1 >= 10 || score2 >= 10) && !pongMasterChecked) {
        unlock_achievement(ACH_PONG_MASTER);
        pongMasterChecked = true;
    }
    
    if((score1 == 10 && score2 == 0) || (score2 == 10 && score1 == 0)) {
        if(!pongPerfectChecked) {
            unlock_achievement(ACH_PONG_PERFECT);
            pongPerfectChecked = true;
        }
    }
}

// Проверка достижений для Pacman
void check_pacman_achievements(int score, bool isFirstGame, bool ateGhost, bool clearedLevel, bool noDeaths) {
    static bool pacmanFirstGameChecked = false;
    static bool pacmanFirstDotChecked = false;
    static bool pacmanGhostEaterChecked = false;
    static bool pacmanClearLevelChecked = false;
    static bool pacmanNoDeathsChecked = false;
    
    if(isFirstGame && !pacmanFirstGameChecked) {
        unlock_achievement(ACH_PACMAN_FIRST_GAME);
        pacmanFirstGameChecked = true;
    }
    
    if(score > 0 && !pacmanFirstDotChecked) {
        unlock_achievement(ACH_PACMAN_FIRST_DOT);
        pacmanFirstDotChecked = true;
    }
    
    if(ateGhost && !pacmanGhostEaterChecked) {
        unlock_achievement(ACH_PACMAN_GHOST_EATER);
        pacmanGhostEaterChecked = true;
    }
    
    if(clearedLevel && !pacmanClearLevelChecked) {
        unlock_achievement(ACH_PACMAN_CLEAR_LEVEL);
        pacmanClearLevelChecked = true;
    }
    
    if(noDeaths && clearedLevel && !pacmanNoDeathsChecked) {
        unlock_achievement(ACH_PACMAN_NO_DEATHS);
        pacmanNoDeathsChecked = true;
    }
}

// =========== ФУНКЦИИ ОБНОВЛЕНИЯ ===========

// Обновление анимации уведомлений
void notifications_update() {
    if(!currentNotification.active) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - currentNotification.startTime;
    unsigned long totalNotificationTime = NOTIFICATION_TRANSITION_TIME * 2 + NOTIFICATION_SHOW_TIME;
    
    // Анимация появления
    if(elapsed < NOTIFICATION_TRANSITION_TIME) {
        currentNotification.positionY = 240.0f - (elapsed / (float)NOTIFICATION_TRANSITION_TIME) * 50.0f;
    }
    // Показываем
    else if(elapsed < NOTIFICATION_TRANSITION_TIME + NOTIFICATION_SHOW_TIME) {
        currentNotification.positionY = 190.0f;
    }
    // Анимация исчезновения
    else if(elapsed < totalNotificationTime) {
        float fadeOut = (elapsed - (NOTIFICATION_TRANSITION_TIME + NOTIFICATION_SHOW_TIME)) / (float)NOTIFICATION_TRANSITION_TIME;
        currentNotification.positionY = 190.0f + fadeOut * 50.0f;
    }
    // Завершаем текущее уведомление и показываем следующее
    else {
        if (!notificationQueue.empty()) {
            // Добавляем небольшую задержку между уведомлениями
            unsigned long nextStartTime = currentTime + NOTIFICATION_BETWEEN_TIME;
            while (millis() < nextStartTime) {
                // Ждем короткое время
            }
            show_next_notification();
        } else {
            currentNotification.active = false;
        }
    }
}

// ДЛЯ ОБРАТНОЙ СОВМЕСТИМОСТИ - achievements_update
void achievements_update() {
    notifications_update();
}

// =========== ФУНКЦИИ ОТРИСОВКИ ===========

// Отрисовка уведомлений
void notifications_render() {
    if(!currentNotification.active) return;
    
    int yPos = (int)currentNotification.positionY;
    
    // Сохраняем текущие настройки спрайта
    uint8_t oldSize = screen.textsize;
    uint16_t oldColor = screen.textcolor;
    uint16_t oldBgColor = screen.textbgcolor;
    
    // Рисуем фон уведомления
    screen.fillSmoothRoundRect(20, yPos, 280, 50, 5, COLOR_NOTIFY_BORDER);
    screen.fillSmoothRoundRect(22, yPos + 2, 276, 46, 4, COLOR_NOTIFY_BG);
    
    if (currentNotification.notification.type == NOTIFICATION_ACHIEVEMENT) {
        // Уведомление о достижении
        Achievement& ach = achievements[currentNotification.notification.achievementId];
        
        // Иконка
        screen.fillSmoothCircle(45, yPos + 25, 15, ach.iconColor);
        screen.setTextColor(TFT_WHITE, ach.iconColor);
        screen.setTextSize(1);
        screen.setCursor(40, yPos + 18);
        screen.print("A");
        
        // Название
        screen.setTextColor(COLOR_NOTIFY_TEXT, COLOR_NOTIFY_BG);
        screen.setCursor(65, yPos + 10);
        screen.print("Achievement Unlocked!");
        
        // Достижение
        screen.setTextColor(COLOR_NOTIFY_TEXT, COLOR_NOTIFY_BG);
        screen.setCursor(65, yPos + 25);
        screen.print(ach.name);
        
        // Описание
        screen.setTextColor(COLOR_NOTIFY_DESC, COLOR_NOTIFY_BG);
        screen.setCursor(65, yPos + 38);
        screen.print(ach.description);
    } else {
        // Пользовательское уведомление
        uint16_t iconColor = currentNotification.notification.customColor;
        
        // Иконка (круг с буквой N)
        screen.fillSmoothCircle(45, yPos + 25, 15, iconColor);
        screen.setTextColor(TFT_WHITE, iconColor);
        screen.setTextSize(1);
        screen.setCursor(40, yPos + 18);
        screen.print("!");
        
        // Заголовок
        screen.setTextColor(COLOR_NOTIFY_TEXT, COLOR_NOTIFY_BG);
        screen.setCursor(65, yPos + 15);
        screen.print(currentNotification.notification.customTitle);
        
        // Сообщение
        screen.setTextColor(COLOR_NOTIFY_DESC, COLOR_NOTIFY_BG);
        screen.setCursor(65, yPos + 30);
        screen.print(currentNotification.notification.customMessage);
    }
    
    // Показываем счетчик очереди
    int queueSize = get_notification_queue_size();
    if (queueSize > 0) {
        screen.setTextColor(TFT_YELLOW, COLOR_NOTIFY_BG);
        screen.setTextSize(1);
        screen.setCursor(280 - 25, yPos + 40);
        screen.print("+" + String(queueSize));
    }
    
    // Восстанавливаем настройки
    screen.setTextSize(oldSize);
    screen.setTextColor(oldColor, oldBgColor);
}

// ДЛЯ ОБРАТНОЙ СОВМЕСТИМОСТИ - achievements_render
void achievements_render() {
    notifications_render();
}

// =========== ФУНКЦИИ ЭКРАНА ДОСТИЖЕНИЙ ===========

// Функция для рисования ползунка прокрутки
void draw_scrollbar(int totalItems, int itemsPerScreen, int scrollOffset, int x, int y, int width, int height) {
    screen.fillRoundRect(x - 1 , y, width + 2, height, 4, COLOR_ACH_ITEM_BG_DARK);
    screen.drawRoundRect(x - 1, y, width + 2, height, 4, COLOR_ACH_BORDER);
    
    if (totalItems <= itemsPerScreen) {
        return;
    }
    
    int trackHeight = height - 4;
    int thumbHeight = (itemsPerScreen * trackHeight) / totalItems;
    thumbHeight = thumbHeight < 8 ? 8 : thumbHeight;
    
    int thumbY = y + 2 + (scrollOffset * (trackHeight - thumbHeight)) / (totalItems - itemsPerScreen);
    
    uint16_t scrollColor = fastScrollMode ? COLOR_ACH_TEXT_HIGHLIGHT : COLOR_ACH_BORDER;
    screen.fillSmoothRoundRect(x + 1, thumbY, width - 3, thumbHeight, 4, scrollColor);
    screen.drawSmoothRoundRect(x + 1, thumbY, 4, 4, width - 3, thumbHeight, COLOR_ACH_TEXT_HIGHLIGHT, scrollColor);
}

// Обновление экрана достижений
void achievements_screen_update() {
    unsigned long currentTime = millis();
    
    int scrollDelay = fastScrollMode ? SCROLL_SPEED_FAST : SCROLL_SPEED_SLOW;
    
    if (currentTime - lastScrollTime < scrollDelay) {
        return;
    }
    
    bool scrolled = false;
    
    if (joy2.y > 3000) {
        if (achievementsScrollOffset > 0) {
            achievementsScrollOffset--;
            scrolled = true;
        }
    }
    else if (joy2.y < 1000) {
        if (achievementsScrollOffset < (int)ACH_TOTAL_COUNT - ACHIEVEMENTS_PER_SCREEN) {
            achievementsScrollOffset++;
            scrolled = true;
        }
    } else {
        fastScrollMode = false;
    }
    
    if (scrolled) {
        lastScrollTime = currentTime;
        CHANGES_BTN = true;
        
        static unsigned long holdStartTime = 0;
        if (holdStartTime == 0) {
            holdStartTime = currentTime;
        } else if (currentTime - holdStartTime > 1000) {
            fastScrollMode = true;
        }
    } else {
        static unsigned long holdStartTime = 0;
        holdStartTime = 0;
        fastScrollMode = false;
    }
    
    if(joy2.button != PRESSED) {
        lastBut = false;
    }
    
    if(!lastBut && joy2.button == PRESSED){
        if (gameState == ACHIEVEMENTS_SCREEN) {
            gameState = MENU;
            gameChanged = false;
            fastScrollMode = false;
            lastBut = true;
        }
    }
}

// Отрисовка экрана достижений
void achievements_screen_render() {
    screen.fillSprite(COLOR_ACH_BG);
    
    screen.drawRoundRect(1, 1, 318, 238, 8, COLOR_ACH_BORDER);
    screen.drawRoundRect(2, 2, 316, 236, 7, COLOR_ACH_BORDER);
    screen.drawFastHLine(1, 30, 318, COLOR_ACH_BORDER);
    screen.drawFastHLine(1, 31, 318, COLOR_ACH_BORDER);
    
    screen.setTextColor(COLOR_ACH_BORDER);
    screen.setTextSize(3);
    screen.setTextDatum(TL_DATUM);
    screen.drawString("ACHIEVEMENTS", 55, 7);
    
    int unlockedCount = 0;
    for(int i = 0; i < (int)ACH_TOTAL_COUNT; i++) {
        if(achievements[i].unlocked) unlockedCount++;
    }
    
    screen.setTextColor(COLOR_ACH_TEXT_NORMAL);
    screen.setTextSize(1);
    screen.drawCentreString(String(unlockedCount) + "/" + String((int)ACH_TOTAL_COUNT) + " unlocked", 160, 40, 1);
    
    if (fastScrollMode) {
        screen.setTextColor(COLOR_ACH_TEXT_HIGHLIGHT);
        screen.setTextSize(1);
        screen.drawCentreString("FAST SCROLL", 160, 55, 1);
    }
    
    int yStart = 70;
    int itemHeight = 30;
    int achievementsWidth = 260;
    
    for(int i = 0; i < ACHIEVEMENTS_PER_SCREEN; i++) {
        int index = i + achievementsScrollOffset;
        if(index >= (int)ACH_TOTAL_COUNT) break;
        
        Achievement& ach = achievements[index];
        int yPos = yStart + i * itemHeight;
        
        if(ach.unlocked) {
            screen.fillSmoothRoundRect(20, yPos, achievementsWidth, 25, 5, COLOR_ACH_BORDER);
            
            screen.fillSmoothCircle(40, yPos + 12, 10, ach.iconColor);
            screen.setTextColor(TFT_WHITE, ach.iconColor);
            screen.drawChar('A', 36, yPos + 6, 1);
            
            screen.setTextColor(COLOR_ACH_TEXT_HIGHLIGHT);
            screen.setCursor(60, yPos + 5);
            screen.print(ach.name);
            
            screen.setTextColor(rgb(4, 194, 14));
            screen.setCursor(60, yPos + 15);
            screen.print(ach.description);
            
            screen.setTextColor(COLOR_ACH_TEXT_HIGHLIGHT);
            screen.drawString("UNLOCKED", achievementsWidth - 40, yPos + 9, 1);
        } else {
            screen.fillSmoothRoundRect(20, yPos, achievementsWidth, 25, 5, COLOR_ACH_ITEM_BG_DARK);
            screen.drawSmoothRoundRect(20, yPos, 5, 5, achievementsWidth, 25, COLOR_ACH_BORDER, COLOR_ACH_ITEM_BG_DARK);
            
            screen.fillSmoothCircle(40, yPos + 12, 10, COLOR_ACH_ITEM_BG_DARK);
            screen.drawSmoothCircle(40, yPos + 12, 10, COLOR_ACH_BORDER, COLOR_ACH_ITEM_BG_DARK);
            screen.setTextColor(COLOR_ACH_BORDER);
            screen.drawChar('?', 36, yPos + 6, 1);
            
            screen.setTextColor(COLOR_ACH_BORDER);
            screen.setCursor(60, yPos + 5);
            screen.print(ach.name);
            
            screen.setTextColor(COLOR_ACH_BORDER);
            screen.setCursor(60, yPos + 15);
            screen.print(ach.description);
            
            screen.setTextColor(COLOR_ACH_BORDER);
            screen.drawString("LOCKED", achievementsWidth - 35, yPos + 9, 1);
        }
    }
    
    draw_scrollbar((int)ACH_TOTAL_COUNT, ACHIEVEMENTS_PER_SCREEN, achievementsScrollOffset, 
                   295, 70, 15, 140);
    
    screen.setTextColor(COLOR_ACH_BORDER);
    screen.drawCentreString("Use JOY2 to scroll, JOY2/BTN_A to exit", 160, 220, 1);
    
    if ((int)ACH_TOTAL_COUNT > ACHIEVEMENTS_PER_SCREEN) {
        screen.setTextColor(COLOR_ACH_TEXT_NORMAL);
        
        int endIndex = achievementsScrollOffset + ACHIEVEMENTS_PER_SCREEN;
        if (endIndex > (int)ACH_TOTAL_COUNT) {
            endIndex = (int)ACH_TOTAL_COUNT;
        }
        
        screen.drawCentreString(String(achievementsScrollOffset + 1) + "-" + 
                               String(endIndex) + 
                               " of " + String((int)ACH_TOTAL_COUNT), 160, 230, 1);
    }
    
    screen.pushSprite(0, 0);
}