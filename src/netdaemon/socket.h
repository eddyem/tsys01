/*
 *                                                                                                  geany_encoding=koi8-r
 * socket.h
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#ifndef __SOCKET_H__
#define __SOCKET_H__

// timeout for socket closing
#define SOCKET_TIMEOUT  (5.0)

// absolute zero: all T < ABS_ZERO_T are wrong
#define ABS_ZERO_T      (-273.15)
// undoubtedly wrong T
#define WRONG_T         (-300.)

void daemonize(char *port);
const char *gotstr(int N);
void TurnOFF();

#endif // __SOCKET_H__
