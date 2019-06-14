/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.h - comand line options for parceargs
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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
#ifndef CMDLNOPTS_H__
#define CMDLNOPTS_H__

/*
 * here are some typedef's for global data
 */
typedef struct{
    char *device;           // serial device name
    char *pidfile;          // name of PID file
    char *outfile;          // logging to this file
    int rewritelog;         // ==1 to rewrite log file
    int speed;              // connection speed
    int rest_pars_num;      // number of rest parameters
    double pauselen;        // pause between reads
    char** rest_pars;       // the rest parameters: array of char*
} glob_pars;


glob_pars *parse_args(int argc, char **argv);

#endif // CMDLNOPTS_H__
