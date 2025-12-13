#ifndef SK6812_H
#define SK6812_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* MCU-specific */
#ifndef PY32F002Bx5
#define PY32F002Bx5
#endif
#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "py32f0xx.h"
#include "py32f002b_ll_bus.h"
#include "py32f002b_ll_rcc.h"
#include "py32f002b_ll_system.h"
#include "py32f002b_ll_gpio.h"
#include "py32f002b_ll_utils.h"

/* Public API */
void sk6812_init(GPIO_TypeDef *port, uint32_t gpio_pin, uint16_t count);
void sk6812_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void sk6812_get_pixel(uint16_t index, uint8_t *r, uint8_t *g, uint8_t *b);
void sk6812_fill(uint8_t r, uint8_t g, uint8_t b);
void sk6812_clear(void);
/**
 * Shows the current LED data.
 * @param ignore_not_dirty If set to non-zero, the data will be sent even if no changes were made.
 */
void sk6812_show(uint8_t ignore_not_dirty);
void sk6812_deinit(void);

#endif
