/*                                                                                                  geany_encoding=koi8-r
 * term.h
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
 */
#pragma once
#ifndef __TERM_H__
#define __TERM_H__

#include <time.h>

#include "sens_place.h" // NCTRLR

// Terminal timeout (seconds)
#define     WAIT_TMOUT          (0.5)
// Main controller polling timeout - 1 second
#define     POLLING_TMOUT       (1.0)
// Thermal polling timeout: 1.5 seconds
#define     T_POLLING_TMOUT     (1.5)
// T measurement time interval - 30 seconds
#define     T_INTERVAL          (30.0)
// interval (in seconds) to remove too old measurements (if sensor not available now)
#define     OLDESTTM            (180)
// amount of measurement to plot mean graphs
#define     GRAPHS_AMOUNT       (15)

// Protocol
#define CMD_SENSORS_OFF         'F'
#define CMD_SENSORS_OFF_LOCAL   'f'
#define CMD_VOLTAGE             'K'
#define CMD_PING                'P'
#define CMD_MEASURE_T           'T'
#define CMD_MEASURE_LOCAL       't'
#define ANS_PONG                "PONG"

extern time_t tmeasured[2][NCHANNEL_MAX+1][NCTRLR_MAX+1];
extern double t_last[2][NCHANNEL_MAX+1][NCTRLR_MAX+1];

// communication errors
typedef enum{
    TRANS_SUCCEED = 0,  // no errors
    TRANS_ERROR,        // some error occured
    TRANS_TIMEOUT       // no data in WAIT_TMOUT
} trans_status;

int try_connect(char *path);
int poll_sensors(int N);
int check_sensors();
void turn_all_off();

#endif // __TERM_H__
