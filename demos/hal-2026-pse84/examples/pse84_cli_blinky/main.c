#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

int main(void)
{
    cy_rslt_t result = cybsp_init();
    if (result != CY_RSLT_SUCCESS) {
        CY_ASSERT(0);
    }

    __enable_irq();

    for (;;) {
        cyhal_gpio_toggle(CYBSP_USER_LED1);
        cyhal_system_delay_ms(500);
    }
}
