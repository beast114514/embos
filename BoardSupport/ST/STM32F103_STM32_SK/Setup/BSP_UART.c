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
Purpose : UART implementation

Additional information:

  Device : STM32F103
  Board  : IAR STM32 Starket Kit

  Unit | UART   | Board connector
  ===============================
  0    | UART2  | RS232_2
  1    | UART1  | Tx=L5, Rx=L6
  2    | UART3  | RS232_3

*/

#include "BSP_UART.h"
#include "RTOS.h"      // For OS_INT_Enter()/OS_INT_Leave(). Remove this line and OS_INT_* functions if not using OS.
#include "stm32f10x.h"   // Device specific header file, contains CMSIS defines.

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define BSP_UART_BAUDRATE     (38400)
#define BSP_UART_CLOCK        (SystemCoreClock)

#define USART1_BASE_ADDR      ((void*)0x40013800)
#define USART2_BASE_ADDR      ((void*)0x40004400)
#define USART3_BASE_ADDR      ((void*)0x40004800)

//
// Used to set/change the order of USART units.
//
#define BSP_USART1            (1)
#define BSP_USART2            (0)
#define BSP_USART3            (2)

#define USART3_REMAP_MASK     (0x00000030u)
#define USART3_REMAP_PARTIAL  (0x00000010u)

#define UART_BRR_VALUE        (BSP_UART_CLOCK / BSP_UART_BAUDRATE)

#define RCC_BASE_ADDR         (0x40021000u)
#define RCC_APB2ENR           (*(volatile unsigned long*)(RCC_BASE_ADDR + 0x18u))
#define RCC_APB1ENR           (*(volatile unsigned long*)(RCC_BASE_ADDR + 0x1Cu))

#define AFIO_BASE_ADDR        (0x40010000u)
#define AFIO_MAPR             (*(volatile unsigned long*)(AFIO_BASE_ADDR + 0x04u))

#define USART_SR(addr)        (*(volatile unsigned long*)((char*)(addr) + 0x00u))
#define USART_DR(addr)        (*(volatile unsigned long*)((char*)(addr) + 0x04u))
#define USART_BRR(addr)       (*(volatile unsigned long*)((char*)(addr) + 0x08u))
#define USART_CR1(addr)       (*(volatile unsigned long*)((char*)(addr) + 0x0Cu))
#define USART_CR2(addr)       (*(volatile unsigned long*)((char*)(addr) + 0x10u))
#define USART_CR3(addr)       (*(volatile unsigned long*)((char*)(addr) + 0x14u))
#define USART_GTPR(addr)      (*(volatile unsigned long*)((char*)(addr) + 0x18u))

#define RCC_USART1EN          (0x00004004uL)
#define RCC_USART2EN          (0x00000014uL)
#define RCC_USART3EN          (0x00000010uL)

#define GPIOA_CRL             (*(volatile unsigned long*)(0x40010800u))
#define GPIOA_CRH             (*(volatile unsigned long*)(0x40010804u))
#define GPIOB_CRH             (*(volatile unsigned long*)(0x40010C04u))
#define GPIOC_CRH             (*(volatile unsigned long*)(0x40011004u))
#define GPIOD_CRL             (*(volatile unsigned long*)(0x40011400u))

#define US_RXRDY              (0x20u)  // RXNE
#define US_TXEMPTY            (0x80u)  // TXE
#define USART_RX_ERROR_FLAGS  (0x0Fu)  // ORE/NE/FE/PE

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static BSP_UART_TX_CB* _pfWriteCB[3];
static BSP_UART_RX_CB* _pfReadCB[3];

/*********************************************************************
*
*       Prototypes
*
*  Declare ISR handler here to avoid "no prototype" warning.
*  They are not declared in any CMSIS header.
*
**********************************************************************
*/

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif

void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);

#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SetBaudrate()
*
*  Function description
*    Configures the UART baud rate.
*
*  Parameters
*    Unit    : Unit number (typically zero-based).
*    Baudrate: Baud rate to configure [Hz].
*/
static void _SetBaudrate(unsigned int Unit, unsigned long Baudrate) {
  BSP_UART_USE_PARA(Unit);
  BSP_UART_USE_PARA(Baudrate);
}

/*********************************************************************
*
*       _UART_IRQHandler()
*
*  Function description
*    Called by UART Rx & Tx interrupt handler.
*
*  Additional information
*    Needs to inform the OS that we are in interrupt context.
*/
static void _UART_IRQHandler(unsigned int Unit, void* const pUSARTBaseAddr) {
  unsigned char Status;
  unsigned char Data;

  OS_INT_EnterNestable();
  Status = USART_SR(pUSARTBaseAddr);                           // Read status register.
  //
  // Handle Rx.
  //
  do {
    if (Status & US_RXRDY) {                                   // Data received?
      Data = USART_DR(pUSARTBaseAddr);
      if ((Status & USART_RX_ERROR_FLAGS) == 0) {              // If no errors
        if (_pfReadCB[Unit] != NULL) {
          _pfReadCB[Unit](Unit, Data);                         // Process actual byte
        }
      }
    }
    Status = USART_SR(pUSARTBaseAddr);                         // Examine current status
  } while (Status & US_RXRDY);
  //
  // Handle Tx.
  //
  if (Status & US_TXEMPTY) {
    if (_pfWriteCB[Unit] != NULL && _pfWriteCB[Unit](Unit)) {  // No more characters to send ?
      USART_CR1(pUSARTBaseAddr) &= ~0x40uL;                    // Disable further tx interrupts
    }
  }
  OS_INT_LeaveNestable();
}

/*********************************************************************
*
*       Global functions, IRQ handler
*
**********************************************************************
*/

/*********************************************************************
*
*       USART1_IRQHandler()
*
*  Function description
*    UART1 Rx & Tx interrupt handler.
*/
void USART1_IRQHandler(void) {
  _UART_IRQHandler(BSP_USART1, USART1_BASE_ADDR);
}

/*********************************************************************
*
*       USART2_IRQHandler()
*
*  Function description
*    UART2 Rx & Tx interrupt handler.
*/
void USART2_IRQHandler(void) {
  _UART_IRQHandler(BSP_USART2, USART2_BASE_ADDR);
}

/*********************************************************************
*
*       USART3_IRQHandler()
*
*  Function description
*    UART3 Rx & Tx interrupt handler.
*/
void USART3_IRQHandler(void) {
  _UART_IRQHandler(BSP_USART3, USART3_BASE_ADDR);
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_UART_Init()
*
*  Function description
*    Initializes the UART peripheral.
*
*  Parameters
*    Unit       : Unit number (typically zero-based).
*    Baudrate   : Baud rate to configure [Hz].
*    NumDataBits: Number of data bits to use.
*    Parity     : One of the following values:
*                   * BSP_UART_PARITY_NONE
*                   * BSP_UART_PARITY_ODD
*                   * BSP_UART_PARITY_EVEN
*    NumStopBits: Number of stop bits to use.
*/
void BSP_UART_Init(unsigned int Unit, unsigned long Baudrate, unsigned char NumDataBits, unsigned char Parity, unsigned char NumStopBits) {
  void* pUSARTBaseAddr;
  //
  // Unused parameters. Hard coded USART settings.
  //
  BSP_UART_USE_PARA(Baudrate);
  BSP_UART_USE_PARA(NumDataBits);
  BSP_UART_USE_PARA(Parity);
  BSP_UART_USE_PARA(NumStopBits);
  //
  // Setup clocks, GPIO ports and NVIC IRQs.
  //
  switch (Unit) {
  case BSP_USART1:
    pUSARTBaseAddr  = USART1_BASE_ADDR;
    RCC_APB2ENR    |= RCC_USART1EN;                // Enable GPIO port used for USART and USART clock
    GPIOA_CRH       = (GPIOA_CRH & 0xFFFFF00FuL) | 0x000004B0uL;
    NVIC_SetPriority(USART1_IRQn, (1 << __NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ(USART1_IRQn);
    break;
  case BSP_USART2:
    pUSARTBaseAddr  = USART2_BASE_ADDR;
    RCC_APB1ENR    |= (1uL << 17);                 // Enable USART2 clock
    RCC_APB2ENR    |= RCC_USART2EN  | (1uL << 0);  // Enable GPIO port needed for USART, enable alternate function
    GPIOA_CRL       = (GPIOA_CRL & 0xFFFF00FFuL) | 0x00004B00uL;
    NVIC_SetPriority(USART2_IRQn, (1 << __NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ(USART2_IRQn);
    break;
  case BSP_USART3:
    pUSARTBaseAddr  = USART3_BASE_ADDR;
    RCC_APB1ENR    |= (1uL << 18);                 // Enable USART3 clock
    RCC_APB2ENR    |= RCC_USART3EN  | (1uL << 0);  // Enable GPIO port needed for USART, enable alternate function
    GPIOC_CRH       = (GPIOB_CRH & 0xFFFF00FFuL) | 0x00004B00uL;
    AFIO_MAPR      &= ~USART3_REMAP_MASK;
    AFIO_MAPR      |= USART3_REMAP_PARTIAL;
    NVIC_SetPriority(USART3_IRQn, (1 << __NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ(USART3_IRQn);
    break;
  default:
    return;
  }
  //
  // Initialize USART
  //
  USART_CR2(pUSARTBaseAddr)  = 0x200;
  USART_CR1(pUSARTBaseAddr)  = 0x2C;
  USART_CR3(pUSARTBaseAddr)  = 0x00;
  if (Unit != BSP_USART1 && BSP_UART_CLOCK > 36000000uL) {
    USART_BRR(pUSARTBaseAddr)  = UART_BRR_VALUE / 2;
  } else {
    USART_BRR(pUSARTBaseAddr)  = UART_BRR_VALUE;
  }
  USART_CR1(pUSARTBaseAddr) |= 0x2000;  // Enable uart
}

/*********************************************************************
*
*       BSP_UART_DeInit()
*
*  Function description
*    De-initializes the UART peripheral.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*/
void BSP_UART_DeInit(unsigned int Unit) {
  switch (Unit) {
  case BSP_USART1:
    NVIC_DisableIRQ(USART1_IRQn);
    break;
  case BSP_USART2:
    NVIC_DisableIRQ(USART2_IRQn);
    break;
  case BSP_USART3:
    NVIC_DisableIRQ(USART3_IRQn);
    break;
  default:
    break;
  }
}

/*********************************************************************
*
*       BSP_UART_SetBaudrate()
*
*  Function description
*    Configures/changes the UART baud rate.
*
*  Parameters
*    Unit    : Unit number (typically zero-based).
*    Baudrate: Baud rate to configure [Hz].
*/
void BSP_UART_SetBaudrate(unsigned int Unit, unsigned long Baudrate) {
  _SetBaudrate(Unit, Baudrate);
}

/*********************************************************************
*
*       BSP_UART_SetReadCallback()
*
*  Function description
*    Sets the callback to execute upon an Rx interrupt.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    pf  : Callback to execute.
*/
void BSP_UART_SetReadCallback(unsigned Unit, BSP_UART_RX_CB* pf) {
  _pfReadCB[Unit] = pf;
}

/*********************************************************************
*
*       BSP_UART_SetWriteCallback()
*
*  Function description
*    Sets the callback to execute upon a Tx interrupt.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    pf  : Callback to execute.
*/
void BSP_UART_SetWriteCallback(unsigned int Unit, BSP_UART_TX_CB* pf) {
  _pfWriteCB[Unit] = pf;
}

/*********************************************************************
*
*       BSP_UART_Write1()
*
*  Function description
*    Sends one byte via UART.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    Data: (First) data byte to send.
*
*  Additional information
*    The first byte of a transfer is typically sent from application
*    context. Further bytes of the transfer are then sent from the
*    Tx interrupt handler by also calling this function from interrupt
*    context.
*/
void BSP_UART_Write1(unsigned int Unit, unsigned char Data) {
  void* pUSARTBaseAddr;

  switch (Unit) {
  case BSP_USART1:
      pUSARTBaseAddr = USART1_BASE_ADDR;
    break;
  case BSP_USART2:
      pUSARTBaseAddr = USART2_BASE_ADDR;
    break;
  case BSP_USART3:
      pUSARTBaseAddr = USART3_BASE_ADDR;
    break;
  default:
    return;
  }
  USART_DR(pUSARTBaseAddr)   = Data;
  USART_CR1(pUSARTBaseAddr) |= 0x40;  // Enable tx interrupt
}

/*************************** End of file ****************************/
