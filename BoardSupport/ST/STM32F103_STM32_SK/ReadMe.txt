ReadMe.txt for the ST STM32F103 start project.

This project was built for Keil MDK uVision V5.36.

Supported hardware:
===================
The sample project for the STM32F103 is prepared to run on
an IAR STM32 Starket Kit. Using different target hardware
may require modifications.

Configurations:
===============
- Debug:
  This configuration is prepared for download into internal
  Flash using J-Link. An embOS debug and profiling library
  is used.
  To use SEGGER SystemView with this configuration, configure
  SystemViewer for STM32F103RB as target device and SWD at
  1000 kHz as target interface.

- Release:
  This configuration is prepared for download into internal
  Flash using J-Link. An embOS release library is used.
