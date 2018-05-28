/*
 *                                                                                                  geany_encoding=koi8-r
 * spi.c
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

#include "spi.h"
#include "usart.h"

extern volatile uint32_t Tms;
static uint32_t tstart;

extern void printu(uint32_t val);
#define CHK(x, msg) do{tstart = Tms; do{if(Tms - tstart > 2){/*printu(SPI1->SR); SEND(msg);*/ return;}}while(x);}while(0)

/**
 * send 1 byte blocking
 * return 1 on success
 */
void spi_write_byte(uint8_t data){
    CHK(!(SPI1->SR & SPI_SR_TXE), "TXEb1\n");
    // AHTUNG! without  *((uint8_t*)  STM32 will try to send TWO bytes (with second byte == 0)
    *((uint8_t*)&(SPI1->DR)) = data;
    CHK(!(SPI1->SR & SPI_SR_TXE), "TXEb2\n");
    CHK(SPI1->SR & SPI_SR_BSY, "BSYb\n");
}

/**
 * Blocking rite data to current SPI
 * @param data - buffer with data
 * @param len  - buffer length
 * @return 0 in case of error (or 1 in case of success)
 */
void spiWrite(uint8_t *data, uint16_t len){
    uint16_t i;
    CHK(!(SPI1->SR & SPI_SR_TXE), "TXE1\n");
    for(i = 0; i < len; ++i){
        *((uint8_t*)&(SPI1->DR)) = data[i];
        CHK(!(SPI1->SR & SPI_SR_TXE), "TXE2\n");
    }
    CHK(SPI1->SR & SPI_SR_BSY, "BSY\n");
}

