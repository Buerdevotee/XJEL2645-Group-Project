# XJEL2645 STM32 Games Console

A multi-game embedded games console built on the **STM32L476 Nucleo board** for the XJEL2645 Embedded Systems module. Three individual games are combined into a single project via a shared menu system, all running on a 240×240 colour LCD.

---

## Games

### Game 1 — Block Defense
> by Zekai Deng

A fast-paced vertical defence game. Control your character to deflect or destroy incoming blocks before they reach the bottom.

### Game 2 — mini Stardew Fishing Simulator
> by Liangyu Mei

A fishing mini-game inspired by Stardew Valley. Cast your line, react when the fish bites, then keep the fish icon inside a moving green bar to reel it in. Features five fish species across three rarity tiers (Common, Rare, Legendary), a persistent fish collection compendium, score tracking, and audio/LED feedback.

### Game 3 — Temple Run 3D Edition
> by Junyi Jiang

A real-time 3D perspective runner rendered entirely on the STM32. Dodge incoming obstacles as the environment scrolls toward you.

---

## Hardware

| Component | Details |
|-----------|---------|
| MCU | STM32L476RG (Nucleo-L476RG) |
| Display | ST7789V2 240×240 IPS LCD, SPI + DMA |
| Input | Analogue joystick (ADC), push button (EXTI) |
| Audio | PWM buzzer (TIM2) |
| LED feedback | PWM LED (TIM4) |
| RNG | Hardware random number generator |

---

## Project Structure

    ├── Core/                        # STM32 HAL init, main loop, GPIO, ADC, timers
    ├── Fish/                        # Fish species definitions and random generation
    ├── FishingEngine/               # Game FSM, physics, and rendering (Game 2)
    ├── Joystick/                    # Analogue joystick driver with circle mapping
    ├── Buzzer/                      # Non-blocking PWM buzzer driver
    ├── PWM/                         # PWM utility for LED feedback
    ├── ST7789V2_Driver_STM32L4/     # LCD driver and drawing API
    ├── Drivers/                     # STM32 HAL drivers
    └── CMakeLists.txt
