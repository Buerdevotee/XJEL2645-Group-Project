/**
 * @file Fish.c
 * @brief Fish species table and weighted random selection.
 */

#include "Fish.h"
#include "FishSprites.h"
#include "Utils.h"   /* Random_U16 */

static const FishType fish_table[] = {
    /* --- Common --- */
    {
        .name          = "Carp",
        .rarity        = RARITY_COMMON,
        .points        = 20,
        .pull_strength = 0.30f,
        .min_size_cm   = 15,
        .max_size_cm   = 35,
        .sprite        = (const uint8_t *)fish_carp_color,
        .sprite_w      = FISH_CARP_COLOR_W,
        .sprite_h      = FISH_CARP_COLOR_H,
        .species_idx   = 0,
    },
    {
        .name          = "Sea Cucumber",
        .rarity        = RARITY_COMMON,
        .points        = 40,
        .pull_strength = 0.45f,
        .min_size_cm   = 10,
        .max_size_cm   = 30,
        .sprite        = (const uint8_t *)fish_sea_cucumber_color,
        .sprite_w      = FISH_SEA_CUCUMBER_COLOR_W,
        .sprite_h      = FISH_SEA_CUCUMBER_COLOR_H,
        .species_idx   = 1,
    },
    /* --- Rare --- */
    {
        .name          = "Tuna",
        .rarity        = RARITY_RARE,
        .points        = 100,
        .pull_strength = 0.65f,
        .min_size_cm   = 50,
        .max_size_cm   = 80,
        .sprite        = (const uint8_t *)fish_tuna_color,
        .sprite_w      = FISH_TUNA_COLOR_W,
        .sprite_h      = FISH_TUNA_COLOR_H,
        .species_idx   = 2,
    },
    {
        .name          = "Sturgeon",
        .rarity        = RARITY_RARE,
        .points        = 150,
        .pull_strength = 0.75f,
        .min_size_cm   = 60,
        .max_size_cm   = 100,
        .sprite        = (const uint8_t *)fish_sturgeon_color,
        .sprite_w      = FISH_STURGEON_COLOR_W,
        .sprite_h      = FISH_STURGEON_COLOR_H,
        .species_idx   = 3,
    },
    /* --- Legendary --- */
    {
        .name          = "Legend",
        .rarity        = RARITY_LEGENDARY,
        .points        = 300,
        .pull_strength = 1.00f,
        .min_size_cm   = 100,
        .max_size_cm   = 150,
        .sprite        = (const uint8_t *)fish_legend_color,
        .sprite_w      = FISH_LEGEND_COLOR_W,
        .sprite_h      = FISH_LEGEND_COLOR_H,
        .species_idx   = 4,
    },
};

Fish Fish_GenerateRandom(void)
{
    Fish f;

    uint8_t species_idx = (uint8_t)Random_U16(FISH_SPECIES_COUNT);
    f.type = fish_table[species_idx];

    /* Randomize size within the species' range */
    uint16_t span = (uint16_t)(f.type.max_size_cm - f.type.min_size_cm + 1);
    f.size_cm = (uint16_t)(f.type.min_size_cm + Random_U16(span));

    return f;
}

const FishType *Fish_GetType(uint8_t idx)
{
    if (idx >= FISH_SPECIES_COUNT) idx = 0;
    return &fish_table[idx];
}

const char *Fish_RarityName(FishRarity r)
{
    switch (r) {
        case RARITY_COMMON:    return "COMMON";
        case RARITY_RARE:      return "RARE";
        case RARITY_LEGENDARY: return "LEGENDARY";
        default:               return "?";
    }
}
