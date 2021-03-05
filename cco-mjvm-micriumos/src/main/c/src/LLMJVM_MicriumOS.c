/*
 * C
 *
 * Copyright 2021 MicroEJ Corp. All rights reserved.
 * This library is provided in source code for use, modification and test, subject to license terms.
 * Any modification of the source code will break MicroEJ Corp. warranties on the whole library.
 */
 
/**
 * @file
 * @brief LLMJVM implementation over MicriumOS.
 * @author @CCO_AUTHOR@
 * @version @CCO_VERSION@
 * @date @CCO_DATE@
 */

/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "microej_time.h"
#include "LLMJVM_impl.h"
#include "microej.h"
#include "sni.h"
#include "os.h"

/* Defines -------------------------------------------------------------------*/

// Initial delay of the timer
#define LLMJVM_MICRIUMOS_WAKEUP_TIMER_INIT_DELAY_MS  100

// The 'period' being repeated for the timer.
#define LLMJVM_MICRIUMOS_WAKEUP_TIMER_INIT_PERIOD_MS 0

/* Globals -------------------------------------------------------------------*/

/*
 * Shared variables
 */
// Absolute time in ms at which timer will be launched
static int64_t LLMJVM_MICRIUMOS_next_wake_up_time = INT64_MAX;

// Set to true when the timer expires, set to true when the timer is started.
volatile bool LLMJVM_MICRIUMOS_timer_expired = false;

// Timer for scheduling next alarm
static OS_TMR LLMJVM_MICRIUMOS_wake_up_timer;

// Binary semaphore to wakeup MicroJVM
static OS_SEM LLMJVM_MICRIUMOS_Semaphore;

/* Private functions ---------------------------------------------------------*/

// Since LLMJVM_schedule() prototype does not match the timer callback prototype,
// we create a wrapper around it and check the ID of the timer.
static void wake_up_timer_callback(void *p_tmr, void *p_arg) {
    (void)p_arg;

    if (p_tmr == &LLMJVM_MICRIUMOS_wake_up_timer) {
        LLMJVM_MICRIUMOS_timer_expired = true;
        LLMJVM_schedule();
    }
}

/* Public functions ----------------------------------------------------------*/

/*
 * Implementation of functions from LLMJVM_impl.h
 * and other helping functions.
 */

// Creates the timer used to callback the LLMJVM_schedule() function.
// After its creation, the timer is idle.
int32_t LLMJVM_IMPL_initialize() {
    RTOS_ERR err;

    if (OSCfg_TmrTaskRate_Hz <= (OS_RATE_HZ) 0) {
        // MicriumOS timer task disabled or not configured correctly.
        return LLMJVM_ERROR;
    }

    // Create a timer to scheduler alarm for the VM. Delay and period values are dummy initialization values which will never be used.
    OSTmrCreate(&LLMJVM_MICRIUMOS_wake_up_timer,
                "MicroJVM wake up",
                microej_time_timeToTick(LLMJVM_MICRIUMOS_WAKEUP_TIMER_INIT_DELAY_MS),
                microej_time_timeToTick(LLMJVM_MICRIUMOS_WAKEUP_TIMER_INIT_PERIOD_MS),
                OS_OPT_TMR_ONE_SHOT,
                wake_up_timer_callback,
                NULL,
                &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        return LLMJVM_ERROR;
    }

    OSSemCreate(&LLMJVM_MICRIUMOS_Semaphore,
                "MicroJVM wake up",
                0,
                &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        return LLMJVM_ERROR;
    }

    return LLMJVM_OK;
}

// Once the task is started, save a handle to it.
int32_t LLMJVM_IMPL_vmTaskStarted() {
    return LLMJVM_OK;
}

// Schedules requests from the VM
int32_t LLMJVM_IMPL_scheduleRequest(int64_t absoluteTime) {
    int64_t relativeTime; // relative from current time to 'absoluteTime'
    int64_t relativeTick;
    CPU_BOOLEAN xTimerStartResult;
    RTOS_ERR err_set;
    RTOS_ERR err_start;
    RTOS_ERR err_stop;

    int64_t currentTime = LLMJVM_IMPL_getCurrentTime(MICROEJ_TRUE);

    relativeTime = absoluteTime - currentTime;
    // Determine relative time/tick
    relativeTick = microej_time_timeToTick(relativeTime);

    if (relativeTick <= 0) {
        // 'absoluteTime' has been reached yet

        // No pending request anymore
        LLMJVM_MICRIUMOS_next_wake_up_time = INT64_MAX;

        // Stop current timer (no delay)
        if (LLMJVM_MICRIUMOS_wake_up_timer.State == OS_TMR_STATE_RUNNING) {
            OSTmrStop(&LLMJVM_MICRIUMOS_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
            EFM_ASSERT((RTOS_ERR_CODE_GET(err_stop) == RTOS_ERR_NONE));
        }

        // Notify the VM now
        return LLMJVM_schedule();
    }

    // We want to schedule a request in the future
    if ((LLMJVM_MICRIUMOS_timer_expired == true) ||
        (absoluteTime < LLMJVM_MICRIUMOS_next_wake_up_time) ||
        (LLMJVM_MICRIUMOS_next_wake_up_time <= currentTime)) {
        // We want to schedule a request in the future but before the existing request
        // or the existing request is already done

        // Save new alarm absolute time
        LLMJVM_MICRIUMOS_next_wake_up_time = absoluteTime;

        // Stop current timer (no delay)
        if (LLMJVM_MICRIUMOS_wake_up_timer.State == OS_TMR_STATE_RUNNING) {
            OSTmrStop(&LLMJVM_MICRIUMOS_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
            EFM_ASSERT((RTOS_ERR_CODE_GET(err_stop) == RTOS_ERR_NONE));
        }
        LLMJVM_MICRIUMOS_timer_expired = false;

        // Schedule the new alarm
        OSTmrSet(&LLMJVM_MICRIUMOS_wake_up_timer,
                 relativeTick,
                 0,
                 wake_up_timer_callback,
                 NULL,
                 &err_set);
        xTimerStartResult = OSTmrStart(&LLMJVM_MICRIUMOS_wake_up_timer, &err_start);

        if ((xTimerStartResult != DEF_TRUE) ||
            (RTOS_ERR_CODE_GET(err_set) != RTOS_ERR_NONE) ||
            (RTOS_ERR_CODE_GET(err_start) != RTOS_ERR_NONE)) {
            return LLMJVM_ERROR;
        }
    }
    // else: there is a pending request that will occur before the new one -> do nothing

    return LLMJVM_OK;
}

// Suspends the VM task if the pending flag is not set
int32_t LLMJVM_IMPL_idleVM() {
    RTOS_ERR err;
    OSSemPend(&LLMJVM_MICRIUMOS_Semaphore,
              0,
              OS_OPT_PEND_BLOCKING,
              NULL,
              &err);

    return RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE ? LLMJVM_OK : LLMJVM_ERROR;
}

// Wakes up the VM task
int32_t LLMJVM_IMPL_wakeupVM() {
    RTOS_ERR err;
    OSSemPost(&LLMJVM_MICRIUMOS_Semaphore,
              OS_OPT_POST_ALL,
              &err);

    return RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE ? LLMJVM_OK : LLMJVM_ERROR;
}

// Clear the pending wake up flag and reset next wake up time
int32_t LLMJVM_IMPL_ackWakeup() {
    return LLMJVM_OK;
}

int32_t LLMJVM_IMPL_getCurrentTaskID() {
    return (int32_t)OSTCBCurPtr;
}

// Sets application time
void LLMJVM_IMPL_setApplicationTime(int64_t t) {
    microej_time_setapplicationtime(t);
}

// Gets the system or the application time in milliseconds
int64_t LLMJVM_IMPL_getCurrentTime(uint8_t system) {
    return microej_time_getcurrenttime(system);
}

// Gets the current system time in nanoseconds
int64_t LLMJVM_IMPL_getTimeNanos() {
    return microej_time_gettimenanos();
}

int32_t LLMJVM_IMPL_shutdown(void) {
    // nothing to do
    return LLMJVM_OK;
}
