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
File    : OS_StartLEDBlink.c
Purpose : embOS sample program running two simple tasks, each toggling
          an LED of the target hardware (as configured in BSP.c).
*/

#include "RTOS.h"
#include "BSP.h"
#include "led.h"

static OS_STACKPTR int StackHP[128], StackLP[128],StackOther[128],StackLed[128] ; // Task stacks
static OS_TASK         TCBHP, TCBLP,TCBOTHER,TCBLED;                // Task control blocks

static void HPTask(void) {
  while (1) {
    BSP_ToggleLED(0);
    OS_TASK_Delay(50);
  }
}

static void LPTask(void) {
  while (1) {
    BSP_ToggleLED(1);
    OS_TASK_Delay(200);
  }
}
static void OtherTask(void){
	while(1)
	{
		BSP_ToggleLED(2);
		OS_TASK_Delay(100);
	}
}
static void LedTask(void){
	while(1)
	{
		LED1^=1;
		OS_TASK_Delay(100);
	}
}
/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  BSP_Init();   // Initialize LED ports
	LED_Init();
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
	OS_TASK_CREATE(&TCBOTHER, "Other Task",  50, OtherTask, StackOther);
	OS_TASK_CREATE(&TCBLED, "LED Task",  25, LedTask, StackLed);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
