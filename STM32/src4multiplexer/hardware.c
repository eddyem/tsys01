/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c - hardware-dependent macros & functions
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

#include "hardware.h"

void gpio_setup(void){
    // Set green led (PB3) as output
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN;
    GPIOB->MODER = GPIO_MODER_MODER3_O;
    // setup of multiplexer channel address bus: PA3..8
    GPIOA->MODER = GPIO_MODER_MODER3_O | GPIO_MODER_MODER4_O | GPIO_MODER_MODER5_O |
                   GPIO_MODER_MODER6_O | GPIO_MODER_MODER7_O | GPIO_MODER_MODER8_O;
}

void i2c_setup(I2C_SPEED speed){
    I2C1->CR1 = 0;
#if I2CPINS == A9A10
/*
 * GPIO Resources: I2C1_SCL - PA9, I2C1_SDA - PA10
 * GPIOA->AFR[1] AF4 -- GPIOA->AFR[1] &= ~0xff0, GPIOA->AFR[1] |= 0x440
 */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock
    GPIOA->AFR[1] &= ~0xff0; // alternate function F4 for PA9/PA10
    GPIOA->AFR[1] |= 0x440;
    GPIOA->MODER &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOA->MODER |= GPIO_MODER_MODER9_AF | GPIO_MODER_MODER10_AF; // alternate function
    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10; // opendrain
    //GPIOA->OTYPER |= GPIO_OTYPER_OT_10; // opendrain
#else // undefined
#error "Not implemented"
#endif
    // I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // timing
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW; // use sysclock for timing
    if(speed == LOW_SPEED){ // 10kHz
        // PRESC=B, SCLDEL=4, SDADEL=2, SCLH=0xC3, SCLL=0xC7
        //I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0xC3<<8) | (0xC7);
        I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0xC3<<8) | (0xB0);
    }else{ // 100kHz
        // Clock = 6MHz, 0.16(6)us, need 5us (*30)
        // PRESC=4 (f/5), SCLDEL=0 (t_SU=5/6us), SDADEL=0 (t_HD=5/6us), SCLL,SCLH=14 (2.(3)us)
        //I2C1->TIMINGR = (4<<28) | (14<<8) | (14); // 0x40000e0e
        I2C1->TIMINGR = (0xB<<28) | (4<<20) | (2<<16) | (0x12<<8) | (0x11);
    }
    I2C1->CR1 = I2C_CR1_PE;// | I2C_CR1_RXIE; // Enable I2C & (interrupt on receive - not supported yet)
}
