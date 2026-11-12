#include "relay.h"

#include "main.h"
#include "config.h"

#ifndef RELAY_ACTIVE_LOW
#error "RELAY_ACTIVE_LOW must be defined in config.h (1U or 0U)"
#endif

#if (RELAY_ACTIVE_LOW == 1U)
#define RELAY_LEVEL_ON   GPIO_PIN_RESET
#define RELAY_LEVEL_OFF  GPIO_PIN_SET
#else
#define RELAY_LEVEL_ON   GPIO_PIN_SET
#define RELAY_LEVEL_OFF  GPIO_PIN_RESET
#endif

void relay_enable(void)
{
	HAL_GPIO_WritePin(RELAY_EN_GPIO_Port, RELAY_EN_Pin, RELAY_LEVEL_ON);
}

void relay_disable(void)
{
	HAL_GPIO_WritePin(RELAY_EN_GPIO_Port, RELAY_EN_Pin, RELAY_LEVEL_OFF);
}
