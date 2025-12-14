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

#define DATA_BUFFER_SIZE 256

typedef enum
{
    CUBE_OK,
    CUBE_DISCONNECTED,
    CUBE_ERROR_TIMEOUT
} cube_status_t;

typedef enum
{
    CUBE_TOP = 0x01,
    CUBE_RIGHT = 0x02,
    CUBE_BOTTOM = 0x04,
    CUBE_LEFT = 0x08
} cube_side_t;

typedef void (*cube_data_callback_t)(cube_side_t cube_side, uint32_t *data, uint32_t length);
typedef void (*cube_connected_callback_t)(cube_side_t cube_side);
typedef void (*cube_disconnected_callback_t)(cube_side_t cube_side);
typedef void (*cube_error_callback_t)(cube_side_t cube_side, cube_status_t error_code);

void cube_init();
void cube_loop();
uint8_t cube_is_connected(cube_side_t cube_side);
cube_status_t cube_init_data_transfer(cube_side_t cube_side);
void cube_send_data(cube_side_t cube_side, uint32_t *data, uint32_t length);
cube_status_t cube_receive_data(cube_side_t cube_side, uint32_t *data, uint32_t max_length, uint32_t *length_received);

void cube_set_data_callback(cube_data_callback_t callback);
void cube_set_connected_callback(cube_connected_callback_t callback);
void cube_set_disconnected_callback(cube_disconnected_callback_t callback);
void cube_set_error_callback(cube_error_callback_t callback);
#endif