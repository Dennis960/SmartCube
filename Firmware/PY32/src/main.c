#include "sk6812.h"
#include "hall.h"
#include "cube.h"
#include "cube_data.h"

#include <string.h>

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

void Error_Handler(void);

/**
 * Enables brown out reset at 3.1-3.2V
 * The device will reset when VDD drops below this threshold (to prevent hanging)
 */
static void enable_bor(void)
{
  FLASH_OBProgramInitTypeDef OBInitCfg;

  /* Read current option bytes */
  HAL_FLASH_OBGetConfig(&OBInitCfg);

  if ((OBInitCfg.USERConfig & (OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2)) != (OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2))
  {
    HAL_FLASH_Unlock();    /* Unlock FLASH */
    HAL_FLASH_OB_Unlock(); /* Unlock Option Bytes */

    /* Prepare only BOR configuration */
    OBInitCfg.OptionType = OPTIONBYTE_USER;
    OBInitCfg.USERType = OB_USER_BOR_EN | OB_USER_BOR_LEV;

    /* Enable BOR with level 3.1-3.2V and iwdg */
    OBInitCfg.USERConfig = OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2;

    /* Program the option bytes */
    HAL_FLASH_OBProgram(&OBInitCfg);

    HAL_FLASH_Lock();    /* Lock FLASH */
    HAL_FLASH_OB_Lock(); /* Lock Option Bytes */

    /* Launch reset to apply */
    HAL_FLASH_OB_Launch();
  }
}

/**
 * Converts a cube side to the corresponding LED pixel index.
 */
uint8_t cube_side_to_pixel(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return 0;
  case CUBE_RIGHT:
    return 1;
  case CUBE_BOTTOM:
    return 2;
  case CUBE_LEFT:
    return 3;
  default:
    return 0xFF; // Invalid
  }
}

static void SystemClock_Config(void)
{
  LL_RCC_HSI_Enable();
  while (!LL_RCC_HSI_IsReady())
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS)
  {
  }

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  LL_Init1msTick(24000000);
  LL_SetSystemCoreClock(24000000);
}

uint8_t data_to_send[12] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
uint32_t counter = 0;

void handle_hall_sensor_data_request(cube_side_t cube_side)
{
  cube_data_packet_t response_packet = {
      .type = DATA_TYPE_HALL_SENSOR_DATA,
      .data.hall_data = {
          .value = hall_read(),
      },
  };
  cube_send_data_packet(cube_side, &response_packet);
}

void handle_led_color_data_received(cube_side_t cube_side, led_color_data_t *led_data)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    sk6812_set_pixel(i,
                     led_data->pixels[i][0],
                     led_data->pixels[i][1],
                     led_data->pixels[i][2]);
  }
}

void handle_hall_sensor_data_received(cube_side_t cube_side, hall_sensor_data_t *hall_data)
{
  float brightness = hall_data->value * hall_data->value; // Square for better contrast
  uint8_t color_value = (uint8_t)(brightness * 255.0f);
  sk6812_set_pixel(cube_side_to_pixel(cube_side), 0, 0, color_value);
}

/**
 * Callback when data is received from another cube.
 * @param cube_side The side of the cube that sent the data
 * @param packet The received cube data packet
 */
void cube_data_received_callback(cube_side_t cube_side, cube_data_packet_t *packet)
{
  if (packet->type == DATA_TYPE_REQUEST_HALL_SENSOR_DATA)
  {
    handle_hall_sensor_data_request(cube_side);
  }
  else if (packet->type == DATA_TYPE_LED_COLOR)
  {
    handle_led_color_data_received(cube_side, &packet->data.led_data);
  }
  else if (packet->type == DATA_TYPE_HALL_SENSOR_DATA)
  {
    handle_hall_sensor_data_received(cube_side, &packet->data.hall_data);
  }
}

/**
 * Callback when an error occurs during communication in the cube_loop.
 * @param cube_side The side of the cube where the error occurred
 * @param cube_status The status code, one of CUBE_ERROR_TIMEOUT
 */
void cube_error_callback(cube_side_t cube_side, cube_status_t cube_status)
{
  sk6812_set_pixel(cube_side_to_pixel(cube_side), 1, 0, 1);
  sk6812_show(1);
}

/**
 * Callback when a cube is connected.
 * @param cube_side The side of the cube that was connected
 */
void cube_connected_callback(cube_side_t cube_side)
{
  sk6812_set_pixel(cube_side_to_pixel(cube_side), 0, 1, 0);
}

/**
 * Callback when a cube is disconnected.
 * @param cube_side The side of the cube that was disconnected
 */
void cube_disconnected_callback(cube_side_t cube_side)
{
  sk6812_set_pixel(cube_side_to_pixel(cube_side), 1, 0, 0);
}

int main(void)
{
  HAL_StatusTypeDef status = HAL_Init();
  enable_bor();
  SystemClock_Config();
  sk6812_init(GPIOB, LL_GPIO_PIN_2, 4);
  sk6812_clear();
  sk6812_show(1);

  cube_hardware_init();

  cube_set_data_callback(cube_data_received_callback);
  cube_set_error_callback(cube_error_callback);
  cube_set_connected_callback(cube_connected_callback);
  cube_set_disconnected_callback(cube_disconnected_callback);

  hall_init();

  uint8_t any_cube_connected = 0;

  while (!any_cube_connected)
  {
    for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
    {
      if (!cube_is_connected(cube_side))
        continue;
      // TODO: request the x,y position of this cube in the grid
      // cube_data_packet_t request_position_packet = {
      //     .type = DATA_TYPE_REQUEST_POSITION,
      // };
      // cube_send_data_packet(cube_side, &request_position_packet);
      any_cube_connected = 1;
    }
  }

  cube_set_idle();

  while (1)
  {
    cube_loop();
    sk6812_show(1);
    cube_data_packet_t packet = {
        .type = DATA_TYPE_REQUEST_HALL_SENSOR_DATA,
    };
    for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
    {
      if (cube_is_connected(cube_side))
      {
        cube_send_data_packet(cube_side, &packet);
      }
    }
  }
}
