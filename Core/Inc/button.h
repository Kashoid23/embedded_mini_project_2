#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	BTN_STATE_IDLE = 0,
	BTN_STATE_DEBOUNCE_PRESS,
	BTN_STATE_PRESSED,
	BTN_STATE_DEBOUNCE_RELEASE,
} button_state_t;

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
	volatile bool edge_flag;
	button_state_t state;
	uint32_t t_start_ms;
	bool press_event;
} button_t;

void button_init(button_t *btn, GPIO_TypeDef *port, uint16_t pin);
void button_on_edge_irq(button_t *btn);
void button_update(button_t *btn, uint32_t now_ms);
bool button_take_press(button_t *btn);

#endif /* BUTTON_H */
