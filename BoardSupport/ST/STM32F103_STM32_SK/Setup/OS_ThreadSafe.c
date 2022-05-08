/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2022 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system                           *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.16.1.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_ThreadSafe.c
Purpose : Thread safe library functions

Additional information:
  This module enables thread and/or interrupt safety for e.g. malloc().
  Per default it ensures thread and interrupt safety by disabling/restoring
  embOS interrupts. Zero latency interrupts are not affected and protected.
  If you need to call e.g. malloc() also from within a zero latency interrupt
  additional handling needs to be added.
  If you don't call such functions from within embOS interrupts you can use
  thread safety instead. This reduces the interrupt latency because a mutex
  is used instead of disabling embOS interrupts.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
  extern "C" {
#endif

int  _mutex_initialize(void* mutex) OS_TEXT_SECTION_ATTRIBUTE(_mutex_initialize);
void _mutex_acquire   (void* mutex) OS_TEXT_SECTION_ATTRIBUTE(_mutex_acquire);
void _mutex_release   (void* mutex) OS_TEXT_SECTION_ATTRIBUTE(_mutex_release);
void _mutex_free      (void* mutex) OS_TEXT_SECTION_ATTRIBUTE(_mutex_free);

#ifdef __cplusplus
  }
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// When set to 1 thread and interrupt safety is guaranteed.
//
// When set to 0 only thread safety is guaranteed. In this case you
// must not call e.g. heap functions from ISRs, software timers or OS_Idle().
//
#ifndef   OS_INTERRUPT_SAFE
  #define OS_INTERRUPT_SAFE  1
#endif

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _mutex_initialize()
*/
int _mutex_initialize(void* mutex) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_USE_PARA(mutex);
  return 1;  // Always return 1 to indicate a multi-tasking environment
#else
  return OS_mutex_initialize(mutex);
#endif
}

/*********************************************************************
*
*       _mutex_free()
*/
void _mutex_free(void* mutex) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_USE_PARA(mutex);
#else
  OS_mutex_free(mutex);
#endif
}

/*********************************************************************
*
*       _mutex_acquire()
*/
void _mutex_acquire(void* mutex) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_USE_PARA(mutex);
  OS_InterruptSafetyLock();
#else
  OS_mutex_acquire(mutex);
#endif
}

/*********************************************************************
*
*       _mutex_release()
*/
void _mutex_release(void* mutex) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_USE_PARA(mutex);
  OS_InterruptSafetyUnlock();
#else
  OS_mutex_release(mutex);
#endif
}

/*************************** End of file ****************************/
