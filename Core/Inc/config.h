#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define T_STARTUP_MS 			3000U
#define T_VEH_GREEN_MIN_MS 		10000U
#define T_VEH_YELLOW_MS 		3000U
#define T_ALL_RED_TO_PED_MS		2000U
#define T_PED_GREEN_MS	 		8000U
#define T_PED_GREEN_BLINK_MS 	4000U
#define T_ALL_RED_TO_VEH_MS 	2000U

#define T_BLINK_HALF_PERIOD_MS 	500U
#define T_DEBOUNCE_MS 			20U

#define RELAY_ACTIVE_LOW		1U
#define BUZZER_TIMER_TICK_HZ	1000000U

#endif /* CONFIG_H */
