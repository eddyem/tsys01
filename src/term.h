/*                                                                                                  geany_encoding=koi8-r
 * term.h - functions to work with serial terminal
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
 */
#pragma once
#ifndef __TERM_H__
#define __TERM_H__

// terminal timeout (seconds)
#define     WAIT_TMOUT      (0.06)
// sensors pairs amount
#define NSENSORS            (48)
// amount of communication tries
#define NTRY                (5)

/******************************** Commands definition ********************************/
#define     CMD_CONSTANTS   "C\n"
#define     CMD_RESET       "L\n"
#define     CMD_REINIT      "I\n"
#define     CMD_GETTEMP     "T\n"
#define     CMD_DISCOVERY   "D\n"

void try_connect(char *device, int speed);
int create_log(char *name, int r);
void begin_logging(int fd, double pause);
int conv_spd(int speed);

#endif // __TERM_H__
