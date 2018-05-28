/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.h
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#pragma once
#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "stm32f0.h"

#define LED0_port   GPIOB
#define LED0_pin    (1<<3)

#ifndef USARTNUM
#define USARTNUM 2
#endif

#define CONCAT(a,b)     a ## b
#define STR_HELPER(s)   #s
#define STR(s) STR_HELPER(s)

#define FORMUSART(X)   CONCAT(USART, X)
#define USARTX  FORMUSART(USARTNUM)

#ifndef I2CPINS
#define I2CPINS A9A10
#endif

#ifndef LED1_port
#define LED1_port   LED0_port
#endif
#ifndef LED1_pin
#define LED1_pin    LED0_pin
#endif
#define LED_blink(x) pin_toggle(x ## _port, x ## _pin)

// oLED
#define RST_PORT    GPIOB
#define RST_PIN     (1<<4)
#define DC_PORT     GPIOB
#define DC_PIN      (1<<5)
#define CS_PORT     GPIOA
#define CS_PIN      (1<<4)
#define RST_HI()    do{pin_set(RST_PORT, RST_PIN);}while(0)
#define RST_LO()    do{pin_clear(RST_PORT, RST_PIN);}while(0)
#define DC_HI()     do{pin_set(DC_PORT, DC_PIN);}while(0)
#define DC_LO()     do{pin_clear(DC_PORT, DC_PIN);}while(0)
#define CS_HI()     do{pin_set(CS_PORT, CS_PIN);}while(0)
#define CS_LO()     do{pin_clear(CS_PORT, CS_PIN);}while(0)

typedef enum{
    LOW_SPEED,
    HIGH_SPEED
} I2C_SPEED;

void gpio_setup(void);
void i2c_setup(I2C_SPEED speed);
void spi_setup();

#endif // __HARDWARE_H__
