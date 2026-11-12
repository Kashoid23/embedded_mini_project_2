#include "led.h"

#include <stdint.h>

#include "main.h"

typedef struct {
	GPIO_TypeDef *port;
	uint16_t      pin;
} led_pin_t;

static const led_pin_t k_led_pins[LED_COUNT] = {
	[LED_VEH_RED]    = { VEH_RED_GPIO_Port,    VEH_RED_Pin    },
	[LED_VEH_YELLOW] = { VEH_YELLOW_GPIO_Port, VEH_YELLOW_Pin },
	[LED_VEH_GREEN]  = { VEH_GREEN_GPIO_Port,  VEH_GREEN_Pin  },
	[LED_PED_RED]    = { PED_RED_GPIO_Port,    PED_RED_Pin    },
	[LED_PED_GREEN]  = { PED_GREEN_GPIO_Port,  PED_GREEN_Pin  },
};

static void led_drive_matching(const led_pattern_t *pattern, bool level)
{
	for (uint32_t id = 0U; id < (uint32_t)LED_COUNT; id++) {
		if (pattern->on[id] == level) {
			led_set((led_id_t)id, level);
		}
	}
}

void led_set(led_id_t id, bool on)
{
	if (id >= LED_COUNT) {
		return;
	}
	HAL_GPIO_WritePin(k_led_pins[id].port, k_led_pins[id].pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_apply_pattern(const led_pattern_t *pattern)
{
	led_drive_matching(pattern, false);
	led_drive_matching(pattern, true);
}
