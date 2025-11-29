#include <stdint.h>
#include "py32f0xx_hal.h"
#include <py32f002bx5.h>
#include <py32f002b_hal_gpio.h>


// ------------------ Pin definitions ------------------
typedef enum
{
    DataRight1 = 0,
    DataRight2,
    ShieldTopRight,
    DataTop2,
    DataTop1,
    ShieldTopLeft,
    HallAdc,
    ShieldBottomRight,
    DataBottom1,
    DataBottom2,
    ShieldBottomLeft,
    LedIn,
    DataLeft1,
    DataLeft2
} PinName_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} PinMap_t;

static const PinMap_t PinMap[] = {
    [DataRight1] = {GPIOA, GPIO_PIN_5},
    [DataRight2] = {GPIOA, GPIO_PIN_6},
    [ShieldTopRight] = {GPIOA, GPIO_PIN_7},
    [DataTop2] = {GPIOC, GPIO_PIN_1},
    [DataTop1] = {GPIOB, GPIO_PIN_7},
    [ShieldTopLeft] = {GPIOB, GPIO_PIN_5},
    [HallAdc] = {GPIOA, GPIO_PIN_4},
    [ShieldBottomRight] = {GPIOA, GPIO_PIN_3},
    [DataBottom1] = {GPIOA, GPIO_PIN_1},
    [DataBottom2] = {GPIOA, GPIO_PIN_0},
    [ShieldBottomLeft] = {GPIOB, GPIO_PIN_1},
    [LedIn] = {GPIOB, GPIO_PIN_2},
    [DataLeft1] = {GPIOB, GPIO_PIN_3},
    [DataLeft2] = {GPIOB, GPIO_PIN_4}};

#define HIGH 1
#define LOW 0

// ------------------ Initialization ------------------
void GPIO_InitCustom(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Enable all required GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    for (int i = 0; i < sizeof(PinMap) / sizeof(PinMap[0]); i++)
    {
        GPIO_InitStruct.Pin = PinMap[i].pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // default as output
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(PinMap[i].port, &GPIO_InitStruct);

        // default low using digitalWrite
        if (LOW)
            HAL_GPIO_WritePin(PinMap[i].port, PinMap[i].pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(PinMap[i].port, PinMap[i].pin, GPIO_PIN_RESET);
        // Note: simplified; the if-statement keeps the pattern of using digital-like API.
    }
}

void digitalWrite(PinName_t pin, uint8_t value)
{
    HAL_GPIO_WritePin(PinMap[pin].port, PinMap[pin].pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int digitalRead(PinName_t pin)
{
    return (HAL_GPIO_ReadPin(PinMap[pin].port, PinMap[pin].pin) == GPIO_PIN_SET) ? HIGH : LOW;
}

GPIO_PinState Pin_Read(PinName_t pin)
{
    return HAL_GPIO_ReadPin(PinMap[pin].port, PinMap[pin].pin);
}

// ------------------ SK6812 timing macros ------------------
#define LED_PIN LedIn

// Timings in CPU cycles for 24MHz (~41.7ns per cycle)
#define T0H_CYCLES 10 // ~417ns high for '0'
#define T0L_CYCLES 19 // ~792ns low for '0'
#define T1H_CYCLES 19 // ~792ns high for '1'
#define T1L_CYCLES 10 // ~417ns low for '1'

// ------------------ Simple delay for bit-banging ------------------
static void delay_cycles(uint32_t cycles)
{
    while (cycles--)
        __asm__("nop");
}

// ------------------ Send a single bit ------------------
static void SK6812_SendBit(uint8_t bit)
{
    if (bit)
    {
        digitalWrite(LED_PIN, HIGH);
        delay_cycles(T1H_CYCLES);
        digitalWrite(LED_PIN, LOW);
        delay_cycles(T1L_CYCLES);
    }
    else
    {
        digitalWrite(LED_PIN, HIGH);
        delay_cycles(T0H_CYCLES);
        digitalWrite(LED_PIN, LOW);
        delay_cycles(T0L_CYCLES);
    }
}

// ------------------ Send a single byte (MSB first) ------------------
static void SK6812_SendByte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--)
        SK6812_SendBit((byte >> i) & 0x01);
}

// ------------------ Send RGB data for 4 LEDs ------------------
// colors[led][0]=R, [1]=G, [2]=B
void SK6812_SendFrame(uint8_t colors[4][3])
{
    // SK6812 expects GRB order
    for (int led = 0; led < 4; led++)
    {
        SK6812_SendByte(colors[led][1]); // G
        SK6812_SendByte(colors[led][0]); // R
        SK6812_SendByte(colors[led][2]); // B
    }

    // Latch: hold low > 80us
    HAL_Delay(1);
}

int main(void)
{
    HAL_Init();
    GPIO_InitCustom();

    uint8_t colors1[4][3] = {
        {0xFF, 0x00, 0x00}, // Red
        {0x00, 0xFF, 0x00}, // Green
        {0x00, 0x00, 0xFF}, // Blue
        {0xFF, 0xFF, 0x00}  // Yellow
    };
    uint8_t colors2[4][3] = {
        {0x00, 0x00, 0x00}, // Off
        {0x20, 0x20, 0x20}, // Dim White
        {0x40, 0x00, 0x00}, // Dim Red
        {0x00, 0x40, 0x00}  // Dim Green
    };

    while (1)
    {
        SK6812_SendFrame(colors1);
        HAL_Delay(500);
        SK6812_SendFrame(colors2);
        HAL_Delay(500);
    }
}
