/**
 * @file PongEngine.h
 * @brief Main game engine for Pong
 *
 * Coordinates ball and paddle updates, handles collisions,
 * and manages game state (lives/score).
 */

#ifndef PONGENGINE_H
#define PONGENGINE_H

#include <stdint.h>
#include "Ball.h"
#include "Paddle.h"
#include "Utils.h"

/**
 * @struct PongEngine_t
 * @brief Main game engine object
 */
typedef struct {
    Ball_t ball;         // Ball object
    Paddle_t paddle;     // Paddle object
    uint8_t lives;       // Remaining lives (game over when 0)
} PongEngine_t;

/**
 * @brief Initialize the Pong game engine
 *
 * @param engine Pointer to game engine
 * @param paddle_x Initial paddle X position
 * @param paddle_y Initial paddle Y position
 * @param paddle_width Paddle width
 * @param paddle_height Paddle height
 * @param ball_size Ball diameter
 * @param ball_speed Initial ball speed
 */
void PongEngine_Init(PongEngine_t* engine,
                     int16_t paddle_x, int16_t paddle_y,
                     int16_t paddle_width, int16_t paddle_height,
                     int16_t ball_size, float ball_speed);

/**
 * @brief Update game state (input + physics + collisions)
 *
 * @param engine Pointer to game engine
 * @param input Joystick input for this frame
 * @return Remaining lives (0 = game over)
 */
uint8_t PongEngine_Update(PongEngine_t* engine, UserInput input);

/**
 * @brief Draw all game objects
 *
 * Draws the ball and paddle to the LCD buffer.
 * Caller is responsible for LCD_Fill_Buffer() and LCD_Refresh().
 *
 * @param engine Pointer to game engine
 */
void PongEngine_Draw(PongEngine_t* engine);

/**
 * @brief Get current lives remaining
 *
 * @param engine Pointer to game engine
 * @return Number of lives remaining
 */
uint8_t PongEngine_GetLives(PongEngine_t* engine);

/**
 * @brief Get current score
 *
 * @param engine Pointer to game engine
 * @return Current score
 */
uint16_t PongEngine_GetScore(PongEngine_t* engine);

#endif // PONGENGINE_H
