/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.h - comand line options for parceargs
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru>
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
#ifndef __CMDLNOPTS_H__
#define __CMDLNOPTS_H__

#include "parseargs.h"
#include "term.h"

/*
 * here are some typedef's for global data
 */
typedef struct{
    char *device;           // serial device name
    char *port;             // port to connect
    int terminal;           // run as terminal
    char *savepath;         // path where data & graphical files would be saved
    int makegraphs;         // ==1 to make graphics with gnuplot
    int rest_pars_num;      // number of rest parameters
    char** rest_pars;       // the rest parameters: array of char* (path to logfile and thrash)
    int testadjfile;        // test format of file with adjustments
    char *adjfilename;      // name of adjustements file
    char *pidfilename;      // name of PID file
    int dumpoff;            // dump sensors data & turn all OFF until next request
} glob_pars;


glob_pars *parse_args(int argc, char **argv);
#endif // __CMDLNOPTS_H__
