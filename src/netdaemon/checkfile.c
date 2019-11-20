/*
 * daemon.c - functions for running in background like a daemon
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

#include <dirent.h>     // opendir
#include "checkfile.h"
#include "usefull_macros.h"

/**
 * @brief readPSname - read process name from /proc/PID/cmdline
 * @param pid - PID of interesting process
 * @return filename or NULL if not found
 *      don't use this function twice for different names without copying
 *      its returning by strdup, because `name` contains in static array
 */
char *readPSname(pid_t pid){
    static char name[PATH_MAX];
    char *pp = name, byte, path[PATH_MAX];
    FILE *file;
    int cntr = 0;
    size_t sz;
    snprintf(path, PATH_MAX, PROC_BASE "/%d/cmdline", pid);
    file = fopen(path, "r");
    if(!file) return NULL; // there's no such file
    do{ // read basename
        sz = fread(&byte, 1, 1, file);
        if(sz != 1) break;
        if(byte != '/') *pp++ = byte;
        else{
            pp = name;
            cntr = 0;
        }
    }while(byte && cntr++ < PATH_MAX-1);
    name[cntr] = 0;
    fclose(file);
    return name;
}

static char *pidfilename_ = NULL; // store the name of pidfile here

/**
 * check wether there is a same running process
 * exit if there is a running process or error
 * Checking have 3 steps:
 *      1) lock executable file
 *      2) check pidfile (if you run a copy?)
 *      3) check /proc for executables with the same name (no/wrong pidfile)
 * @param pidfilename - name of pidfile or NULL if none
 * @return 0 if all OK or PID of first running process found
 */
pid_t check4running(char *pidfilename){
    DIR *dir;
    FILE *pidfile;
    struct dirent *de;
    struct stat s_buf;
    pid_t pid = 0, self;
    char *name, *myname;
    self = getpid(); // get self PID
    if(!(dir = opendir(PROC_BASE))){ // open /proc directory
        ERR(PROC_BASE);
    }
    if(!(name = readPSname(self))){ // error reading self name
        ERR("Can't read self name");
    }
    myname = strdup(name);
    if(pidfilename && stat(pidfilename, &s_buf) == 0){ // pidfile exists
        pidfile = fopen(pidfilename, "r");
        if(pidfile){
            if(fscanf(pidfile, "%d", &pid) > 0){ // read PID of (possibly) running process
                if((name = readPSname(pid)) && strncmp(name, myname, 255) == 0){
                    fclose(pidfile);
                    goto procfound;
                }
            }
            fclose(pidfile);
        }
    }
    // There is no pidfile or it consists a wrong record
    while((de = readdir(dir))){ // scan /proc
        if(!(pid = (pid_t)atoi(de->d_name)) || pid == self) // pass non-PID files and self
            continue;
        if((name = readPSname(pid)) && strncmp(name, myname, 255) == 0)
            goto procfound;
    }
    pid = 0;  // OK, not found -> create pid-file if need
    if(pidfilename){
        pidfile = fopen(pidfilename, "w");
        if(!pidfile){
            WARN("Can't open PID file");
        }else{
            fprintf(pidfile, "%d\n", self); // write self PID to pidfile
            fclose(pidfile);
            if(pidfilename_) FREE(pidfilename_);
            pidfilename_ = strdup(pidfilename);
        }
    }
procfound:
    closedir(dir);
    free(myname);
    return pid;
}

/**
 * @brief unlink_pidfile - remove pidfile @ exit
 */
void unlink_pidfile(){
    if(!pidfilename_) return;
    unlink(pidfilename_);
    FREE(pidfilename_);
}
