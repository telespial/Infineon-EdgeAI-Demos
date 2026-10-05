#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "cy_pdl.h"
#include "cybsp.h"

#include "xensiv_bgt60trxx.h"
#include "xensiv_bgt60trxx_platform.h"
#include "xensiv_bgt60trxx_regs.h"

#include "radar_settings.h"

#define XENSIV_BGT60TRXX_CONF_DEVICE P0_XENSIV_BGT60TRXX_CONF_DEVICE
#define XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP P0_XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP
#define XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME P0_XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME
#define XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS P0_XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS
#define XENSIV_BGT60TRXX_CONF_NUM_REGS P0_XENSIV_BGT60TRXX_CONF_NUM_REGS
#define register_list register_list_p0

#define RADAR_SPI_TRANSFER_TIMEOUT_US (200000U)
#define RADAR_POLL_PERIOD_MS          (2U)
#define GESTURE_SCORE_THRESHOLD       (0.30f)
#define BASELINE_ALPHA_NUM            (31U)
#define BASELINE_ALPHA_DEN            (32U)
#define GESTURE_RUN_DIVIDER           (1U)
#define MOTION_ON_DELTA               (20)
#define MOTION_RELEASE_FRAMES         (16U)
#define PUSH_DEBOUNCE_FRAMES          (3U)
#define TAP_GROUP_TIMEOUT_FRAMES      (110U)
#define TAP_DERIV_MIN                 (12)
#define TAP_DERIV_MARGIN              (8)
#define TAP_PROX_DELTA_MIN            (10)
#define TAP_CONFIRM_TIMEOUT_FRAMES    (10U)
#define TAP_FAST_MARGIN               (6U)
#define TAP_DELTA_STRONG_MIN          (40)
#define TAP_ABS_DE_STRONG_MIN         (60U)
#define TAP_NEG_DE_STRONG_MIN         (45U)
#define SWIPE_SCORE_MIN               (0.08f)
#define SWIPE_CONFIRM_FRAMES          (4U)
#define SWIPE_SUPPRESS_FRAMES         (120U)
#define SWIPE_GUARD_SCORE_MIN         (0.03f)
#define SWIPE_GUARD_FRAMES            (40U)
#define SWIPE_TAP_LOCKOUT_FRAMES      (40U)
#define SWIPE_VOTE_TRANSITION_LOCKOUT_FRAMES (16U)
#define SWIPE_DOPPLER_MAX_DE          (18U)
#define SWIPE_ZONE_DELTA_MAX          (20)
#define DEBUG_STREAM_DIVIDER          (1U)
#define DEBUG_WAIT_DIVIDER            (250U)
#define EVENT_ONLY_UART               (1U)
#define RGB_ACTIVE_LOW                (0U)
#define ML_TAP_CONF_MIN               (0.25f)
#define ML_SWIPE_CONF_MIN             (0.18f)
#define ML_SWIPE_GUARD_CONF_MIN       (0.14f)
#define ML_FEATURE_SCALE              (64.0f)
#define HEARTBEAT_PERIOD_TICKS        (125U)
#define HEARTBEAT_ON_TICKS            (25U)
#define EVENT_COOLDOWN_FRAMES         (160U)
#define TAP_ARM_MAX_AGE               (28U)
#define TAP_REFRACTORY_FRAMES         (20U)
#define TAP_MIN_ARM_AGE               (2U)

#define NUM_SAMPLES_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * \
                               XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * \
                               XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

typedef struct
{
    CySCB_Type* base;
    cy_stc_scb_spi_context_t* context;
} radar_iface_t;

static cy_stc_scb_spi_context_t spi_context;
static radar_iface_t radar_iface = {
    .base = CYBSP_SPI_CONTROLLER_HW,
    .context = &spi_context,
};

static xensiv_bgt60trxx_t radar_dev;
static uint16_t radar_samples[NUM_SAMPLES_PER_FRAME];
static float32_t gesture_frame[NUM_SAMPLES_PER_FRAME];
static cy_stc_scb_uart_context_t debug_uart_context;
typedef struct
{
    uint32_t idx;
    float score;
} inference_results_t;

static int32_t g_swipe_integrator = 0;

typedef enum
{
    ML_CLASS_BACKGROUND = 0U,
    ML_CLASS_TAP = 1U,
    ML_CLASS_SWIPE_UP = 2U,
    ML_CLASS_SWIPE_DOWN = 3U
} ml_class_t;

typedef struct
{
    int32_t ema_delta;
    int32_t ema_de;
    uint32_t ema_abs_de;
} ml_state_t;

typedef struct
{
    ml_class_t klass;
    float confidence;
} ml_result_t;

static ml_state_t g_ml_state = {0, 0, 0};

static float f_abs(float x)
{
    return (x < 0.0f) ? -x : x;
}

static float f_clamp01(float x)
{
    if (x < 0.0f)
    {
        return 0.0f;
    }
    if (x > 1.0f)
    {
        return 1.0f;
    }
    return x;
}

static ml_result_t ml_predict(
    int32_t delta,
    int32_t d_energy,
    uint32_t abs_de,
    uint32_t model_idx,
    float model_score)
{
    ml_result_t out = { ML_CLASS_BACKGROUND, 0.0f };
    float up_hint = (model_idx == 6U) ? model_score : 0.0f;
    float down_hint = (model_idx == 7U) ? model_score : 0.0f;
    float score_sum = up_hint + down_hint;

    g_ml_state.ema_delta = ((g_ml_state.ema_delta * 7) + delta) / 8;
    g_ml_state.ema_de = ((g_ml_state.ema_de * 7) + d_energy) / 8;
    g_ml_state.ema_abs_de = ((g_ml_state.ema_abs_de * 7U) + abs_de) / 8U;

    float d_n = (float)delta / ML_FEATURE_SCALE;
    float de_n = (float)d_energy / ML_FEATURE_SCALE;
    float abs_de_n = (float)abs_de / ML_FEATURE_SCALE;
    float ema_d_n = (float)g_ml_state.ema_delta / ML_FEATURE_SCALE;
    float ema_de_n = (float)g_ml_state.ema_de / ML_FEATURE_SCALE;
    float ema_abs_n = (float)g_ml_state.ema_abs_de / ML_FEATURE_SCALE;

    float s_bg = 0.85f
        - 0.25f * f_abs(d_n)
        - 0.85f * abs_de_n
        - 0.25f * f_abs(ema_de_n)
        - 1.20f * score_sum;

    float s_tap = -1.25f
        + 0.70f * d_n
        + 1.05f * de_n
        + 1.10f * abs_de_n
        + 0.45f * ema_abs_n
        - 0.80f * score_sum;

    float s_up = -0.95f
        - 0.35f * abs_de_n
        - 0.20f * f_abs(de_n)
        - 0.15f * f_abs(ema_d_n)
        + 2.35f * up_hint
        - 0.55f * down_hint;

    float s_down = -0.95f
        - 0.35f * abs_de_n
        - 0.20f * f_abs(de_n)
        - 0.15f * f_abs(ema_d_n)
        + 2.35f * down_hint
        - 0.55f * up_hint;

    float scores[4] = { s_bg, s_tap, s_up, s_down };
    uint32_t top_idx = 0U;
    uint32_t second_idx = 1U;

    for (uint32_t i = 1U; i < 4U; ++i)
    {
        if (scores[i] > scores[top_idx])
        {
            second_idx = top_idx;
            top_idx = i;
        }
        else if ((i != top_idx) && (scores[i] > scores[second_idx]))
        {
            second_idx = i;
        }
    }

    float margin = scores[top_idx] - scores[second_idx];
    out.klass = (ml_class_t)top_idx;
    out.confidence = f_clamp01(0.45f * margin);
    return out;
}

static const char* ml_class_name(ml_class_t klass)
{
    switch (klass)
    {
    case ML_CLASS_BACKGROUND: return "BG";
    case ML_CLASS_TAP: return "TAP";
    case ML_CLASS_SWIPE_UP: return "SWIPE_UP";
    case ML_CLASS_SWIPE_DOWN: return "SWIPE_DOWN";
    default: return "UNK";
    }
}

static inline void spi_set_data_width(CySCB_Type* base, uint32_t data_width)
{
    CY_REG32_CLR_SET(SCB_TX_CTRL(base), SCB_TX_CTRL_DATA_WIDTH, data_width - 1U);
    CY_REG32_CLR_SET(SCB_RX_CTRL(base), SCB_RX_CTRL_DATA_WIDTH, data_width - 1U);
}

static void debug_uart_init(void)
{
    if (Cy_SCB_UART_Init(CYBSP_DEBUG_UART_HW, &CYBSP_DEBUG_UART_config, &debug_uart_context) == CY_SCB_UART_SUCCESS)
    {
        Cy_SCB_UART_Enable(CYBSP_DEBUG_UART_HW);
        Cy_SCB_UART_PutString(CYBSP_DEBUG_UART_HW, "\r\n--- PSE84 Radar Gesture (RGB Event Mapping) ---\r\n");
    }
}

static void debug_log(const char* fmt, ...)
{
    char msg[224];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    if (n <= 0)
    {
        return;
    }

    size_t len = (size_t)n;
    if (len >= sizeof(msg))
    {
        len = sizeof(msg) - 1u;
    }
    Cy_SCB_UART_PutArrayBlocking(CYBSP_DEBUG_UART_HW, msg, (uint32_t)len);
}

static int32_t radar_hw_init(void)
{
    Cy_GPIO_Pin_FastInit(CYBSP_SPI_CS_PORT, CYBSP_SPI_CS_PIN, CY_GPIO_DM_STRONG_IN_OFF, 1u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_RADAR_RESET_PORT, CYBSP_RADAR_RESET_PIN, CY_GPIO_DM_STRONG_IN_OFF, 1u, HSIOM_SEL_GPIO);

    Cy_SCB_SPI_Disable(CYBSP_SPI_CONTROLLER_HW, &spi_context);
    if (Cy_SCB_SPI_Init(CYBSP_SPI_CONTROLLER_HW, &CYBSP_SPI_CONTROLLER_config, &spi_context) != CY_SCB_SPI_SUCCESS)
    {
        return -1;
    }
    Cy_SCB_SPI_Enable(CYBSP_SPI_CONTROLLER_HW);
    return 0;
}

static int32_t radar_init_and_start(void)
{
    if (radar_hw_init() != 0)
    {
        return -1;
    }

    xensiv_bgt60trxx_platform_rst_set(&radar_iface, true);
    xensiv_bgt60trxx_platform_spi_cs_set(&radar_iface, true);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(&radar_iface, false);
    xensiv_bgt60trxx_platform_delay(1U);
    xensiv_bgt60trxx_platform_rst_set(&radar_iface, true);
    xensiv_bgt60trxx_platform_delay(5U);

    int32_t status = xensiv_bgt60trxx_init(&radar_dev, &radar_iface, false);
    if (status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return status;
    }

    status = xensiv_bgt60trxx_config(&radar_dev, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    if (status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return status;
    }

    /* Read one complete frame per transaction; keep FIFO interrupt at full-frame level. */
    status = xensiv_bgt60trxx_set_fifo_limit(&radar_dev, NUM_SAMPLES_PER_FRAME);
    if (status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return status;
    }

    return xensiv_bgt60trxx_start_frame(&radar_dev, true);
}

static uint32_t radar_fstat_addr(void)
{
    switch (xensiv_bgt60trxx_get_device(&radar_dev))
    {
    case XENSIV_DEVICE_BGT60TR13C:
        return XENSIV_BGT60TRXX_REG_FSTAT_TR13C;
    case XENSIV_DEVICE_BGT60UTR13D:
        return XENSIV_BGT60TRXX_REG_FSTAT_UTR13D;
    case XENSIV_DEVICE_BGT60UTR11:
        return XENSIV_BGT60TRXX_REG_FSTAT_UTR11;
    default:
        return XENSIV_BGT60TRXX_REG_FSTAT_TR13C;
    }
}

static int32_t radar_recover_fifo(void)
{
    int32_t status = xensiv_bgt60trxx_start_frame(&radar_dev, false);
    if (status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return status;
    }

    status = xensiv_bgt60trxx_soft_reset(&radar_dev, XENSIV_BGT60TRXX_RESET_FIFO);
    if (status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return status;
    }

    return xensiv_bgt60trxx_start_frame(&radar_dev, true);
}

static void deinterleave_antennas(const uint16_t* buffer_ptr)
{
    uint8_t antenna = 0;
    uint32_t index = 0;

    for (uint32_t i = 0; i < NUM_SAMPLES_PER_FRAME; ++i)
    {
        gesture_frame[index + (antenna * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *
                               XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME)] = (float32_t)buffer_ptr[i];
        antenna++;
        if (antenna == XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)
        {
            antenna = 0;
            index++;
        }
    }
}

static const char* gesture_name(uint32_t idx)
{
    static const char* classes[] = {
        "BACKGROUND", "PUSH", "SWIPE_LEFT", "SWIPE_RIGHT",
        "UNKNOWN_1", "UNKNOWN_2", "SWIPE_UP", "SWIPE_DOWN"
    };
    return (idx < (sizeof(classes) / sizeof(classes[0]))) ? classes[idx] : "OUT_OF_RANGE";
}

static void gestures_init(void)
{
    g_swipe_integrator = 0;
}

static void gestures_run(const float32_t* frame, inference_results_t* results)
{
    CY_UNUSED_PARAMETER(frame);
    if (results == NULL)
    {
        return;
    }

    results->idx = 0U;
    results->score = 0.0f;
}

static void pulse_led(GPIO_PRT_Type* port, uint32_t pin, uint32_t on_ms, uint32_t off_ms, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        Cy_GPIO_Write(port, pin, 1u);
        Cy_SysLib_Delay(on_ms);
        Cy_GPIO_Write(port, pin, 0u);
        if (off_ms > 0u)
        {
            Cy_SysLib_Delay(off_ms);
        }
    }
}

static void rgb_write_channel(GPIO_PRT_Type* port, uint32_t pin, bool on)
{
    uint32_t level = on ? 1u : 0u;
    if (RGB_ACTIVE_LOW)
    {
        level ^= 1u;
    }
    Cy_GPIO_Write(port, pin, level);
}

static void rgb_all_off(void)
{
    rgb_write_channel(CYBSP_LED_RGB_RED_PORT, CYBSP_LED_RGB_RED_PIN, false);
    rgb_write_channel(CYBSP_LED_RGB_GREEN_PORT, CYBSP_LED_RGB_GREEN_PIN, false);
    rgb_write_channel(CYBSP_LED_RGB_BLUE_PORT, CYBSP_LED_RGB_BLUE_PIN, false);
}

static void rgb_pulse_channel(GPIO_PRT_Type* port, uint32_t pin, uint32_t on_ms, uint32_t off_ms, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        rgb_write_channel(port, pin, true);
        Cy_SysLib_Delay(on_ms);
        rgb_write_channel(port, pin, false);
        if (off_ms > 0u)
        {
            Cy_SysLib_Delay(off_ms);
        }
    }
}

static void rgb_startup_self_test(void)
{
    rgb_all_off();
    rgb_pulse_channel(CYBSP_LED_RGB_RED_PORT, CYBSP_LED_RGB_RED_PIN, 100U, 70U, 1U);
        rgb_pulse_channel(CYBSP_LED_RGB_BLUE_PORT, CYBSP_LED_RGB_BLUE_PIN, 100U, 70U, 1U);
    rgb_all_off();
}

typedef struct
{
    bool active;
    bool led_on;
    uint16_t phase_ticks;
    uint16_t on_ticks;
    uint16_t off_ticks;
    uint8_t flashes_left;
    uint8_t color_mask;
} led_event_state_t;

static led_event_state_t g_led_evt = {0};

static void rgb_write_mask(uint8_t mask, bool on)
{
    rgb_write_channel(CYBSP_LED_RGB_RED_PORT, CYBSP_LED_RGB_RED_PIN, on && ((mask & 0x1u) != 0u));
    rgb_write_channel(CYBSP_LED_RGB_GREEN_PORT, CYBSP_LED_RGB_GREEN_PIN, on && ((mask & 0x2u) != 0u));
    rgb_write_channel(CYBSP_LED_RGB_BLUE_PORT, CYBSP_LED_RGB_BLUE_PIN, on && ((mask & 0x4u) != 0u));
}

static uint16_t ms_to_ticks(uint16_t ms)
{
    uint16_t ticks = (uint16_t)((ms + (RADAR_POLL_PERIOD_MS - 1U)) / RADAR_POLL_PERIOD_MS);
    return (ticks == 0U) ? 1U : ticks;
}

static void led_event_start(uint8_t color_mask, uint8_t flashes, uint16_t on_ms, uint16_t off_ms)
{
    g_led_evt.active = true;
    g_led_evt.led_on = true;
    g_led_evt.phase_ticks = ms_to_ticks(on_ms);
    g_led_evt.on_ticks = ms_to_ticks(on_ms);
    g_led_evt.off_ticks = ms_to_ticks(off_ms);
    g_led_evt.flashes_left = flashes;
    g_led_evt.color_mask = color_mask;
    rgb_write_mask(color_mask, true);
}

static void led_event_step(void)
{
    if (!g_led_evt.active)
    {
        return;
    }

    if (g_led_evt.phase_ticks > 0U)
    {
        --g_led_evt.phase_ticks;
        if (g_led_evt.phase_ticks > 0U)
        {
            return;
        }
    }

    if (g_led_evt.led_on)
    {
        g_led_evt.led_on = false;
        rgb_write_mask(g_led_evt.color_mask, false);
        if (g_led_evt.flashes_left > 0U)
        {
            --g_led_evt.flashes_left;
        }

        if (g_led_evt.flashes_left == 0U)
        {
            g_led_evt.active = false;
            rgb_all_off();
            return;
        }

        g_led_evt.phase_ticks = g_led_evt.off_ticks;
    }
    else
    {
        g_led_evt.led_on = true;
        rgb_write_mask(g_led_evt.color_mask, true);
        g_led_evt.phase_ticks = g_led_evt.on_ticks;
    }
}

static void indicate_tap_group(uint32_t taps)
{
    uint8_t pulse_count = (taps > 3U) ? 3U : (uint8_t)taps;
    if (pulse_count == 0U)
    {
        return;
    }
    led_event_start(0x4u, pulse_count, 90U, 80U);
}

static void indicate_swipe_up(void)
{
    led_event_start(0x4u, 2U, 170U, 100U);
}

static void indicate_swipe_down(void)
{
    led_event_start(0x1u, 2U, 170U, 100U);
}

void xensiv_bgt60trxx_platform_rst_set(const void* iface, bool val)
{
    CY_UNUSED_PARAMETER(iface);
    Cy_GPIO_Write(CYBSP_RADAR_RESET_PORT, CYBSP_RADAR_RESET_PIN, (uint32_t)val);
}

void xensiv_bgt60trxx_platform_spi_cs_set(const void* iface, bool val)
{
    CY_UNUSED_PARAMETER(iface);
    Cy_GPIO_Write(CYBSP_SPI_CS_PORT, CYBSP_SPI_CS_PIN, (uint32_t)val);
}

int32_t xensiv_bgt60trxx_platform_spi_transfer(void* iface, uint8_t* tx_data, uint8_t* rx_data, uint32_t len)
{
    radar_iface_t* ifc = (radar_iface_t*)iface;
    uint8_t tx_dummy = 0x00;
    spi_set_data_width(ifc->base, 8U);
    Cy_SCB_SetByteMode(ifc->base, true);

    cy_en_scb_spi_status_t status = Cy_SCB_SPI_Transfer(
        ifc->base,
        (tx_data != NULL) ? tx_data : &tx_dummy,
        rx_data,
        len,
        ifc->context);

    if (status != CY_SCB_SPI_SUCCESS)
    {
        return XENSIV_BGT60TRXX_STATUS_COM_ERROR;
    }

    uint32_t timeout = RADAR_SPI_TRANSFER_TIMEOUT_US;
    while ((Cy_SCB_SPI_GetTransferStatus(ifc->base, ifc->context) & CY_SCB_SPI_TRANSFER_ACTIVE) != 0UL)
    {
        Cy_SCB_SPI_Interrupt(ifc->base, ifc->context);
        Cy_SysLib_DelayUs(1U);
        if (timeout-- == 0U)
        {
            return XENSIV_BGT60TRXX_STATUS_TIMEOUT_ERROR;
        }
    }

    return XENSIV_BGT60TRXX_STATUS_OK;
}

int32_t xensiv_bgt60trxx_platform_spi_fifo_read(void* iface, uint16_t* rx_data, uint32_t len)
{
    radar_iface_t* ifc = (radar_iface_t*)iface;
    spi_set_data_width(ifc->base, 12U);
    Cy_SCB_SetByteMode(ifc->base, false);

    cy_en_scb_spi_status_t status = Cy_SCB_SPI_Transfer(ifc->base, NULL, rx_data, len, ifc->context);
    if (status != CY_SCB_SPI_SUCCESS)
    {
        return XENSIV_BGT60TRXX_STATUS_COM_ERROR;
    }

    uint32_t timeout = RADAR_SPI_TRANSFER_TIMEOUT_US;
    while ((Cy_SCB_SPI_GetTransferStatus(ifc->base, ifc->context) & CY_SCB_SPI_TRANSFER_ACTIVE) != 0UL)
    {
        Cy_SCB_SPI_Interrupt(ifc->base, ifc->context);
        Cy_SysLib_DelayUs(1U);
        if (timeout-- == 0U)
        {
            return XENSIV_BGT60TRXX_STATUS_TIMEOUT_ERROR;
        }
    }

    return XENSIV_BGT60TRXX_STATUS_OK;
}

void xensiv_bgt60trxx_platform_delay(uint32_t ms)
{
    Cy_SysLib_Delay(ms);
}

uint32_t xensiv_bgt60trxx_platform_word_reverse(uint32_t x)
{
    return __REV(x);
}

void xensiv_bgt60trxx_platform_assert(bool expr)
{
    CY_ASSERT(expr);
    (void)expr;
}

int main(void)
{
    if (cybsp_init() != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    __enable_irq();

    debug_uart_init();

    /* LED1 idle heartbeat/ready */
    Cy_GPIO_Pin_FastInit(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, CY_GPIO_DM_STRONG_IN_OFF, 1u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_USER_LED2_PORT, CYBSP_USER_LED2_PIN, CY_GPIO_DM_STRONG_IN_OFF, 0u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_LED_RGB_RED_PORT, CYBSP_LED_RGB_RED_PIN, CYBSP_LED_RGB_RED_DRIVEMODE, 0u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_LED_RGB_GREEN_PORT, CYBSP_LED_RGB_GREEN_PIN, CYBSP_LED_RGB_GREEN_DRIVEMODE, 0u, HSIOM_SEL_GPIO);
    Cy_GPIO_Pin_FastInit(CYBSP_LED_RGB_BLUE_PORT, CYBSP_LED_RGB_BLUE_PIN, CYBSP_LED_RGB_BLUE_DRIVEMODE, 0u, HSIOM_SEL_GPIO);
    rgb_all_off();
    rgb_startup_self_test();

    int32_t radar_status = radar_init_and_start();
    debug_log("radar_init_and_start=%ld\r\n", (long)radar_status);
    if (radar_status != XENSIV_BGT60TRXX_STATUS_OK)
    {
        for (;;)
        {
            Cy_SysLib_Delay(250U);
            Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
        }
    }

    gestures_init();
#if !EVENT_ONLY_UART
    debug_log("DBG legend: e=energy_ps b=baseline d=e-b de=deltaE abs=|de| idx=class score=nn ml=class conf=nn up=upVotes dn=dnVotes guard=tapBlock cand=tapArmed taps=tapCount\r\n");
#endif

    uint32_t heartbeat_phase = 0U;
    bool heartbeat_led_on = false;
    uint32_t baseline = 0U;
    uint32_t frame_count = 0U;
    uint32_t fstat_addr = radar_fstat_addr();
    uint32_t fifo_err_count = 0U;
    uint32_t motion_release_count = 0U;
    uint32_t push_debounce = 0U;
    uint32_t tap_group_timeout = 0U;
    uint32_t tap_count = 0U;
    uint32_t prev_energy = 0U;
    uint32_t deriv_noise = 1U;
    uint32_t tap_confirm_timeout = 0U;
    uint32_t tap_arm_age = 0U;
    uint32_t tap_refractory = 0U;
    uint32_t swipe_up_votes = 0U;
    uint32_t swipe_down_votes = 0U;
    uint32_t swipe_suppress = 0U;
    uint32_t swipe_guard = 0U;
    uint32_t swipe_tap_lockout = 0U;
    uint32_t swipe_vote_transition_lockout = 0U;
    uint32_t debug_stream_div = 0U;
    uint32_t wait_log_div = 0U;
    uint32_t event_cooldown = 0U;
    bool motion_latched = false;
    bool tap_candidate = false;
    bool swipe_vote_active_prev = false;

    for (;; )
    {
        led_event_step();

        if (heartbeat_phase == 0U)
        {
            /* LED1 is active-low on this board: drive low to turn on. */
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 0u);
            heartbeat_led_on = true;
        }

        ++heartbeat_phase;

        if (heartbeat_led_on && (heartbeat_phase >= HEARTBEAT_ON_TICKS))
        {
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 1u);
            heartbeat_led_on = false;
        }

        if (heartbeat_phase >= HEARTBEAT_PERIOD_TICKS)
        {
            heartbeat_phase = 0U;
        }

        uint32_t fstat = 0U;
        int32_t reg_status = xensiv_bgt60trxx_get_reg(&radar_dev, fstat_addr, &fstat);
        if (reg_status != XENSIV_BGT60TRXX_STATUS_OK)
        {
            debug_log("ERR fstat status=%ld\r\n", (long)reg_status);
            Cy_SysLib_Delay(RADAR_POLL_PERIOD_MS);
            continue;
        }

        uint32_t fill_samples = (fstat & XENSIV_BGT60TRXX_REG_FSTAT_FILL_STATUS_MSK);
        if (fill_samples < NUM_SAMPLES_PER_FRAME)
        {
            if (!EVENT_ONLY_UART && (++wait_log_div >= DEBUG_WAIT_DIVIDER))
            {
                wait_log_div = 0U;
                debug_log("WAIT fill=%lu need=%u fstat=0x%08lx\r\n",
                          (unsigned long)fill_samples,
                          (unsigned int)NUM_SAMPLES_PER_FRAME,
                          (unsigned long)fstat);
            }
            Cy_SysLib_Delay(RADAR_POLL_PERIOD_MS);
            continue;
        }
        wait_log_div = 0U;

        int32_t fifo_status = xensiv_bgt60trxx_get_fifo_data(&radar_dev, radar_samples, NUM_SAMPLES_PER_FRAME);
        if (fifo_status != XENSIV_BGT60TRXX_STATUS_OK)
        {
            ++fifo_err_count;
            if (fifo_status == XENSIV_BGT60TRXX_STATUS_GSR0_ERROR)
            {
                int32_t rec_status = radar_recover_fifo();
                if ((fifo_err_count % 32U) == 1U)
                {
                    debug_log("WARN fifo recover=%ld count=%lu fstat=0x%08lx fill=%lu\r\n",
                              (long)rec_status,
                              (unsigned long)fifo_err_count,
                              (unsigned long)fstat,
                              (unsigned long)fill_samples);
                }
            }
            Cy_SysLib_Delay(RADAR_POLL_PERIOD_MS);
            continue;
        }

        uint64_t energy_sum = 0ULL;
        for (uint32_t i = 0; i < NUM_SAMPLES_PER_FRAME; ++i)
        {
            energy_sum += (uint64_t)radar_samples[i];
        }
        uint32_t energy_per_sample = (uint32_t)(energy_sum / NUM_SAMPLES_PER_FRAME);
        if (prev_energy == 0U)
        {
            prev_energy = energy_per_sample;
        }

        if (baseline == 0U)
        {
            baseline = energy_per_sample;
        }
        baseline = (uint32_t)(((uint64_t)baseline * BASELINE_ALPHA_NUM + energy_per_sample) / BASELINE_ALPHA_DEN);

        inference_results_t results;
        results.idx = 0U;
        results.score = 0.0f;

        if ((frame_count++ % GESTURE_RUN_DIVIDER) == 0U)
        {
            deinterleave_antennas(radar_samples);
            gestures_run(gesture_frame, &results);
        }

        bool gesture_detected = (results.idx != 0U) && (results.score >= GESTURE_SCORE_THRESHOLD);
        int32_t delta = (int32_t)energy_per_sample - (int32_t)baseline;
        bool motion_on = (delta >= MOTION_ON_DELTA);
        int32_t d_energy = (int32_t)energy_per_sample - (int32_t)prev_energy;
        uint32_t abs_de = (uint32_t)((d_energy >= 0) ? d_energy : -d_energy);
        ml_result_t ml = ml_predict(delta, d_energy, abs_de, results.idx, results.score);
        deriv_noise = ((deriv_noise * 15U) + abs_de) / 16U;
        uint32_t deriv_thresh = deriv_noise + TAP_DERIV_MARGIN;
        if (deriv_thresh < (uint32_t)TAP_DERIV_MIN)
        {
            deriv_thresh = (uint32_t)TAP_DERIV_MIN;
        }
        uint32_t neg_thresh = (deriv_thresh / 2U);
        if (neg_thresh < 4U)
        {
            neg_thresh = 4U;
        }
        uint32_t tap_fast_thresh = deriv_thresh + TAP_FAST_MARGIN;
        bool radial_fast = (abs_de >= tap_fast_thresh);
        prev_energy = energy_per_sample;

        if (push_debounce > 0U)
        {
            --push_debounce;
        }
        if (swipe_suppress > 0U)
        {
            --swipe_suppress;
        }
        if (swipe_guard > 0U)
        {
            --swipe_guard;
        }
        if (swipe_tap_lockout > 0U)
        {
            --swipe_tap_lockout;
        }
        if (swipe_vote_transition_lockout > 0U)
        {
            --swipe_vote_transition_lockout;
        }
        if (event_cooldown > 0U)
        {
            --event_cooldown;
        }
        if (tap_refractory > 0U)
        {
            --tap_refractory;
        }
        if (tap_group_timeout > 0U)
        {
            --tap_group_timeout;
            if (tap_group_timeout == 0U && tap_count > 0U)
            {
                debug_log("taps=%lu\r\n", (unsigned long)tap_count);
                indicate_tap_group(tap_count);
                tap_count = 0U;
                event_cooldown = EVENT_COOLDOWN_FRAMES;
            }
        }

        bool swipe_hint_up = false;
        bool swipe_hint_down = false;

        bool in_swipe_zone_now = (delta >= -(int32_t)SWIPE_ZONE_DELTA_MAX) &&
                                 (delta <= (int32_t)SWIPE_ZONE_DELTA_MAX);
        if (abs_de <= (SWIPE_DOPPLER_MAX_DE + 8U) && in_swipe_zone_now)
        {
            g_swipe_integrator += d_energy;
            if (g_swipe_integrator > 240)
            {
                g_swipe_integrator = 240;
            }
            else if (g_swipe_integrator < -240)
            {
                g_swipe_integrator = -240;
            }
        }
        else
        {
            g_swipe_integrator = (g_swipe_integrator * 3) / 4;
        }

        if (event_cooldown == 0U &&
            ml.klass == ML_CLASS_SWIPE_UP &&
            ml.confidence >= ML_SWIPE_CONF_MIN)
        {
            bool in_swipe_zone = (delta >= -(int32_t)SWIPE_ZONE_DELTA_MAX) && (delta <= (int32_t)SWIPE_ZONE_DELTA_MAX);
            if (abs_de <= SWIPE_DOPPLER_MAX_DE && in_swipe_zone)
            {
                swipe_hint_up = true;
                if (swipe_up_votes < 255U)
                {
                    ++swipe_up_votes;
                }
            }
        }

        if (event_cooldown == 0U && g_swipe_integrator >= 70)
        {
            swipe_hint_up = true;
            if (swipe_up_votes < 255U)
            {
                ++swipe_up_votes;
            }
            if (swipe_down_votes > 0U)
            {
                --swipe_down_votes;
            }
        }
        else if (swipe_up_votes > 0U)
        {
            --swipe_up_votes;
        }

        if (event_cooldown == 0U &&
            ml.klass == ML_CLASS_SWIPE_DOWN &&
            ml.confidence >= ML_SWIPE_CONF_MIN)
        {
            bool in_swipe_zone = (delta >= -(int32_t)SWIPE_ZONE_DELTA_MAX) && (delta <= (int32_t)SWIPE_ZONE_DELTA_MAX);
            if (abs_de <= SWIPE_DOPPLER_MAX_DE && in_swipe_zone)
            {
                swipe_hint_down = true;
                if (swipe_down_votes < 255U)
                {
                    ++swipe_down_votes;
                }
            }
        }

        if (event_cooldown == 0U && g_swipe_integrator <= -70)
        {
            swipe_hint_down = true;
            if (swipe_down_votes < 255U)
            {
                ++swipe_down_votes;
            }
            if (swipe_up_votes > 0U)
            {
                --swipe_up_votes;
            }
        }
        else if (swipe_down_votes > 0U)
        {
            --swipe_down_votes;
        }

        /* Any confident swipe estimate guards against false tap counting. */
        if ((ml.klass == ML_CLASS_SWIPE_UP || ml.klass == ML_CLASS_SWIPE_DOWN) &&
            ml.confidence >= ML_SWIPE_GUARD_CONF_MIN &&
            abs_de <= (SWIPE_DOPPLER_MAX_DE + 8U))
        {
            swipe_guard = SWIPE_GUARD_FRAMES;
            swipe_tap_lockout = SWIPE_TAP_LOCKOUT_FRAMES;
            tap_candidate = false;
            tap_confirm_timeout = 0U;
            tap_arm_age = 0U;
        }

        bool swipe_vote_active = (swipe_up_votes > 0U) || (swipe_down_votes > 0U);
        if (swipe_vote_active != swipe_vote_active_prev)
        {
            swipe_vote_transition_lockout = SWIPE_VOTE_TRANSITION_LOCKOUT_FRAMES;
            tap_candidate = false;
            tap_confirm_timeout = 0U;
            tap_arm_age = 0U;
        }
        swipe_vote_active_prev = swipe_vote_active;

        if (tap_confirm_timeout > 0U)
        {
            --tap_confirm_timeout;
            if (tap_confirm_timeout == 0U)
            {
                tap_candidate = false;
                tap_arm_age = 0U;
                tap_refractory = TAP_REFRACTORY_FRAMES;
            }
        }

        bool swipe_pending = (swipe_up_votes > 0U) || (swipe_down_votes > 0U) ||
                             swipe_hint_up || swipe_hint_down || (swipe_guard > 0U);
        bool strong_tap_fallback = (delta >= TAP_DELTA_STRONG_MIN) &&
                                   (d_energy >= 0) &&
                                   radial_fast &&
                                   (abs_de >= TAP_ABS_DE_STRONG_MIN) &&
                                   (abs_de >= (SWIPE_DOPPLER_MAX_DE + 10U)) &&
                                   (g_swipe_integrator >= -8) &&
                                   (g_swipe_integrator <= 8);

        if (event_cooldown == 0U &&
            tap_refractory == 0U &&
            swipe_suppress == 0U &&
            swipe_tap_lockout == 0U &&
            swipe_vote_transition_lockout == 0U &&
            !swipe_pending &&
            !tap_candidate &&
            push_debounce == 0U &&
            strong_tap_fallback)
        {
            tap_candidate = true;
            tap_confirm_timeout = TAP_CONFIRM_TIMEOUT_FRAMES;
            tap_arm_age = 0U;
        }

        if (event_cooldown == 0U &&
            swipe_suppress == 0U &&
            swipe_tap_lockout == 0U &&
            swipe_vote_transition_lockout == 0U &&
            !swipe_pending &&
            tap_candidate)
        {
            ++tap_arm_age;
            bool fall_confirm = (d_energy < 0) &&
                                ((uint32_t)(-d_energy) >= neg_thresh) &&
                                ((uint32_t)(-d_energy) >= TAP_NEG_DE_STRONG_MIN) &&
                                radial_fast &&
                                (tap_arm_age >= TAP_MIN_ARM_AGE) &&
                                (tap_arm_age <= TAP_ARM_MAX_AGE);
            if (fall_confirm)
            {
                if (tap_count < 3U)
                {
                    ++tap_count;
                }
                tap_group_timeout = TAP_GROUP_TIMEOUT_FRAMES;
                push_debounce = PUSH_DEBOUNCE_FRAMES;
                tap_candidate = false;
                tap_confirm_timeout = 0U;
                tap_arm_age = 0U;
                tap_refractory = TAP_REFRACTORY_FRAMES;
            }
        }

        if (!EVENT_ONLY_UART && (++debug_stream_div >= DEBUG_STREAM_DIVIDER))
        {
            debug_stream_div = 0U;
            debug_log("DBG e=%lu b=%lu d=%ld de=%ld abs=%lu idx=%lu score=%.3f ml=%s conf=%.2f up=%lu dn=%lu guard=%lu cand=%u taps=%lu\r\n",
                      (unsigned long)energy_per_sample,
                      (unsigned long)baseline,
                      (long)delta,
                      (long)d_energy,
                      (unsigned long)abs_de,
                      (unsigned long)results.idx,
                      (double)results.score,
                      ml_class_name(ml.klass),
                      (double)ml.confidence,
                      (unsigned long)swipe_up_votes,
                      (unsigned long)swipe_down_votes,
                      (unsigned long)swipe_guard,
                      tap_candidate ? 1u : 0u,
                      (unsigned long)tap_count);
        }

        /* Board orientation is reversed vs model labels on this setup:
         * model idx6 behaves like down, idx7 behaves like up. */
        if (swipe_down_votes >= SWIPE_CONFIRM_FRAMES)
        {
            debug_log("swipe up\r\n");
            indicate_swipe_up();
            swipe_up_votes = 0U;
            swipe_down_votes = 0U;
            swipe_suppress = SWIPE_SUPPRESS_FRAMES;
            swipe_guard = SWIPE_GUARD_FRAMES;
            swipe_tap_lockout = SWIPE_TAP_LOCKOUT_FRAMES;
            tap_candidate = false;
            tap_confirm_timeout = 0U;
            tap_arm_age = 0U;
            tap_count = 0U;
            tap_group_timeout = 0U;
            event_cooldown = EVENT_COOLDOWN_FRAMES;
        }
        else if (swipe_up_votes >= SWIPE_CONFIRM_FRAMES)
        {
            debug_log("swipe down\r\n");
            indicate_swipe_down();
            swipe_up_votes = 0U;
            swipe_down_votes = 0U;
            swipe_suppress = SWIPE_SUPPRESS_FRAMES;
            swipe_guard = SWIPE_GUARD_FRAMES;
            swipe_tap_lockout = SWIPE_TAP_LOCKOUT_FRAMES;
            tap_candidate = false;
            tap_confirm_timeout = 0U;
            tap_arm_age = 0U;
            tap_count = 0U;
            tap_group_timeout = 0U;
            event_cooldown = EVENT_COOLDOWN_FRAMES;
        }

        if (motion_on)
        {
            motion_latched = true;
            motion_release_count = 0U;
        }
        else if (motion_latched)
        {
            if (++motion_release_count >= MOTION_RELEASE_FRAMES)
            {
                motion_latched = false;
                motion_release_count = 0U;
            }
        }

        CY_UNUSED_PARAMETER(gesture_detected);
    }
}
