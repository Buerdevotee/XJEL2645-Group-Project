/**
 * @file FishingEngine.c
 * @brief mini Stardew Fishing Simulator — FSM, fishing mechanics, rendering.
 */

#include "FishingEngine.h"
#include "FishSprites.h"
#include "LCD.h"
#include "Buzzer.h"
#include "PWM.h"
#include "Utils.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* External hardware configs (defined in main.c) */
extern Buzzer_cfg_t buzzer_cfg;
extern PWM_cfg_t    pwm_cfg;

/* ===== Screen layout ===== */
#define SCREEN_W            240
#define SCREEN_H            240

#define HUD_H               16
#define WATER_LINE_Y        80
#define UNDERWATER_BOTTOM   180

/* Palette indices */
#define COL_BLACK    0
#define COL_WHITE    1
#define COL_RED      2
#define COL_GREEN    3
#define COL_BLUE     4
#define COL_YELLOW   6
#define COL_PINK     7
#define COL_NAVY     9
#define COL_GOLD    10
#define COL_BROWN   12
#define COL_GREY    13
#define COL_SKY_BLUE 15

/* ===== Game tuning ===== */
#define CAST_DURATION_MS         800
#define BITE_REACTION_MS        1500   /* time to hook the fish */
#define RESULT_DURATION_MS      4000
#define ESCAPED_DURATION_MS     3000
#define MENU_INPUT_DELAY_MS      600   /* ignore spurious button on first entry */
#define BITE_DELAY_MIN_MS       2000
#define BITE_DELAY_MAX_MS       6000

/* ===== Fishing minigame physics ===== */
#define FISH_SPD_BASE   0.00007f  /* fish speed (track units/ms) for weakest fish */
#define FISH_SPD_SCALE  0.00013f  /* extra speed per pull_strength unit */
#define BAR_GRAVITY     0.00020f  /* green bar fall acceleration per ms */
#define BAR_LIFT        0.00055f  /* green bar rise acceleration per ms (N held) */
#define BAR_DAMP        0.96f     /* velocity damping applied each frame */
#define PROGRESS_RATE   0.003f    /* progress gain per normalised frame in zone */

/* ===== Fishing minigame screen layout ===== */
#define MINI_TX  100   /* track left edge */
#define MINI_TY   22   /* track top edge */
#define MINI_TW   32   /* track width */
#define MINI_TH  162   /* track height */

#define BUZZ_VOL 60

/* ===== Per-state private state ===== */
static uint32_t s_bite_delay_ms;
static uint8_t  s_buzzer_active;
static uint32_t s_buzzer_off_tick;
static uint8_t  s_beep2_pending;   /* 1 if a second beep is queued */
static uint32_t s_beep2_freq;
static uint32_t s_beep2_dur;
static uint32_t s_beep2_when;     /* HAL_GetTick() value to fire second beep */
static uint32_t s_w_hold_ms;        /* accumulated W-hold time for menu escape */
static uint32_t s_cursor_debounce;  /* HAL tick after which menu cursor can move */

/* Fishing minigame state */
static float    s_fish_y;           /* fish icon position 0.0=top 1.0=bottom */
static float    s_fish_target;      /* fish current target position */
static float    s_fish_spd;         /* fish move speed, track-units per ms */
static uint32_t s_fish_change_ms;   /* how often the fish picks a new target */
static uint32_t s_fish_change_timer;/* ms since last forced target change */
static float    s_bar_y;            /* green bar CENTER position */
static float    s_bar_vy;           /* green bar velocity (positive = down) */

#define BAR_HALF  0.20f             /* fixed green bar half-height (fraction of track) */

/* ===== Forward declarations ===== */
static void enter_state(FishingEngine_t *e, GameState s);
static void update_buzzer(void);
static void beep(uint32_t freq, uint32_t duration_ms);
static void beep_double(uint32_t freq1, uint32_t dur1,
                        uint32_t freq2, uint32_t dur2, uint32_t gap_ms);

static void update_menu        (FishingEngine_t *e, UserInput in, uint8_t btn);
static void update_casting     (FishingEngine_t *e);
static void update_waiting     (FishingEngine_t *e);
static void update_fish_bite   (FishingEngine_t *e, uint8_t btn);
static void update_reeling     (FishingEngine_t *e, UserInput in, uint32_t dt);
static void update_catch_result(FishingEngine_t *e);
static void update_fish_escaped(FishingEngine_t *e);
static void update_collection  (FishingEngine_t *e, uint8_t btn);
static void update_instruction (FishingEngine_t *e, uint8_t btn);

static void draw_menu        (FishingEngine_t *e);
static void draw_collection  (FishingEngine_t *e);
static void draw_instruction (void);
static void draw_scene_bg    (void);
static void draw_status_panel(uint8_t border_col);
static void draw_underwater_details(uint32_t state_timer);
static void draw_panel_frame(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t fill_col, uint8_t border_col);
static void draw_casting     (FishingEngine_t *e);
static void draw_waiting     (FishingEngine_t *e);
static void draw_fish_bite   (FishingEngine_t *e);
static void draw_reeling     (FishingEngine_t *e);
static void draw_catch_result(FishingEngine_t *e);
static void draw_fish_escaped(FishingEngine_t *e);
static void draw_hud         (FishingEngine_t *e);
static void draw_rod         (void);

/* Init */

void FishingEngine_Init(FishingEngine_t *e)
{
    memset(e, 0, sizeof(*e));
    e->state        = STATE_MENU;
    e->state_timer  = 0;
    e->menu_cursor  = 0;
    e->caught_flags   = 0;
    e->catch_progress = 0.0f;
}

/* Update */

void FishingEngine_Update(FishingEngine_t *e,
                          UserInput in,
                          uint8_t   btn,
                          uint32_t  dt)
{
    e->state_timer += dt;

    /* Hold joystick W for 2 s from any game state to escape to menu */
    if (e->state != STATE_MENU && e->state != STATE_COLLECTION) {
        if (in.direction == W) {
            s_w_hold_ms += dt;
            if (s_w_hold_ms >= 2000) {
                s_w_hold_ms = 0;
                enter_state(e, STATE_MENU);
                update_buzzer();
                return;
            }
        } else {
            s_w_hold_ms = 0;
        }
    }

    switch (e->state) {
        case STATE_MENU:         update_menu        (e, in, btn); break;
        case STATE_CASTING:      update_casting     (e);          break;
        case STATE_WAITING:      update_waiting     (e);          break;
        case STATE_FISH_BITE:    update_fish_bite   (e, btn);     break;
        case STATE_REELING:      update_reeling     (e, in, dt);  break;
        case STATE_CATCH_RESULT: update_catch_result(e);          break;
        case STATE_FISH_ESCAPED: update_fish_escaped(e);          break;
        case STATE_COLLECTION:   update_collection  (e, btn);     break;
        case STATE_INSTRUCTION:  update_instruction (e, btn);     break;
    }

    update_buzzer();
}

/* ---------- State entry helper ---------- */

static void enter_state(FishingEngine_t *e, GameState s)
{
    e->state = s;
    e->state_timer = 0;

    PWM_SetDuty(&pwm_cfg, 0);

    switch (s) {
        case STATE_MENU:
            buzzer_off(&buzzer_cfg);
            s_buzzer_active  = 0;
            s_beep2_pending  = 0;
            e->menu_cursor   = 0;
            break;

        case STATE_WAITING:
            s_bite_delay_ms = BITE_DELAY_MIN_MS +
                              Random_U16(BITE_DELAY_MAX_MS - BITE_DELAY_MIN_MS + 1);
            break;

        case STATE_FISH_BITE:
            beep_double(880, 100, 880, 100, 50);
            break;

        case STATE_REELING:
            e->current_fish   = Fish_GenerateRandom();
            e->catch_progress = 0.10f;
            s_fish_y            = 0.5f;
            s_fish_target       = 0.05f + (float)Random_U16(91) / 100.0f;
            s_fish_spd          = FISH_SPD_BASE +
                                  e->current_fish.type.pull_strength * FISH_SPD_SCALE;
            s_fish_change_ms    = (uint32_t)(4800.0f -
                                  e->current_fish.type.pull_strength * 3600.0f);
            s_fish_change_timer = 0;
            s_bar_y             = 0.5f;
            s_bar_vy            = 0.0f;
            break;

        case STATE_CATCH_RESULT:
            e->caught_flags |= (1u << e->current_fish.type.species_idx);
            beep(NOTE_C5, 100);
            break;

        case STATE_FISH_ESCAPED:
            beep_double(NOTE_A4, 100, NOTE_E4, 100, 70);
            break;

        default:
            break;
    }
}

/* ---------- Per-state update ---------- */

static void update_menu(FishingEngine_t *e, UserInput in, uint8_t btn)
{
    uint32_t now = HAL_GetTick();
    if (now >= s_cursor_debounce) {
        if (in.direction == N || in.direction == NE || in.direction == NW) {
            if (e->menu_cursor > 0) { e->menu_cursor--; s_cursor_debounce = now + 300; }
        } else if (in.direction == S || in.direction == SE || in.direction == SW) {
            if (e->menu_cursor < 2) { e->menu_cursor++; s_cursor_debounce = now + 300; }
        }
    }

    if (btn && e->state_timer >= MENU_INPUT_DELAY_MS) {
        if (e->menu_cursor == 0) {
            e->score = 0; e->fish_caught = 0;
            e->fish_escaped = 0; e->lines_snapped = 0;
            enter_state(e, STATE_CASTING);
        } else if (e->menu_cursor == 1) {
            enter_state(e, STATE_COLLECTION);
        } else {
            enter_state(e, STATE_INSTRUCTION);
        }
    }
}

static void update_casting(FishingEngine_t *e)
{
    if (e->state_timer >= CAST_DURATION_MS) {
        enter_state(e, STATE_WAITING);
    }
}

static void update_waiting(FishingEngine_t *e)
{
    if (e->state_timer >= s_bite_delay_ms) {
        enter_state(e, STATE_FISH_BITE);
    }
}

static void update_fish_bite(FishingEngine_t *e, uint8_t btn)
{
    PWM_SetDuty(&pwm_cfg, ((e->state_timer / 200) & 1u) ? 100 : 0);

    if (btn) {
        enter_state(e, STATE_REELING);
        return;
    }
    if (e->state_timer >= BITE_REACTION_MS) {
        e->fish_escaped++;
        enter_state(e, STATE_FISH_ESCAPED);
    }
}

static void update_reeling(FishingEngine_t *e, UserInput in, uint32_t dt)
{
    float pull = e->current_fish.type.pull_strength;
    float dt_f = (float)dt;

    /* ── Fish icon: dart toward target ── */
    float step = s_fish_spd * dt_f;
    float diff = s_fish_target - s_fish_y;
    if (diff > step)        s_fish_y += step;
    else if (diff < -step)  s_fish_y -= step;
    else                    s_fish_y = s_fish_target;
    if (s_fish_y < 0.0f) s_fish_y = 0.0f;
    if (s_fish_y > 1.0f) s_fish_y = 1.0f;

    /* Pick a new target when arrived OR when restlessness timer fires */
    s_fish_change_timer += dt;
    if (s_fish_y == s_fish_target || s_fish_change_timer >= s_fish_change_ms) {
        s_fish_target       = 0.05f + (float)Random_U16(91) / 100.0f;
        s_fish_change_timer = 0;
    }

    /* ── Green bar physics ── */
    /* Gravity pulls bar down; N lifts it up */
    s_bar_vy += BAR_GRAVITY * dt_f;
    if (in.direction == N || in.direction == NE || in.direction == NW)
        s_bar_vy -= BAR_LIFT * dt_f * in.magnitude;
    s_bar_vy *= BAR_DAMP;
    s_bar_y  += s_bar_vy;
    /* Clamp so bar stays fully inside the track */
    if (s_bar_y < BAR_HALF) { s_bar_y = BAR_HALF; s_bar_vy = 0.0f; }
    if (s_bar_y > 1.0f - BAR_HALF) { s_bar_y = 1.0f - BAR_HALF; s_bar_vy = 0.0f; }

    /* Progress: fish inside bar → up, outside → slow decay */
    float frame   = dt_f / 16.6f;
    float overlap = BAR_HALF - fabsf(s_fish_y - s_bar_y);
    if (overlap > 0.0f) {
        float gain = PROGRESS_RATE * frame * (overlap / BAR_HALF);
        gain /= (0.5f + pull);
        e->catch_progress += gain;
        if (e->catch_progress > 1.0f) e->catch_progress = 1.0f;
    } else {
        e->catch_progress -= (PROGRESS_RATE * 0.5f) * frame;
        if (e->catch_progress < 0.0f) e->catch_progress = 0.0f;
    }

/* Fail: progress fully drained (1 s grace at start) */
    if (e->catch_progress <= 0.0f && e->state_timer >= 1000) {
        e->fish_escaped++;
        enter_state(e, STATE_FISH_ESCAPED);
        return;
    }

    /*  Win check */
    if (e->catch_progress >= 1.0f) {
        e->fish_caught++;
        e->score += e->current_fish.type.points;
        enter_state(e, STATE_CATCH_RESULT);
    }
}

static void update_catch_result(FishingEngine_t *e)
{
    /* Three-note jingle: C5 -> E5 -> G5 spaced 180ms apart */
    if (e->state_timer >= 180 && e->state_timer < 180 + 16) beep(NOTE_E5, 100);
    if (e->state_timer >= 360 && e->state_timer < 360 + 16) beep(NOTE_G5, 150);

    if (e->state_timer >= RESULT_DURATION_MS) {
        /* Skip re-cast; go straight to waiting for the next bite */
        enter_state(e, STATE_WAITING);
    }
}

static void update_fish_escaped(FishingEngine_t *e)
{
    if (e->state_timer >= ESCAPED_DURATION_MS) {
        enter_state(e, STATE_WAITING);
    }
}

static void update_collection(FishingEngine_t *e, uint8_t btn)
{
    (void)e;
    if (btn) {
        enter_state(e, STATE_MENU);
    }
}

static void update_instruction(FishingEngine_t *e, uint8_t btn)
{
    (void)e;
    if (btn) {
        enter_state(e, STATE_MENU);
    }
}

/* ---------- Buzzer non-blocking helpers ---------- */

static void beep(uint32_t freq, uint32_t duration_ms)
{
    buzzer_tone(&buzzer_cfg, freq, BUZZ_VOL);
    s_buzzer_active   = 1;
    s_buzzer_off_tick = HAL_GetTick() + duration_ms;
    s_beep2_pending   = 0;
}

static void beep_double(uint32_t freq1, uint32_t dur1,
                        uint32_t freq2, uint32_t dur2, uint32_t gap_ms)
{
    uint32_t now = HAL_GetTick();
    buzzer_tone(&buzzer_cfg, freq1, BUZZ_VOL);
    s_buzzer_active   = 1;
    s_buzzer_off_tick = now + dur1;
    s_beep2_pending   = 1;
    s_beep2_freq      = freq2;
    s_beep2_dur       = dur2;
    s_beep2_when      = now + dur1 + gap_ms;
}

static void update_buzzer(void)
{
    uint32_t now = HAL_GetTick();

    if (s_beep2_pending && (int32_t)(now - s_beep2_when) >= 0) {
        buzzer_tone(&buzzer_cfg, s_beep2_freq, BUZZ_VOL);
        s_buzzer_active   = 1;
        s_buzzer_off_tick = now + s_beep2_dur;
        s_beep2_pending   = 0;
    }

    if (s_buzzer_active && (int32_t)(now - s_buzzer_off_tick) >= 0) {
        buzzer_off(&buzzer_cfg);
        s_buzzer_active = 0;
    }
}

/* Draw */

void FishingEngine_Draw(FishingEngine_t *e)
{
    /* Full-screen states handle their own background */
    switch (e->state) {
        case STATE_MENU:        draw_menu(e);        return;
        case STATE_COLLECTION:  draw_collection(e);  return;
        case STATE_INSTRUCTION: draw_instruction();  return;
        case STATE_REELING:     draw_reeling(e);     return;
        default: break;
    }

    /* Remaining gameplay states share the scene backdrop + HUD */
    draw_scene_bg();
    draw_hud(e);

    switch (e->state) {
        case STATE_CASTING:      draw_casting(e);      break;
        case STATE_WAITING:      draw_waiting(e);      break;
        case STATE_FISH_BITE:    draw_fish_bite(e);    break;
        case STATE_CATCH_RESULT: draw_catch_result(e); break;
        case STATE_FISH_ESCAPED: draw_fish_escaped(e); break;
        default: break;
    }
}

/* ---------- Backgrounds ---------- */

static void draw_scene_bg(void)
{
    /* Sky */
    LCD_Draw_Rect(0, HUD_H, SCREEN_W, 18, COL_SKY_BLUE, 1);
    LCD_Draw_Rect(0, HUD_H + 18, SCREEN_W, 20, COL_SKY_BLUE, 1);
    LCD_Draw_Rect(0, HUD_H + 38, SCREEN_W, WATER_LINE_Y - HUD_H - 38, COL_SKY_BLUE, 1);

    /* Sun */
    LCD_Draw_Circle(34, 34, 14, COL_YELLOW, 1);


    /* Clouds */
    LCD_Draw_Circle(188, 32, 10, COL_WHITE, 1);
    LCD_Draw_Circle(202, 28, 12, COL_WHITE, 1);
    LCD_Draw_Circle(218, 33, 10, COL_WHITE, 1);
    LCD_Draw_Rect(186, 32, 36, 10, COL_WHITE, 1);
    LCD_Draw_Circle(92, 48, 8, COL_WHITE, 1);
    LCD_Draw_Circle(104, 44, 10, COL_WHITE, 1);
    LCD_Draw_Circle(118, 49, 8, COL_WHITE, 1);
    LCD_Draw_Rect(92, 48, 26, 8, COL_WHITE, 1);

    /* Water band */
    LCD_Draw_Rect(0, WATER_LINE_Y, SCREEN_W, UNDERWATER_BOTTOM - WATER_LINE_Y,
                  COL_BLUE, 1);
    for (int x = 0; x < SCREEN_W; x += 8) {
        LCD_Draw_Line(x,     WATER_LINE_Y,     x + 4, WATER_LINE_Y - 2, COL_WHITE);
        LCD_Draw_Line(x + 4, WATER_LINE_Y - 2, x + 8, WATER_LINE_Y,     COL_WHITE);
    }

    /* Status strip */
    LCD_Draw_Rect(0, UNDERWATER_BOTTOM, SCREEN_W, SCREEN_H - UNDERWATER_BOTTOM,
                  COL_BLACK, 1);
    LCD_Draw_Line(0, UNDERWATER_BOTTOM, SCREEN_W - 1, UNDERWATER_BOTTOM, COL_GREY);
}

static void draw_panel_frame(int16_t x, int16_t y, int16_t w, int16_t h,
                             uint8_t fill_col, uint8_t border_col)
{
    LCD_Draw_Rect(x, y, w, h, fill_col, 1);
    LCD_Draw_Rect(x, y, w, h, border_col, 0);
    LCD_Draw_Rect(x + 2, y + 2, w - 4, h - 4, border_col, 0);
}

static void draw_status_panel(uint8_t border_col)
{
    draw_panel_frame(8, 188, SCREEN_W - 16, SCREEN_H - 196, COL_BLACK, border_col);
}

static void draw_underwater_details(uint32_t state_timer)
{
    for (int i = 0; i < 5; ++i) {
        int16_t x = 30 + (i * 38) + (int16_t)((state_timer / (18 + i * 3)) % 10);
        int16_t y = UNDERWATER_BOTTOM - 12 - (int16_t)((state_timer / (20 + i * 5) + i * 13) % 54);
        uint8_t r = (uint8_t)(2 + (i & 1));
        LCD_Draw_Circle(x, y, r, COL_WHITE, 0);
    }
}

static void draw_hud(FishingEngine_t *e)
{
    LCD_Draw_Rect(0, 0, SCREEN_W, 20, COL_NAVY, 1);
    LCD_Draw_Line(0, 19, SCREEN_W - 1, 19, COL_SKY_BLUE);
    LCD_Draw_Rect(2, 2, 80, 16, COL_BLACK, 1);
    LCD_Draw_Rect(2, 2, 80, 16, COL_SKY_BLUE, 0);
    LCD_Draw_Rect(SCREEN_W - 76, 2, 74, 16, COL_BLACK, 1);
    LCD_Draw_Rect(SCREEN_W - 76, 2, 74, 16, COL_YELLOW, 0);

    char buf[24];
    sprintf(buf, "Score:%u", e->score);
    LCD_printString(buf, 6, 5, COL_WHITE, 1);

    sprintf(buf, "Caught:%u", e->fish_caught);
    LCD_printString(buf, SCREEN_W - 72, 5, COL_YELLOW, 1);
}

/* ---------- Rod ---------- */

static void draw_rod(void)
{
    LCD_Draw_Rect(194, 68, 12, 12, COL_BROWN, 1);
    LCD_Draw_Rect(194, 68, 12, 12, COL_BLACK, 0);
    LCD_Draw_Line(200, 74, 172, 56, COL_BROWN);
    LCD_Draw_Line(201, 74, 173, 56, COL_BROWN);
    LCD_Draw_Line(199, 75, 171, 57, COL_BROWN);

    LCD_Draw_Line(172, 56, 130, 30, COL_BROWN);
    LCD_Draw_Line(172, 56, 131, 31, COL_BROWN);
}

/* ---------- MENU ---------- */

static void draw_menu(FishingEngine_t *e)
{
    LCD_Fill_Buffer(COL_NAVY);

    /* Sky and water backdrops */
    LCD_Draw_Rect(0,   0, SCREEN_W, 120, COL_SKY_BLUE, 1);
    LCD_Draw_Rect(0, 120, SCREEN_W,  54, COL_BLUE, 1);

    /* Water ripple */
    for (int x = 0; x < SCREEN_W; x += 8) {
        LCD_Draw_Line(x,     120, x + 4, 118, COL_WHITE);
        LCD_Draw_Line(x + 4, 118, x + 8, 120, COL_WHITE);
    }

    /* Title card */
    draw_panel_frame(4, 4, SCREEN_W - 8, 112, COL_NAVY, COL_GOLD);
    LCD_printString("mini",      96, 10, COL_WHITE, 2);
    LCD_printString("STARDEW",    36, 27, COL_GOLD,  4);
    //LCD_Draw_Line(30, 55, 210, 55, COL_GOLD);
    LCD_printString("Fishing",    57,64, COL_SKY_BLUE, 3);
    LCD_printString("Simulator",  66, 90, COL_SKY_BLUE,  2);

    /* Three menu options evenly spaced between y=120 and y=232
     * 3 panels×24px + 4 gaps×10px = 112px total */
    static const char * const opt_label[3]     = {"  FISHING",   "  COLLECTION", "  INSTRUCT"};
    static const char * const opt_label_sel[3] = {"> FISHING",   "> COLLECTION", "> INSTRUCT"};
    static const int16_t      opt_y[3]         = {130, 164, 198};

    for (int i = 0; i < 3; i++) {
        uint8_t sel = (e->menu_cursor == (uint8_t)i);
        draw_panel_frame(28, opt_y[i], 184, 24,
                         COL_BLACK, sel ? COL_GOLD : COL_GREY);
        LCD_printString(sel ? opt_label_sel[i] : opt_label[i],
                        48, opt_y[i] + 4,
                        sel ? COL_GOLD : COL_GREY, 2);
    }

    /* Hint */
    LCD_printString("N/S->Move  Button->Select", 50, 228, COL_WHITE, 1);
}

/* ---------- CASTING ---------- */

static void draw_casting(FishingEngine_t *e)
{
    float p = (float)e->state_timer / CAST_DURATION_MS;
    if (p > 1.0f) p = 1.0f;

    int16_t bx = 130 + (int16_t)(sinf(p * 3.14159f) * -50.0f);
    int16_t by =  30 + (int16_t)(p * (float)(WATER_LINE_Y - 4 - 30));

    draw_rod();
    LCD_Draw_Line(130, 30, bx, by, COL_WHITE);
    LCD_Draw_Sprite(bx - BOBBER_W / 2, by,
                    BOBBER_H, BOBBER_W,
                    (const uint8_t *)bobber_sprite);

    draw_underwater_details(e->state_timer);
    draw_status_panel(COL_SKY_BLUE);
    LCD_printString("CASTING", 84, 193, COL_WHITE, 2);
    LCD_printString("Watch the bobber land cleanly", 28, 214, COL_SKY_BLUE, 1);
}

/* ---------- WAITING ---------- */

static void draw_waiting(FishingEngine_t *e)
{
    draw_rod();
    LCD_Draw_Line(130, 30, 130, WATER_LINE_Y + 4, COL_WHITE);

    int16_t bob_y = WATER_LINE_Y - 4 +
                    (int16_t)((e->state_timer / 200) % 3);
    LCD_Draw_Sprite(130 - BOBBER_W / 2, bob_y,
                    BOBBER_H, BOBBER_W,
                    (const uint8_t *)bobber_sprite);

    draw_underwater_details(e->state_timer);
    draw_status_panel(COL_SKY_BLUE);
    LCD_printString("WAITING", 84, 193, COL_WHITE, 2);
    /* "Stay ready!  W:Menu" = 19 chars x6 = 114px -> x=(240-114)/2=63 */
    LCD_printString("Stay ready! Hold W->Menu", 51, 214, COL_SKY_BLUE, 1);
}

/* ---------- FISH BITE ---------- */

static void draw_fish_bite(FishingEngine_t *e)
{
    draw_rod();
    LCD_Draw_Line(130, 30, 130, WATER_LINE_Y + 14, COL_WHITE);
    LCD_Draw_Sprite(130 - BOBBER_W / 2, WATER_LINE_Y + 10,
                    BOBBER_H, BOBBER_W,
                    (const uint8_t *)bobber_sprite);

    draw_underwater_details(e->state_timer);
    draw_status_panel(COL_RED);

    uint8_t col = ((e->state_timer / 120) & 1) ? COL_RED : COL_YELLOW;
    LCD_printString("BITE!", 75, 122, col, 5);
    LCD_printString("HOOK IT NOW", 56, 193, COL_WHITE, 2);
    LCD_printString("Press before the fish escapes", 30, 214, COL_YELLOW, 1);
}

/* ---------- REELING ---------- */

static void draw_reeling(FishingEngine_t *e)
{
    /*Light blue background*/
    LCD_Fill_Buffer(COL_SKY_BLUE);

    /* ── Vertical catch progress bar (left of track, gap=20, w=20) ── */
    #define PROG_X  60
    #define PROG_W  20
    LCD_Draw_Rect(PROG_X, MINI_TY, PROG_W, MINI_TH, COL_NAVY, 0);
    int16_t fill_h = (int16_t)(e->catch_progress * (float)MINI_TH);
    if (fill_h > 0) {
        uint8_t fill_col = (e->catch_progress > 0.7f) ? COL_GOLD : COL_GREEN;
        LCD_Draw_Rect(PROG_X, MINI_TY + MINI_TH - fill_h,
                      PROG_W, fill_h, fill_col, 1);
    }

    /* ── Vertical track water background ── */
    LCD_Draw_Rect(MINI_TX, MINI_TY, MINI_TW, MINI_TH, COL_BLUE, 1);
    LCD_Draw_Rect(MINI_TX, MINI_TY, MINI_TW, MINI_TH, COL_NAVY, 0);

    /* Green bar (player-controlled) */
    int16_t bar_half_px = (int16_t)(BAR_HALF * (float)MINI_TH);
    int16_t bar_ctr_px  = MINI_TY + (int16_t)(s_bar_y * (float)MINI_TH);
    int16_t bar_top     = bar_ctr_px - bar_half_px;
    int16_t bar_bot     = bar_ctr_px + bar_half_px;
    if (bar_top < MINI_TY)            bar_top = MINI_TY;
    if (bar_bot > MINI_TY + MINI_TH)  bar_bot = MINI_TY + MINI_TH;
    LCD_Draw_Rect(MINI_TX, bar_top, MINI_TW, bar_bot - bar_top, COL_GREEN, 1);

    /* Fish icon — drawn on top of the green bar so it's always visible */
    int16_t fish_py = MINI_TY + (int16_t)(s_fish_y * (float)MINI_TH);
    int16_t fish_x  = MINI_TX + (MINI_TW - e->current_fish.type.sprite_w) / 2;
    int16_t fish_y  = fish_py - e->current_fish.type.sprite_h / 2;
    if (fish_y < MINI_TY) fish_y = MINI_TY;
    if (fish_y + e->current_fish.type.sprite_h > MINI_TY + MINI_TH)
        fish_y = MINI_TY + MINI_TH - e->current_fish.type.sprite_h;
    LCD_Draw_Sprite(fish_x, fish_y,
                    e->current_fish.type.sprite_h,
                    e->current_fish.type.sprite_w,
                    e->current_fish.type.sprite);

    LCD_printString("N->Up  Hold W->Menu", 10, 224, COL_NAVY, 2);
}

/* ---------- CATCH RESULT ---------- */

static void draw_catch_result(FishingEngine_t *e)
{
    draw_panel_frame(16, 100, SCREEN_W - 32, 122, COL_BLACK, COL_GOLD);
    LCD_Draw_Rect(24, 108, SCREEN_W - 48, 18, COL_NAVY, 1);

    LCD_printString("CATCH OF THE DAY", 35, 113, COL_GOLD, 1);
    LCD_printString("CAUGHT!", 70, 129, COL_GOLD, 3);

    int16_t fish_x = SCREEN_W / 2 - e->current_fish.type.sprite_w;
    LCD_Draw_Sprite_Scaled(fish_x+30, 154,
                          e->current_fish.type.sprite_h,
                          e->current_fish.type.sprite_w,
                          e->current_fish.type.sprite,
                          2);

    char buf[32];
    sprintf(buf, "%s", e->current_fish.type.name);
    LCD_printString(buf, 28, 192, COL_WHITE, 2);

    sprintf(buf, "%ucm  +%u pts",
            e->current_fish.size_cm, e->current_fish.type.points);
    LCD_printString(buf, 28, 240, COL_YELLOW, 1);

    LCD_printString(Fish_RarityName(e->current_fish.type.rarity),
                    28, 165, COL_PINK, 2);
}

/* ---------- FISH ESCAPED ---------- */

static void draw_fish_escaped(FishingEngine_t *e)
{
    draw_underwater_details(e->state_timer);
    draw_panel_frame(22, 124, SCREEN_W - 44, 76, COL_BLACK, COL_RED);
    LCD_printString("GOT AWAY!", 44, 142, COL_RED, 3);
    
    LCD_printString("Next bite coming...", 30, 173, COL_WHITE, 1);
    LCD_printString("Hold W->Menu", 30, 183, COL_WHITE, 1);
}

/* ---------- COLLECTION ---------- */

static void draw_collection(FishingEngine_t *e)
{
    LCD_Fill_Buffer(COL_NAVY);

    /* Title bar */
    LCD_Draw_Rect(0, 0, SCREEN_W, 22, COL_BLACK, 1);
    LCD_Draw_Line(0, 21, SCREEN_W - 1, 21, COL_GOLD);

    LCD_printString("COLLECTION", 60, 4, COL_GOLD, 2);

    /* Five fish cards */
    for (int i = 0; i < FISH_SPECIES_COUNT; i++) {
        int16_t ry = 24 + i * 40;
        const FishType *ft = Fish_GetType((uint8_t)i);
        uint8_t caught = (e->caught_flags >> i) & 1u;

        /* Card: gold border if caught, grey if not */
        draw_panel_frame(4, ry, SCREEN_W - 8, 38,
                         COL_BLACK, caught ? COL_GOLD : COL_GREY);

        if (caught) {
            /* Sprite at scale 2 */
            LCD_Draw_Sprite_Scaled(8, ry + 5,
                                   ft->sprite_h, ft->sprite_w,
                                   ft->sprite, 2);
            /* Name and rarity to the right of the sprite */
            int16_t tx = (int16_t)(8 + ft->sprite_w * 2 + 6);
            LCD_printString(ft->name, tx, ry + 5, COL_WHITE, 2);
            uint8_t rcol = (ft->rarity == RARITY_LEGENDARY) ? COL_GOLD :
                           (ft->rarity == RARITY_RARE)      ? COL_PINK : COL_GREEN;
            LCD_printString(Fish_RarityName(ft->rarity), tx, ry + 23, rcol, 1.5);
            /* Gold star at right edge */
            LCD_printString("*", SCREEN_W - 18, ry + 12, COL_GOLD, 2);
        } else {
            /* Grey silhouette placeholder */
            LCD_Draw_Rect(8, ry + 5,
                          (int16_t)(ft->sprite_w * 2),
                          (int16_t)(ft->sprite_h * 2)-4, COL_GREY, 1);
            int16_t tx = (int16_t)(8 + ft->sprite_w * 2 + 6);
            LCD_printString("???", tx, ry + 10, COL_GREY, 2);
        }
    }

    LCD_printString("Button->Back", 90, 228, COL_WHITE, 1);
}

/* ---------- INSTRUCTION ---------- */

static void draw_instruction(void)
{
    LCD_Fill_Buffer(COL_SKY_BLUE);

    draw_panel_frame(4, 4, SCREEN_W - 8, 22, COL_NAVY, COL_GOLD);
    LCD_printString("HOW TO FISH", 54, 8, COL_GOLD, 2);
   
    /* Fish */
    int16_t fx = 4  + (112 - (int16_t)FISH_CARP_COLOR_W) / 2;
    int16_t fy = 32 + (58  - (int16_t)FISH_CARP_COLOR_H) / 2;
    /* Green bar rectangle centred on the fish */
    int16_t bx = fx + (int16_t)FISH_CARP_COLOR_W / 2 - 11;
    int16_t by = fy + (int16_t)FISH_CARP_COLOR_H / 2 - 30;
    LCD_Draw_Rect(bx, by+20, 22, 60, COL_GREEN, 1);
    LCD_Draw_Sprite(fx, fy+20, FISH_CARP_COLOR_H, FISH_CARP_COLOR_W,
                    (const uint8_t *)fish_carp_color);

    /* ── Right-side text ── */
    LCD_printString("KEEP FISH",  113, 55, COL_NAVY,  2);
    LCD_printString("IN THE",     113, 75, COL_NAVY,  2);
    LCD_printString("GREEN BAR!", 113, 95, COL_NAVY, 2);

    /* ── Controls ── */
    LCD_Draw_Line(4, 123, 236, 123, COL_NAVY);

    LCD_printString("TO CONTROL THE BAR:", 11, 134, COL_NAVY, 2);
    /* "HOLD N: Up" */
    LCD_printString("HOLD N->Up", 60, 154, COL_NAVY, 2);
    /* "Let go: Down" */
    LCD_printString("LET GO->Down", 48, 174, COL_NAVY, 2);

    /* ── Footer ── */
    LCD_Draw_Line(4, 203, 236, 203, COL_NAVY);
    /* "Button: Back" = 12 chars x12 = 144px -> x=48 */
    LCD_printString("Button->Back", 48, 215, COL_BLACK, 2);
}