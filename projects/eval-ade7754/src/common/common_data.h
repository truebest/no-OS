/***************************************************************************//**
 *   @file   common_data.h
 *   @brief  Defines common data to be used by ADE7754 example project
 *   @author REtz (radu.etz@analog.com)
********************************************************************************
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/
#ifndef __COMMON_DATA_H__
#define __COMMON_DATA_H__

#include "ade7754.h"
#include "no_os_uart.h"
#include "no_os_pwm.h"
#include "no_os_delay.h"
#include "no_os_gpio.h"
#include "no_os_spi.h"
#include "no_os_print_log.h"
#include "no_os_units.h"
#include "no_os_util.h"
#include "no_os_error.h"
#include "maxim_uart.h"
#include "maxim_gpio.h"
#include "maxim_uart_stdio.h"
#include "maxim_pwm.h"
#include "maxim_spi.h"
#include "maxim_irq.h"

/* UART init params */
extern struct no_os_uart_init_param uart_ip;
/* GPIO LED init params */
extern struct no_os_gpio_init_param gpio_led1_ip;
/* SPI init params */
extern struct no_os_spi_init_param ade7754_spi_ip;
/* GPIO RESET init params */
extern struct no_os_gpio_init_param gpio_reset_ip;
/* GPIO IRQ init params */
extern struct no_os_gpio_init_param ade7754_gpio_irq_ip;
/* GPIO interrupt init params */
extern struct no_os_irq_init_param ade7754_gpio_int_ip;

/* Read data interval in irq multiple */
#define PRINT_INTERVAL			50

/* Setup values for ADE7754 */
/* value for R_small defined by user */
#define RSMALL				1
/* value for R high defined by user */
#define RHIGH                       	0
#define V_GAIN                      	((RSMALL + RHIGH) / RSMALL)
/* Change the gain accordingly to the current sensor
and sense circuit */
#define I_GAIN                      	1
/* RMS full scale code value defined by user */
#define VOLTAGE_RMS_FS_CODE        	1848772
#define CURRENT_RMS_FS_CODE         	1898124
/* ADC input at full scale rms voltage mV */
#define ADC_FS_RMS_IN               	353.5
/* Period resolution µs/LSB */
#define PERIOD_RES                     	2.4
/* Temperature resolution 4 °C / LSB */
#define TEMP_G                     	4
/* Temperature °C correspnding to 00h code */
#define ADE7754_AMB_T_CELSIUS       	129

/* Read measurements */
int read_rms_measurements(struct ade7754_dev *dev);

/* Print measurements */
int print_measurements(struct ade7754_dev *dev);

/* Toggle user LED */
int interface_toggle_led(struct no_os_gpio_desc *gpio_led_desc);

#endif /* __COMMON_DATA_H__ */
