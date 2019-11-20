/*
 * This file is part of the Zphocus project.
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
#ifndef CHECKFILE_H__
#define CHECKFILE_H__

#include <unistd.h>     // getpid, unlink

#ifndef PROC_BASE
#define PROC_BASE "/proc"
#endif
#ifndef WEAK
#define WEAK        __attribute__ ((weak))
#endif

// check that our process is exclusive
pid_t check4running(char *pidfilename);
// read name of process by its PID
char *readPSname(pid_t pid);
void unlink_pidfile();


#endif // CHECKFILE_H__
