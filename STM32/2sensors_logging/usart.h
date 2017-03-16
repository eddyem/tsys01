/*
 * usart.h
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 */
#pragma once
#ifndef __USART_H__
#define __USART_H__

// input and output buffers size
#define UARTBUFSZ  (64)
// timeout between data bytes
#ifndef TIMEOUT_MS
#define TIMEOUT_MS (1500)
#endif

typedef enum{
    ALL_OK,
    LINE_BUSY,
    STR_TOO_LONG
} TXstatus;

#define usart2rx()  (linerdy)
#define usart2ovr() (bufovr)

extern int linerdy, bufovr, txrdy;

void usart2_setup();
int usart2_getline(char **line);
TXstatus usart2_send(const char *str, int len);
TXstatus usart2_send_blocking(const char *str, int len);

#endif // __USART_H__
