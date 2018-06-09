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
#include "can.h"

#pragma message("USARTNUM=" STR(USARTNUM))
#pragma message("I2CPINS=" STR(I2CPINS))
#ifdef EBUG
#pragma message("Debug mode")
#else
#pragma message("Release mode")
#endif

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

void iwdg_setup(){
    /* Enable the peripheral clock RTC */
    /* (1) Enable the LSI (40kHz) */
    /* (2) Wait while it is not ready */
    RCC->CSR |= RCC_CSR_LSION; /* (1) */
    while((RCC->CSR & RCC_CSR_LSIRDY) != RCC_CSR_LSIRDY); /* (2) */
    /* Configure IWDG */
    /* (1) Activate IWDG (not needed if done in option bytes) */
    /* (2) Enable write access to IWDG registers */
    /* (3) Set prescaler by 64 (1.6ms for each tick) */
    /* (4) Set reload value to have a rollover each 2s */
    /* (5) Check if flags are reset */
    /* (6) Refresh counter */
    IWDG->KR = IWDG_START; /* (1) */
    IWDG->KR = IWDG_WRITE_ACCESS; /* (2) */
    IWDG->PR = IWDG_PR_PR_1; /* (3) */
    IWDG->RLR = 1250; /* (4) */
    while(IWDG->SR); /* (5) */
    IWDG->KR = IWDG_REFRESH; /* (6) */
}

int main(void){
    uint32_t lastT = 0, lastS = 0;
    int16_t L = 0;
    char *txt;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    i2c_setup(LOW_SPEED);
    iwdg_setup();
    readCANaddr();

    SEND("Greetings! My address is ");
    printu(getCANaddr());
    newline();

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
        }
        if(lastS > Tms || Tms - lastS > 5){ // run sensors proc. once per 5ms
            sensors_process();
            lastS = Tms;
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                switch(_1st){
                    case 'C': // 'C' - show coefficients
                        showcoeffs();
                    break;
                    case 'O':
                        sensors_on();
                    break;
                    case 'T': // 'T' - get temperature
                        showtemperature();
                    break;
                    case 'R':
                        i2c_setup(CURRENT_SPEED);
                        SEND("Reinit I2C\n");
                    break;
                    case 'V':
                        i2c_setup(VERYLOW_SPEED);
                        SEND("Very low speed\n");
                    break;
                    case 'L':
                        i2c_setup(LOW_SPEED);
                        SEND("Low speed\n");
                    break;
                    case 'H':
                        i2c_setup(HIGH_SPEED);
                        SEND("High speed\n");
                    break;
                    case 'G':
                        SEND("Can address: ");
                        printu(getCANaddr());
                        newline();
                    break;
#ifdef EBUG
                    case 'd':
                    case 'g':
                    case 't':
                    case 's':
                        senstest(_1st);
                    break;
                    case 'p':
                        sensors_process();
                    break;
#endif
                    default: // help
                        SEND("'C' - show coefficients\n"
                        "'O' - turn On sensors\n"
                        "'T' - get raw temperature\n"
                        "'R' - reinit I2C\n"
                        "'V' - very low speed\n"
                        "'L' - low speed\n"
                        "'H' - high speed\n"
                        "'G' - get CAN address\n"
#ifdef EBUG
                        "\t\tTEST OPTIONS\n"
                        "'d' - discovery\n"
                        "'g' - get coeff\n"
                        "'t' - measure temper\n"
                        "'s' - show temper measured\n"
                        "'p' - sensors_process()\n"
#endif
                        );
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

