#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

/* KIT_PSE84_AI RGB LED is typically active-low; set to 0 for active-high boards. */
#define RGB_ACTIVE_LOW (1)

static uint32_t lfsr_state = 0xA5A5u;

static uint16_t lfsr16_next(void)
{
    uint16_t l = (uint16_t)lfsr_state;
    uint16_t bit = (uint16_t)(((l >> 0) ^ (l >> 2) ^ (l >> 3) ^ (l >> 5)) & 1u);
    l = (uint16_t)((l >> 1) | (bit << 15));
    lfsr_state = l;
    return l;
}

static uint8_t tri8(uint8_t phase)
{
    return (phase < 128u) ? (uint8_t)(phase << 1) : (uint8_t)((255u - phase) << 1);
}

static inline void rgb_write_level(cyhal_gpio_t pin, bool on)
{
#if RGB_ACTIVE_LOW
    cyhal_gpio_write(pin, on ? CYBSP_LED_STATE_OFF : CYBSP_LED_STATE_ON);
#else
    cyhal_gpio_write(pin, on ? CYBSP_LED_STATE_ON : CYBSP_LED_STATE_OFF);
#endif
}

static void rgb_write(bool r_on, bool g_on, bool b_on)
{
    rgb_write_level(CYBSP_LED_RGB_RED, r_on);
    rgb_write_level(CYBSP_LED_RGB_GREEN, g_on);
    rgb_write_level(CYBSP_LED_RGB_BLUE, b_on);
}

static void rgb_pwm_slice(uint8_t r, uint8_t g, uint8_t b, uint16_t duration_ms)
{
    const uint16_t frames = (uint16_t)(duration_ms / 2u);
    for (uint16_t f = 0; f < frames; ++f) {
        for (uint16_t t = 0; t < 256u; ++t) {
            rgb_write(t < r, t < g, t < b);
            cyhal_system_delay_us(8);
        }
    }
}

int main(void)
{
    cy_rslt_t result = cybsp_init();
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }

    __enable_irq();

    cyhal_gpio_init(CYBSP_LED_RGB_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    cyhal_gpio_init(CYBSP_LED_RGB_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    cyhal_gpio_init(CYBSP_LED_RGB_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    uint8_t pr = 7u;
    uint8_t pg = 103u;
    uint8_t pb = 197u;

    for (;;) {
        uint16_t rnd = lfsr16_next();

        /* Slow, living base motion with per-channel phase drift. */
        pr = (uint8_t)(pr + 1u + (rnd & 0x01u));
        pg = (uint8_t)(pg + 2u + ((rnd >> 1) & 0x01u));
        pb = (uint8_t)(pb + 3u + ((rnd >> 2) & 0x01u));

        uint8_t base = tri8((uint8_t)(pr + pg));
        uint8_t r = (uint8_t)((tri8(pr) * (uint16_t)(80u + (base >> 2))) >> 8);
        uint8_t g = (uint8_t)((tri8(pg) * (uint16_t)(96u + (base >> 3))) >> 8);
        uint8_t b = (uint8_t)((tri8(pb) * (uint16_t)(112u + (base >> 3))) >> 8);

        /* Irregular stress spikes to make it feel "breaking/alive". */
        if ((rnd & 0x003Fu) == 0u) {
            for (uint8_t i = 0; i < 4u; ++i) {
                uint16_t g2 = lfsr16_next();
                uint8_t gr = (uint8_t)((g2 & 0xFFu) >> 1);
                uint8_t gg = (uint8_t)(((g2 >> 4) & 0xFFu) >> 1);
                uint8_t gb = (uint8_t)(((g2 >> 8) & 0xFFu) >> 1);
                rgb_pwm_slice(gr, gg, gb, 18u);
            }
        }

        rgb_pwm_slice(r, g, b, 36u);
    }
}
