/**
 * @file Ball.c
 * @brief Ball object implementation for Pong game
 */

#include "Ball.h"
#include "LCD.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

void Ball_Init(Ball_t* ball, int16_t size, float speed) {
    ball->size = size;
    ball->x = (SCREEN_WIDTH - size) / 2;
    ball->y = (SCREEN_HEIGHT - size) / 2;

    // Start moving at 45 degrees (down and to the right)
    // Using 0.707 ≈ sin(45°) for diagonal movement at constant speed
    ball->velocity.x = speed * 0.707f;  // cos(45°)
    ball->velocity.y = speed * 0.707f;  // sin(45°)
}

void Ball_Update(Ball_t* ball) {
    ball->x += (int16_t)ball->velocity.x;
    ball->y += (int16_t)ball->velocity.y;
}

void Ball_Draw(Ball_t* ball) {
    LCD_Draw_Circle(
        ball->x + ball->size / 2,  // center x
        ball->y + ball->size / 2,  // center y
        ball->size / 2,            // radius
        15,                        // white
        1                          // filled
    );
}

AABB Ball_GetAABB(Ball_t* ball) {
    AABB box;
    box.x = ball->x;
    box.y = ball->y;
    box.width = ball->size;
    box.height = ball->size;
    return box;
}

void Ball_SetVelocity(Ball_t* ball, float vx, float vy) {
    ball->velocity.x = vx;
    ball->velocity.y = vy;
}

Position2D Ball_GetPos(Ball_t* ball) {
    Position2D pos;
    pos.x = ball->x;
    pos.y = ball->y;
    return pos;
}

void Ball_SetPos(Ball_t* ball, Position2D pos) {
    ball->x = pos.x;
    ball->y = pos.y;
}

Vector2D Ball_GetVelocity(Ball_t* ball) {
    return ball->velocity;
}

int16_t Ball_GetSize(Ball_t* ball) {
    return ball->size;
}
