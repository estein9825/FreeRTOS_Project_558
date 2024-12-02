/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <stdint.h>
// #include <stdio.h>
#include "tiny_print.h"
#include "cmsis_gcc.h"  // This provides the necessary CMSIS functions


/* FreeRTOS interrupt handlers. */
extern void vPortSVCHandler( void );
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );
extern void TIMER0_Handler( void );
extern void TIMER1_Handler( void );

/* Exception handlers. */
static void HardFault_Handler( void ) __attribute__( ( naked ) );
static void Default_Handler( void ) __attribute__( ( naked ) );
void Reset_Handler( void ) __attribute__( ( naked ) );

extern int main( void );
extern uint32_t _estack;

/* Vector table. */
const uint32_t* isr_vector[] __attribute__((section(".isr_vector"), used)) =
{
    ( uint32_t * ) &_estack,
    ( uint32_t * ) &Reset_Handler,     // Reset                -15
    ( uint32_t * ) &Default_Handler,   // NMI_Handler          -14
    ( uint32_t * ) &HardFault_Handler, // HardFault_Handler    -13
    ( uint32_t * ) &Default_Handler,   // MemManage_Handler    -12
    ( uint32_t * ) &Default_Handler,   // BusFault_Handler     -11
    ( uint32_t * ) &Default_Handler,   // UsageFault_Handler   -10
    0, // reserved   -9
    0, // reserved   -8
    0, // reserved   -7
    0, // reserved   -6
    ( uint32_t * ) &vPortSVCHandler,    // SVC_Handler          -5
    ( uint32_t * ) &Default_Handler,    // DebugMon_Handler     -4
    0, // reserved   -3
    ( uint32_t * ) &xPortPendSVHandler, // PendSV handler       -2
    ( uint32_t * ) &xPortSysTickHandler,// SysTick_Handler      -1
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // Ethernet   13
};

void Reset_Handler( void )
{
    main();
}
/* Variables used to store the value of registers at the time a hardfault
 * occurs. These are volatile to try and prevent the compiler/linker optimizing
 * them away as the variables never actually get used. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr; /* Program status register. */

#define SCB_CFSR    (*(volatile uint32_t*)0xE000ED28)
#define SCB_HFSR    (*(volatile uint32_t*)0xE000ED2C)


/* Called from the hardfault handler to provide information on the processor
 * state at the time of the fault. */
__attribute__((used)) void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress)
{
    // Save the stack register values
    r0 = pulFaultStackAddress[0];
    r1 = pulFaultStackAddress[1];
    r2 = pulFaultStackAddress[2];
    r3 = pulFaultStackAddress[3];
    
    r12 = pulFaultStackAddress[4];
    lr = pulFaultStackAddress[5];
    pc = pulFaultStackAddress[6];
    psr = pulFaultStackAddress[7];

    // Print the register values for debugging
    printf("HardFault Handler: Registers at the time of the fault:\n");
    printf("r0  = 0x%08lx\n", r0);
    printf("r1  = 0x%08lx\n", r1);
    printf("r2  = 0x%08lx\n", r2);
    printf("r3  = 0x%08lx\n", r3);
    printf("r12 = 0x%08lx\n", r12);
    printf("lr  = 0x%08lx\n", lr);
    printf("pc  = 0x%08lx\n", pc);
    printf("psr = 0x%08lx\n", psr);

    // Check if the fault was caused by an exception in the handler
    if (psr & (1 << 24)) {
        printf("Fault caused by exception in handler\n");
    } else {
        printf("Fault caused by exception in non-handler\n");
    }

    // Additional info: Check if we're in Thread or Handler mode
    if (psr & (1 << 24)) {
        printf("We are in Handler mode (IRQ context)\n");
    } else {
        printf("We are in Thread mode (Non-IRQ context)\n");
    }

    // If you have a debugger or logging system, you can also print the MSP and PSP
    uint32_t msp = __get_MSP(); // Get the Main Stack Pointer
    uint32_t psp = __get_PSP(); // Get the Process Stack Pointer
    printf("MSP = 0x%08lx\n", msp);
    printf("PSP = 0x%08lx\n", psp);
    printf("CFSR = 0x%08lx\n", SCB_CFSR);
    printf("HFSR = 0x%08lx\n", SCB_HFSR);
    // Loop indefinitely to halt execution after printing information
    while (1);
}


void Default_Handler( void )
{
    __asm volatile
    (
        ".align 8                                \n"
        " ldr r3, =0xe000ed04                    \n" /* Load the address of the interrupt control register into r3. */
        " ldr r2, [r3, #0]                       \n" /* Load the value of the interrupt control register into r2. */
        " uxtb r2, r2                            \n" /* The interrupt number is in the least significant byte - clear all other bits. */
        "Infinite_Loop:                          \n" /* Sit in an infinite loop - the number of the executing interrupt is held in r2. */
        " b  Infinite_Loop                       \n"
        " .ltorg                                 \n"
    );
}

void HardFault_Handler( void )
{
    __asm volatile
    (
        ".align 8                                                   \n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, =prvGetRegistersFromStack                         \n"
        " bx r2                                                     \n"
        " .ltorg                                                    \n"
    );
}


