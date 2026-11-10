#include <button.h>
#include <config.h>

static bool button_is_down(const button_t *btn) {
	return (HAL_GPIO_ReadPin(btn->port, btn->pin) == GPIO_PIN_RESET);
}

void button_init(button_t *btn, GPIO_TypeDef *port, uint16_t pin) {
	btn->port 		 = port;
	btn->pin 		 = pin;
	btn->edge_flag 	 = false;
	btn->state 		 = BTN_STATE_IDLE;
	btn->t_start_ms	 = 0;
	btn->press_event = false;
}

void button_on_edge_irq(button_t *btn) {
	btn->edge_flag = true;
}

void button_update(button_t *btn, uint32_t now_ms) {
	switch (btn->state) {
		case BTN_STATE_IDLE:
			if (btn->edge_flag) {
				btn->edge_flag = false;
				btn->state = BTN_STATE_DEBOUNCE_PRESS;
				btn->t_start_ms = now_ms;
			}
			break;

		case BTN_STATE_DEBOUNCE_PRESS:
			if ((now_ms - btn->t_start_ms) >= T_DEBOUNCE_MS) {
				if (button_is_down(btn)) {
					btn->press_event = true;
					btn->state = BTN_STATE_PRESSED;
				} else {
					btn->state = BTN_STATE_IDLE;
				}
			}
			break;

		case BTN_STATE_PRESSED:
			if (!button_is_down(btn)) {
				btn->t_start_ms = now_ms;
				btn->state = BTN_STATE_DEBOUNCE_RELEASE;
			}
			break;

		case BTN_STATE_DEBOUNCE_RELEASE:
			if ((now_ms - btn->t_start_ms) >= T_DEBOUNCE_MS) {
				if (button_is_down(btn)) {
					btn->state = BTN_STATE_PRESSED;
				} else {
					btn->edge_flag = false;
					btn->state = BTN_STATE_IDLE;
				}

			}
			break;

		default:
			btn->state = BTN_STATE_IDLE;
			break;
	}
}

bool button_take_press(button_t *btn) {
	const bool pressed = btn->press_event;
	btn->press_event = false;
	return pressed;
}
