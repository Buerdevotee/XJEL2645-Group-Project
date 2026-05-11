/**
 * @file Ball.h
 * @brief Ball object for Pong game
 *
 * Controls the ball position, velocity, and rendering.
 * The ball moves continuously and bounces off walls.
 */

#ifndef BALL_H
#define BALL_H

#include <stdint.h>
#include "Utils.h"
#include "Joystick.h"

/**
 * @struct Ball_t
 * @brief Ball object containing position, size, and velocity
 */
typedef struct {
    int16_t x;              // Ball X position (top-left)
    int16_t y;              // Ball Y position (top-left)
    int16_t size;           // Ball size (diameter in pixels)
    Vector2D velocity;      // Velocity per frame (x, y components)
} Ball_t;

/**
 * @brief Initialize ball at center of screen
 *
 * @param ball Pointer to ball object
 * @param size Diameter of ball in pixels
 * @param speed Initial speed in pixels/frame
 */
void Ball_Init(Ball_t* ball, int16_t size, float speed);

/**
 * @brief Update ball position based on velocity
 *
 * Moves the ball by its velocity vector each frame.
 * Does NOT handle wall collisions (done by PongEngine).
 *
 * @param ball Pointer to ball object
 */
void Ball_Update(Ball_t* ball);

/**
 * @brief Draw ball on LCD
 *
 * Draws the ball as a filled circle at its current position.
 *
 * @param ball Pointer to ball object
 */
void Ball_Draw(Ball_t* ball);

/**
 * @brief Get ball bounding box for collision detection
 *
 * @param ball Pointer to ball object
 * @return AABB structure representing the ball's collision box
 */
AABB Ball_GetAABB(Ball_t* ball);

/**
 * @brief Set ball velocity
 *
 * @param ball Pointer to ball object
 * @param vx Velocity X component (pixels/frame)
 * @param vy Velocity Y component (pixels/frame)
 */
void Ball_SetVelocity(Ball_t* ball, float vx, float vy);

/**
 * @brief Get ball position
 *
 * @param ball Pointer to ball object
 * @return Current position
 */
Position2D Ball_GetPos(Ball_t* ball);

/**
 * @brief Set ball position
 *
 * @param ball Pointer to ball object
 * @param pos New position
 */
void Ball_SetPos(Ball_t* ball, Position2D pos);

/**
 * @brief Get ball velocity
 *
 * @param ball Pointer to ball object
 * @return Current velocity
 */
Vector2D Ball_GetVelocity(Ball_t* ball);

/**
 * @brief Get ball size
 *
 * @param ball Pointer to ball object
 * @return Ball size (diameter)
 */
int16_t Ball_GetSize(Ball_t* ball);

#endif // BALL_H
