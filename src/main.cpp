#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include "display.h"
#include "audio_simple.h"
#include "input.h"
#include "menu.h"
#include "pong.h"
#include "pacman.h"
#include "achievements.h"
#include <Preferences.h>
#include "shared.h"
#include "fps.h"
#include "doom.h"
#include "seabattle.h"
#include <SPI.h>
#include "lost_trail.h"
#include "sprite_loader.h"

Preferences preferences;
RawSprite menuSprite;
TFT_eSPI tft;

// Спрайты
TFT_eSprite menuBgSprite = TFT_eSprite(&tft);
TFT_eSprite menuItemSprite = TFT_eSprite(&tft);
TFT_eSprite menuHighlightSprite = TFT_eSprite(&tft);
TFT_eSprite screen = TFT_eSprite(&tft);
TFT_eSprite test = TFT_eSprite(&tft);
TFT_eSprite snowSprite = TFT_eSprite(&tft);

void renderTask(void *pvParameters);
void logicTask(void *pvParameters);

// Простой FPS
TFT_eSprite fpsSprite(&tft);
unsigned long fpsLastTime = 0;
int fpsCounter = 0;
int currentFPS = 0; 
bool FPSrend = true;
static bool fpsInited = false;

// Флаги для аудио (для отображения статуса)
bool audioStatus = false;
char currentTrack[32] = "";

void runIntro()
{
    Serial.println("Starting intro...");

    intro_init(&tft);

    tft.setRotation(3);
    tft.invertDisplay(false);
    tft.fillScreen(TFT_BLACK);

    unsigned long startTime = millis();
    
    audio_play("/intro.mp3");
    while (introActive)
    {
        unsigned long currentTime = millis();

        introActive = intro_update(&tft, &screen);
        intro_render(&screen);
        screen.pushSprite(0, 0);

        delay(16);
    }

    Serial.println("Intro completed!");
}

void updateFPS()
{
    fpsCounter++;
    unsigned long now = millis();
    if (now - fpsLastTime >= 1000)
    {
        currentFPS = fpsCounter;
        fpsCounter = 0;
        fpsLastTime = now;
    }
}

void drawFPS()
{
    if (!fpsInited)
    {
        fpsSprite.createSprite(60, 20);
        fpsSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        fpsSprite.setTextSize(1);
        fpsInited = true;
    }

    fpsSprite.fillSprite(TFT_BLACK);
    fpsSprite.setCursor(2, 2);
    fpsSprite.print("FPS:");
    fpsSprite.print(currentFPS);

    fpsSprite.pushSprite(tft.width() - 62, 2);
}

void testJoystick()
{
    screen.fillSprite(TFT_BLACK);

    if (joy1.button == PRESSED)
    {
        screen.fillSmoothCircle(joy1.x, joy1.y, 20, TFT_GREEN);
    }
    else
    {
        screen.fillSmoothCircle(joy1.x, joy1.y, 20, TFT_RED);
    }

    if (joy2.button == PRESSED)
    {
        screen.fillSmoothCircle(joy2.x, joy2.y, 20, TFT_GREEN);
    }
    else
    {
        screen.fillSmoothCircle(joy2.x, joy2.y, 20, TFT_RED);
    }

    screen.drawString("X: " + String(joy1.x) + " Y: " + String(joy1.y), 10, 10);
    screen.drawString("X: " + String(joy2.x) + " Y: " + String(joy2.y), 10, 30);
    screen.pushSprite(0, 0);
}

bool loadTask1() { /*audio_play("/Blood_type.mp3"); */return true; }
bool loadTask2()
{
    delay(150);
    Serial.println("Task 2");
    return true;
}
bool loadTask3()
{
    delay(200);
    Serial.println("Task 3");
    return true;
}
bool loadTask4()
{
    delay(100);
    Serial.println("Task 4");
    return true;
}
bool loadTask5()
{
    delay(250);
    Serial.println("Task 5");
    return true;
}
bool loadTask6()
{
    delay(150);
    Serial.println("Task 6");
    return true;
}
bool loadTask7()
{
    delay(100);
    Serial.println("Task 7");
    return true;
}
bool loadTask8()
{
    delay(200);
    Serial.println("Task 8");
    return true;
}
bool loadTask9()
{
    delay(150);
    Serial.println("Task 9");
    return true;
}
bool loadTask10()
{
    delay(100);
    Serial.println("Task 10");
    return true;
}

void setup()
{
    Serial.begin(115200);

    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_A, 0);

    FPSrend = true;

    // Инициализация компонентов
    input_init();
    menu_init(&tft);
    achievements_init();

    
    audio_init();
    tft.init();
    disableCore0WDT();

    input_update();

    add_loading_task(loadTask1);
    add_loading_task(loadTask2);
    add_loading_task(loadTask3);
    add_loading_task(loadTask4);
    add_loading_task(loadTask5);
    add_loading_task(loadTask6);
    add_loading_task(loadTask7);
    add_loading_task(loadTask8);
    add_loading_task(loadTask9);
    add_loading_task(loadTask10);

    // Запускаем загрузку на ядре 1
    start_background_loading();

    // Интро идет 5 секунд, прогресс-бар заполняется от задач
    runIntro();

    // Создание задач
    xTaskCreatePinnedToCore(
        renderTask,
        "RenderTask",
        32768,
        NULL,
        2,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        logicTask,
        "LogicTask",
        32768,
        NULL,
        1,
        NULL,
        1);
    disableCore0WDT();
    disableCore1WDT();
    analogReadResolution(12);
    gameState = MENU;
}

void loop()
{
    vTaskDelete(NULL);
}

void renderTask(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    tft.setRotation(3);
    tft.invertDisplay(false);

    tft.fillScreen(TFT_BLACK);

    while (true)
    {
        fps.beginFrame();

        switch (gameState)
        {
        case MENU:
            menu_render(&tft);
            break;

        case PONG:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            pong_render(&tft);
            break;

        case PACMAN:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            pacman_render(&tft);
            break;

        case ACHIEVEMENTS_SCREEN:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            achievements_screen_render();
            break;

        case DOOM:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            doom_render(&tft);
            break;

        case SEABATTLE:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            seabattle_render(&tft);
            break;
        case SETTINGS:
            menu_render(&tft);
            break;
        case LOST_TRAIL:
            if (gameChanged)
            {
                tft.fillScreen(TFT_BLACK);
                screen.fillSprite(TFT_BLACK);
                screen.pushSprite(0, 0);
                gameChanged = false;
            }
            lost_trail_render(&tft);
            break;
        }

        // Уведомления теперь рендерятся ВСЕГДА, независимо от gameState
        achievements_render();
        
        fps.endFrame();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void logicTask(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    static bool needMenuInit = false;

    while (true)
    {
        input_update();
        achievements_update();
        updateAnimations();
        updateColorAnimations();
        updateSpinnerColor();
        updateSpinners();  

        if (needMenuInit && gameState == MENU)
        {
            menu_init(&tft);
            needMenuInit = false;
        }

        switch (gameState)
        {
        case MENU:
            menu_update();
            break;

        case PONG:
            pong_update();
            break;

        case PACMAN:
            pacman_update();
            break;

        case ACHIEVEMENTS_SCREEN:
            achievements_screen_update();
            break;

        case DOOM:
            doom_update();
            break;

        case SEABATTLE:
            seabattle_update();
            break;
        case SETTINGS:
            menu_update();
            break;
        case LOST_TRAIL:
            lost_trail_update();
            break;
        }

        if (gameState != lastGameState)
        {
            if (lastGameState == MENU)
            {
                if (menuBgSprite.created())
                    menuBgSprite.deleteSprite();
                if (menuItemSprite.created())
                    menuItemSprite.deleteSprite();
                if (menuHighlightSprite.created())
                    menuHighlightSprite.deleteSprite();
            }

            if (gameState == MENU)
            {
                needMenuInit = true;
            }

            lastGameState = gameState;
        }

        if (gameChanged)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}