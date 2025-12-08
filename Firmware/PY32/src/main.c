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

void Error_Handler(void);

static void SystemClock_Config(void);
static void GPIO_Init(void);

static inline void delay_cycles(volatile uint32_t cycles)
{
  while (cycles--)
  {
    __NOP();
  }
}

static inline void send_bit(uint8_t bit)
{
  if (bit)
  {
    GPIOB->BSRR = LL_GPIO_PIN_2;
    __NOP();
    __NOP();
    __NOP();
    GPIOB->BRR = LL_GPIO_PIN_2;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
  }
  else
  {
    GPIOB->BSRR = LL_GPIO_PIN_2;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    GPIOB->BRR = LL_GPIO_PIN_2;
    __NOP();
    __NOP();
    __NOP();
  }
}

static inline void send_byte(uint8_t byte)
{
  for (int i = 0; i < 8; i++)
  {
    send_bit((byte >> (7 - i)) & 0x01);
  }
}

void send_led_data(uint8_t r, uint8_t g, uint8_t b)
{
  send_byte(255-g); // GRB order
  send_byte(255-r);
  send_byte(255-b);
}

void latch_data()
{
  GPIOB->BRR = LL_GPIO_PIN_2;
  LL_mDelay(1);
}

int main(void)
{
  SystemClock_Config();
  GPIO_Init();

  while (1)
  {
    send_led_data(1, 0, 0);
    send_led_data(0, 1, 0);
    send_led_data(0, 0, 1);
    send_led_data(1, 1, 1);
    latch_data();

    LL_mDelay(500);

    send_led_data(0, 0, 0);
    send_led_data(0, 0, 0);
    send_led_data(0, 0, 0);
    send_led_data(0, 0, 0);
    latch_data();

    LL_mDelay(500);
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

static void GPIO_Init(void)
{
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  LL_GPIO_InitTypeDef gpio = {0};
  gpio.Pin = LL_GPIO_PIN_2;
  gpio.Mode = LL_GPIO_MODE_OUTPUT;
  gpio.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &gpio);

  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2);
}
