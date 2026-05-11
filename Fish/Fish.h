/**
 * @file Fish.h
 * @brief Fish type definitions and weighted random selection
 *
 * Defines five species of fish (common -> legendary) plus a Fish instance
 * struct that bundles a chosen FishType with a randomized size. Used by
 * the FishingEngine to drive the reeling mini-game and scoring.
 */

#ifndef FISH_H
#define FISH_H

#include <stdint.h>

/**
 * @enum FishRarity
 * @brief Drop-rate tier for fish selection.
 */
typedef enum {
    RARITY_COMMON = 0,
    RARITY_RARE,
    RARITY_LEGENDARY
} FishRarity;

/**
 * @struct FishType
 * @brief Static, read-only description of a fish species.
 *
 * One instance per species lives in flash (see fish_table[] in Fish.c).
 */
typedef struct {
    const char *name;          // Display name
    FishRarity  rarity;        // Drop tier
    uint16_t    points;        // Score awarded on catch
    float       pull_strength; // 0.0..1.0 - how aggressively the fish pulls
    uint16_t    min_size_cm;   // Lower bound for randomized size
    uint16_t    max_size_cm;   // Upper bound for randomized size
    const uint8_t *sprite;     // Pointer to sprite row-major data
    uint8_t     sprite_w;      // Sprite width  in pixels
    uint8_t     sprite_h;      // Sprite height in pixels
    uint8_t     species_idx;   // Index into fish_table[] (0-4), used for caught_flags
} FishType;

/**
 * @struct Fish
 * @brief A specific instance of a fish - a FishType plus randomized size.
 */
typedef struct {
    FishType type;
    uint16_t size_cm;
} Fish;

/**
 * @brief Generate a random fish using weighted rarity probabilities.
 *
 * Uses the hardware RNG (via Random_U16). Common fish are most likely,
 * legendary the rarest. The chosen species is then assigned a random
 * size in [min_size_cm, max_size_cm].
 *
 * @return Fully populated Fish ready to be displayed/caught.
 */
#define FISH_SPECIES_COUNT 5

Fish Fish_GenerateRandom(void);

/**
 * @brief Get a fish type by species index (0 to FISH_SPECIES_COUNT-1).
 */
const FishType *Fish_GetType(uint8_t idx);

/**
 * @brief Get a human-readable rarity label (e.g. "RARE").
 */
const char *Fish_RarityName(FishRarity r);

#endif /* FISH_H */
