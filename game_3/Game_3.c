#include "Game_3.h"
#include "InputHandler.h"
#include "Menu.h"
#include "LCD.h"
#include "PWM.h"
#include "Buzzer.h"
#include "Joystick.h"
#include "stm32l4xx_hal.h"
#include "rng.h"

#include <stdio.h>
#include <math.h>

extern ST7789V2_cfg_t   cfg0;
extern PWM_cfg_t        pwm_cfg;
extern Buzzer_cfg_t     buzzer_cfg;
extern Joystick_cfg_t   joystick_cfg;
extern Joystick_t       joystick_data;
extern RNG_HandleTypeDef hrng;

// ===== 屏幕 / 帧率 =====
#define SCREEN_W        240
#define SCREEN_H        240   
#define FPS             30
#define FRAME_TIME_MS   (1000 / FPS)

// ===== 3D 透视参数 =====
#define HORIZON_Y       100
#define BOTTOM_Y        220   
#define LANE_OFFSET_MAX 80
#define MAX_ITEMS       3

#define MAGNET_DURATION_FRAMES  (5 * FPS)
#define MOUNT_DURATION_FRAMES   (10 * FPS)
#define PLAYER_Y_OFFSET         20   

// ===== 游戏颜色映射 (基于原生 LCD 宏，确保 100% 正确显示) =====
#define COL_BG           LCD_COLOUR_0     // 背景: 黑色
#define COL_CAT          LCD_COLOUR_14    // 猫猫: 亮橘色
#define COL_CAT_SHIELD   LCD_COLOUR_6     // 护盾状态: 青色
#define COL_CAT_MOUNT    LCD_COLOUR_12    // 骑乘状态: 蓝色

#define COL_ITEM_COIN    LCD_COLOUR_15    // 金币: 纯白色
#define COL_ITEM_SHIELD  LCD_COLOUR_2     // 护盾道具: 绿色
#define COL_ITEM_MAG     LCD_COLOUR_5     // 磁铁道具: 洋红色

#define COL_OBS_JUMP     LCD_COLOUR_1     // 障碍(地刺-需跳): 红色
#define COL_OBS_SLIDE    LCD_COLOUR_13    // 障碍(横梁-需钻): 灰色

#define COL_TRACK        LCD_COLOUR_6     // 跑道线: 青色
#define COL_MENU_TITLE   LCD_COLOUR_14    // 菜单标题: 橘黄色
#define COL_MENU_SUB     LCD_COLOUR_6     // 菜单副标题: 青色

// ===== 游戏状态机 =====
typedef enum {
    STATE_MENU,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_GAMEOVER
} GameState;

typedef struct {
    int lane;
    float jump_y;
    float vy;
    uint8_t is_sliding;
    uint8_t has_shield;
    int magnet_timer;
    int mount_timer;
} Player3D;

typedef struct { int lane; float z; int type;  uint8_t active; } Obstacle3D;
typedef struct { int lane; float z; int type;  uint8_t active; } Item3D;

// ===== 游戏全局变量 =====
static GameState  g_state;
static Player3D   g_player;
static Obstacle3D g_obstacle;
static Item3D     g_items[MAX_ITEMS];

static uint32_t g_score;
static float    g_speed;
static float    g_speed_inc;
static int      g_difficulty;   

// 音效管理器变量
static uint32_t g_last_sound_time = 0;
static uint32_t g_sound_duration = 0;
static uint32_t g_last_jump_time = 0;
static uint32_t g_last_lane_time = 0;
static uint8_t  g_joy_y_handled = 0;

// ===== 辅助：生成随机数 =====
static uint32_t game_rand(void) {
    uint32_t v = 0;
    HAL_RNG_GenerateRandomNumber(&hrng, &v);
    return v;
}

// ===== 辅助：播放音效 (统一管理器) =====
static void play_sound(uint32_t now, uint32_t freq, uint8_t vol, uint32_t duration_ms) {
    buzzer_tone(&buzzer_cfg, freq, vol);
    g_last_sound_time = now;
    g_sound_duration = duration_ms;
}

// ===== 重置游戏 =====
static void reset_game(void) {
    g_player.lane = 0; g_player.jump_y = 0; g_player.vy = 0;
    g_player.is_sliding = 0; g_player.has_shield = 0;
    g_player.magnet_timer = 0; g_player.mount_timer = 0;

    g_obstacle.lane = 0; g_obstacle.z = 100.0f;
    g_obstacle.type = 0; g_obstacle.active = 1;

    for (int i = 0; i < MAX_ITEMS; i++) g_items[i].active = 0;
    g_score = 0;

    PWM_SetDuty(&pwm_cfg, 0); // 确保震动马达关闭

    if      (g_difficulty == 0) { g_speed = 2.0f;  g_speed_inc = 0.01f;  }
    else if (g_difficulty == 1) { g_speed = 3.0f;  g_speed_inc = 0.02f;  }
    else                        { g_speed = 4.5f;  g_speed_inc = 0.035f; }
}

// ===== 3D 物体绘制 =====
static void draw_3d_object(int lane, float z, uint16_t colour,
                           int obj_type, float jump_y) {
    if (z < 0 || z > 100) return;
    float scale = (100.0f - z) / 100.0f;
    if (scale < 0.05f) return;

    int screen_y = HORIZON_Y + (int)((BOTTOM_Y - HORIZON_Y) * scale);
    int screen_x = 120 + (int)(lane * LANE_OFFSET_MAX * scale);
    int w, h, draw_y;

    if (obj_type == 1) {        // 障碍：横梁
        w = (int)(60 * scale); h = (int)(15 * scale);
        draw_y = screen_y - h - (int)(40 * scale) + (int)(jump_y * scale) - PLAYER_Y_OFFSET;
    } else if (obj_type == 0) { // 障碍：地刺
        w = (int)(40 * scale); h = (int)(40 * scale);
        draw_y = screen_y - h + (int)(jump_y * scale) - PLAYER_Y_OFFSET;
    } else if (obj_type == 2) { // 道具：金币
        w = (int)(20 * scale); h = (int)(20 * scale);
        draw_y = screen_y - h - (int)(20 * scale) + (int)(jump_y * scale) - PLAYER_Y_OFFSET;
    } else if (obj_type == 3) { // 道具：护盾
        w = (int)(25 * scale); h = (int)(25 * scale);
        draw_y = screen_y - h - (int)(20 * scale) + (int)(jump_y * scale) - PLAYER_Y_OFFSET;
    } else if (obj_type == 4) { // 道具：磁铁
        w = (int)(10 * scale); h = (int)(10 * scale);
        draw_y = screen_y - h - (int)(25 * scale) + (int)(jump_y * scale) - PLAYER_Y_OFFSET;
    }

    LCD_Draw_Rect(screen_x - w/2, draw_y, w, h, colour, 1);
    // 高光轮廓
    LCD_Draw_Line(screen_x - w/2, draw_y, screen_x + w/2, draw_y, LCD_COLOUR_15);
}

// ===== 绘制背景 =====
static void draw_background(void) {
    LCD_Draw_Line(0, HORIZON_Y, SCREEN_W, HORIZON_Y, LCD_COLOUR_5);
    LCD_Draw_Line(120, HORIZON_Y, 120 - LANE_OFFSET_MAX - 20, BOTTOM_Y, COL_TRACK);
    LCD_Draw_Line(120, HORIZON_Y, 120 + LANE_OFFSET_MAX + 20, BOTTOM_Y, COL_TRACK);
    LCD_Draw_Line(0, HORIZON_Y, 30, HORIZON_Y-20, LCD_COLOUR_13);
    LCD_Draw_Line(30, HORIZON_Y-20, 60, HORIZON_Y, LCD_COLOUR_13);
    LCD_Draw_Rect(190, 25, 12, 12, LCD_COLOUR_3, 1);
}

// ===== Game3_Run =====
MenuState Game3_Run(void) {
    g_difficulty      = 1;   
    g_state           = STATE_MENU;
    g_joy_y_handled   = 0;
    g_last_sound_time = 0;
    g_sound_duration  = 0;
    g_last_lane_time  = 0;
    g_last_jump_time  = 0;

    reset_game();
    uint32_t last_tick = HAL_GetTick();

    while (1) {
        uint32_t now = HAL_GetTick();
        if ((now - last_tick) < FRAME_TIME_MS) continue;
        last_tick = now;

        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);
        uint8_t btn_pressed = current_input.btn3_pressed;   
        uint8_t btn2_pressed = current_input.btn2_pressed;  

        // ---- 全局音效关闭管理器 ----
        if (now - g_last_sound_time > g_sound_duration) {
            buzzer_off(&buzzer_cfg);
        }

        // 清屏: 黑色背景
        LCD_Fill_Buffer(COL_BG);

        // ==================================================
        //  STATE_MENU
        // ==================================================
        if (g_state == STATE_MENU) {
            if (btn2_pressed) {
                buzzer_off(&buzzer_cfg); PWM_SetDuty(&pwm_cfg, 0);
                return MENU_STATE_HOME;
            }

            if (fabs(joystick_data.coord_mapped.y) < 0.2f) {
                g_joy_y_handled = 0;
            } else if (!g_joy_y_handled) {
                if (joystick_data.coord_mapped.y > 0.5f && g_difficulty > 0) {
                    g_difficulty--; g_joy_y_handled = 1; 
                    play_sound(now, 1500, 50, 50); // 菜单选择音
                } else if (joystick_data.coord_mapped.y < -0.5f && g_difficulty < 2) {
                    g_difficulty++; g_joy_y_handled = 1; 
                    play_sound(now, 1500, 50, 50); // 菜单选择音
                }
            }

            LCD_Draw_Rect(15, 30, 210, 50, LCD_COLOUR_1, 0); 
            LCD_printString("TEMPLE RUN", 25, 45, COL_MENU_TITLE, 3);
            LCD_printString("- SELECT DIFF -", 30, 105, COL_MENU_SUB, 2);

            uint16_t c_easy = (g_difficulty == 0) ? LCD_COLOUR_2 : LCD_COLOUR_13;
            uint16_t c_norm = (g_difficulty == 1) ? LCD_COLOUR_3 : LCD_COLOUR_13;
            uint16_t c_hard = (g_difficulty == 2) ? LCD_COLOUR_1 : LCD_COLOUR_13;

            LCD_printString(g_difficulty == 0 ? "> EASY <" : "  EASY  ", 56, 140, c_easy, 2);
            LCD_printString(g_difficulty == 1 ? "> NORMAL <" : "  NORMAL  ", 40, 170, c_norm, 2);
            LCD_printString(g_difficulty == 2 ? "> HARD <" : "  HARD  ", 56, 200, c_hard, 2);

            if (btn_pressed) {
                reset_game(); g_state = STATE_RUNNING;
                play_sound(now, 2500, 100, 300); // 游戏开始音
            }
        }

        // ==================================================
        //  STATE_PAUSED / STATE_GAMEOVER
        // ==================================================
        else if (g_state == STATE_PAUSED || g_state == STATE_GAMEOVER) {
            draw_background();
            if (g_state == STATE_PAUSED) {
                LCD_printString("PAUSED", 60, 100, COL_MENU_TITLE, 3);
                LCD_printString("BTN3:RESUME", 35, 155, COL_MENU_SUB, 2);
                if (btn_pressed) {
                    g_state = STATE_RUNNING;
                    play_sound(now, 2000, 50, 100);
                }
            } else {
                LCD_printString("GAME OVER", 35, 70, LCD_COLOUR_1, 3);
                char buf[32]; sprintf(buf, "Score: %lu", (unsigned long)g_score);
                LCD_printString(buf, 45, 120, LCD_COLOUR_15, 2);
                if (btn_pressed && now - g_last_jump_time > 1000) {
                    g_state = STATE_MENU;
                    PWM_SetDuty(&pwm_cfg, 0); // 关闭死亡震动
                }
            }
            if (btn2_pressed) {
                buzzer_off(&buzzer_cfg); PWM_SetDuty(&pwm_cfg, 0);
                return MENU_STATE_HOME;
            }
        }

        // ==================================================
        //  STATE_RUNNING
        // ==================================================
        else if (g_state == STATE_RUNNING) {
            if (btn_pressed) {
                g_state = STATE_PAUSED;
                play_sound(now, 1000, 100, 150); // 暂停音效
            }

            // 磁铁逻辑与倒计时音效
            if (g_player.magnet_timer > 0) {
                g_player.magnet_timer--;
                // 倒计时最后 2 秒 (FPS=30, 即剩余 60 帧以内)
                if (g_player.magnet_timer <= 60) {
                    // 每 15 帧（半秒）响一声短促高音
                    if (g_player.magnet_timer % 15 == 0) {
                        play_sound(now, 4000, 60, 40); // 磁铁倒计时警告音
                    }
                }
            }
            if (g_player.mount_timer > 0) { g_player.mount_timer--; g_score++; }

            // 左右换道
            if (now - g_last_lane_time > 150) {
                if (joystick_data.coord_mapped.x < -0.5f && g_player.lane > -1) {
                    g_player.lane--; g_last_lane_time = now;
                } else if (joystick_data.coord_mapped.x > 0.5f && g_player.lane < 1) {
                    g_player.lane++; g_last_lane_time = now;
                }
            }

            // 跳跃 / 滑行
            if (joystick_data.coord_mapped.y > 0.5f && g_player.jump_y >= 0.0f && !g_player.is_sliding) {
                if (now - g_last_jump_time > 200) { 
                    g_player.vy = -12.0f; 
                    g_last_jump_time = now; 
                    play_sound(now, 1200, 50, 100); // 跳跃音效
                }
            }
            g_player.is_sliding = (joystick_data.coord_mapped.y < -0.5f && g_player.jump_y >= 0.0f);
            
            g_player.vy += 1.5f; g_player.jump_y += g_player.vy;
            if (g_player.jump_y >= 0.0f) { g_player.jump_y = 0.0f; g_player.vy = 0; }

            // 障碍物生成逻辑
            g_obstacle.z -= g_speed;
            if (g_obstacle.z < -10.0f) {
                uint32_t r = game_rand(); g_obstacle.z = 100.0f; g_score += 10; g_speed += g_speed_inc;
                g_obstacle.lane = (int)(r % 3) - 1; g_obstacle.type = (int)((r >> 4) % 2); g_obstacle.active = 1;
            }

            // 道具生成与磁铁吸附逻辑
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (g_items[i].active) {
                    g_items[i].z -= g_speed;
                    if (g_player.magnet_timer > 0 && g_items[i].type == 0 && g_items[i].z < 60.0f) g_items[i].lane = g_player.lane;
                    if (g_items[i].z < -10.0f) g_items[i].active = 0;
                } else if (game_rand() % 100 < 5) {
                    g_items[i].active = 1; g_items[i].z = 100.0f; g_items[i].lane = (int)(game_rand() % 3) - 1;
                    int t = game_rand() % 100;
                    g_items[i].type = (t < 70) ? 0 : (t < 85 ? 1 : 2);
                }
            }

            // 碰撞检测：道具
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (g_items[i].active && g_items[i].z < 5.0f && g_items[i].z > -5.0f && g_items[i].lane == g_player.lane) {
                    g_items[i].active = 0;
                    if (g_items[i].type == 0) {
                        g_score += 50;
                        play_sound(now, 1500, 50, 80); // 金币音效
                    }
                    else if (g_items[i].type == 1) {
                        g_player.has_shield = 1;
                        play_sound(now, 2000, 50, 150); // 护盾音效
                    }
                    else {
                        g_player.magnet_timer = MAGNET_DURATION_FRAMES;
                        play_sound(now, 2500, 80, 250); // 吃到磁铁音效 (高音长音)
                    }
                }
            }

            // 碰撞检测：障碍物 (护盾破裂 / 死亡)
            if (g_obstacle.active && g_obstacle.z < 5.0f && g_obstacle.z > -5.0f && g_obstacle.lane == g_player.lane) {
                if ((g_obstacle.type == 0 && g_player.jump_y > -30.0f) || (g_obstacle.type == 1 && !g_player.is_sliding)) {
                    if (g_player.has_shield) { 
                        g_player.has_shield = 0; 
                        g_obstacle.active = 0; 
                        play_sound(now, 800, 80, 300); // 护盾撞破音效 (低音碎裂感)
                    }
                    else { 
                        g_state = STATE_GAMEOVER; 
                        g_last_jump_time = now; // 借用变量记录死亡时间
                        PWM_SetDuty(&pwm_cfg, 100); // 死亡震动开启
                        play_sound(now, 200, 100, 800); // 死亡低沉长音
                    }
                }
            }

            // --- 渲染 ---
            draw_background();
            if (g_obstacle.active) draw_3d_object(g_obstacle.lane, g_obstacle.z, (g_obstacle.type == 0 ? COL_OBS_JUMP : COL_OBS_SLIDE), g_obstacle.type, 0);
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (!g_items[i].active) continue;
                uint16_t c = (g_items[i].type == 0 ? COL_ITEM_COIN : (g_items[i].type == 1 ? COL_ITEM_SHIELD : COL_ITEM_MAG));
                draw_3d_object(g_items[i].lane, g_items[i].z, c, g_items[i].type + 2, 0);
            }

            // 绘制猫猫 (包含耳朵)
            int px = 120 + g_player.lane * LANE_OFFSET_MAX - 20;
            int py = BOTTOM_Y - 40 - PLAYER_Y_OFFSET + (int)g_player.jump_y;
            uint16_t pc = g_player.has_shield ? COL_CAT_SHIELD : COL_CAT;
            
            if (g_player.is_sliding) {
                LCD_Draw_Rect(px + 5,  py + 30, 30, 10, pc, 1); 
                LCD_Draw_Rect(px + 25, py + 25, 12, 12, pc, 1); 
                LCD_Draw_Rect(px + 27, py + 20,  6,  6, pc, 1); 
            } else {
                LCD_Draw_Rect(px + 10, py + 15, 20, 25, pc, 1); 
                LCD_Draw_Rect(px + 5,  py,      30, 20, pc, 1); 
                LCD_Draw_Rect(px + 5,  py - 6,   8,  8, pc, 1); 
                LCD_Draw_Rect(px + 27, py - 6,   8,  8, pc, 1); 
            }

            // HUD UI
            char ui[32]; sprintf(ui, "Score:%lu", (unsigned long)g_score);
            LCD_printString(ui, 5, 5, LCD_COLOUR_15, 1);
            if (g_player.magnet_timer > 0) {
                sprintf(ui, "MAG:%ds", (g_player.magnet_timer / FPS) + 1);
                // 倒计时最后两秒文字变红警告
                uint16_t mag_col = (g_player.magnet_timer <= 60) ? LCD_COLOUR_1 : COL_ITEM_MAG;
                LCD_printString(ui, 5, 18, mag_col, 1);
            }
        }

        LCD_Refresh(&cfg0);
    }
}