/*
 * C
 *
 * Copyright 2020-2021 MicroEJ Corp. All rights reserved.
 * This library is provided in source code for use, modification and test, subject to license terms.
 * Any modification of the source code will break MicroEJ Corp. warranties on the whole library.
 */
 
/**
 * @file
 * @brief LLMJVM implementation over MicriumOS.
 * @author MicroEJ Developer Team
 * @version 1.0.0
 */

/* Includes ------------------------------------------------------------------*/

#include "misra_2004_conf.h"

MISRA_2004_DISABLE_ALL
#include "microej_time.h"
#include "LLMJVM_impl.h"
#include "microej.h"
#include "sni.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <os.h>
MISRA_2004_ENABLE_ALL

/* Defines -------------------------------------------------------------------*/

/* Initial delay of the timer */
#define LLMJVM_MICRIUMOS_WAKEUP_TIMER_DELAY_MS          ((int64_t)100)

/* The 'period' being repeated for the timer. */
#define LLMJVM_MICRIUMOS_WAKEUP_TIMER_PERIOD_MS         ((int64_t)0)

/* Globals -------------------------------------------------------------------*/

/*
 * Shared variables
 */
/* Absolute time in ms at which timer will be launched */
static int64_t LLMJVM_MICRIUMOS_next_wake_up_time = INT64_MAX;

/* Set to true when the timer expires, set to true when the timer is started. */
volatile bool LLMJVM_MICRIUMOS_timer_expired = false;

/* Timer for scheduling next alarm */
static OS_TMR LLMJVM_MICRIUMOS_wake_up_timer;

/* Binary semaphore to wakeup MicroJVM */
static OS_SEM LLMJVM_MICRIUMOS_Semaphore;

/* Private functions ---------------------------------------------------------*/

static void wake_up_timer_callback(void *p_tmr, void *p_arg);

/* Since LLMJVM_schedule() prototype does not match the timer callback prototype,
   we create a wrapper around it and check the ID of the timer. */
static void wake_up_timer_callback(void *p_tmr, void *p_arg) {
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

/* Creates the timer used to callback the LLMJVM_schedule() function.
   After its creation, the timer is idle. */
int32_t LLMJVM_IMPL_initialize(void) {
  int32_t rslt = LLMJVM_OK;
  RTOS_ERR err;

  if (OSCfg_TmrTaskRate_Hz <= (OS_RATE_HZ) 0) {
      /* MicriumOS timer task disabled or not configured correctly. */
      rslt = LLMJVM_ERROR;
  }

  if (rslt == LLMJVM_OK) {
    /* Create a timer to scheduler alarm for the VM. Delay and period values are dummy initialization values which will never be used. */
    OSTmrCreate(&LLMJVM_MICRIUMOS_wake_up_timer,
                "MicroJVM wake up",
                (OS_TICK)microej_time_time_to_tick(LLMJVM_MICRIUMOS_WAKEUP_TIMER_DELAY_MS),
                (OS_TICK)microej_time_time_to_tick(LLMJVM_MICRIUMOS_WAKEUP_TIMER_PERIOD_MS),
                OS_OPT_TMR_ONE_SHOT,
                wake_up_timer_callback,
                NULL,
                &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        rslt = LLMJVM_ERROR;
    }
  }

  if (rslt == LLMJVM_OK) {
    OSSemCreate(&LLMJVM_MICRIUMOS_Semaphore,
                "MicroJVM wake up",
                (CPU_INT32U)0,
                &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
        rslt = LLMJVM_ERROR;
    }
  }

  return rslt;
}

/*
 * Once the task is started, save a handle to it.
 */
int32_t LLMJVM_IMPL_vmTaskStarted(void) {
    return LLMJVM_OK;
}

/*
 * Schedules requests from the VM
 */
int32_t LLMJVM_IMPL_scheduleRequest(int64_t absoluteTime) {
  int32_t rslt = LLMJVM_OK;
  int64_t relativeTime, relativeTick; 
  CPU_BOOLEAN xTimerStartResult;
  RTOS_ERR err_set, err_start, err_stop;

  int64_t currentTime = LLMJVM_IMPL_getCurrentTime((uint8_t)MICROEJ_TRUE);

  relativeTime = absoluteTime - currentTime;
  /* Determine relative time/tick */
  relativeTick = microej_time_time_to_tick(relativeTime);

  if (relativeTick <= 0) {
    /* 'absoluteTime' has been reached yet */

    /* No pending request anymore */
    LLMJVM_MICRIUMOS_next_wake_up_time = INT64_MAX;

    /* Stop current timer (no delay) */
    if (LLMJVM_MICRIUMOS_wake_up_timer.State == OS_TMR_STATE_RUNNING) {
        OSTmrStop(&LLMJVM_MICRIUMOS_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
        EFM_ASSERT((RTOS_ERR_CODE_GET(err_stop) == RTOS_ERR_NONE));
    }

    /* Notify the VM now */
    rslt = LLMJVM_schedule();
  } else if ((LLMJVM_MICRIUMOS_timer_expired == true) ||
        (absoluteTime < LLMJVM_MICRIUMOS_next_wake_up_time) ||
        (LLMJVM_MICRIUMOS_next_wake_up_time <= currentTime)) {
      /* We want to schedule a request in the future but before the existing request
         or the existing request is already done */

      /* Save new alarm absolute time */
      LLMJVM_MICRIUMOS_next_wake_up_time = absoluteTime;

      /* Stop current timer (no delay) */
      if (OS_TMR_STATE_RUNNING == LLMJVM_MICRIUMOS_wake_up_timer.State) {
          OSTmrStop(&LLMJVM_MICRIUMOS_wake_up_timer, OS_OPT_TMR_NONE, NULL, &err_stop);
          EFM_ASSERT((RTOS_ERR_CODE_GET(err_stop) == RTOS_ERR_NONE));
      }
      LLMJVM_MICRIUMOS_timer_expired = false;

      /* Schedule the new alarm */
      OSTmrSet(&LLMJVM_MICRIUMOS_wake_up_timer,
               (OS_TICK)relativeTick,
               (OS_TICK)0,
               wake_up_timer_callback,
               NULL,
               &err_set);
      xTimerStartResult = OSTmrStart(&LLMJVM_MICRIUMOS_wake_up_timer, &err_start);

      if ((xTimerStartResult != DEF_TRUE) ||
          (RTOS_ERR_CODE_GET(err_set) != RTOS_ERR_NONE) ||
          (RTOS_ERR_CODE_GET(err_start) != RTOS_ERR_NONE)) {
          rslt = LLMJVM_ERROR;
      }
  } else {
    /* else: there is a pending request that will occur before the new one -> do nothing */
  }
  
  return rslt;
}

/*
 * Suspends the VM task if the pending flag is not set
 */
int32_t LLMJVM_IMPL_idleVM(void) {
    RTOS_ERR err;
    OSSemPend(&LLMJVM_MICRIUMOS_Semaphore,
              (OS_TICK)0,
              OS_OPT_PEND_BLOCKING,
              NULL,
              &err);

    return (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) ? (int32_t)LLMJVM_OK : (int32_t)LLMJVM_ERROR;
}

/*
 * Wakes up the VM task
 */
int32_t LLMJVM_IMPL_wakeupVM(void) {
    RTOS_ERR err;
    OSSemPost(&LLMJVM_MICRIUMOS_Semaphore,
              OS_OPT_POST_ALL,
              &err);

    return (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) ? (int32_t)LLMJVM_OK : (int32_t)LLMJVM_ERROR;
}

/*
 * Clear the pending wake up flag and reset next wake up time
 */
int32_t LLMJVM_IMPL_ackWakeup(void) {
    return LLMJVM_OK;
}

/* 
 * Gets current task identifier
 */
MISRA_2004_DISABLE_RULE_11_3
int32_t LLMJVM_IMPL_getCurrentTaskID(void) {
    return (int32_t)OSTCBCurPtr;
}
MISRA_2004_ENABLE_ALL

/* 
 * Sets application time
 */
void LLMJVM_IMPL_setApplicationTime(int64_t t) {
    microej_time_set_application_time(t);
}

/* 
 * Gets the system or the application time in milliseconds
 */
MISRA_2004_DISABLE_RULE_16_4
int64_t LLMJVM_IMPL_getCurrentTime(uint8_t sys) {
    return microej_time_get_current_time(sys);
}
MISRA_2004_ENABLE_ALL

/*
 * Gets the current system time in nanoseconds
 */
int64_t LLMJVM_IMPL_getTimeNanos(void) {
    return microej_time_get_time_nanos();
}

/*
 * Shuts the system down
 */
int32_t LLMJVM_IMPL_shutdown(void) {
    return LLMJVM_OK;
}
