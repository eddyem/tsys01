/*                                                                                                  geany_encoding=koi8-r
 * client.c - terminal parser
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
#include "usefull_macros.h"
#include "term.h"
#include <strings.h> // strncasecmp
#include <time.h>    // time(NULL)
#include <limits.h>  // INT_MAX, INT_MIN

#define BUFLEN 1024

// == 1 if given controller (except 0) presents on CAN bus
int8_t ctrlr_present[8] = {1,0,0};
// UNIX time of temperatures measurement: [Ngroup][Nsensor][Ncontroller]
time_t tmeasured[2][8][8];
// last temperatures read: [Ngroup][Nsensor][Ncontroller]
double t_last[2][8][8];

/**
 * read strings from terminal (ending with '\n') with timeout
 * @param L       - its length
 * @return NULL if nothing was read or pointer to static buffer
 */
static char *read_string(){
    size_t r = 0, l;
    static char buf[BUFLEN];
    int LL = BUFLEN - 1;
    char *ptr = NULL;
    static char *optr = NULL;
    if(optr && *optr){
        ptr = optr;
        optr = strchr(optr, '\n');
        if(optr) ++optr;
        //DBG("got data, roll to next; ptr=%s\noptr=%s",ptr,optr);
        return ptr;
    }
    ptr = buf;
    double d0 = dtime();
    do{
        if((l = read_tty(ptr, LL))){
            r += l; LL -= l; ptr += l;
            if(ptr[-1] == '\n') break;
            d0 = dtime();
        }
    }while(dtime() - d0 < WAIT_TMOUT && LL);
    if(r){
        buf[r] = 0;
        //DBG("r=%zd, got string: %s", r, buf);
        optr = strchr(buf, '\n');
        if(optr) ++optr;
        return buf;
    }
    return NULL;
}

/**
 * Try to connect to `device` at BAUD_RATE speed
 * @return connection speed if success or 0
 */
void try_connect(char *device){
    if(!device) return;
    char tmpbuf[4096];
    fflush(stdout);
    tty_init(device);
    read_tty(tmpbuf, 4096); // clear rbuf
    putlog("Connected to %s", device);
}

/**
 * run terminal emulation: send user's commands and show answers
 */
void run_terminal(){
    green(_("Work in terminal mode without echo\n"));
    int rb;
    char buf[BUFLEN];
    size_t l;
    setup_con();
    while(1){
        if((l = read_tty(buf, BUFLEN - 1))){
            buf[l] = 0;
            printf("%s", buf);
        }
        if((rb = read_console())){
            buf[0] = (char) rb;
            write_tty(buf, 1);
        }
    }
}

/**
 * parser
 * @param buf (i) - data frame ending with \n or \0,
 *      format: Tx_y=N (x - controller No, y - sensor No in pair, N - temperature*100
 * @param N       - controller number
 * @return 1 if all OK
 */
static int parse_answer(char *buf, int N){
    if(!buf) return 0;
    ++buf;
    // safely read integer from buffer
    int getint(){
        char *endptr;
        if(!buf || !*buf) return INT_MIN;
        long l = strtol(buf, &endptr, 10);
        if(l < INT_MIN || l > INT_MAX) return INT_MIN;
        if(endptr == buf) return INT_MIN; // NAN
        buf = endptr;
        return (int)l;
    }
    int i = getint(buf);
    //DBG("buf: %s", buf);
    //DBG("controller #%d", i);
    if(i != N) return 0;
    if(*buf != '_') return 0; // wrong format
    ++buf;
    //DBG("buf: %s", buf);
    int v = getint(buf);
    //DBG("sensor #%d", v);
    if(v < 0 || v > 81) return 0;
    i = v/10; v -= i*10;
    //DBG("i=%d, v=%d", i,v);
    if((v & 1) != v) return 0;
    if(*buf != '=' ) return 0;
    ++buf;
    int T = getint(buf);
    if(T < -27300 || T > 30000){
        t_last[v][i][N] = -300.;
        return 0;
    }
    t_last[v][i][N] = ((double)T) / 100.;
    tmeasured[v][i][N] = time(NULL);
    return 1;
}

/**
 * send command over tty & check answer
 * @param N   - controller number
 * @param cmd - command
 * @return 0 if all OK
 */
static int send_cmd(int N, char cmd){
    if(N < 0 || N > 7) return 1;
    char buf[4] = {(char)N + '0', cmd, '\n', 0};
    char *rtn;
    // clear all incomint data
    while(read_string());
    //DBG("send cmd %s", buf);
    if(write_tty(buf, 3)) return 1;
    if((rtn = read_string())){
        if(*rtn == cmd) return 0;
    }
    return 1;
}

/**
 * Poll sensor for new dataportion
 * @param N - number of controller (1..7)
 * @return: 0 if no data received or controller is absent, 1 if valid data received
 */
int poll_sensors(int N){
    if(!ctrlr_present[N]) return 0;
    char *ans;
    if(send_cmd(N, CMD_MEASURE_T)){ // start polling
        WARNX(_("can't request temperature"));
        return 0;
    }
    int ngot = 0;
    double t0 = dtime();
    while(dtime() - t0 < T_POLLING_TMOUT && ngot < 16){ // timeout reached or got data from all
        if((ans = read_string())){ // parse new data
            //DBG("got %s", ans);
            if(*ans == 'T'){ // data from sensor
                //DBG("ptr: %s", ans);
                ngot += parse_answer(ans, N);
            }
        }
    }
    return 1;
}

/**
 * check whether connected device is main Thermal controller
 * @return 1 if NO
 */
int check_sensors(){
    green(_("Check if there's a sensors...\n"));
    int i, v, N, found = 0;
    char *ans;
    for(N=0;N<8;++N)for(i=0;i<8;++i)for(v=0;v<2;++v) t_last[v][i][N] = -300.; // clear data
    for(i = 1; i < 8; ++i){
        //red("i = %d\n", i);
        double t0 = dtime();
        while(dtime() - t0 < POLLING_TMOUT){
            if(send_cmd(i, CMD_PING)) continue;
            if((ans = read_string())){
                //DBG("got: %s", ans);
                if(0 == strncmp(ans, ANS_PONG, sizeof(ANS_PONG)-1)){
                    //DBG("PONG from %d", ans[sizeof(ANS_PONG)-1] - '0');
                    if(i == ans[sizeof(ANS_PONG)-1] - '0'){
                        ++found;
                        ctrlr_present[i] = 1;
                        green(_("Found controller #%d\n"), i);
                        putlog("Found controller #%d", i);
                        break;
                    }
                }
            }
        }
    }
    if(found) return 0;
    WARNX(_("No sensors detected!"));
    return 1;
}