/*
 * Copyright (c) 2018-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       C3235SF_LAUNCHXL.h
 *
 *  @brief      CC3235 Board Specific APIs
 *
 *  The CC3235SF_LAUNCHXL header file should be included in an application as
 *  follows:
 *  @code
 *  #include <CC3235SF_LAUNCHXL.h>
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __CC3235SF_LAUNCHXL_H
#define __CC3235SF_LAUNCHXL_H

#ifdef __cplusplus
extern "C" {
#endif

#define CC3235SF_LAUNCHXL_GPIO_LED_OFF (0)
#define CC3235SF_LAUNCHXL_GPIO_LED_ON  (1)

/*!
 *  @def    CC3235SF_LAUNCHXL_ADCName
 *  @brief  Enum of ADC names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_ADCName {
    CC3235SF_LAUNCHXL_ADC0 = 0,
    CC3235SF_LAUNCHXL_ADC1,

    CC3235SF_LAUNCHXL_ADCCOUNT
} CC3235SF_LAUNCHXL_ADCName;

/*!
 *  @def    CC3235SF_LAUNCHXL_CaptureName
 *  @brief  Enum of Capture names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_CaptureName {
    CC3235SF_LAUNCHXL_CAPTURE0 = 0,
    CC3235SF_LAUNCHXL_CAPTURE1,

    CC3235SF_LAUNCHXL_CAPTURECOUNT
} CC3235SF_LAUNCHXL_CaptureName;

/*!
 *  @def    CC3235SF_LAUNCHXL_CryptoName
 *  @brief  Enum of Crypto names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_CryptoName {
    CC3235SF_LAUNCHXL_CRYPTO0 = 0,

    CC3235SF_LAUNCHXL_CRYPTOCOUNT
} CC3235SF_LAUNCHXL_CryptoName;

/*!
 *  @def    CC3235SF_LAUNCHXL_GPIOName
 *  @brief  Enum of GPIO names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_GPIOName {
    CC3235SF_LAUNCHXL_GPIO_SW2 = 0,
    CC3235SF_LAUNCHXL_GPIO_SW3,
    CC3235SF_LAUNCHXL_SPI_MASTER_READY,
    CC3235SF_LAUNCHXL_SPI_SLAVE_READY,
    CC3235SF_LAUNCHXL_GPIO_LED_BLUE,

    /*
     *  CC3235SF_LAUNCHXL_GPIO_LED_RED and CC3235SF_LAUNCHXL_GPIO_LED_GREEN are shared with the
     *  I2C and PWM peripherals. In order for those examples to work, these
     *  LEDs are  taken out of gpioPinCOnfig[]
     */
    /* CC3235SF_LAUNCHXL_GPIO_LED_RED, */
    /* CC3235SF_LAUNCHXL_GPIO_LED_GREEN, */

    /* Sharp LCD Pins */
    CC3235SF_LAUNCHXL_LCD_CS,
    CC3235SF_LAUNCHXL_LCD_POWER,
    CC3235SF_LAUNCHXL_LCD_ENABLE,

    CC3235SF_LAUNCHXL_GPIOCOUNT
} CC3235SF_LAUNCHXL_GPIOName;

/*!
 *  @def    CC3235SF_LAUNCHXL_I2CName
 *  @brief  Enum of I2C names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_I2CName {
    CC3235SF_LAUNCHXL_I2C0 = 0,

    CC3235SF_LAUNCHXL_I2CCOUNT
} CC3235SF_LAUNCHXL_I2CName;

/*!
 *  @def    CC3235SF_LAUNCHXL_I2SName
 *  @brief  Enum of I2S names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_I2SName {
    CC3235SF_LAUNCHXL_I2S0 = 0,

    CC3235SF_LAUNCHXL_I2SCOUNT
} CC3235SF_LAUNCHXL_I2SName;

/*!
 *  @def    CC3235SF_LAUNCHXL_PWMName
 *  @brief  Enum of PWM names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_PWMName {
    CC3235SF_LAUNCHXL_PWM6 = 0,
    CC3235SF_LAUNCHXL_PWM7,

    CC3235SF_LAUNCHXL_PWMCOUNT
} CC3235SF_LAUNCHXL_PWMName;

/*!
 *  @def    CC3235SF_LAUNCHXL_SDFatFSName
 *  @brief  Enum of SDFatFS names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_SDFatFSName {
    CC3235SF_LAUNCHXL_SDFatFS0 = 0,

    CC3235SF_LAUNCHXL_SDFatFSCOUNT
} CC3235SF_LAUNCHXL_SDFatFSName;

/*!
 *  @def    CC3235SF_LAUNCHXL_SDName
 *  @brief  Enum of SD names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_SDName {
    CC3235SF_LAUNCHXL_SD0 = 0,

    CC3235SF_LAUNCHXL_SDCOUNT
} CC3235SF_LAUNCHXL_SDName;

/*!
 *  @def    CC3235SF_LAUNCHXL_SPIName
 *  @brief  Enum of SPI names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_SPIName {
    CC3235SF_LAUNCHXL_SPI0 = 0,
    CC3235SF_LAUNCHXL_SPI1,

    CC3235SF_LAUNCHXL_SPICOUNT
} CC3235SF_LAUNCHXL_SPIName;

/*!
 *  @def    CC3235SF_LAUNCHXL_TimerName
 *  @brief  Enum of Timer names on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_TimerName {
    CC3235SF_LAUNCHXL_TIMER0 = 0,
    CC3235SF_LAUNCHXL_TIMER1,
    CC3235SF_LAUNCHXL_TIMER2,

    CC3235SF_LAUNCHXL_TIMERCOUNT
} CC3235SF_LAUNCHXL_TimerName;

/*!
 *  @def    CC3235SF_LAUNCHXL_UARTName
 *  @brief  Enum of UARTs on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_UARTName {
    CC3235SF_LAUNCHXL_UART0 = 0,
    CC3235SF_LAUNCHXL_UART1,

    CC3235SF_LAUNCHXL_UARTCOUNT
} CC3235SF_LAUNCHXL_UARTName;

/*!
 *  @def    CC3235SF_LAUNCHXL_WatchdogName
 *  @brief  Enum of Watchdogs on the CC3235SF_LAUNCHXL dev board
 */
typedef enum CC3235SF_LAUNCHXL_WatchdogName {
    CC3235SF_LAUNCHXL_WATCHDOG0 = 0,

    CC3235SF_LAUNCHXL_WATCHDOGCOUNT
} CC3235SF_LAUNCHXL_WatchdogName;

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
extern void CC3235SF_LAUNCHXL_initGeneral(void);

#ifdef __cplusplus
}
#endif

#endif /* __CC3235SF_LAUNCHXL_H */
