#ifndef BUZZER_H
#define BUZZER_H

#include "stm32f4xx_hal.h"

#include <stdint.h>

void buzzer_init(TIM_HandleTypeDef *htim, uint32_t channel);
void buzzer_start_crossing_sound(uint32_t now_ms);
void buzzer_stop(void);
void buzzer_update(uint32_t now_ms);

#endif /* BUZZER_H */
