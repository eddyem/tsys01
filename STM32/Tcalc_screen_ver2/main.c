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
#include "i2c.h"
#include "proto.h"
#include "ssd1306.h"
#include "usart.h"
#include "usb.h"

static uint16_t coefficients[2][5]; // Coefficients for given sensors
static const uint8_t Taddr[2] = {TSYS01_ADDR0, TSYS01_ADDR1};
volatile uint32_t Tms = 0;

#define TBUFLEN 11
static char Tbuf[2][TBUFLEN] = {0}; // buffer for temperatures
static uint8_t refreshdisplay = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

uint8_t sprintu(char *buf, uint8_t buflen, uint32_t val){
    uint8_t i, l = 0, bpos = 0;
    char rbuf[10];
    if(!val){
        buf[0] = '0';
        buf[1] = 0;
        return 1;
    }else{
        while(val){
            rbuf[l++] = val % 10 + '0';
            val /= 10;
        }
        if(buflen < l+1) return 0;
        bpos = l;
        buf[l] = 0;
        for(i = 0; i < l; ++i){
            buf[--bpos] = rbuf[i];
        }
    }
    return l;
}
/*
// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11];
    uint8_t l = sprintu(buf, 11, val);
    while(LINE_BUSY == usart_send_blocking(buf, l));
}*/

void getcoeffs(uint8_t addr){ // show norm coefficiens
    int i;
    const uint8_t regs[5] = {0xAA, 0xA8, 0xA6, 0xA4, 0xA2}; // commands for coefficients
    uint32_t K;
    char numbr = (addr == TSYS01_ADDR0) ? '0' : '1';
    uint16_t *coef = coefficients[numbr-'0'];
    for(i = 0; i < 5; ++i){
        if(write_i2c(addr, regs[i])){
            if(read_i2c(addr, &K, 2)){
                coef[i] = K;
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
        getcoeffs(Taddr[i]);
    }
    if(coefficients[i][0] == 0){
        USEND("no sensor\n");
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
    Tbuf[i][0] = 'T'; Tbuf[i][1] = i + '0';
    Tbuf[i][2] = '='; Tbuf[i][3] = ' ';
    char *ptr = &Tbuf[i][4];
    uint8_t l = TBUFLEN - 4, s;
    if(tmp < 0.){
        *ptr++ = '-';
        tmp = -tmp;
    }
    uint32_t x = (uint32_t)tmp;
    if(x > 120){
        *ptr = 0;
        return 0; // wrong value
    }
    s = sprintu(ptr, l, x);
    ptr += s; l -= s;
    tmp -= x;
    *ptr++ = '.'; --l;
    x = (uint32_t)(tmp*100);
    if(x < 10){
        *ptr++ = '0'; --l;
    }
    s += sprintu(ptr, l, x);
    refreshdisplay = 1;
    return 1;
}

int main(void){
    uint32_t lastT = 0;
    int16_t uptime = 0;
    uint8_t i;
    uint32_t started[2] = {0, 0}; // time of measurements for given sensor started
    uint32_t tgot[2] = {0, 0};  // time of last measurements
    uint8_t errcnt[2] = {0,0};
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    LED_on(LED0);
    usart_setup();
    USB_setup();
    i2c_setup(LOW_SPEED);
    spi_setup();
    // reset on start
    write_i2c(TSYS01_ADDR0, TSYS01_RESET);
    write_i2c(TSYS01_ADDR1, TSYS01_RESET);

    ssd1306_Fill(0);

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(refreshdisplay && uptime < UPTIME - 3){
            ssd1306_Fill(0);
            ssd1306_WriteString(Tbuf[0], Font_11x18, 1);
            ssd1306_SetCursor(0, 32);
            ssd1306_WriteString(Tbuf[1], Font_11x18, 1);
            ssd1306_UpdateScreen();
            refreshdisplay = 0;
        }
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
            ssd1306_UpdateScreen();
            if(USBisOn()){
                LED_on(LED1);
                uptime = 0;
            }else{
                LED_off(LED1);
                if(++uptime > UPTIME - 3){
                    ssd1306_Fill(0);
                    ssd1306_WriteString("Power off!", Font_16x26, 1);
                    ssd1306_UpdateScreen();
                }
                if(uptime > UPTIME) POWEROFF();
            }
        }
        ssd1306_process();
        for(i = 0; i < 2; ++i){
            uint8_t err = 1;
            if(started[i]){
                if(Tms - started[i] > CONV_TIME){ // poll sensor i
                    if(write_i2c(Taddr[i], TSYS01_ADC_READ)){
                        uint32_t t;
                        if(read_i2c(Taddr[i], &t, 3) && t){
                            if(!calc_t(t, i)){
                                //USEND("!calc ");
                                write_i2c(Taddr[i], TSYS01_RESET);
                            }else{
                                err = 0;
                                tgot[i] = Tms;
                            }
                            started[i] = 0;
                        }else{
                            //USEND("can't read ");
                        }
                    }else{
                        //USEND("can't write ");
                    }
                }else{
                    err = 0;
                }
            }else{
                if(Tms - tgot[i] > WAIT_TIME){
                    if(write_i2c(Taddr[i], TSYS01_START_CONV)){
                        started[i] = Tms ? Tms : 1;
                        err = 0;
                    }else{
                        //USEND("can't start conv\n");
                        started[i] = 0;
                    }
                }
            }
            if(err){
                if(!write_i2c(Taddr[i], TSYS01_RESET)){ // sensor's absent? clear its coeff.
                    i2c_setup(LOW_SPEED);
                    if(++errcnt[i] > 10){
                        errcnt[i] = 0;
                        Tbuf[i][0] = '-';
                        tgot[i] = 0;
                        Tbuf[i][1] = 0;
                        coefficients[i][0] = 0;
                        refreshdisplay = 1;
                    }
                }
            }else errcnt[i] = 0;
        }
        usb_proc();
        uint8_t r = 0;
        char inbuf[256];
        if((r = USB_receive(inbuf, 255))){
            inbuf[r] = 0;
            cmd_parser(inbuf, 1);
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            char *txt = NULL;
            r = usart_getline(&txt);
            txt[r] = 0;
            cmd_parser(txt, 0);
        }
    }
    return 0;
}

