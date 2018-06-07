/*
 *                                                                                                  geany_encoding=koi8-r
 * sensors_manage.h
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
#ifndef __SENSORS_MANAGE_H__
#define __SENSORS_MANAGE_H__

#include "hardware.h"

// time between two readings (3sec)
#define SLEEP_TIME          (3000)
// error in measurement == -300degrC
#define BAD_TEMPERATURE     (-30000)

typedef enum{
     SENS_INITING           // power on
    ,SENS_RESETING          // discovery sensors resetting them
    ,SENS_GET_COEFFS        // get coefficients from all sensors
    ,SENS_SLEEPING          // wait for a time to process measurements
    ,SENS_START_MSRMNT      // send command 2 start measurement
    ,SENS_WAITING           // wait for measurements end
    ,SENS_GATHERING         // collect information
    ,SENS_OFF               // sensors' power is off by external command
    ,SENS_OVERCURNT         // overcurrent detected @ any stage
    ,SENS_OVERCURNT_OFF     // sensors' power is off due to continuous overcurrent
} SensorsState;

SensorsState sensors_get_state();
void sensors_process();

void sensors_off();
void sensors_on();

void showcoeffs();
void showtemperature();

#endif // __SENSORS_MANAGE_H__
