#ifndef CUBE_H
#define CUBE_H

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

typedef enum
{
    CUBE_STATE_INIT,       /**< The cube was just connected to power and is in the initialization phase */
    CUBE_STATE_IDLE,       /**< The cube is idle, waiting for further instructions */
    CUBE_STATE_ERROR,      /**< The cube has encountered an error and is in an error state */
    CUBE_STATE_SOFT_ERROR, /**< The cube has encountered a recoverable error and will go back into idle mode soon*/
    CUBE_STATE_BUSY        /**< The cube is busy performing an operation */
} cube_state_t;

typedef enum
{
    CUBE_TOP = 0x01,
    CUBE_RIGHT = 0x02,
    CUBE_BOTTOM = 0x04,
    CUBE_LEFT = 0x08
} cube_side_t;

/**
 * Represents the current state of the cube.
 */
extern cube_state_t cube_state;

extern uint8_t cube_is_master;

void cube_init();
void cube_set_idle();
void cube_loop(uint32_t *data, uint32_t length);
uint32_t cube_init_data_transfer(cube_side_t cube_side);
void cube_send_data(cube_side_t cube_side, uint32_t *data, uint32_t length);
uint32_t cube_receive_data(cube_side_t cube_side, uint32_t *data, uint32_t max_length);
#endif