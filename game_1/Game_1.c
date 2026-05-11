// ================================================
// Game 1 - BLOCK DEFENSE - 240x240 Fullscreen Tetris Defense
// Version: Fullscreen XY + Tetris Collision + Bullets + Flying Attackers + Coin Popup UI
// ================================================

#include "main.h"
#include "Menu.h"
#include "InputHandler.h"
#include "Joystick.h"
#include "LCD.h"
#include "Buzzer.h"
#include <stdio.h>
#include <stdlib.h>

extern ST7789V2_cfg_t   cfg0;
extern InputState       current_input;
extern ADC_HandleTypeDef hadc1;
extern Buzzer_cfg_t     buzzer_cfg;

#define LCD_W           240
#define LCD_H           240
#define UI_TOP_H        28
#define GAME_TOP        UI_TOP_H
#define GAME_BOTTOM     239
#define GAME_HEIGHT     (GAME_BOTTOM - GAME_TOP + 1)

#define BLOCK_SIZE      10
#define WALL_WIDTH      12
#define GAP_WIDTH       48
#define WALL_X1         78
#define GAP_START_X     (WALL_X1 + WALL_WIDTH)
#define GAP_END_X       (GAP_START_X + GAP_WIDTH)
#define WALL_X2         GAP_END_X
#define WALL_Y          GAME_TOP
#define WALL_HEIGHT     GAME_HEIGHT

#define MAX_ATTACKERS   8
#define MAX_BLOCKS      20
#define MAX_BULLETS     12
#define MAX_BLOOD       16
#define MAX_COIN_POPUPS 12

#define NORMAL_SPEED    16
#define FAST_SPEED      4
#define LONG_PRESS_TIME 1500
#define BLOCK_FALL_TICK             3
#define DEFENDER_ATTACK_INTERVAL    22
#define DEFENDER_ATTACK_RANGE       130
#define BULLET_SPEED                6
#define FIRST_SPAWN_DELAY           150
#define SPAWN_INTERVAL              35

#define COLOR_WALL      1
#define COLOR_ATTACKER  2
#define COLOR_BLOCK     3
#define COLOR_CURSOR    6
#define COLOR_COIN      10
#define COLOR_BULLET    6
#define COLOR_BLOOD     2
#define COLOR_DARK_BROWN 7
#define COLOR_FADE_COIN  1
#define BEEP_GAMEOVER   500

typedef enum { GAME_MENU, GAME_RUNNING, GAME_GAMEOVER } GameInnerState;
typedef enum { SHAPE_SQUARE, SHAPE_LINE, SHAPE_T, SHAPE_L, SHAPE_Z } TetrisShape;
typedef enum { ATTACKER_GROUND, ATTACKER_FLYING } AttackerType;

typedef struct { int x, y, active, falling, targetY; TetrisShape shape; int rotation; } Block;
typedef struct { int x, y, health, side, active, reward; AttackerType type; int moveCounter, animFrame; } Attacker;
typedef struct { int x, y, attackPower, active, attackCooldown, parentBlock; } DefenseUnit;
typedef struct { int x, y, active, targetIdx, damage; } Bullet;
typedef struct { int x, y, vy, life, active; } BloodDrop;
typedef struct { int x, y, value, life, active; } CoinPopup;

static GameInnerState innerState = GAME_MENU;
static int8_t menuSelection = 0;
static Direction last_direction = CENTRE;
static int exitgame = 0;
static int wallHealth[2] = {100, 100};
static int coins = 0, score = 0, totalAttackPower = 0, wave = 1, enemiesThisWave = 3, enemiesKilled = 0;
static Block currentBlock, placedBlocks[MAX_BLOCKS];
static Attacker attackers[MAX_ATTACKERS];
static DefenseUnit defenders[MAX_BLOCKS];
static Bullet bullets[MAX_BULLETS];
static BloodDrop bloodDrops[MAX_BLOOD];
static CoinPopup coinPopups[MAX_COIN_POPUPS];
static int cursorX = GAP_START_X, cursorY = WALL_Y + WALL_HEIGHT / 2;
static uint32_t btn3_press_start = 0;
static int btn3_pressed_flag = 0, game_speed = NORMAL_SPEED, yellowLED_active = 0, blockFallCounter = 0, spawnCounter = 0;

static const int8_t tetrominoCells[5][4][2] = {
    {{0,0}, {1,0}, {0,1}, {1,1}},
    {{0,0}, {1,0}, {2,0}, {3,0}},
    {{0,0}, {1,0}, {2,0}, {1,1}},
    {{0,0}, {0,1}, {1,1}, {2,1}},
    {{0,0}, {1,0}, {1,1}, {2,1}}
};

static Joystick_cfg_t js1 = { .adc=&hadc1, .x_channel=ADC_CHANNEL_1, .y_channel=ADC_CHANNEL_2, .sampling_time=ADC_SAMPLETIME_47CYCLES_5, .deadzone=JOYSTICK_DEADZONE };
static Joystick_t d1;

#define LED_RED_PORT     GPIOA
#define LED_RED_PIN      GPIO_PIN_9
#define LED_YELLOW_PORT  GPIOB
#define LED_YELLOW_PIN   GPIO_PIN_6

static void Game_Menu(void); static void Game_Run(void); static void Game_Over(void);
static void SpawnAttacker(void); static void UpdateAttackers(void); static void UpdateDefenders(void); static int SelectTargetClosestToWall(void);
static void SpawnNewBlock(void); static int IsBlockInsideScreen(int x,int y,TetrisShape shape); static int IsBlockInsideWallGap(int x,int y,TetrisShape shape);
static int CanPlaceBlockAt(int x,int y,TetrisShape shape); static int FindDropY(int x,int y); static void PlaceBlock(void); static void UpdateFallingBlock(void); static void DestroyCurrentBlock(void);
static void FireBullet(int startX,int startY,int targetIdx,int damage); static void UpdateBullets(void); static void SpawnBlood(int x,int y); static void UpdateBlood(void);
static void SpawnCoinPopup(int x,int y,int value); static void UpdateCoinPopups(void); static void DrawCoinPopups(void);
static void PurchaseUpgrade(void); static void DrawGame(void); static void DrawWalls(void); static void DrawAttackers(void); static void DrawBlocks(void); static void DrawCursor(void); static void DrawUI(void); static void DrawBullets(void); static void DrawBlood(void);
static void DrawTetrisBlock(Block *b); static void DrawParachute(int x,int y); static void DrawTinySoldier(int x,int y,int faceRight);
static void DrawAttackerLeftGround(int x,int y); static void DrawAttackerRightGround(int x,int y); static void DrawAttackerLeftFlying(int x,int y,int flap); static void DrawAttackerRightFlying(int x,int y,int flap);

static void LED_Control(void) {
    if (wallHealth[0] < 30 || wallHealth[1] < 30) {
        if (!yellowLED_active) { yellowLED_active = 1; HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET); }
    } else if (yellowLED_active) { yellowLED_active = 0; HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET); }
}

MenuState Game1_Run(void) {
    Joystick_Init(&js1); Joystick_Calibrate(&js1);
    innerState = GAME_MENU; menuSelection = 0; last_direction = CENTRE; exitgame = 0; yellowLED_active = 0;
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET); HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
    LCD_Fill_Buffer(0); LCD_Refresh(&cfg0);
    while (1) {
        Input_Read(); Joystick_Read(&js1, &d1);
        switch (innerState) { case GAME_MENU: Game_Menu(); break; case GAME_RUNNING: Game_Run(); break; case GAME_GAMEOVER: Game_Over(); break; }
        if (exitgame) break;
        HAL_Delay(game_speed);
    }
    return MENU_STATE_HOME;
}

static void Game_Menu(void) {
    LCD_Fill_Buffer(0);
    LCD_printString("BLOCK DEFENSE", 35, 20, 1, 3);
    LCD_printString("1. START GAME", 45, 85, 1, 2);
    LCD_printString("2. EXIT", 45, 115, 1, 2);
    if (menuSelection == 0) LCD_printString(">>", 20, 85, 1, 2);
    if (menuSelection == 1) LCD_printString(">>", 20, 115, 1, 2);
    LCD_printString("Joystick Up/Down", 25, 190, 1, 1);
    LCD_printString("BT3 confirm", 25, 210, 1, 1);
    LCD_Refresh(&cfg0);

    Direction current_dir = d1.direction;
    if (current_dir == N && last_direction != N) { menuSelection--; if (menuSelection < 0) menuSelection = 1; }
    else if (current_dir == S && last_direction != S) { menuSelection++; if (menuSelection > 1) menuSelection = 0; }
    last_direction = current_dir;

    if (current_input.btn3_pressed) {
        HAL_Delay(200);
        if (menuSelection == 0) {
            wallHealth[0] = wallHealth[1] = 100; coins = 50; score = 0; totalAttackPower = 0; wave = 1; enemiesThisWave = 3; enemiesKilled = 0;
            cursorX = GAP_START_X; cursorY = WALL_Y + WALL_HEIGHT / 2;
            game_speed = NORMAL_SPEED; yellowLED_active = 0; blockFallCounter = 0; spawnCounter = -FIRST_SPAWN_DELAY; btn3_pressed_flag = 0;
            HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET); HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
            for (int i=0;i<MAX_BLOCKS;i++){ placedBlocks[i].active=0; placedBlocks[i].falling=0; defenders[i].active=0; defenders[i].attackCooldown=0; defenders[i].parentBlock=i; }
            for (int i=0;i<MAX_ATTACKERS;i++) attackers[i].active=0;
            for (int i=0;i<MAX_BULLETS;i++) bullets[i].active=0;
            for (int i=0;i<MAX_BLOOD;i++) bloodDrops[i].active=0;
            for (int i=0;i<MAX_COIN_POPUPS;i++) coinPopups[i].active=0;
            SpawnNewBlock(); innerState = GAME_RUNNING;
        } else exitgame = 1;
    }
}

static void SpawnNewBlock(void) {
    currentBlock.active = 1; currentBlock.falling = 0; currentBlock.shape = rand() % 5; currentBlock.rotation = 0;
    if (!IsBlockInsideScreen(cursorX, cursorY, currentBlock.shape)) { cursorX = GAP_START_X; cursorY = WALL_Y + WALL_HEIGHT / 2; }
    currentBlock.x = cursorX; currentBlock.y = GAME_TOP; currentBlock.targetY = GAME_TOP;
}

static int IsBlockInsideScreen(int x, int y, TetrisShape shape) {
    for (int c=0;c<4;c++) {
        int bx=x+tetrominoCells[shape][c][0]*BLOCK_SIZE, by=y+tetrominoCells[shape][c][1]*BLOCK_SIZE;
        if (bx < 0 || bx + BLOCK_SIZE > LCD_W || by < WALL_Y || by + BLOCK_SIZE > LCD_H) return 0;
    }
    return 1;
}

static int IsBlockInsideWallGap(int x, int y, TetrisShape shape) {
    for (int c=0;c<4;c++) {
        int bx=x+tetrominoCells[shape][c][0]*BLOCK_SIZE, by=y+tetrominoCells[shape][c][1]*BLOCK_SIZE;
        if (bx < GAP_START_X || bx + BLOCK_SIZE > GAP_END_X || by < WALL_Y || by + BLOCK_SIZE > WALL_Y + WALL_HEIGHT) return 0;
    }
    return 1;
}

static int CanPlaceBlockAt(int x, int y, TetrisShape shape) {
    if (!IsBlockInsideWallGap(x, y, shape)) return 0;
    for (int c=0;c<4;c++) {
        int bx=x+tetrominoCells[shape][c][0]*BLOCK_SIZE, by=y+tetrominoCells[shape][c][1]*BLOCK_SIZE;
        for (int i=0;i<MAX_BLOCKS;i++) if (placedBlocks[i].active) {
            for (int pc=0;pc<4;pc++) {
                int px=placedBlocks[i].x+tetrominoCells[placedBlocks[i].shape][pc][0]*BLOCK_SIZE;
                int py=placedBlocks[i].y+tetrominoCells[placedBlocks[i].shape][pc][1]*BLOCK_SIZE;
                if (px == bx && py == by) return 0;
            }
        }
    }
    return 1;
}

static int FindDropY(int x, int y) {
    int snappedY = WALL_Y + ((y - WALL_Y) / BLOCK_SIZE) * BLOCK_SIZE;
    if (snappedY < WALL_Y) snappedY = WALL_Y;
    if (snappedY > WALL_Y + WALL_HEIGHT - BLOCK_SIZE) snappedY = WALL_Y + WALL_HEIGHT - BLOCK_SIZE;
    while (snappedY >= WALL_Y) { if (CanPlaceBlockAt(x, snappedY, currentBlock.shape)) return snappedY; snappedY -= BLOCK_SIZE; }
    return -1;
}

static void DestroyCurrentBlock(void) { currentBlock.active=0; currentBlock.falling=0; if (coins > 0) coins--; SpawnNewBlock(); }

static void PlaceBlock(void) {
    if (!currentBlock.active || currentBlock.falling) return;
    if (!IsBlockInsideScreen(cursorX, cursorY, currentBlock.shape)) return;
    if (!IsBlockInsideWallGap(cursorX, cursorY, currentBlock.shape)) { DestroyCurrentBlock(); return; }
    int dropY = FindDropY(cursorX, cursorY);
    if (dropY < 0) { DestroyCurrentBlock(); return; }
    currentBlock.x = cursorX; currentBlock.y = GAME_TOP; currentBlock.targetY = dropY; currentBlock.falling = 1;
    if (!IsBlockInsideWallGap(currentBlock.x, currentBlock.y, currentBlock.shape)) DestroyCurrentBlock();
}

static void UpdateFallingBlock(void) {
    if (!currentBlock.active || !currentBlock.falling) return;
    blockFallCounter++; if (blockFallCounter < BLOCK_FALL_TICK) return; blockFallCounter = 0;
    if (!IsBlockInsideWallGap(currentBlock.x, currentBlock.y, currentBlock.shape)) { DestroyCurrentBlock(); return; }
    int nextY = currentBlock.y + BLOCK_SIZE;
    if (nextY > currentBlock.targetY || !CanPlaceBlockAt(currentBlock.x, nextY, currentBlock.shape)) {
        if (!CanPlaceBlockAt(currentBlock.x, currentBlock.y, currentBlock.shape)) { DestroyCurrentBlock(); return; }
        int stored = 0;
        for (int i=0;i<MAX_BLOCKS;i++) if (!placedBlocks[i].active) {
            placedBlocks[i] = currentBlock; placedBlocks[i].active=1; placedBlocks[i].falling=0;
            defenders[i].active=1; defenders[i].x=currentBlock.x+BLOCK_SIZE; defenders[i].y=currentBlock.y+BLOCK_SIZE/2;
            defenders[i].attackPower=10; defenders[i].attackCooldown=0; defenders[i].parentBlock=i;
            totalAttackPower += 10; stored=1; break;
        }
        if (!stored) currentBlock.active = 0;
        SpawnNewBlock(); return;
    }
    currentBlock.y = nextY;
}

static void PurchaseUpgrade(void) {
    if (coins >= 20) { coins -= 20; for (int i=0;i<MAX_BLOCKS;i++) if (defenders[i].active) { defenders[i].attackPower += 5; totalAttackPower += 5; break; } }
}

static void SpawnAttacker(void) {
    for (int i=0;i<MAX_ATTACKERS;i++) if (!attackers[i].active) {
        attackers[i].active=1; attackers[i].side=rand()%2; attackers[i].x=(attackers[i].side==0)?10:214;
        attackers[i].type = (wave <= 5) ? ATTACKER_GROUND : ((rand()%2==0) ? ATTACKER_GROUND : ATTACKER_FLYING);
        if (attackers[i].type == ATTACKER_GROUND) { attackers[i].y=LCD_H-14; attackers[i].reward=6+rand()%5; attackers[i].health=18+wave*4; }
        else { int flyArea = LCD_H - GAME_TOP - 50; if (flyArea < 40) flyArea=40; attackers[i].y=GAME_TOP+8+rand()%flyArea; if (attackers[i].y > LCD_H-50) attackers[i].y=LCD_H-50; attackers[i].reward=15; attackers[i].health=20+wave*5; }
        attackers[i].moveCounter=0; attackers[i].animFrame=0; break;
    }
}

static void UpdateAttackers(void) {
    for (int i=0;i<MAX_ATTACKERS;i++) if (attackers[i].active) {
        attackers[i].moveCounter++; attackers[i].animFrame++;
        int shouldMove = (attackers[i].type == ATTACKER_GROUND) ? (attackers[i].moveCounter % 2 == 0) : 1;
        if (!shouldMove) continue;
        int step = (attackers[i].type == ATTACKER_FLYING) ? 2 : 1;
        if (attackers[i].side == 0) { attackers[i].x += step; if (attackers[i].x >= WALL_X1 - 10) { wallHealth[0]-=10; attackers[i].active=0; LED_Control(); } }
        else { attackers[i].x -= step; if (attackers[i].x <= WALL_X2 + WALL_WIDTH - 2) { wallHealth[1]-=10; attackers[i].active=0; LED_Control(); } }
    }
}

static int SelectTargetClosestToWall(void) {
    int bestIdx=-1, bestDist=9999;
    for (int i=0;i<MAX_ATTACKERS;i++) if (attackers[i].active) {
        int dist = (attackers[i].side==0) ? (WALL_X1 - attackers[i].x) : (attackers[i].x - (WALL_X2 + WALL_WIDTH));
        if (dist < 0) dist=0;
        if (dist < bestDist) { bestDist=dist; bestIdx=i; }
    }
    return bestIdx;
}

static void FireBullet(int startX, int startY, int targetIdx, int damage) {
    if (targetIdx < 0 || targetIdx >= MAX_ATTACKERS || !attackers[targetIdx].active) return;
    for (int i=0;i<MAX_BULLETS;i++) if (!bullets[i].active) {
        bullets[i].active=1; bullets[i].x=startX; bullets[i].y=startY; bullets[i].targetIdx=targetIdx; bullets[i].damage=damage; break;
    }
}

static void UpdateDefenders(void) {
    int targetIdx = SelectTargetClosestToWall(); if (targetIdx < 0) return;
    for (int i=0;i<MAX_BLOCKS;i++) if (defenders[i].active) {
        if (defenders[i].attackCooldown > 0) { defenders[i].attackCooldown--; continue; }
        if (!attackers[targetIdx].active) continue;
        int dist = abs(attackers[targetIdx].x - defenders[i].x) + abs(attackers[targetIdx].y - defenders[i].y);
        if (dist < DEFENDER_ATTACK_RANGE) { FireBullet(defenders[i].x, defenders[i].y, targetIdx, defenders[i].attackPower); defenders[i].attackCooldown = DEFENDER_ATTACK_INTERVAL; }
    }
}

static void SpawnBlood(int x, int y) {
    for (int n=0;n<4;n++) for (int i=0;i<MAX_BLOOD;i++) if (!bloodDrops[i].active) {
        bloodDrops[i].active=1; bloodDrops[i].x=x+(rand()%9)-4; bloodDrops[i].y=y+(rand()%5); bloodDrops[i].vy=1+rand()%3; bloodDrops[i].life=24+rand()%20; break;
    }
}

static void UpdateBlood(void) {
    for (int i=0;i<MAX_BLOOD;i++) if (bloodDrops[i].active) {
        bloodDrops[i].y += bloodDrops[i].vy; bloodDrops[i].life--;
        if (bloodDrops[i].y >= LCD_H-1 || bloodDrops[i].life <= 0) bloodDrops[i].active = 0;
    }
}

static void SpawnCoinPopup(int x, int y, int value) {
    for (int i=0;i<MAX_COIN_POPUPS;i++) if (!coinPopups[i].active) {
        coinPopups[i].active=1;
        coinPopups[i].x=x;
        coinPopups[i].y=y;
        coinPopups[i].value=value;
        coinPopups[i].life=36;
        break;
    }
}

static void UpdateCoinPopups(void) {
    for (int i=0;i<MAX_COIN_POPUPS;i++) if (coinPopups[i].active) {
        if ((coinPopups[i].life % 3) == 0) coinPopups[i].y -= 1;
        coinPopups[i].life--;
        if (coinPopups[i].life <= 0) coinPopups[i].active = 0;
    }
}

static void DrawCoinPopups(void) {
    char temp[20];
    for (int i=0;i<MAX_COIN_POPUPS;i++) if (coinPopups[i].active) {
        int color = COLOR_COIN;
        if (coinPopups[i].life < 22) color = COLOR_CURSOR;
        if (coinPopups[i].life < 10) color = COLOR_FADE_COIN;
        sprintf(temp,"+%dcoins",coinPopups[i].value);
        LCD_printString(temp,coinPopups[i].x,coinPopups[i].y,color,1);
    }
}

static void UpdateBullets(void) {
    for (int i=0;i<MAX_BULLETS;i++) if (bullets[i].active) {
        int t=bullets[i].targetIdx;
        if (t<0 || t>=MAX_ATTACKERS || !attackers[t].active) { bullets[i].active=0; continue; }
        int tx=attackers[t].x, ty=attackers[t].y+6;
        if (tx > bullets[i].x) bullets[i].x += BULLET_SPEED; else if (tx < bullets[i].x) bullets[i].x -= BULLET_SPEED;
        if (ty - bullets[i].y > 3) bullets[i].y += 2; else if (bullets[i].y - ty > 3) bullets[i].y -= 2;
        if (bullets[i].x < 0 || bullets[i].x >= LCD_W || bullets[i].y < GAME_TOP || bullets[i].y >= LCD_H) { bullets[i].active=0; continue; }
        if (abs(bullets[i].x - tx) <= 7 && abs(bullets[i].y - ty) <= 7) {
            attackers[t].health -= bullets[i].damage; bullets[i].active=0;
            if (attackers[t].health <= 0) { SpawnBlood(attackers[t].x, attackers[t].y); SpawnCoinPopup(attackers[t].x, attackers[t].y, attackers[t].reward); coins += attackers[t].reward; score += attackers[t].reward; enemiesKilled++; attackers[t].active=0; }
        }
    }
}

static void Game_Run(void) {
    if (spawnCounter >= SPAWN_INTERVAL) { spawnCounter=0; if (enemiesKilled >= enemiesThisWave) { wave++; enemiesThisWave=3+wave/2; enemiesKilled=0; } if (rand()%3==0) SpawnAttacker(); }
    spawnCounter++;
    Direction current_dir = d1.direction;
    if (!currentBlock.falling) {
        if (current_dir == N && last_direction != N) cursorY -= BLOCK_SIZE;
        else if (current_dir == S && last_direction != S) cursorY += BLOCK_SIZE;
        else if (current_dir == W && last_direction != W) cursorX -= BLOCK_SIZE;
        else if (current_dir == E && last_direction != E) cursorX += BLOCK_SIZE;
        if (cursorX < 0) cursorX=0; if (cursorX > LCD_W-BLOCK_SIZE) cursorX=LCD_W-BLOCK_SIZE;
        if (cursorY < WALL_Y) cursorY=WALL_Y; if (cursorY > WALL_Y+WALL_HEIGHT-BLOCK_SIZE) cursorY=WALL_Y+WALL_HEIGHT-BLOCK_SIZE;
        while (!IsBlockInsideScreen(cursorX, cursorY, currentBlock.shape) && cursorX > 0) cursorX -= BLOCK_SIZE;
    }
    last_direction = current_dir;
    if (current_input.btn3_pressed && !btn3_pressed_flag) { btn3_press_start=HAL_GetTick(); btn3_pressed_flag=1; game_speed=FAST_SPEED; }
    if (!current_input.btn3_pressed && btn3_pressed_flag) { uint32_t press_duration=HAL_GetTick()-btn3_press_start; if (press_duration < LONG_PRESS_TIME) PlaceBlock(); btn3_pressed_flag=0; game_speed=NORMAL_SPEED; }
    if (current_input.btn2_pressed) PurchaseUpgrade();
    UpdateFallingBlock(); UpdateAttackers(); UpdateDefenders(); UpdateBullets(); UpdateBlood(); UpdateCoinPopups();
    if (wallHealth[0] <= 0 || wallHealth[1] <= 0) innerState = GAME_GAMEOVER;
    DrawGame();
}

static void Game_Over(void) {
    LCD_Fill_Buffer(0); LCD_printString("GAME OVER", 40, 60, 1, 4);
    char temp[32]; sprintf(temp,"Wave: %d",wave); LCD_printString(temp,50,110,1,2); sprintf(temp,"Coins: %d",coins); LCD_printString(temp,50,135,1,2);
    LCD_printString("Press BT3 to exit",25,200,1,1); LCD_Refresh(&cfg0);
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    buzzer_tone(&buzzer_cfg, BEEP_GAMEOVER, 60); HAL_Delay(800); buzzer_off(&buzzer_cfg); HAL_Delay(200); buzzer_tone(&buzzer_cfg, BEEP_GAMEOVER, 60); HAL_Delay(800); buzzer_off(&buzzer_cfg);
    if (current_input.btn3_pressed) { HAL_Delay(200); HAL_GPIO_WritePin(LED_RED_PORT,LED_RED_PIN,GPIO_PIN_RESET); HAL_GPIO_WritePin(LED_YELLOW_PORT,LED_YELLOW_PIN,GPIO_PIN_RESET); innerState=GAME_MENU; }
}

static void DrawGame(void) { LCD_Fill_Buffer(0); DrawWalls(); DrawAttackers(); DrawBlocks(); DrawBullets(); DrawBlood(); DrawCoinPopups(); DrawCursor(); DrawUI(); LCD_Refresh(&cfg0); }

static void DrawWalls(void) {
    char temp[20]; LCD_Draw_Rect(WALL_X1,WALL_Y,WALL_WIDTH,WALL_HEIGHT,COLOR_WALL,1); LCD_Draw_Rect(WALL_X2,WALL_Y,WALL_WIDTH,WALL_HEIGHT,COLOR_WALL,1);
    for (int y=WALL_Y+8; y<LCD_H-12; y+=24) { LCD_Draw_Rect(WALL_X1+2,y,8,10,COLOR_DARK_BROWN,0); LCD_Draw_Rect(WALL_X2+2,y,8,10,COLOR_DARK_BROWN,0); LCD_Draw_Rect(WALL_X1+5,y,1,10,COLOR_DARK_BROWN,1); LCD_Draw_Rect(WALL_X2+5,y,1,10,COLOR_DARK_BROWN,1); }
    sprintf(temp,"L:%d",wallHealth[0]); LCD_printString(temp,WALL_X1-30,6,1,1); sprintf(temp,"R:%d",wallHealth[1]); LCD_printString(temp,WALL_X2+18,6,1,1);
}

static void DrawAttackerLeftGround(int x,int y){ LCD_Draw_Rect(x+8,y+1,4,4,COLOR_ATTACKER,1); LCD_Draw_Rect(x+4,y+5,8,6,COLOR_ATTACKER,1); LCD_Draw_Rect(x+12,y+6,4,2,COLOR_ATTACKER,1); LCD_Draw_Rect(x+5,y+11,2,3,COLOR_ATTACKER,1); LCD_Draw_Rect(x+9,y+11,2,3,COLOR_ATTACKER,1); }
static void DrawAttackerRightGround(int x,int y){ LCD_Draw_Rect(x+0,y+1,4,4,COLOR_ATTACKER,1); LCD_Draw_Rect(x+0,y+5,8,6,COLOR_ATTACKER,1); LCD_Draw_Rect(x-4,y+6,4,2,COLOR_ATTACKER,1); LCD_Draw_Rect(x+1,y+11,2,3,COLOR_ATTACKER,1); LCD_Draw_Rect(x+5,y+11,2,3,COLOR_ATTACKER,1); }
static void DrawAttackerLeftFlying(int x,int y,int flap){ LCD_Draw_Rect(x+8,y+2,4,4,COLOR_ATTACKER,1); LCD_Draw_Rect(x+5,y+6,8,5,COLOR_ATTACKER,1); LCD_Draw_Rect(x+13,y+7,4,2,COLOR_ATTACKER,1); if(flap){LCD_Draw_Rect(x+2,y+3,4,2,COLOR_CURSOR,1);LCD_Draw_Rect(x+2,y+10,4,2,COLOR_CURSOR,1);}else{LCD_Draw_Rect(x+1,y+5,4,2,COLOR_CURSOR,1);LCD_Draw_Rect(x+1,y+8,4,2,COLOR_CURSOR,1);} }
static void DrawAttackerRightFlying(int x,int y,int flap){ LCD_Draw_Rect(x+0,y+2,4,4,COLOR_ATTACKER,1); LCD_Draw_Rect(x+0,y+6,8,5,COLOR_ATTACKER,1); LCD_Draw_Rect(x-4,y+7,4,2,COLOR_ATTACKER,1); if(flap){LCD_Draw_Rect(x+8,y+3,4,2,COLOR_CURSOR,1);LCD_Draw_Rect(x+8,y+10,4,2,COLOR_CURSOR,1);}else{LCD_Draw_Rect(x+9,y+5,4,2,COLOR_CURSOR,1);LCD_Draw_Rect(x+9,y+8,4,2,COLOR_CURSOR,1);} }

static void DrawAttackers(void) {
    for (int i=0;i<MAX_ATTACKERS;i++) if (attackers[i].active) {
        int flap=(attackers[i].animFrame/5)%2;
        if (attackers[i].type == ATTACKER_GROUND) { if (attackers[i].side==0) DrawAttackerLeftGround(attackers[i].x,attackers[i].y); else DrawAttackerRightGround(attackers[i].x,attackers[i].y); }
        else { if (attackers[i].side==0) DrawAttackerLeftFlying(attackers[i].x,attackers[i].y,flap); else DrawAttackerRightFlying(attackers[i].x,attackers[i].y,flap); }
    }
}

static void DrawTinySoldier(int x,int y,int faceRight){ LCD_Draw_Rect(x+3,y+1,3,3,COLOR_COIN,1); LCD_Draw_Rect(x+2,y+4,5,4,COLOR_COIN,1); if(faceRight) LCD_Draw_Rect(x+7,y+5,3,1,COLOR_COIN,1); else LCD_Draw_Rect(x-2,y+5,3,1,COLOR_COIN,1); LCD_Draw_Rect(x+2,y+8,2,2,COLOR_COIN,1); LCD_Draw_Rect(x+5,y+8,2,2,COLOR_COIN,1); }
static void DrawParachute(int x,int y){ if(y < GAME_TOP+10) return; LCD_Draw_Rect(x+8,y-10,24,3,COLOR_CURSOR,1); LCD_Draw_Rect(x+4,y-7,32,2,COLOR_CURSOR,1); LCD_Draw_Rect(x+10,y-5,1,6,COLOR_CURSOR,1); LCD_Draw_Rect(x+28,y-5,1,6,COLOR_CURSOR,1); }
static void DrawTetrisBlock(Block *b){ for(int c=0;c<4;c++){ int bx=b->x+tetrominoCells[b->shape][c][0]*BLOCK_SIZE; int by=b->y+tetrominoCells[b->shape][c][1]*BLOCK_SIZE; LCD_Draw_Rect(bx,by,BLOCK_SIZE,BLOCK_SIZE,COLOR_BLOCK,0); DrawTinySoldier(bx+1,by,1); } if(b->falling) DrawParachute(b->x,b->y); }
static void DrawBlocks(void){ for(int i=0;i<MAX_BLOCKS;i++) if(placedBlocks[i].active) DrawTetrisBlock(&placedBlocks[i]); if(currentBlock.active && currentBlock.falling) DrawTetrisBlock(&currentBlock); }
static void DrawBullets(void){ for(int i=0;i<MAX_BULLETS;i++) if(bullets[i].active) LCD_Draw_Rect(bullets[i].x,bullets[i].y,3,3,COLOR_BULLET,1); }
static void DrawBlood(void){ for(int i=0;i<MAX_BLOOD;i++) if(bloodDrops[i].active) LCD_Draw_Rect(bloodDrops[i].x,bloodDrops[i].y,2,3,COLOR_BLOOD,1); }

static void DrawCursor(void) {
    if (innerState == GAME_RUNNING && !currentBlock.falling) {
        int color = IsBlockInsideWallGap(cursorX,cursorY,currentBlock.shape) ? COLOR_CURSOR : COLOR_ATTACKER;
        for (int c=0;c<4;c++) {
            int bx=cursorX+tetrominoCells[currentBlock.shape][c][0]*BLOCK_SIZE;
            int by=cursorY+tetrominoCells[currentBlock.shape][c][1]*BLOCK_SIZE;
            if (bx >= 0 && bx < LCD_W && by >= GAME_TOP && by < LCD_H) LCD_Draw_Rect(bx-1,by-1,BLOCK_SIZE+2,BLOCK_SIZE+2,color,0);
        }
    }
}

static void DrawUI(void) {
    char temp[32]; sprintf(temp,"W:%d S:%d C:%d",wave,score,coins); LCD_printString(temp,4,4,1,1); sprintf(temp,"Pwr:%d L:%d R:%d",totalAttackPower,wallHealth[0],wallHealth[1]); LCD_printString(temp,4,16,1,1);
}
