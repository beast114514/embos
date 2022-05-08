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
File    : OS_ObjectIdentifier.c
Purpose : embOS sample program for Human readable object identifiers
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128];  // Task stacks
static OS_TASK         TCBHP;         // Task control blocks
static OS_U8           MailboxA_Buffer[10];
static OS_U8           MailboxB_Buffer[10];
static OS_MAILBOX      MailboxA, MailboxB;
static OS_OBJNAME      MailboxA_Name, MailboxB_Name;

static void _PrintMailboxNames(void) {
  OS_COM_SendString("\nMailboxA: ");
  OS_COM_SendString(OS_DEBUG_GetObjName(&MailboxA));
  OS_COM_SendString("\nMailboxB: ");
  OS_COM_SendString(OS_DEBUG_GetObjName(&MailboxB));
}

static void HPTask(void) {
  _PrintMailboxNames();
  while (1) {
    OS_TASK_Delay(50);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_CREATEMB(&MailboxA, 5, 2, MailboxA_Buffer);
  OS_CREATEMB(&MailboxB, 5, 2, MailboxB_Buffer);
  OS_DEBUG_SetObjName(&MailboxA_Name, &MailboxA, "Mailbox A");
  OS_DEBUG_SetObjName(&MailboxB_Name, &MailboxB, "Mailbox B");
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
