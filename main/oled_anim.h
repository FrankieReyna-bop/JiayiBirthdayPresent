#pragma once
#include <stdint.h>

// Provided by frames.c (auto-generated)
extern const uint8_t* animation_frames[];
extern const int animation_frame_count;

// Initialize and play animation
void oled_anim_init(void);
void oled_anim_play(int delay_ms);
void ssd1306_clear(void);