#include "traffic_light.h"
#include "main.h"
#include "config.h"
#include "buzzer.h"

#include <stdbool.h>

#ifndef RELAY_ACTIVE_LOW
#error "RELAY_ACTIVE_LOW must be defined in config.h (1U or 0U)"
#endif

#if (RELAY_ACTIVE_LOW == 1U)
#define RELAY_LEVEL_ON GPIO_PIN_RESET
#define RELAY_LEVEL_OFF GPIO_PIN_SET
#else
#define RELAY_LEVEL_ON GPIO_PIN_SET
#define RELAY_LEVEL_OFF GPIO_PIN_RESET
#endif

typedef enum {
	LED_VEH_RED = 0,
	LED_VEH_YELLOW,
	LED_VEH_GREEN,
	LED_PED_RED,
	LED_PED_GREEN,
	LED_COUNT
} led_id_t;

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} led_t;

typedef struct {
	bool active[LED_COUNT];
} tl_led_pattern_t;

static const led_t k_leds[LED_COUNT] = {
	[LED_VEH_RED] 	= { VEH_RED_GPIO_Port, 	VEH_RED_Pin },
	[LED_VEH_YELLOW] = { VEH_YELLOW_GPIO_Port, VEH_YELLOW_Pin },
	[LED_VEH_GREEN] = { VEH_GREEN_GPIO_Port, VEH_GREEN_Pin },
	[LED_PED_RED] 	= { PED_RED_GPIO_Port, 	PED_RED_Pin },
	[LED_PED_GREEN] = { PED_GREEN_GPIO_Port, PED_GREEN_Pin },
};

static const tl_led_pattern_t k_all_red = { .active = { [LED_VEH_RED] = true, [LED_PED_RED] = true } };
static const tl_led_pattern_t k_veh_green = { .active = { [LED_VEH_GREEN] = true, [LED_PED_RED] = true } };
static const tl_led_pattern_t k_veh_yellow = { .active = { [LED_VEH_YELLOW] = true, [LED_PED_RED] = true } };
static const tl_led_pattern_t k_ped_green = { .active = { [LED_VEH_RED] = true, [LED_PED_GREEN] = true } };
static const tl_led_pattern_t k_all_dark = { .active = { false } };

typedef struct {
	tl_state_t state;
	uint32_t t_start_ms;
	bool crossing_requested;
	tl_fault_t fault;
} tl_ctx_t;

static tl_ctx_t s_tl;

static void led_set (led_id_t id, bool active) {
	HAL_GPIO_WritePin(k_leds[id].port, k_leds[id].pin, active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void leds_set_state (const tl_led_pattern_t *pattern, bool state) {
    for (uint32_t id = 0U; id < (uint32_t)LED_COUNT; id++) {
        if (pattern->active[id] == state) {
            led_set((led_id_t)id, state);
        }
    }
}

static void leds_set (const tl_led_pattern_t *pattern) {
	leds_set_state(pattern, false);
	leds_set_state(pattern, true);
}

static void relay_set (bool active) {
	HAL_GPIO_WritePin(RELAY_EN_GPIO_Port, RELAY_EN_Pin, active ? RELAY_LEVEL_ON : RELAY_LEVEL_OFF);
}

static void wait_led_set (bool active) {
	led_set(LED_VEH_YELLOW, active);
}

static void led_fsm (tl_state_t next, uint32_t now_ms) {
	s_tl.state = next;
	s_tl.t_start_ms = now_ms;

	switch (next) {
		case TL_STATE_STARTUP:
		case TL_STATE_ALL_RED_TO_PED:
		case TL_STATE_ALL_RED_TO_VEH:
			buzzer_stop();
			leds_set(&k_all_red);
			break;

		case TL_STATE_VEH_GREEN:
			leds_set(&k_veh_green);
			break;

		case TL_STATE_VEH_YELLOW:
			leds_set(&k_veh_yellow);
			break;

		case TL_STATE_PED_GREEN:
			s_tl.crossing_requested = false;
			leds_set(&k_ped_green);
			buzzer_start_crossing_sound(now_ms);
			break;

		case TL_STATE_PED_GREEN_BLINK:
			buzzer_stop();
			break;

		case TL_STATE_SAFE_FLASH:
			leds_set(&k_all_dark);
			break;

		default:
			break;
	}
}

void tl_init (uint32_t now_ms) {
	s_tl.crossing_requested = false;
	s_tl.fault = TL_FAULT_NONE;

	leds_set(&k_all_red);
	relay_set(true);
	led_fsm(TL_STATE_STARTUP, now_ms);
}

void tl_request_crossing (void) {
	const bool accepted = (s_tl.state == TL_STATE_STARTUP)   ||
	                      (s_tl.state == TL_STATE_VEH_GREEN) ||
	                      (s_tl.state == TL_STATE_ALL_RED_TO_VEH);

	if (accepted) {
		s_tl.crossing_requested = true;
	}
}

void tl_set_fault (tl_fault_t fault, uint32_t now_ms) {
	if (s_tl.state == TL_STATE_SAFE_FLASH) {
		return;
	}

	relay_set(false);
	buzzer_stop();
	s_tl.crossing_requested = false;
	s_tl.fault = fault;
	led_fsm(TL_STATE_SAFE_FLASH, now_ms);
}

void tl_update (uint32_t now_ms) {
	const uint32_t elapsed = now_ms - s_tl.t_start_ms;
	const bool blink_on = ((elapsed / T_BLINK_HALF_PERIOD_MS) % 2U) == 0U;

	switch (s_tl.state) {
		case TL_STATE_STARTUP:
			if (elapsed >= T_STARTUP_MS) {
				led_fsm(TL_STATE_VEH_GREEN, now_ms);
			}
			break;

		case TL_STATE_VEH_GREEN:
			if (s_tl.crossing_requested && (elapsed >= T_VEH_GREEN_MIN_MS)) {
				led_fsm(TL_STATE_VEH_YELLOW, now_ms);
			}
			break;

		case TL_STATE_VEH_YELLOW:
			if (elapsed >= T_VEH_YELLOW_MS) {
				led_fsm(TL_STATE_ALL_RED_TO_PED, now_ms);
			}
			break;

		case TL_STATE_ALL_RED_TO_PED:
			if (elapsed >= T_ALL_RED_TO_PED_MS) {
				led_fsm(TL_STATE_PED_GREEN, now_ms);
			}
			break;

		case TL_STATE_PED_GREEN:
			if (elapsed >= T_PED_GREEN_MS) {
				led_fsm(TL_STATE_PED_GREEN_BLINK, now_ms);
			}
			break;

		case TL_STATE_PED_GREEN_BLINK:
			led_set(LED_PED_GREEN, blink_on);
			if (elapsed >= T_PED_GREEN_BLINK_MS) {
				led_fsm(TL_STATE_ALL_RED_TO_VEH, now_ms);
			}
			break;

		case TL_STATE_ALL_RED_TO_VEH:
			if (elapsed >= T_ALL_RED_TO_VEH_MS) {
				led_fsm(TL_STATE_VEH_GREEN, now_ms);
			}
			break;

		case TL_STATE_SAFE_FLASH:
			wait_led_set(blink_on);
			break;

		default:
			tl_set_fault(TL_FAULT_ERROR, now_ms);
			break;
	}
}

tl_state_t tl_get_state (void) {
	return s_tl.state;
}
