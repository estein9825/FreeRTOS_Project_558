// system_init.c (or a similar file)

#include "FreeRTOSConfig.h"
#include "CMSDK_CM3.h"
#include "cmsis_gcc.h"
#include "tiny_print.h"

// #define configPRIO_BITS                             ( 4 )
// #define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     ( 15 )
// #define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY ( 5 )

void vApplicationSetupInterrupts(void)
{
    // Print debug information
    printf("configKERNEL_INTERRUPT_PRIORITY: %u\n", configKERNEL_INTERRUPT_PRIORITY);
    printf("configMAX_SYSCALL_INTERRUPT_PRIORITY: %u\n", configMAX_SYSCALL_INTERRUPT_PRIORITY);

    // Set priorities for system exceptions

    // PendSV: Lowest priority (unshifted: configLIBRARY_LOWEST_INTERRUPT_PRIORITY)
    NVIC_SetPriority(PendSV_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
    printf("PendSV Priority: %lu\n", NVIC_GetPriority(PendSV_IRQn));

    // SysTick: Maximum syscall priority (unshifted: configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
    NVIC_SetPriority(SysTick_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    printf("SysTick Priority: %lu\n", NVIC_GetPriority(SysTick_IRQn));

    // HardFault: Highest priority (0)
    NVIC_SetPriority(HardFault_IRQn, 0);

    // Peripheral interrupts with priorities just above syscall priority
    NVIC_SetPriority(TIMER0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_EnableIRQ(TIMER0_IRQn);

    NVIC_SetPriority(TIMER1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_EnableIRQ(TIMER1_IRQn);

    // UART RX/TX with lower priority than Timer0/Timer1
    NVIC_SetPriority(UARTRX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_EnableIRQ(UARTRX0_IRQn);

    NVIC_SetPriority(UARTTX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_EnableIRQ(UARTTX0_IRQn);
}
