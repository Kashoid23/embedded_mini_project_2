#include "traffic_light.h"

#include <stdbool.h>

#include "config.h"
#include "led.h"
#include "relay.h"
#include "buzzer.h"

#define TL_FOREVER  (UINT32_MAX)

#define PATTERN_ALL_RED     { .on = { [LED_VEH_RED]    = true, [LED_PED_RED]   = true } }
#define PATTERN_VEH_GREEN   { .on = { [LED_VEH_GREEN]  = true, [LED_PED_RED]   = true } }
#define PATTERN_VEH_YELLOW  { .on = { [LED_VEH_YELLOW] = true, [LED_PED_RED]   = true } }
#define PATTERN_PED_GREEN   { .on = { [LED_VEH_RED]    = true, [LED_PED_GREEN] = true } }
#define PATTERN_ALL_DARK    { .on = { false } }

typedef struct {
	led_pattern_t pattern;
	led_id_t      blink_led;
	bool          sound;
	bool          accepts_request;
	bool          needs_request;
	uint32_t      duration_ms;
	tl_state_t    next;
} tl_state_desc_t;

static const tl_state_desc_t k_states[TL_STATE_COUNT] = {
	[TL_STATE_STARTUP] = {
		.pattern = PATTERN_ALL_RED,    .blink_led = LED_NONE,
		.accepts_request = true,
		.duration_ms = T_STARTUP_MS,          .next = TL_STATE_VEH_GREEN,
	},
	[TL_STATE_VEH_GREEN] = {
		.pattern = PATTERN_VEH_GREEN,  .blink_led = LED_NONE,
		.accepts_request = true,  .needs_request = true,
		.duration_ms = T_VEH_GREEN_MIN_MS,    .next = TL_STATE_VEH_YELLOW,
	},
	[TL_STATE_VEH_YELLOW] = {
		.pattern = PATTERN_VEH_YELLOW, .blink_led = LED_NONE,
		.duration_ms = T_VEH_YELLOW_MS,       .next = TL_STATE_ALL_RED_TO_PED,
	},
	[TL_STATE_ALL_RED_TO_PED] = {
		.pattern = PATTERN_ALL_RED,    .blink_led = LED_NONE,
		.duration_ms = T_ALL_RED_TO_PED_MS,   .next = TL_STATE_PED_GREEN,
	},
	[TL_STATE_PED_GREEN] = {
		.pattern = PATTERN_PED_GREEN,  .blink_led = LED_NONE,  .sound = true,
		.duration_ms = T_PED_GREEN_MS,        .next = TL_STATE_PED_GREEN_BLINK,
	},
	[TL_STATE_PED_GREEN_BLINK] = {
		.pattern = PATTERN_PED_GREEN,  .blink_led = LED_PED_GREEN,
		.duration_ms = T_PED_GREEN_BLINK_MS,  .next = TL_STATE_ALL_RED_TO_VEH,
	},
	[TL_STATE_ALL_RED_TO_VEH] = {
		.pattern = PATTERN_ALL_RED,    .blink_led = LED_NONE,
		.accepts_request = true,
		.duration_ms = T_ALL_RED_TO_VEH_MS,   .next = TL_STATE_VEH_GREEN,
	},
	[TL_STATE_SAFE_FLASH] = {
		.pattern = PATTERN_ALL_DARK,   .blink_led = LED_VEH_YELLOW,
		.duration_ms = TL_FOREVER,            .next = TL_STATE_SAFE_FLASH,
	},
};

typedef struct {
	tl_state_t          state;
	uint32_t            t_start_ms;
	volatile bool       crossing_requested;
	volatile tl_fault_t pending_fault;
	tl_fault_t          fault;
} tl_ctx_t;

static tl_ctx_t s_tl;

static void tl_enter(tl_state_t next, uint32_t now_ms)
{
	const tl_state_desc_t *desc = &k_states[next];

	s_tl.state      = next;
	s_tl.t_start_ms = now_ms;

	led_apply_pattern(&desc->pattern);

	if (desc->sound) {
		buzzer_start_crossing_sound(now_ms);
	} else {
		buzzer_stop();
	}
}

void tl_init(uint32_t now_ms)
{
	s_tl.crossing_requested = false;
	s_tl.pending_fault      = TL_FAULT_NONE;
	s_tl.fault              = TL_FAULT_NONE;

	tl_enter(TL_STATE_STARTUP, now_ms);
	relay_enable();
}

void tl_request_crossing(void)
{
	const tl_state_t state = s_tl.state;

	if ((state < TL_STATE_COUNT) && k_states[state].accepts_request) {
		s_tl.crossing_requested = true;
	}
}

void tl_set_fault(tl_fault_t fault)
{
	relay_disable();

	if (s_tl.pending_fault == TL_FAULT_NONE) {
		s_tl.pending_fault = fault;
	}
}

void tl_update(uint32_t now_ms)
{
	if ((s_tl.pending_fault != TL_FAULT_NONE) && (s_tl.state != TL_STATE_SAFE_FLASH)) {
		s_tl.fault = s_tl.pending_fault;
		tl_enter(TL_STATE_SAFE_FLASH, now_ms);
	}

	if (s_tl.state >= TL_STATE_COUNT) {
		tl_set_fault(TL_FAULT_ERROR);
		s_tl.fault = TL_FAULT_ERROR;
		tl_enter(TL_STATE_SAFE_FLASH, now_ms);
	}

	const tl_state_desc_t *desc     = &k_states[s_tl.state];
	const uint32_t         elapsed  = now_ms - s_tl.t_start_ms;
	const bool             blink_on = ((elapsed / T_BLINK_HALF_PERIOD_MS) % 2U) == 0U;

	if (desc->blink_led != LED_NONE) {
		led_set(desc->blink_led, blink_on);
	}

	if ((desc->duration_ms == TL_FOREVER) || (elapsed < desc->duration_ms)) {
		return;
	}

	if (desc->needs_request) {
		if (!s_tl.crossing_requested) {
			return;
		}
		s_tl.crossing_requested = false;
	}

	tl_enter(desc->next, now_ms);
}

tl_state_t tl_get_state(void)
{
	return s_tl.state;
}
