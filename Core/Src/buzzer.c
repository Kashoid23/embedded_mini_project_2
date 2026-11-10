#include "buzzer.h"
#include "config.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	uint16_t freq_hz;
	uint16_t dur_ms;
} tone_step_t;

static const tone_step_t crossing_tone[] = {
	{ 2800U, 26U }, { 2550U, 26U }, { 2300U, 26U }, { 2050U, 26U }, { 1800U, 26U },
	{ 0U, 1170U },
	{ 2800U, 26U }, { 2550U, 26U }, { 2300U, 26U }, { 2050U, 26U }, { 1800U, 26U },
	{ 0U, 90U },
	{ 2800U, 26U }, { 2550U, 26U }, { 2300U, 26U }, { 2050U, 26U }, { 1800U, 26U },
	{ 0U, 1150U },
};

#define CROSSING_TONE_STEPS_COUNT (sizeof(crossing_tone) / sizeof(crossing_tone[0]))

static TIM_HandleTypeDef *s_htim 	= NULL;
static uint32_t s_channel 			= 0U;
static bool s_active 				= false;
static size_t s_step  				= 0U;
static uint32_t s_t_step_ms			= 0U;

static void buzzer_set_tone (uint16_t freq_hz) {
	if (s_htim == NULL) {
		return;
	}

	if (freq_hz == 0U) {
		__HAL_TIM_SET_COMPARE(s_htim, s_channel, 0U);
	} else {
		const uint32_t arr = (BUZZER_TIMER_TICK_HZ / freq_hz) - 1U;
		__HAL_TIM_SET_AUTORELOAD(s_htim, arr);
		__HAL_TIM_SET_COMPARE(s_htim, s_channel, (arr + 1U) / 2U);
		__HAL_TIM_SET_COUNTER(s_htim, 0U);
	}
}

void buzzer_init (TIM_HandleTypeDef *htim, uint32_t channel) {
	s_htim = htim;
	s_channel = channel;
	s_active = false;
	buzzer_set_tone(0U);
	(void)HAL_TIM_PWM_Start(s_htim, s_channel);
}

void buzzer_start_crossing_sound(uint32_t now_ms) {
	s_active = true;
	s_step = 0U;
	s_t_step_ms = now_ms;
	buzzer_set_tone(crossing_tone[s_step].freq_hz);
}

void buzzer_stop(void) {
	s_active = false;
	buzzer_set_tone(0U);
}

void buzzer_update (uint32_t now_ms) {
	if (!s_active) {
		return;
	}

	if ((now_ms - s_t_step_ms) >= crossing_tone[s_step].dur_ms) {
		s_t_step_ms += crossing_tone[s_step].dur_ms;
		s_step = (s_step + 1U) % CROSSING_TONE_STEPS_COUNT;
		buzzer_set_tone(crossing_tone[s_step].freq_hz);
	}
}
