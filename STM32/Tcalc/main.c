/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "hardware.h"
#include "usart.h"
#include "i2c.h"

static uint16_t coefficients[2][5]; // Coefficients for given sensors
volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], rbuf[10];
    int l = 0, bpos = 0;
    if(!val){
        buf[0] = '0';
        l = 1;
    }else{
        while(val){
            rbuf[l++] = val % 10 + '0';
            val /= 10;
        }
        int i;
        bpos += l;
        for(i = 0; i < l; ++i){
            buf[--bpos] = rbuf[i];
        }
    }
    while(LINE_BUSY == usart_send_blocking(buf, l+bpos));
}

void showcoeffs(uint8_t addr, uint8_t verb){ // show norm coefficiens
    int i;
    const uint8_t regs[5] = {0xAA, 0xA8, 0xA6, 0xA4, 0xA2}; // commands for coefficients
    uint32_t K;
    char numbr = (addr == TSYS01_ADDR0) ? '0' : '1';
    uint16_t *coef = coefficients[numbr-'0'];
    for(i = 0; i < 5; ++i){
        if(write_i2c(addr, regs[i])){
            if(read_i2c(addr, &K, 2)){
                coef[i] = K;
                if(verb){
                    char b[4] = {'K', numbr, i+'0', '='};
                    while(ALL_OK != usart_send_blocking(b, 4));
                    printu(K);
                    newline();
                }
            }
        }
    }
}

/**
 * Get temperature & calculate it by polinome
 * T =    (-2) * k4 * 10^{-21} * ADC16^4
 *      +   4  * k3 * 10^{-16} * ADC16^3
 *      + (-2) * k2 * 10^{-11} * ADC16^2
 *      +   1  * k1 * 10^{-6}  * ADC16
 *      +(-1.5)* k0 * 10^{-2}
 * k0*(-1.5e-2) + 1e-6*val*(k1 + 1e-5*val*(-2*k2 + 1e-5*val*(4*k3 + -2e-5*k4*val)))
 * answer is in 100th
 */
uint8_t calc_t(uint32_t t, int i){
    if(coefficients[i][0] == 0){
        if(i == 0) showcoeffs(TSYS01_ADDR0, 0);
        else showcoeffs(TSYS01_ADDR1, 0);
    }
    if(coefficients[i][0] == 0){
        SEND("no sensor\n");
        return 0;
    }
    if (t < 6500000 || t > 13000000) return 0; // wrong value - too small or too large
    int j;
    double d = (double)t/256., tmp = 0.;
    // k0*(-1.5e-2) + 0.1*1e-5*val*(1*k1 + 1e-5*val*(-2.*k2 + 1e-5*val*(4*k3 + 1e-5*val*(-2*k4))))
    const double mul[5] = {-1.5e-2, 1., -2., 4., -2.};
    for(j = 4; j > 0; --j){
        tmp += mul[j] * (double)coefficients[i][j];
        tmp *= 1e-5*d;
    }
    tmp = tmp/10. + mul[0]*coefficients[i][0];
    char b[8] = "TdegC0=";
    if(i) b[5] = '1';
    while(ALL_OK != usart_send_blocking(b, 7));
    if(tmp < 0.){
        SEND("-");
        tmp = -tmp;
    }
    uint32_t x = (uint32_t)tmp;
    if(x > 120) return 0; // wrong value
    printu(x);
    tmp -= x;
    SEND(".");
    x = (uint32_t)(tmp*100);
    if(x < 10) SEND("0");
    printu(x);
    newline();
    return 1;
}
/*
uint8_t calc_t(uint32_t t, int i){
    if(coefficients[i][0] == 0){
        if(i == 0) showcoeffs(TSYS01_ADDR0, 0);
        else showcoeffs(TSYS01_ADDR1, 0);
    }
    if(coefficients[i][0] == 0){
        SEND("no sensor\n");
        return 0;
    }
    if (t < 6500000 || t > 13000000) return 0; // wrong value - too small or too large
    int j;
    int64_t d = t, tmp = 0.;
    // k0*(-1.5e-2) + 0.1*1e-5*val*(1*k1 + 1e-5*val*(-2.*k2 + 1e-5*val*(4*k3 + 1e-5*val*(-2*k4))))
    int8_t mul[5] = {0, 1, -2, 4, -2};
    for(j = 4; j > 0; --j){
        tmp /= 100000;
        tmp += mul[j] * coefficients[i][j];
        tmp *= d;
        tmp >>= 8; // (/256)
    }
    tmp /= 10000;
    uint16_t K = coefficients[i][0];
    K += K/2;
    tmp -= K;
    char b[8] = "TdegC0=";
    if(i) b[5] = '1';
    while(ALL_OK != usart_send_blocking(b, 7));
    if(tmp < 0.){
        SEND("-");
        tmp = -tmp;
    }
    uint32_t x = (uint32_t)(tmp/100);
    printu(x);
    tmp -= 100*x;
    SEND(".");
    printu((uint32_t)tmp);
    newline();
    return 1;
}*/

int main(void){
    uint32_t lastT = 0;
    int16_t L = 0;
    uint32_t started0=0, started1=0; // time of measurements for given sensor started
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    i2c_setup(LOW_SPEED);
    // reset on start
    write_i2c(TSYS01_ADDR0, TSYS01_RESET);
    write_i2c(TSYS01_ADDR1, TSYS01_RESET);

    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(started0 && Tms - started0 > CONV_TIME){ // poll sensor0
            if(write_i2c(TSYS01_ADDR0, TSYS01_ADC_READ)){
                uint32_t t;
                if(read_i2c(TSYS01_ADDR0, &t, 3) && t){
                    if(!calc_t(t, 0)) write_i2c(TSYS01_ADDR0, TSYS01_RESET);
                    started0 = 0;
                }
            }
        }
        if(started1 && Tms - started1 > CONV_TIME){ // poll sensor1
            if(write_i2c(TSYS01_ADDR1, TSYS01_ADC_READ)){
                uint32_t t;
                if(read_i2c(TSYS01_ADDR1, &t, 3) && t){
                    if(!calc_t(t, 1)) write_i2c(TSYS01_ADDR1, TSYS01_RESET);
                    started1 = 0;
                }
            }
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                uint32_t tstart = Tms;
                switch(_1st){
                    case 'C': // 'C' - show coefficients
                        showcoeffs(TSYS01_ADDR0, 1);
                        showcoeffs(TSYS01_ADDR1, 1);
                    break;
                    case 'R': // 'R' - reset both
                        SEND("Reset\n");
                        write_i2c(TSYS01_ADDR0, TSYS01_RESET);
                        write_i2c(TSYS01_ADDR1, TSYS01_RESET);
                    break;
                    case 'D':
                        if(write_i2c(TSYS01_ADDR0, TSYS01_RESET)) SEND("0");
                        if(write_i2c(TSYS01_ADDR1, TSYS01_RESET)) SEND("1");
                        newline();
                    break;
                    case 'T': // 'T' - get temperature
                        if(tstart == 0) tstart = 1;
                        if(write_i2c(TSYS01_ADDR0, TSYS01_START_CONV)) started0 = tstart;
                        else{
                            started0 = 0;
                        }
                        if(write_i2c(TSYS01_ADDR1, TSYS01_START_CONV)) started1 = tstart;
                        else{
                            started1 = 0;
                        }
                    break;
                    case 'L':
                        i2c_setup(LOW_SPEED);
                        SEND("Low speed\n");
                    break;
                    case 'H':
                        i2c_setup(HIGH_SPEED);
                        SEND("High speed\n");
                    break;
                    default: // help
                        SEND("'C' - show coefficients\n"
                        "'D' - slave discovery\n"
                        "'R' - reset both\n"
                        "'T' - get raw temperature\n"
                        "'L' - low speed\n"
                        "'H' - high speed\n");
                    break;
                }
            }
        }
        if(L){ // text waits for sending
            while(LINE_BUSY == usart_send(txt, L));
            L = 0;
        }
    }
    return 0;
}

