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

----------------------------------------------------------------------
File    : BSP.c
Purpose : BSP for STM32F103 (IAR STM32-SK)
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "BSP.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/****** LED port assignement ****************************************/
#define GPIO_PA0_BASE_ADDR    (0x40010800u)
#define GPIOA_CRL             (*(volatile unsigned int*)(GPIO_PA0_BASE_ADDR))
#define GPIOA_ODR             (*(volatile unsigned int*)(GPIO_PA0_BASE_ADDR + 0x0Cu))
#define GPIOA_BSRR            (*(volatile unsigned int*)(GPIO_PA0_BASE_ADDR + 0x10u))
#define GPIOA_BRR             (*(volatile unsigned int*)(GPIO_PA0_BASE_ADDR + 0x14u))

/****** SFRs used for LED-Port **************************************/
#define SYSCTRL_RCC_APB2      (*(volatile unsigned int*)(0x40021018u))
#define SYSCTRL_LEDPORT_BIT   (2u)

/****** Assign LEDs to Ports ****************************************/
#define LED0_BIT              (4u)  // L1
#define LED1_BIT              (5u)  // L2
#define LED2_BIT              (6u)  // L3
#define LED3_BIT              (7u)  // L4
#define LED_MASK_ALL          ((1u << LED0_BIT) | (1u << LED1_BIT) | (1u << LED2_BIT) | (1u << LED3_BIT) )

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  SYSCTRL_RCC_APB2 |= (1 << SYSCTRL_LEDPORT_BIT);
  GPIOA_CRL         = (GPIOA_CRL & ~(0xFFFF0000uL)) | 0x33330000uL;
  GPIOA_BSRR        |= LED_MASK_ALL;
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  GPIOA_BRR |= (1uL << (LED0_BIT + Index));
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  GPIOA_BSRR  |= (1uL << (LED0_BIT + Index));
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if ((GPIOA_ODR & (1u << (LED0_BIT + Index))) == 0u) {
    BSP_ClrLED(Index);
  } else {
    BSP_SetLED(Index);
  }
}

/****** End Of File *************************************************/
