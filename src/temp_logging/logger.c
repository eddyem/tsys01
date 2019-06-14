/*
 * This file is part of the usefull_macros project.
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru>.
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

#include "cmdlnopts.h"
#include <signal.h>         // signal
#include <stdio.h>          // printf
#include <stdlib.h>         // exit, free
#include <string.h>         // strdup
#include <unistd.h>         // sleep
#include <usefull_macros.h>


#include <termios.h>		// tcsetattr
#include <unistd.h>			// tcsetattr, close, read, write
#include <sys/ioctl.h>		// ioctl
#include <stdio.h>			// printf, getchar, fopen, perror
#include <stdlib.h>			// exit
#include <sys/stat.h>		// read
#include <fcntl.h>			// read
#include <signal.h>			// signal
#include <time.h>			// time
#include <string.h>			// memcpy
#include <stdint.h>			// int types
#include <sys/time.h>		// gettimeofday

/**
 * This is an example of usage:
 *  - command line arguments,
 *  - log file,
 *  - check of another file version running,
 *  - signals management,
 *  - serial port reading/writing.
 * The `cmdlnopts.[hc]` are intrinsic files of this demo.
 */

static TTY_descr *dev = NULL; // shoul be global to restore if die
static glob_pars *GP = NULL;  // for GP->pidfile need in `signals`
FILE *flog = NULL;
/**
 * We REDEFINE the default WEAK function of signal processing
 */
void signals(int sig){
    if(sig){
        signal(sig, SIG_IGN);
        DBG("Get signal %d, quit.\n", sig);
    }
    if(flog)
        fclose(flog);
    if(GP->pidfile) // remove unnesessary PID file
        unlink(GP->pidfile);
    restore_console();
    close_tty(&dev);
    exit(sig);
}

void iffound_default(pid_t pid){
    ERRX("Another copy of this process found, pid=%d. Exit.", pid);
}

/**
 * Create log file (open in exclusive mode: error if file exists)
 * @param name - file name
 * @param r    - 1 to rewrite existing file
 */
static void create_log(char *name, int r){
    int fdlog;
    int oflag = O_WRONLY | O_CREAT;
    if(r) oflag |= O_TRUNC;
    else oflag |= O_EXCL;
    if((fdlog = open(name, oflag,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1){
        ERR("open(%s) failed", name);
    }
    flog = fdopen(fdlog, "w");
    if(!flog) ERR("fdopen() failed");
    DBG("%s opened", name);
}

static void parse_T(){
    static double temperatures[16][2];
    static time_t tfirst = 0;
    static int tcount = 0, ttotal = 0;
    static bool printhdr = true, firstrun = true;
    if(firstrun){
        firstrun = false;
        // init T values
        for(int i = 0; i < 16; ++i){
            temperatures[i][0] = -300.;
            temperatures[i][1] = -300.;
        }
    }
    void print_hdr(int n, int N){
        if(temperatures[n][N] < -299.) return;
        int idx = ((n>>1)*10) | (n & 1);
        //DBG("header[%d][%d] -> T%d_%d", n, N, N, idx);
        fprintf(flog, "T%d_%d\t", N, idx);
        printf("T%d_%d\t", N, idx);
    }
    void print_val(int n, int N){
        if(temperatures[n][N] < -299.) return;
        fprintf(flog, "%.2f\t", temperatures[n][N]);
        printf("%.2f\t", temperatures[n][N]);
    }
    if(read_tty(dev)){
        if(tfirst == 0) tfirst = time(NULL);
        //DBG("Got %zd bytes from port: %s", dev->buflen, dev->buf);
        char *ptr = dev->buf, *estr = NULL;
        int N, n, T;
        do{
            estr = strchr((char*)ptr, '\n');
            if(estr){*estr = 0; ++estr;}
            size_t amount = sscanf((char*)ptr, "T%d_%d=%d", &N, &n, &T);
            //DBG("in str\"%s\" got %zd values", ptr, amount);
            if(3 == amount){ // good value
                int idx = (n / 10);
                idx *= 2;
                idx += n % 10;
                temperatures[idx][N] = (double)T / 100.;
                DBG("T[%d][%d] = %g", idx, N, temperatures[idx][N]);
                ++tcount;
            }
            ptr = estr;
        }while(estr && *estr);
    }else if((ttotal && tcount == ttotal) || (tfirst && time(NULL) - tfirst > 10)){
        DBG("LOG @ %zd, tcount=%d, ttotal=%d", tfirst, tcount, ttotal);
        if(printhdr){
            fprintf(flog, "# UNIX_time\t");
            printf("# UNIX_time\t");
            for(int j = 0; j < 2; ++j){
                for(int i = 0; i < 16; ++i){
                    print_hdr(i, j);
                }
            }
            fprintf(flog, "\n");
            printf("\n");
            printhdr = false;
        }
        if(ttotal == 0) ttotal = tcount;
        if(ttotal == tcount){
            fprintf(flog, "%zd\t", tfirst);
            printf("%zd\t", tfirst);
            for(int j = 0; j < 2; ++j){
                for(int i = 0; i < 16; ++i){
                    print_val(i, j);
                }
            }
            fprintf(flog, "\n");
            printf("\n");
        }
        tfirst = 0;
        tcount = 0;
        for(int i = 0; i < 16; ++i){
            temperatures[i][0] = -300.;
            temperatures[i][1] = -300.;
        }
    }
}

int main(int argc, char *argv[]){
    initial_setup();
    char *self = strdup(argv[0]);
    GP = parse_args(argc, argv);
    if(GP->rest_pars_num){
        printf("%d extra options:\n", GP->rest_pars_num);
        for(int i = 0; i < GP->rest_pars_num; ++i)
            printf("%s\n", GP->rest_pars[i]);
    }
    check4running(self, GP->pidfile);
    free(self);
    signal(SIGTERM, signals); // kill (-15) - quit
    signal(SIGHUP, SIG_IGN);  // hup - ignore
    signal(SIGINT, signals);  // ctrl+C - quit
    signal(SIGQUIT, signals); // ctrl+\ - quit
    signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
    setup_con();
    if(GP->rest_pars_num){
        for(int i = 0; i < GP->rest_pars_num; ++i)
            printf("Extra argument: %s\n", GP->rest_pars[i]);
    }
    if(GP->device){
        putlog("Try to open serial %s", GP->device);
        dev = new_tty(GP->device, GP->speed, 1024);
        if(dev) dev = tty_open(dev, 1);
        if(!dev){
            putlog("Can't open %s with speed %d. Exit.", GP->device, GP->speed);
            signals(0);
        }
    }
    create_log(GP->outfile, GP->rewritelog);
    double t0 = dtime();
    write_tty(dev->comfd, "t\n", 2);
    write_tty(dev->comfd, "T\n", 2);
    while(1){
        if(dtime() - t0 >= GP->pauselen){
            t0 = dtime();
            // send commands to take data
            write_tty(dev->comfd, "t\n", 2);
            write_tty(dev->comfd, "T\n", 2);
        }
        parse_T();
    }
    // clean everything
    signals(0);
    return 0;
}
