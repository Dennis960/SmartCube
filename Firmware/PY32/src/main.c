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

int effect[] = {
  0x010000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x010000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x010000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x010000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000100, 0x010000, 0x000001, 0x010101,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000100, 0x010000, 0x000001, 0x010101,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000100, 0x010000, 0x000001, 0x010101,
  0x000000, 0x000000, 0x000000, 0x000000,
};

int main(void)
{
  SystemClock_Config();
  GPIO_Init();

  while (1)
  {
    for (int i = 0; i < sizeof(effect) / sizeof(effect[0]) / 4; i++)
    {
      int color1 = effect[i * 4];
      int color2 = effect[i * 4 + 1];
      int color3 = effect[i * 4 + 2];
      int color4 = effect[i * 4 + 3];
      send_led_data((color1 >> 16) & 0xFF, (color1 >> 8) & 0xFF, color1 & 0xFF);
      send_led_data((color2 >> 16) & 0xFF, (color2 >> 8) & 0xFF, color2 & 0xFF);
      send_led_data((color3 >> 16) & 0xFF, (color3 >> 8) & 0xFF, color3 & 0xFF);
      send_led_data((color4 >> 16) & 0xFF, (color4 >> 8) & 0xFF, color4 & 0xFF);
      latch_data();
      LL_mDelay(200);
    }
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
