/**
 * @file FishingEngine.h
 * @brief Main game engine for the mini Stardew Fishing Simulator
 *
 * FSM-driven open-ended fishing game with a main menu and fish collection.
 * No time limit — the player fishes at their own pace and can return to the
 * menu at any time by pushing the joystick West.
 */

#ifndef FISHINGENGINE_H
#define FISHINGENGINE_H

#include <stdint.h>
#include "Fish.h"
#include "Joystick.h"

/**
 * @enum GameState
 * @brief All possible FSM states.
 */
typedef enum {
    STATE_MENU = 0,      // Main menu (Fishing / Collection)
    STATE_CASTING,       // Brief casting animation
    STATE_WAITING,       // Bobber on water, waiting for a bite
    STATE_FISH_BITE,     // Bobber dipped: short window to hook
    STATE_REELING,       // Reeling mini-game (tension + progress)
    STATE_CATCH_RESULT,  // Show the caught fish + points
    STATE_FISH_ESCAPED,  // "Got away!" screen
    STATE_COLLECTION,    // Fish collection / compendium viewer
    STATE_INSTRUCTION    //  How-to-play tutorial screen
} GameState;

/**
 * @struct FishingEngine_t
 * @brief Top-level engine state.
 */
typedef struct {
    GameState state;
    uint32_t  state_timer;    // ms since entering current state

    /* Session stats (reset each time the player starts fishing) */
    uint16_t  score;
    uint8_t   fish_caught;
    uint8_t   fish_escaped;
    uint8_t   lines_snapped;

    Fish      current_fish;      // Fish currently being fought / displayed

    float     catch_progress;    // 0.0 .. 1.0 (1.0 = caught)

    /* Persistent across sessions */
    uint8_t   caught_flags;  // Bit i set when species i has been caught at least once
    uint8_t   menu_cursor;   // 0 = Fishing, 1 = Collection
} FishingEngine_t;

/**
 * @brief One-time initialisation. Sets STATE_MENU and zeroes everything.
 */
void FishingEngine_Init(FishingEngine_t *engine);

/**
 * @brief Step the FSM by one frame.
 *
 * @param engine   Pointer to engine state
 * @param input    Joystick input this frame
 * @param button   1 if BTN2 was pressed this frame, 0 otherwise
 * @param delta_ms ms elapsed since previous call
 */
void FishingEngine_Update(FishingEngine_t *engine,
                          UserInput input,
                          uint8_t   button,
                          uint32_t  delta_ms);

/**
 * @brief Render the current frame into the LCD buffer.
 *
 * The caller must call LCD_Fill_Buffer() before and LCD_Refresh() after.
 */
void FishingEngine_Draw(FishingEngine_t *engine);

#endif /* FISHINGENGINE_H */
