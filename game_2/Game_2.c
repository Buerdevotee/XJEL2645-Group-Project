#include "Game_2.h"
#include "FishingEngine.h"
#include "InputHandler.h"
#include "Joystick.h"
#include "LCD.h"
#include "Buzzer.h"
#include "PWM.h"
#include "stm32l4xx_hal.h"

extern ST7789V2_cfg_t  cfg0;
extern Joystick_cfg_t  joystick_cfg;
extern Joystick_t      joystick_data;
extern Buzzer_cfg_t    buzzer_cfg;
extern PWM_cfg_t       pwm_cfg;

MenuState Game2_Run(void)
{
    static FishingEngine_t engine;
    FishingEngine_Init(&engine);

    uint32_t prev_tick = HAL_GetTick();

    while (1) {
        uint32_t now = HAL_GetTick();
        uint32_t dt = now - prev_tick;
        if (dt == 0) dt = 1;
        prev_tick = now;

        // input
        Input_Read();
        Joystick_Read(&joystick_cfg, &joystick_data);

        // BT3: exit to main menu at any time
        if (current_input.btn3_pressed) {
            return MENU_STATE_HOME;
        }

        // Build UserInput (BT2 = fishing action button)
        UserInput in = {
            .direction = joystick_data.direction,
            .magnitude = joystick_data.magnitude
        };
        uint8_t btn = current_input.btn2_pressed;

        // Update game engine
        FishingEngine_Update(&engine, in, btn, dt);

        // Render frame
        LCD_clear();
        FishingEngine_Draw(&engine);
        LCD_Refresh(&cfg0);
    }
}
