/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

//#include "xparameters.h"
//#include <platform/gic.h>

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/*
 * The FreeRTOS Cortex-A port implements a full interrupt nesting model.
 *
 * Interrupts that are assigned a priority at or below
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which counter-intuitively in the ARM
 * generic interrupt controller [GIC] means a priority that has a numerical
 * value above configMAX_API_CALL_INTERRUPT_PRIORITY) can call FreeRTOS safe API
 * functions and will nest.
 *
 * Interrupts that are assigned a priority above
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which in the GIC means a numerical
 * value below configMAX_API_CALL_INTERRUPT_PRIORITY) cannot call any FreeRTOS
 * API functions, will nest, and will not be masked by FreeRTOS critical
 * sections (although it is necessary for interrupts to be globally disabled
 * extremely briefly as the interrupt mask is updated in the GIC).
 *
 * FreeRTOS functions that can be called from an interrupt are those that end in
 * "FromISR".  FreeRTOS maintains a separate interrupt safe API to enable
 * interrupt entry to be shorter, faster, simpler and smaller.
 *
 * For the purpose of setting configMAX_API_CALL_INTERRUPT_PRIORITY 255
 * represents the lowest priority.
 */

/* symbol from linker */
extern const unsigned long __internal_heap_size;
extern const unsigned long __external_heap_size;

#define configMAX_API_CALL_INTERRUPT_PRIORITY	18

#define configAPPLICATION_ALLOCATED_HEAP        1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1
#define configUSE_TICKLESS_IDLE					0
#define configTICK_RATE_HZ						( ( TickType_t ) 1000 )
#define configPERIPHERAL_CLOCK_HZ  				( 12000000UL )
#define configTICK_OVF_VAL						( configPERIPHERAL_CLOCK_HZ / configTICK_RATE_HZ )
#define configUSE_PREEMPTION					1
#define configUSE_IDLE_HOOK						1
#define configUSE_TICK_HOOK						1
#define configMAX_PRIORITIES					( 32 )
#define configMINIMAL_STACK_SIZE				( ( unsigned short ) 512 )
#define configTOTAL_INTERNAL_HEAP_SIZE			(size_t)(&__internal_heap_size) /* internal heap size */
#define configTOTAL_EXTERNAL_HEAP_SIZE			(size_t)(&__external_heap_size) /* external heap size */
#define configMAX_TASK_NAME_LEN					( 20 )
#define configUSE_TRACE_FACILITY				1
#define configUSE_16_BIT_TICKS					0
#define configIDLE_SHOULD_YIELD					1
#define configUSE_MUTEXES						1
#define configQUEUE_REGISTRY_SIZE				8
#define configCHECK_FOR_STACK_OVERFLOW			2
#define configUSE_RECURSIVE_MUTEXES				1
#define configUSE_MALLOC_FAILED_HOOK			1
#define configUSE_APPLICATION_TASK_TAG			0
#define configUSE_COUNTING_SEMAPHORES			1
#define configUSE_QUEUE_SETS					1
#define configUSE_TASK_NOTIFICATIONS			1


/* This demo creates RTOS objects using both static and dynamic allocation. */
#define configSUPPORT_STATIC_ALLOCATION			1
#define configSUPPORT_DYNAMIC_ALLOCATION		1 /* Defaults to 1 anyway. */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 					0
#define configMAX_CO_ROUTINE_PRIORITIES 		2

/* Software timer definitions. */
#define configUSE_TIMERS						1
#define configTIMER_TASK_PRIORITY				( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH				10
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 4 ) //unit portSTACK_TYPE

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskCleanUpResources			1
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTimerPendFunctionCall			1
#define INCLUDE_eTaskGetState					1
#define INCLUDE_xTaskAbortDelay					1
#define INCLUDE_xTaskGetHandle					1
#define INCLUDE_xSemaphoreGetMutexHolder		1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark		1

/* This demo makes use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	0

/* Run time stats are not generated.  portCONFIGURE_TIMER_FOR_RUN_TIME_STATS and
portGET_RUN_TIME_COUNTER_VALUE must be defined if configGENERATE_RUN_TIME_STATS
is set to 1. */
#define configGENERATE_RUN_TIME_STATS 			1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()

unsigned long long vPortGetCurrentTimeUs( void );
#define portGET_RUN_TIME_COUNTER_VALUE() vPortGetCurrentTimeUs()

/* The size of the global output buffer that is available for use when there
are multiple command interpreters running at once (for example, one on a UART
and one on TCP/IP).  This is done to prevent an output buffer being defined by
each implementation - which would waste RAM.  In this case, there is only one
command interpreter running. */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE 		2096

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
void vMainAssertCalled( const char *pcFileName, uint32_t ulLineNumber );
#define configASSERT( x ) if( ( x ) == 0 ) { vMainAssertCalled( __FILE__, __LINE__ ); }

/* If configTASK_RETURN_ADDRESS is not defined then a task that attempts to
return from its implementing function will end up in a "task exit error"
function - which contains a call to configASSERT().  However this can give GCC
some problems when it tries to unwind the stack, as the exit error function has
nothing to return to.  To avoid this define configTASK_RETURN_ADDRESS to 0.  */

#define configTASK_RETURN_ADDRESS				NULL

#define configTASK_TIMER_STACK_SIZE				configTIMER_TASK_STACK_DEPTH
#define configTASK_IDLE_STACK_SIZE				128

/****** Hardware specific settings. *******************************************/

/*
 * The application must provide a function that configures a peripheral to
 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in FreeRTOSConfig.h to call the function.  This file contains a function
 * that is suitable for use on the Zynq MPU.  FreeRTOS_Tick_Handler() must
 * be installed as the peripheral's interrupt handler.
 */
void vConfigureTickInterrupt( void );
#define configSETUP_TICK_INTERRUPT() vConfigureTickInterrupt()

void vClearTickInterrupt( void );
#define configCLEAR_TICK_INTERRUPT() vClearTickInterrupt()

/* The following constant describe the hardware, and are correct for the
semidrive. */

/******************* GIC was defined in target_res *******************/
#include <target_res.h>

#define configINTERRUPT_CONTROLLER_BASE_ADDRESS 		( GIC_BASE + GICD_OFFSET)
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET ( GIC_BASE + GICC_OFFSET - configINTERRUPT_CONTROLLER_BASE_ADDRESS)
/***********************************************************************************/
#define configUNIQUE_INTERRUPT_PRIORITIES				( 32 )

void traceTASK_SWITCHED_OUT_HOOK(void);
#define traceTASK_SWITCHED_OUT() traceTASK_SWITCHED_OUT_HOOK()
void traceTASK_SWITCHED_IN_HOOK(void);
#define traceTASK_SWITCHED_IN() traceTASK_SWITCHED_IN_HOOK()


/****************** Semidrive Kernel Monitor Configuration ******************/

#if (SDRV_KERNEL_MONITOR == 1)
    #include <spinlock.h>
    #include <stdio.h>
    #define SKTrace(...) do \
    {\
        spin_lock_t dbglock = SPIN_LOCK_INITIAL_VALUE;\
        spin_lock_saved_state_t state;\
        spin_lock_irqsave(&dbglock,state);\
        printf("<SKTrace>:    ");\
        printf(__VA_ARGS__);\
        spin_unlock_irqrestore(&dbglock,state);\
    } while(0)

#else
    #define SKTrace(...) do{}while(0)

#endif /* SDRV_KERNEL_MONITOR */

#endif /* FREERTOS_CONFIG_H */


