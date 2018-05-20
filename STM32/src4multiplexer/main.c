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

void showcoeffs(uint8_t addr){ // show norm coefficiens
    int i;
    const uint8_t regs[5] = {0xAA, 0xA8, 0xA6, 0xA4, 0xA2}; // commands for coefficients
    uint32_t K;
    char numbr = (addr == TSYS01_ADDR0) ? '0' : '1';
    for(i = 0; i < 5; ++i){
        if(write_i2c(addr, regs[i])){
            if(read_i2c(addr, &K, 2)){
                char b[4] = {'K', numbr, i+'0', '='};
                while(ALL_OK != usart_send_blocking(b, 4));
                printu(K);
                while(ALL_OK != usart_send_blocking("\n", 1));
            }
        }
    }
}

void ch_addr(char *str){
    char ch;
    uint32_t L = 0;
    for(ch = *str++; ch && ch > '/' && ch < ':'; ch = *str++){
        L = L * 10 + ch - '0';
        if(L > 47){
            SEND("Bad address!\n");
            return;
        }
    }
    MUL_ADDRESS(L);
    printu(L);
    SEND(" channel active\n");
}

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
                if(read_i2c(TSYS01_ADDR0, &t, 3)){
                    while(ALL_OK != usart_send_blocking("T0=", 3));
                    printu(t);
                    while(ALL_OK != usart_send_blocking("\n", 1));
                    started0 = 0;
                }
            }
        }
        if(started1 && Tms - started1 > CONV_TIME){ // poll sensor1
            if(write_i2c(TSYS01_ADDR1, TSYS01_ADC_READ)){
                uint32_t t;
                if(read_i2c(TSYS01_ADDR1, &t, 3)){
                    while(ALL_OK != usart_send_blocking("T1=", 3));
                    printu(t);
                    while(ALL_OK != usart_send_blocking("\n", 1));
                    started1 = 0;
                }
            }
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(_1st > '/' && _1st < ':'){
                ch_addr(txt);
                L = 0;
            }else if(L == 2 && txt[1] == '\n'){
                L = 0;
                uint32_t tstart = Tms;
                switch(_1st){
                    case 'C': // 'C' - show coefficients
                        showcoeffs(TSYS01_ADDR0);
                        showcoeffs(TSYS01_ADDR1);
                    break;
                    case 'R': // 'R' - reset both
                        SEND("Reset\n");
                        write_i2c(TSYS01_ADDR0, TSYS01_RESET);//) SEND("0 - err\n");
                        write_i2c(TSYS01_ADDR1, TSYS01_RESET);//) SEND("1 - err\n");
                    break;
                    case 'D':
                        if(write_i2c(TSYS01_ADDR0, TSYS01_RESET)) SEND("0");
                        if(write_i2c(TSYS01_ADDR1, TSYS01_RESET)) SEND("1");
                        SEND("\n");
                    break;
                    case 't':
                        SEND("USART");
                        SEND(" test ");
                        printu(111);
                        SEND(" got number 111?\n");
                    break;
                    case 'T': // 'T' - get temperature
                        if(tstart == 0) tstart = 1;
                        if(write_i2c(TSYS01_ADDR0, TSYS01_START_CONV)) started0 = tstart;
                        else{
                            //SEND("0 BAD\n");
                            started0 = 0;
                        }
                        if(write_i2c(TSYS01_ADDR1, TSYS01_START_CONV)) started1 = tstart;
                        else{
                            //SEND("1 BAD\n");
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
                        "'T' - get temperature\n"
                        "'t' - USART test\n"
                        "number - set active channel\n"
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

