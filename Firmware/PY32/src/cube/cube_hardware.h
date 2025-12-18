#ifndef CUBE_HARDWARE_H
#define CUBE_HARDWARE_H

/* MCU-specific */
#ifndef PY32F002Bx5
#define PY32F002Bx5
#endif
#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f002b_ll_bus.h"
#include "py32f002b_ll_rcc.h"
#include "py32f002b_ll_system.h"
#include "py32f002b_ll_gpio.h"
#include "py32f002b_ll_utils.h"

#define HIGH 1
#define LOW 0

typedef enum
{
    CUBE_TOP = 0x01,
    CUBE_RIGHT = 0x02,
    CUBE_BOTTOM = 0x04,
    CUBE_LEFT = 0x08
} cube_side_t;

GPIO_TypeDef *cube_side_to_port1(cube_side_t cube_side);
uint32_t cube_side_to_pin1(cube_side_t cube_side);
GPIO_TypeDef *cube_side_to_port2(cube_side_t cube_side);
uint32_t cube_side_to_pin2(cube_side_t cube_side);

void cube_hardware_init();

static inline uint8_t read_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
    return (LL_GPIO_IsInputPinSet(gpio_port, gpio_pin)) ? HIGH : LOW;
}

static inline uint8_t wait_for_pin_state(GPIO_TypeDef *port, uint32_t pin, uint8_t desired_state, uint32_t timeout)
{
    uint8_t state = read_data_pin(port, pin);
    while (state != desired_state && timeout-- > 0)
    {
        state = read_data_pin(port, pin);
    }
    return state == desired_state;
}

cube_side_t cube_side_opposite(cube_side_t cube_side);
cube_side_t cube_side_rotate_clockwise(cube_side_t cube_side);
cube_side_t cube_side_rotate_counterclockwise(cube_side_t cube_side);
uint8_t cube_side_to_index(cube_side_t cube_side);

#endif // CUBE_HARDWARE_H
