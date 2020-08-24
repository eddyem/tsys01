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

// UNIX time of temperatures measurement: [Ngroup][Nsensor][Ncontroller]
time_t tmeasured[2][NCHANNEL_MAX+1][NCTRLR_MAX+1];
// last temperatures read: [Ngroup][Nsensor][Ncontroller]
double t_last[2][NCHANNEL_MAX+1][NCTRLR_MAX+1];

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
    LOG("Connected to %s", device);
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
    //if(v < 0 || v > 81) return 0;
    i = v/10; v -= i*10;
    if(i < 0 || i > NCHANNEL_MAX) return 0;
    //DBG("i=%d, v=%d", i,v);
    if((v & 1) != v) return 0; // should be only 0 or 1
    if(*buf != '=' ) return 0;
    ++buf;
    int T = getint(buf);
    if(T < -27300 || T > 30000){
        t_last[v][i][N] = -300.;
        return 0;
    }
    t_last[v][i][N] = ((double)T) / 100.;
    DBG("T(%d_%d%d)=%g", N, i, v, T/100.);
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
    if(N < 0 || N > NCTRLR_MAX) return 1;
    char buf[3] = {0};
    int n = 3;
    char *rtn;
    if(N){ // CAN-bus
        buf[0] = (char)N + '0';
        buf[1] = cmd;
        buf[2] = '\n';
    }else{ // local command
        n = 2;
        buf[0] = cmd;
        buf[1] = '\n';
    }
    // clear all incomint data
    while(read_string());
    DBG("send cmd %s", buf);
    if(write_tty(buf, n)) return 1;
    if(N == 0) return 0;
    if((rtn = read_string())){
    DBG("read_string: %s", rtn);
        if(*rtn == cmd) return 0;
    }
    return 1;
}

/**
 * Poll sensor for new dataportion
 * @param N - number of controller (1..7)
 * @return: 0 if no data received or controller is absent, number of data points if valid data received
 */
int poll_sensors(int N){
    char *ans;
    DBG("Poll controller #%d", N);
    char cmd = CMD_MEASURE_LOCAL;
    if(N) cmd = CMD_MEASURE_T;
    if(send_cmd(N, cmd)){ // start polling
        WARNX(_("can't request temperature"));
        return 0;
    }
    int ngot = 0;
    double t0 = dtime();
    while(dtime() - t0 < T_POLLING_TMOUT && ngot < 2*(NCHANNEL_MAX+1)){ // timeout reached or got data from all
        if((ans = read_string())){ // parse new data
            //DBG("got %s", ans);
            if(*ans == CMD_MEASURE_T){ // data from sensor
                //DBG("ptr: %s", ans);
                ngot += parse_answer(ans, N);
            }
        }
    }
    return 1;
}

/**
 * @brief turn_all_off - turn all sensors OFF
 */
void turn_all_off(){
    send_cmd(0, CMD_SENSORS_OFF_LOCAL);
    for(int i = 1; i <= NCTRLR_MAX; ++i) send_cmd(i, CMD_SENSORS_OFF);
}

/**
 * check whether connected device is main Thermal controller
 * @return 1 if NO
 */
int check_sensors(){
    green(_("Check if there's a sensors...\n"));
    int i, v, N, found = 0;
    char *ans;
    for(N=0;N<=NCTRLR_MAX;++N)for(i=0;i<=NCHANNEL_MAX;++i)for(v=0;v<2;++v) t_last[v][i][N] = -300.; // clear data
    for(i = 1; i <= NCTRLR_MAX; ++i){
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
                        green(_("Found controller #%d\n"), i);
                        LOG("Found controller #%d", i);
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
