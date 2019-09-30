/*
 * This file is part of the TSYS01_netdaemon project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef SENS_PLACE_H__
#define SENS_PLACE_H__

// max number of controller
#define NCTRLR_MAX      (5)
// max number of channel
#define NCHANNEL_MAX    (7)

typedef struct{
    float dt;
    int X;
    int Y;
    int Z;
} sensor_data;

const sensor_data *get_sensor_location(int Nct, int Nch, int Ns);

#endif // SENS_PLACE_H__
