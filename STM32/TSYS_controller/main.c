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
#include "sensors_manage.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

int main(void){
    uint32_t lastT = 0;
    int16_t L = 0;
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
        sensors_process();
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                switch(_1st){
                    case 'C': // 'C' - show coefficients
                        showcoeffs();
                    break;
                    case 'D':
                        sensors_on();
                    break;
                    case 'T': // 'T' - get temperature
                        showtemperature();
                    break;
                    case 'R':
                        i2c_setup(CURRENT_SPEED);
                        SEND("Reinit I2C\n");
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
                        "'T' - get raw temperature\n"
                        "'R' - reinit I2C\n"
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

