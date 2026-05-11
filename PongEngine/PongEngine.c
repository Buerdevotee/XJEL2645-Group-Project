/**
 * @file PongEngine.c
 * @brief Main game engine implementation for Pong
 *
 * Handles collision detection using AABB method and game logic.
 */

#include "PongEngine.h"
#include "Buzzer.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240
#define BALL_RESET_OFFSET    20
#define BUZZER_WALL_FREQ_HZ  1200
#define BUZZER_PADDLE_FREQ_HZ 800
#define BUZZER_VOLUME        50
#define BUZZER_BEEP_MS       40

extern Buzzer_cfg_t buzzer_cfg;

static uint32_t buzzer_stop_tick = 0;

static void PongEngine_Beep(uint32_t freq_hz) {
    buzzer_tone(&buzzer_cfg, freq_hz, BUZZER_VOLUME);
    buzzer_stop_tick = HAL_GetTick() + BUZZER_BEEP_MS;
}

static void PongEngine_UpdateBuzzer(void) {
    if (buzzer_stop_tick != 0 && (int32_t)(HAL_GetTick() - buzzer_stop_tick) >= 0) {
        buzzer_off(&buzzer_cfg);
        buzzer_stop_tick = 0;
    }
}

static void PongEngine_CheckWallCollision(PongEngine_t* engine) {
    Ball_t* ball = &engine->ball;
    Vector2D vel = Ball_GetVelocity(ball);

    // Top wall
    if (ball->y <= 0) {
        ball->y = 2;
        Ball_SetVelocity(ball, vel.x, -vel.y);
        PongEngine_Beep(BUZZER_WALL_FREQ_HZ);
    }
    // Bottom wall
    else if (ball->y + ball->size >= SCREEN_HEIGHT) {
        ball->y = SCREEN_HEIGHT - ball->size - 2;
        Ball_SetVelocity(ball, vel.x, -vel.y);
        PongEngine_Beep(BUZZER_WALL_FREQ_HZ);
    }

    // Right wall
    if (ball->x + ball->size >= SCREEN_WIDTH) {
        ball->x = SCREEN_WIDTH - ball->size - 2;
        Ball_SetVelocity(ball, -vel.x, vel.y);
        PongEngine_Beep(BUZZER_WALL_FREQ_HZ);
    }
}

static void PongEngine_CheckPaddleCollision(PongEngine_t* engine) {
    Ball_t* ball = &engine->ball;
    Paddle_t* paddle = &engine->paddle;
    Vector2D vel = Ball_GetVelocity(ball);

    AABB ball_box   = Ball_GetAABB(ball);
    AABB paddle_box = Paddle_GetAABB(paddle);

    if (AABB_Collides(&ball_box, &paddle_box)) {
        Ball_SetVelocity(ball, -vel.x, vel.y);
        // Push ball out of paddle to prevent sticking
        ball->x = paddle_box.x + paddle_box.width;
        Paddle_AddScore(paddle);
        PongEngine_Beep(BUZZER_PADDLE_FREQ_HZ);
    }
}

static void PongEngine_CheckGoal(PongEngine_t* engine) {
    Ball_t* ball = &engine->ball;

    if (ball->x < 0) {
        // Guard against underflow: only decrement if lives > 0
        if (engine->lives > 0) {
            engine->lives--;
        }

        // Reset ball to center with small random offset
        Position2D center;
        center.x = (SCREEN_WIDTH  - ball->size) / 2;
        center.y = (SCREEN_HEIGHT - ball->size) / 2;

        int16_t dx = (int16_t)Random_U16(2 * BALL_RESET_OFFSET + 1) - BALL_RESET_OFFSET;
        int16_t dy = (int16_t)Random_U16(2 * BALL_RESET_OFFSET + 1) - BALL_RESET_OFFSET;
        center.x += dx;
        center.y += dy;

        Ball_SetPos(ball, center);
        Ball_SetVelocity(ball, 8.0f * 0.707f, 8.0f * 0.707f);
    }
}

void PongEngine_Init(PongEngine_t* engine,
                     int16_t paddle_x, int16_t paddle_y,
                     int16_t paddle_width, int16_t paddle_height,
                     int16_t ball_size, float ball_speed) {
    Ball_Init(&engine->ball, ball_size, ball_speed);
    Paddle_Init(&engine->paddle, paddle_x, paddle_y,
                paddle_width, paddle_height, 6);
    engine->lives = 4;
}

uint8_t PongEngine_Update(PongEngine_t* engine, UserInput input) {
    Paddle_Update(&engine->paddle, input);
    Ball_Update(&engine->ball);
    PongEngine_CheckWallCollision(engine);
    PongEngine_CheckPaddleCollision(engine);
    PongEngine_CheckGoal(engine);
    PongEngine_UpdateBuzzer();
    return engine->lives;
}

void PongEngine_Draw(PongEngine_t* engine) {
    Ball_Draw(&engine->ball);
    Paddle_Draw(&engine->paddle);
}

uint8_t PongEngine_GetLives(PongEngine_t* engine) {
    return engine->lives;
}

uint16_t PongEngine_GetScore(PongEngine_t* engine) {
    return Paddle_GetScore(&engine->paddle);
}
