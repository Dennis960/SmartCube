#ifndef DATA_H
#define DATA_H

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

void data_init();
void data_set_idle();
uint8_t data_read_dt1();
uint8_t data_read_dr1();
uint8_t data_read_db1();
uint8_t data_read_dl1();
#endif