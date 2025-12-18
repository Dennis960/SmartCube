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
int8_t cube_position_x = 0;
int8_t cube_position_y = 0;
int8_t cube_position_origin_x = -1;
int8_t cube_position_origin_y = 0;
cube_side_t cube_position_origin_side = CUBE_LEFT; // The side of this cube where the origin is located

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

cube_side_t opposite_side(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return CUBE_BOTTOM;
  case CUBE_RIGHT:
    return CUBE_LEFT;
  case CUBE_BOTTOM:
    return CUBE_TOP;
  case CUBE_LEFT:
    return CUBE_RIGHT;
  default:
    return CUBE_TOP; // Invalid
  }
}
cube_side_t rotate_side_clockwise(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return CUBE_RIGHT;
  case CUBE_RIGHT:
    return CUBE_BOTTOM;
  case CUBE_BOTTOM:
    return CUBE_LEFT;
  case CUBE_LEFT:
    return CUBE_TOP;
  default:
    return CUBE_TOP; // Invalid
  }
}
cube_side_t rotate_side_counterclockwise(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return CUBE_LEFT;
  case CUBE_RIGHT:
    return CUBE_TOP;
  case CUBE_BOTTOM:
    return CUBE_RIGHT;
  case CUBE_LEFT:
    return CUBE_BOTTOM;
  default:
    return CUBE_TOP; // Invalid
  }
}

void handle_position_data_request(cube_side_t cube_side)
{
  int8_t new_x, new_y;
  int8_t delta_x = cube_position_x - cube_position_origin_x;
  int8_t delta_y = cube_position_y - cube_position_origin_y;
  // Determine new position based on which side the request came from
  if (cube_side == opposite_side(cube_position_origin_side))
  {
    new_x = cube_position_x + delta_x;
    new_y = cube_position_y + delta_y;
  }
  else if (cube_side == rotate_side_clockwise(cube_position_origin_side))
  {
    new_x = cube_position_x - delta_y;
    new_y = cube_position_y + delta_x;
  }
  else if (cube_side == rotate_side_counterclockwise(cube_position_origin_side))
  {
    new_x = cube_position_x + delta_y;
    new_y = cube_position_y - delta_x;
  }
  else
  {
    // Same side, return origin
    new_x = cube_position_origin_x;
    new_y = cube_position_origin_y;
  }
  cube_data_packet_t response_packet = {
      .type = DATA_TYPE_POSITION_DATA,
      .data.position_data = {
          .origin_x = cube_position_x,
          .origin_y = cube_position_y,
          .x = new_x,
          .y = new_y,
      },
  };
  cube_send_data_packet(cube_side, &response_packet);
}

void handle_position_data_received(cube_side_t cube_side, position_data_t *position_data)
{
  cube_position_x = position_data->x;
  cube_position_y = position_data->y;
  cube_position_origin_x = position_data->origin_x;
  cube_position_origin_y = position_data->origin_y;
  cube_position_origin_side = cube_side;
  uint8_t brightnesses[3] = {0xFF, 0x80, 0x00};
  int8_t sum = cube_position_x + cube_position_y;
  uint8_t index = ((sum % 3) + 3) % 3;
  sk6812_fill(brightnesses[index],
              brightnesses[2 - index],
              0);
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
  else if (packet->type == DATA_TYPE_REQUEST_POSITION_DATA)
  {
    handle_position_data_request(cube_side);
  }
  else if (packet->type == DATA_TYPE_POSITION_DATA)
  {
    handle_position_data_received(cube_side, &packet->data.position_data);
  }
}

/**
 * Show a binary code from 0-15 on the SK6812 LEDs for debugging.
 * Each bit corresponds to one LED pixel (LSB = pixel 0).
 * @param code The binary code to show (0-15)
 */
void sk6812_show_binary_code(uint8_t code)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    if (code & (1 << i))
    {
      sk6812_set_pixel(i, 1, 1, 1);
    }
    else
    {
      sk6812_set_pixel(i, 0, 0, 0);
    }
  }
  sk6812_show(1);
}

/**
 * Callback when an error occurs during communication in the cube_loop.
 * @param cube_side The side of the cube where the error occurred
 * @param status The status code, one of CUBE_DATA_TRANSMISSION_ERROR_DESERIALIZATION, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT
 * @param packet The data packet involved in the error, if any, else NULL
 */
void cube_error_callback(cube_side_t cube_side, cube_data_transmission_status_t status, cube_data_packet_t *packet)
{
  if (status == CUBE_DATA_TRANSMISSION_ERROR_DESERIALIZATION)
    sk6812_show_binary_code(1);
  else if (status == CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT)
    sk6812_show_binary_code(2);
  else if (status == CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT)
    sk6812_show_binary_code(3);
  else if (status == CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT)
    sk6812_show_binary_code(4);
  else if (status == CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT)
    sk6812_show_binary_code(5);
  else
    sk6812_show_binary_code(0xF);
}

/**
 * Callback when a connection error occurs.
 * @param cube_side The side of the cube where the connection error occurred
 * @param status The connection status code, one of CUBE_CONNECTION_ERROR_ANNOUNCE_TIMEOUT, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_PRESENCE_TIMEOUT, CUBE_CONNECTION_ERROR_INIT_HANDSHAKE_TIMEOUT, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_HANDSHAKE_TIMEOUT, CUBE_CONNECTION_ERROR_COMPLETE_HANDSHAKE_TIMEOUT
 */
void cube_connection_error_callback(cube_side_t cube_side, cube_connection_status_t status)
{
  if (status == CUBE_CONNECTION_ERROR_ANNOUNCE_TIMEOUT)
  {
    sk6812_show_binary_code(6);
  }
  else if (status == CUBE_CONNECTION_ERROR_ACKNOWLEDGE_PRESENCE_TIMEOUT)
  {
    sk6812_show_binary_code(7);
    LL_mDelay(1000);
  }
  else if (status == CUBE_CONNECTION_ERROR_INIT_HANDSHAKE_TIMEOUT)
  {
    sk6812_show_binary_code(8);
    LL_mDelay(1000);
  }
  else if (status == CUBE_CONNECTION_ERROR_ACKNOWLEDGE_HANDSHAKE_TIMEOUT)
  {
    sk6812_show_binary_code(9);
    LL_mDelay(1000);
  }
  else if (status == CUBE_CONNECTION_ERROR_COMPLETE_HANDSHAKE_TIMEOUT)
  {
    sk6812_show_binary_code(10);
    LL_mDelay(1000);
  }
  else
  {
    sk6812_show_binary_code(0xE);
  }
}

/**
 * Callback when a cube is connected.
 * @param cube_side The side of the cube that was connected
 */
void cube_connected_callback(cube_side_t cube_side)
{
  // sk6812_set_pixel(cube_side_to_pixel(cube_side), 0, 1, 0);
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

  cube_hardware_init();

  cube_set_data_callback(cube_data_received_callback);
  cube_set_error_callback(cube_error_callback);
  cube_set_connected_callback(cube_connected_callback);
  cube_set_disconnected_callback(cube_disconnected_callback);
  cube_set_connection_error_callback(cube_connection_error_callback);

  hall_init();

  cube_side_t cube_side = wait_for_cube_connection();

  cube_data_packet_t request_position_packet = {
      .type = DATA_TYPE_REQUEST_POSITION_DATA,
  };
  cube_send_data_packet(cube_side, &request_position_packet);

  while (1)
  {
    cube_loop();
    sk6812_show(1);
  }
}
