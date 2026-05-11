/**
 * @file Paddle.h
 * @brief Paddle object for Pong game
 *
 * Controls the paddle position, movement, and rendering.
 * The paddle is controlled by joystick input.
 */

#ifndef PADDLE_H
#define PADDLE_H

#include <stdint.h>
#include "Utils.h"
#include "Joystick.h"

/**
 * @struct Paddle_t
 * @brief Paddle object containing position, size, and score
 */
typedef struct {
    int16_t x;        // Paddle X position (left edge)
    int16_t y;        // Paddle Y position (top edge)
    int16_t width;    // Paddle width
    int16_t height;   // Paddle height
    int16_t speed;    // Movement speed (pixels per frame)
    uint16_t score;   // Game score (incremented on successful hit)
} Paddle_t;

/**
 * @brief Initialize paddle
 *
 * @param paddle Pointer to paddle object
 * @param x Starting X position
 * @param y Starting Y position
 * @param width Paddle width in pixels
 * @param height Paddle height in pixels
 * @param speed Movement speed in pixels/frame
 */
void Paddle_Init(Paddle_t* paddle, int16_t x, int16_t y, int16_t width, int16_t height, int16_t speed);

/**
 * @brief Update paddle position based on joystick input
 *
 * Moves paddle up/down based on joystick Y direction.
 * Constrains paddle to stay within screen bounds.
 *
 * @param paddle Pointer to paddle object
 * @param input Joystick input
 */
void Paddle_Update(Paddle_t* paddle, UserInput input);

/**
 * @brief Draw paddle on LCD
 *
 * Draws the paddle as a filled rectangle at its current position.
 *
 * @param paddle Pointer to paddle object
 */
void Paddle_Draw(Paddle_t* paddle);

/**
 * @brief Get paddle bounding box for collision detection
 *
 * @param paddle Pointer to paddle object
 * @return AABB structure representing the paddle's collision box
 */
AABB Paddle_GetAABB(Paddle_t* paddle);

/**
 * @brief Increment paddle score
 *
 * Called when paddle successfully hits the ball.
 *
 * @param paddle Pointer to paddle object
 */
void Paddle_AddScore(Paddle_t* paddle);

/**
 * @brief Get current score
 *
 * @param paddle Pointer to paddle object
 * @return Current score
 */
uint16_t Paddle_GetScore(Paddle_t* paddle);

/**
 * @brief Get paddle position
 *
 * @param paddle Pointer to paddle object
 * @return Current position (top-left corner)
 */
Position2D Paddle_GetPos(Paddle_t* paddle);

/**
 * @brief Get paddle height
 *
 * @param paddle Pointer to paddle object
 * @return Height in pixels
 */
int16_t Paddle_GetHeight(Paddle_t* paddle);

/**
 * @brief Get paddle width
 *
 * @param paddle Pointer to paddle object
 * @return Width in pixels
 */
int16_t Paddle_GetWidth(Paddle_t* paddle);

#endif // PADDLE_H
