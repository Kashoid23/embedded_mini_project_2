#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdint.h>

typedef enum {
	TL_STATE_STARTUP = 0,
	TL_STATE_VEH_GREEN,
	TL_STATE_VEH_YELLOW,
	TL_STATE_ALL_RED_TO_PED,
	TL_STATE_PED_GREEN,
	TL_STATE_PED_GREEN_BLINK,
	TL_STATE_ALL_RED_TO_VEH,
	TL_STATE_SAFE_FLASH,
} tl_state_t;

typedef enum {
	TL_FAULT_NONE = 0,
	TL_FAULT_ERROR,
	TL_FAULT_WATCHDOG,
} tl_fault_t;

void tl_init(uint32_t now_ms);
void tl_update(uint32_t now_ms);
void tl_request_crossing(void);
void tl_set_fault(tl_fault_t fault, uint32_t now_ms);

tl_state_t tl_get_state(void);

#endif /* TRAFFIC_LIGHT_H */
