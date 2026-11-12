#ifndef LED_H
#define LED_H

#include <stdbool.h>

typedef enum {
	LED_VEH_RED = 0,
	LED_VEH_YELLOW,
	LED_VEH_GREEN,
	LED_PED_RED,
	LED_PED_GREEN,
	LED_COUNT,
	LED_NONE = LED_COUNT
} led_id_t;

typedef struct {
	bool on[LED_COUNT];
} led_pattern_t;

void led_set(led_id_t id, bool on);
void led_apply_pattern(const led_pattern_t *pattern);

#endif /* LED_H */
