/*                                                                                                  geany_encoding=koi8-r
 * client.c - simple terminal client
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
#ifndef CLIENT

#include "usefull_macros.h"
#include "term.h"
#include <strings.h> // strncasecmp
#include <time.h>    // time(NULL)

#define BUFLEN 1024
uint8_t buf[BUFLEN+1]; // buffer for tty data

typedef struct {
    int speed;  // communication speed in bauds/s
    int bspeed; // baudrate from termios.h
} spdtbl;

static spdtbl speeds[] = {
    {50, B50},
    {75, B75},
    {110, B110},
    {134, B134},
    {150, B150},
    {200, B200},
    {300, B300},
    {600, B600},
    {1200, B1200},
    {1800, B1800},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000},
    {0,0}
};

static uint16_t coefficients[2][5] = {{0,0,0,0,0}, {0,0,0,0,0}}; // polinome coefficients for both termometers

/**
 * test if `speed` is in .speed of `speeds` array
 * if not, exit with error code
 * if all OK, return `Bxxx` speed for given baudrate
 */
int conv_spd(int speed){
    spdtbl *spd = speeds;
    int curspeed = 0;
    do{
        curspeed = spd->speed;
        if(curspeed == speed)
            return spd->bspeed;
        ++spd;
    }while(curspeed);
    ERRX(_("Wrong speed value: %d!"), speed);
    return 0;
}

/**
 * Create log file (open in exclusive mode: error if file exists)
 * @param name - file name
 * @param r    - 1 to rewrite existing file
 * @return fd of opened file if all OK, 0 in case of error
 */
int create_log(char *name, int r){
    int fd;
    int oflag = O_WRONLY | O_CREAT;
    if(r) oflag |= O_TRUNC;
    else oflag |= O_EXCL;
    if((fd = open(name, oflag,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1){
        ERR("open(%s) failed", name);
        return 0;
    }
    DBG("%s opened", name);
    return fd;
}

/**
 * read string from terminal (with timeout) into buf
 * @return number of characters read
 */
static size_t read_string(){
    size_t r = 0, l, L = BUFLEN;
    uint8_t *ptr = buf;
    double d0 = dtime();
    do{
        if((l = read_tty(ptr, L))){
            r += l; L -= l; ptr += l;
            d0 = dtime();
        }
    }while(dtime() - d0 < WAIT_TMOUT);
    *ptr = 0;
    DBG("GOT string: %s, len: %zd\n", buf, r);
    return r;
}

// send command. Return 1 if OK
static int send_command(char *cmd, int chk){
    DBG("Send %s", cmd);
    if(write_tty((uint8_t*)cmd, 2)){
        DBG("Bad write");
        return 0;
    }
    if(chk){
        size_t L = read_string();
        if(L != 2 || buf[0] != cmd[0]){
        DBG("Bad answer");
        return 0;
    }}
    return 1;
}

// try to read coefficitents @return amount of sensors if all OK
static int get_coefficients(){
    send_command(CMD_CONSTANTS,0);
    size_t L = read_string();
    DBG("%zd", L);
    if(!L) return 0;
    uint8_t *ptr = buf, *estr = NULL;
    do{
        char N, n; // number of sensor & coefficient
        int C;
        estr = (uint8_t*)strchr((char*)ptr, '\n');
        if(estr){*estr = 0; ++estr;}
        size_t amount = sscanf((char*)ptr, "K%c%c=%d", &N, &n, &C);
        DBG("in str\"%s\" got %zd values", ptr, amount);
        if(3 == amount){
            N -= '0'; n -= '0';
            if((N==0 || N==1) && n < 5){
                DBG("K[%d][%d] = %d", N, n, C);
                coefficients[(int)N][(int)n] = (uint16_t)C;
            }
        }
        ptr = estr;
    }while(estr && *estr);
    // now check for coeffs
    int i, found = 0;
    for(i = 0; i < 2; ++i){ // sensor's number
        int j, k = 0;
        for(j = 0; j < 5; ++j){ // coeff.
            if(coefficients[i][j]) ++k;
        }
        if(k == 5){
            green(_("Found sensor number %d\n"), i);
            ++found;
        }
    }
    return found;
}

/**
 * Try to connect to `device` at given speed
 * Exits with error code if failed
 */
void try_connect(char *device, int speed){
    if(!device) return;
    tty_init(device, speed);
    green(_("Connected to %s, try to get coefficients\n"), device);
    if(!send_command(CMD_REINIT,1) || !send_command(CMD_RESET,1))
        ERRX(_("Can't do communications!"));
    int i; // 10 tries to get constants
    for(i = 0; i < 10; ++i){
        green("Try %d\n", i);
        if(get_coefficients()) return;
        sleep(1);
    }
    ERRX(_("No sensors found!"));
}

void write_log(int fd, char *str){ // write string to log file
    if(fd < 1) return;
    size_t x = strlen(str);
    if(write(fd, str, x))return;
}

/**
 * Get temperature & calculate it by polinome
 * T =    (-2) * k4 * 10^{-21} * ADC16^4
 *      +   4  * k3 * 10^{-16} * ADC16^3
 *      + (-2) * k2 * 10^{-11} * ADC16^2
 *      +   1  * k1 * 10^{-6}  * ADC16
 *      +(-1.5)* k0 * 10^{-2}
 * k0*(-1.5e-2) + 1e-6*val*(k1 + 1e-5*val*(-2*k2 + 1e-5*val*(4*k3 + -2e-5*k4*val)))
 */
static void gettemp(int fd, size_t L){
    if(!L) return;
    char *ptr = (char*)buf, *estr = NULL;
    int32_t Ti[2] = {0,0}; // array for raw temp values
    do{
        char N; // number of sensor
        int32_t T;
        estr = strchr(ptr, '\n');
        if(estr){*estr = 0; ++estr;}
        size_t sc = sscanf(ptr, "T%c=%d", &N, &T);
        if(2 == sc){
            N -= '0';
            if((N==0 || N==1)) Ti[(int)N] = T;
        }
        ptr = estr;
    }while(estr && *estr);
    if(!Ti[0] && !Ti[1]) return; // this isn't T
    double Td[2] = {-300.,-300.};
    int i;
    for(i = 0; i < 2; ++i){
        if(!Ti[i]) continue;
        // check coefficients & try to get them again if absent
        int C=0, j;
        for(j = 0; j < 5; ++j) if(coefficients[i][j]) ++C;
        if(C != 5 && !get_coefficients()) continue;
        double d = (double)Ti[i]/256., tmp = 0.;
        DBG("val256=%g", d);
        // k0*(-1.5e-2) + 0.1*1e-5*val*(1*k1 + 1e-5*val*(-2.*k2 + 1e-5*val*(4*k3 + 1e-5*val*(-2*k4))))
        double mul[5] = {-1.5e-2, 1., -2., 4., -2.};
        for(j = 4; j > 0; --j){
            tmp += mul[j] * (double)coefficients[i][j];
            tmp *= 1e-5*d;
            DBG("tmp=%g, K=%d, mul=%g", tmp, coefficients[i][j], mul[j]);
        }
        DBG("tmp: %g, mul[0]=%g, c0=%d", tmp, mul[0], coefficients[i][0]);
        tmp = tmp/10. + mul[0]*coefficients[i][0];
        Td[i] = tmp;
        DBG("Got temp: %g", tmp);
    }
    time_t utm = time(NULL);
    snprintf((char*)buf, BUFLEN, "%zd\t%.4f\t%.4f\n", utm, Td[0], Td[1]);
    printf("%s", buf);
    write_log(fd, (char*)buf);
}

/**
 * begin logging to stdout and given fd (if >0)
 * Log format: "UNIX_TIME\tT0\tT1\n"
 * if thermometer N is absent, T=-300
 */
void begin_logging(int fd, double pause){
    int ntry;
    while(1){
        double tcmd = dtime();
        if(!send_command(CMD_GETTEMP,0)) continue;
        size_t L = read_string();
        if(!L){
            WARNX(_("No answer, reinit"));
            if(!send_command(CMD_REINIT,1) || !send_command(CMD_RESET,1)){
                write_log(fd, "\nExit on communication error\n");
                ERRX(_("Communications problem!"));
            }
            if(++ntry > 10){
                write_log(fd, "\nNo sensors!\n");
                ERRX(_("No sensors!"));
            }
        }
        ntry = 0;
        // try to convert temperature
        gettemp(fd, L);
        while(dtime() - tcmd < pause);
    }
}

/**
 * run terminal emulation: send user's commands with checksum and show answers
 *
void run_terminal(){
    green(_("Work in terminal mode without echo\n"));
    int rb;
    uint8_t buf[BUFLEN];
    size_t L;
    setup_con();
    while(1){
        if((L = read_tty(buf, BUFLEN))){
            printf(_("Get %zd bytes: "), L);
            uint8_t *ptr = buf;
            while(L--){
                uint8_t c = *ptr++;
                printf("0x%02x", c);
                if(c > 31) printf("(%c)", (char)c);
                printf(" ");
            }
            printf("\n");
        }
        if((rb = read_console())){
            if(rb > 31){
                printf("Send command: %c ... ", (char)rb);
                send_cmd((uint8_t)rb);
                if(TRANS_SUCCEED != wait_checksum()) printf(_("Error.\n"));
                else printf(_("Done.\n"));
            }
        }
    }
}*/




#endif // CLIENT
