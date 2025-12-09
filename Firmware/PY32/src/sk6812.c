#include "sk6812.h"

/* ------------------------------------------------------------------ */
/* Internal LED buffer                                                */
/* ------------------------------------------------------------------ */

static uint8_t *leds = 0;
static uint16_t led_count = 0;
static uint32_t pin_mask = 0;
static GPIO_TypeDef *gpio_port = GPIOB;

/* ------------------------------------------------------------------ */
/* Low-level SK6812 bit timing                                        */
/* ------------------------------------------------------------------ */

static inline void send_bit(uint8_t bit)
{
    if (bit)
    {
        gpio_port->BSRR = pin_mask; // pointer dereference takes a lot of time
        // 600ns high
        __NOP();
        __NOP();
        gpio_port->BRR = pin_mask;
        // 600ns low
        __NOP();
        __NOP();
    }
    else
    {
        gpio_port->BSRR = pin_mask;
        // 300ns high
        gpio_port->BRR = pin_mask;
        // 900ns low
        __NOP();
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

static inline void send_led_data(uint8_t r, uint8_t g, uint8_t b)
{
    send_byte(g); // GRB order
    send_byte(r);
    send_byte(b);
}

static inline void latch_data()
{
    LL_mDelay(1);
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

void sk6812_init(GPIO_TypeDef *port, uint32_t gpio_pin, uint16_t count)
{
    gpio_port = port;
    pin_mask = gpio_pin;
    led_count = count;

    leds = malloc(count * 3);
    if (!leds)
        return;

    memset(leds, 0, count * 3);

    switch ((uint32_t)gpio_port)
    {
    case (uint32_t)GPIOA:
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
        break;
    case (uint32_t)GPIOB:
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
        break;
    case (uint32_t)GPIOC:
        LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
        break;
    default:
        return;
    }

    LL_GPIO_InitTypeDef g = {0};
    g.Pin = gpio_pin;
    g.Mode = LL_GPIO_MODE_OUTPUT;
    g.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    g.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    g.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(gpio_port, &g);

    LL_GPIO_ResetOutputPin(gpio_port, gpio_pin);
}

void sk6812_set_pixel(uint16_t i, uint8_t r, uint8_t g, uint8_t b)
{
    if (i >= led_count)
        return;

    leds[i * 3 + 0] = g;
    leds[i * 3 + 1] = r;
    leds[i * 3 + 2] = b;
}

void sk6812_get_pixel(uint16_t i, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (i >= led_count)
        return;
    *g = leds[i * 3 + 0];
    *r = leds[i * 3 + 1];
    *b = leds[i * 3 + 2];
}

void sk6812_fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0; i < led_count; i++)
        sk6812_set_pixel(i, r, g, b);
}

void sk6812_clear(void)
{
    memset(leds, 0, led_count * 3);
}
void sk6812_show(void)
{
    for (uint16_t i = 0; i < led_count; i++)
    {
        uint8_t g = leds[i * 3 + 0];
        uint8_t r = leds[i * 3 + 1];
        uint8_t b = leds[i * 3 + 2];
        send_led_data(r, g, b);
    }
    latch_data();
}

void sk6812_deinit(void)
{
    if (leds)
        free(leds);
    leds = 0;
}
