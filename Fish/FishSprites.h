/**
 * @file FishSprites.h
 * @brief Pixel art sprite data for the Fishing Simulator
 *
 * All sprites are 2D arrays where each cell is either a colour palette
 * index (0-15) or 255 for transparent. Fish silhouettes use index 1 and
 * are rendered with LCD_Draw_Sprite_Colour() so the body colour can be
 * overridden per fish type.
 *
 * Sprites are declared as `static const` so they live in flash and do not
 * consume RAM. This file is intended to be included only from Fish.c.
 */

#ifndef FISHSPRITES_H
#define FISHSPRITES_H

#include <stdint.h>

/* Local helper macro: keeps sprite tables visually compact.
 * Undefined at the end of the header to avoid leaking the symbol. */
#define T 255

/* ===== Bobber sprite (multi-colour, render with LCD_Draw_Sprite) =====
 * Uses palette index 2 (RED) for the float top and 1 (WHITE) for the
 * stem below. Drawn directly without colour override. */
#define BOBBER_W 5
#define BOBBER_H 9
static const uint8_t bobber_sprite[BOBBER_H][BOBBER_W] = {
    { T, 2, 2, 2, T },
    { 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2 },
    { T, 2, 2, 2, T },
    { T, T, 1, T, T },
    { T, 1, 1, 1, T },
    { T, 1, 1, 1, T },
    { T, 1, 1, 1, T },
    { T, T, 1, T, T },
};


/* ===== Stardew Valley fish sprites — 16x16, multi-colour ===== */
/* Render with LCD_Draw_Sprite_Scaled  */

#define FISH_CARP_COLOR_W 16
#define FISH_CARP_COLOR_H 16
static const uint8_t fish_carp_color[FISH_CARP_COLOR_H][FISH_CARP_COLOR_W] = {
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T, 12, 12, 12, 12,  T,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T, 12, 12, 12,  5,  5,  5, 12,  T },
    {  T,  T,  T,  T,  T, 12, 12, 12, 12, 12,  5,  5,  0,  5,  5, 12 },
    {  T,  T,  T,  T, 12,  5, 12, 12,  5, 12,  5,  5,  5,  5,  5, 12 },
    {  T,  T,  T, 12,  5,  5, 12,  5,  5,  5, 12,  5,  5,  5,  7, 12 },
    {  T,  T,  T, 12,  5, 12, 12,  5,  5,  5,  5, 12,  7,  5, 12,  T },
    {  T,  T,  T,  T, 12, 12, 12,  5,  5,  5,  7,  7,  7, 12,  T,  T },
    {  T,  T,  T,  T,  T, 12,  5,  5,  5,  7,  7,  7,  5, 12,  T,  T },
    {  T,  T,  T,  T, 12, 12,  5,  5,  7,  7,  7,  7, 12,  T,  T,  T },
    {  T,  T,  T,  T, 12,  5,  5,  7,  7,  7,  5, 12, 12,  T,  T,  T },
    {  T, 12, 12, 12,  5,  7,  7,  7,  5, 12, 12,  5, 12,  T,  T,  T },
    { 12,  7,  7,  5,  5,  5, 12, 12, 12,  T, 12,  5, 12,  T,  T,  T },
    {  T, 12, 12,  5,  5, 12,  T,  T,  T,  T,  T, 12,  T,  T,  T,  T },
    {  T,  T,  T, 12,  7, 12,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T, 12,  7, 12,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  T, 12,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T }
};

#define FISH_LEGEND_COLOR_W 16
#define FISH_LEGEND_COLOR_H 16
static const uint8_t fish_legend_color[FISH_LEGEND_COLOR_H][FISH_LEGEND_COLOR_W] = {
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  0,  0,  0,  0,  0,  0,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  0,  0,  3,  3,  3,  3,  0,  0 },
    {  T,  T,  0,  0,  T,  0,  0,  0,  0,  0,  6,  0,  3,  0,  7,  0 },
    {  T,  T,  0,  3,  0,  3,  0,  0,  0,  3,  5,  6,  3,  0, 13,  0 },
    {  T,  0,  0,  3,  3,  0,  0,  3,  0,  0,  3,  3,  3, 13, 13,  0 },
    {  T,  0,  3,  3,  3,  0,  3,  0,  0,  3,  3, 13, 13,  7,  7,  0 },
    {  T,  T,  0,  3,  0,  0,  3,  0,  3,  3,  3,  0, 13, 13,  7,  0 },
    {  T,  T,  T,  0,  0,  0,  3,  0,  3,  0,  0, 13, 13,  7, 13,  0 },
    {  T,  T,  T,  T,  0,  3,  3,  0,  0, 13, 13,  7, 13,  7,  0,  T },
    {  T,  0,  T,  T,  0,  3,  3,  3, 13, 13, 13,  7,  7, 13,  0,  T },
    {  0,  0,  0,  0,  0,  3,  3, 13, 13, 13,  7,  7, 13,  0,  T,  T },
    {  0,  0,  3,  3,  0,  3,  0,  0, 13, 13, 13,  0,  0,  T,  T,  T },
    {  T,  0,  0,  0,  3,  0,  T,  T,  0,  0,  0,  T,  T,  T,  T,  T },
    {  T,  T,  T,  0,  3,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  0,  0,  0,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  T,  0,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T }
};

#define FISH_TUNA_COLOR_W 16
#define FISH_TUNA_COLOR_H 16
static const uint8_t fish_tuna_color[FISH_TUNA_COLOR_H][FISH_TUNA_COLOR_W] = {
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  9,  9, 13, 13,  T,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  9, 13, 14, 14,  1, 13,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  9, 13,  1,  0,  1,  1, 11,  T },
    {  T,  T,  T,  T,  T,  T,  T,  9, 13, 14, 14,  1,  1,  1, 11,  T },
    {  T,  T,  T,  T,  T,  T,  9, 13, 14, 14,  1,  1,  1,  1,  1, 13 },
    {  T,  T,  T, 13, 13,  9, 13, 13, 14, 13,  1,  1,  1,  1, 13,  T },
    {  T,  T, 13, 14, 13,  9, 13, 14, 13,  1,  1, 13,  1,  1, 13,  T },
    {  T,  T, 13, 13,  9, 13, 14, 14, 13, 14, 13, 14,  1, 13,  T,  T },
    {  T,  T,  T,  T,  9, 13, 14,  1, 14, 13, 14,  1, 13,  T,  T,  T },
    {  T,  9,  T,  9, 13, 13, 14,  1,  1,  1,  1, 13, 13,  T,  T,  T },
    {  9, 14,  9, 13, 13, 13, 14,  1,  1, 13, 13, 14, 13,  T,  T,  T },
    {  9, 14, 13, 13, 13, 13, 14, 13, 13,  T,  T, 13,  T,  T,  T,  T },
    {  T,  9, 14, 14, 13, 13, 13, 14, 13,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  9, 13,  9,  T,  T, 13,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  9, 14,  9,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  9,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T }
};

#define FISH_SEA_CUCUMBER_COLOR_W 16
#define FISH_SEA_CUCUMBER_COLOR_H 16
static const uint8_t fish_sea_cucumber_color[FISH_SEA_CUCUMBER_COLOR_H][FISH_SEA_CUCUMBER_COLOR_W] = {
    {  T,  T,  T,  T,  T,  T,  T,  T,  T, 12, 12, 12,  T, 12,  T,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T, 12,  5,  5,  5, 12, 12,  T,  T },
    {  T,  T,  T,  T,  T,  T,  T, 12, 12,  5, 10, 10, 10,  5, 12,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T, 12,  5, 10,  6, 10,  5, 12 },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T, 12, 10, 10, 10,  5, 12 },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T, 12,  6, 10,  5, 10, 12 },
    {  T,  T,  T,  T,  T,  T,  T, 12,  T, 12,  5, 10, 10, 10,  5, 12 },
    {  T,  T,  T,  T, 12,  T,  T, 12, 12,  5, 10, 10, 10,  5,  5, 12 },
    {  T,  T,  T,  T, 12, 12, 12,  5, 10, 10,  5, 10,  6, 10, 12,  T },
    {  T,  T, 12, 12,  5, 10, 10, 10, 10,  6, 10, 10, 10,  5, 12,  T },
    {  T, 12,  5, 10, 10, 10,  5, 10, 10, 10, 10, 10,  5, 12,  T,  T },
    { 12,  5,  5,  5,  5, 10, 10,  6, 10, 10, 10,  5,  5, 12, 12,  T },
    { 12, 10, 12, 12,  5, 10,  5, 10,  5,  5,  5,  5, 12,  T,  T,  T },
    { 12, 10, 12, 12,  5,  5,  5,  5,  5,  5, 12, 12,  T,  T,  T,  T },
    {  T, 12, 10, 10,  5, 12, 12, 12, 12, 12,  T, 12,  T,  T,  T,  T },
    {  T,  T, 12, 12, 12,  T,  T, 12,  T,  T,  T,  T,  T,  T,  T,  T }
};

#define FISH_STURGEON_COLOR_W 16
#define FISH_STURGEON_COLOR_H 16
static const uint8_t fish_sturgeon_color[FISH_STURGEON_COLOR_H][FISH_STURGEON_COLOR_W] = {
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  0,  0,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  0,  0,  1,  0,  0 },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  0, 13, 13, 13, 13,  0 },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  0, 13,  1,  0, 13,  0,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  0, 13, 13, 13, 13, 13,  0,  T },
    {  T,  T,  T,  T,  T,  T,  T,  0, 13, 13,  1, 13, 13, 13,  0,  T },
    {  T,  T,  T,  T,  T,  T,  0, 13, 13,  1, 13, 13, 13,  0,  0,  T },
    {  T,  T,  T,  T,  0,  0,  0, 13,  1, 13, 13, 13,  0, 13,  0,  T },
    {  T,  T,  T,  0, 13,  0,  0,  1, 13, 13, 13, 13,  0, 13,  0,  T },
    {  T,  T,  T,  0,  0,  0, 13, 13, 13, 13,  0,  0,  T,  0,  T,  T },
    {  T,  T,  0,  0, 13,  0,  0,  0,  0,  0, 13,  0,  T,  T,  T,  T },
    {  T,  0,  1, 13, 13, 13,  0,  T,  T,  0,  0,  T,  T,  T,  T,  T },
    {  0,  1,  0,  0, 13, 13,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  0,  0,  T,  0,  1,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  0,  0,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T },
    {  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T }
};

#undef T

#endif /* FISHSPRITES_H */