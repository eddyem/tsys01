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
#include "can_process.h"
#include "proto.h"

#pragma message("USARTNUM=" STR(USARTNUM))
#pragma message("I2CPINS=" STR(I2CPINS))
#ifdef EBUG
#pragma message("Debug mode")
#else
#pragma message("Release mode")
#endif

volatile uint32_t Tms = 0;
volatile uint8_t canerror = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

static void iwdg_setup(){
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
    uint8_t gotmeasurement = 0;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    usart_setup();
    i2c_setup(LOW_SPEED);
    iwdg_setup();
    CAN_setup();

    SEND("Greetings! My address is ");
    printuhex(getCANID());
    newline();

    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        SEND("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        SEND("SOFTRESET=1\n");
    }
    RCC->CSR |= RCC_CSR_RMVF; // remove reset flags

    while (1){
        IWDG->KR = IWDG_REFRESH; // refresh watchdog
        if(lastT > Tms || Tms - lastT > 499){
            LED_blink(LED0);
            lastT = Tms;
            // send dummy command to noone to test CAN bus
            can_send_cmd(NOONE_ID, CMD_DUMMY0);
        }
        if(lastS > Tms || Tms - lastS > 5){ // run sensors proc. once per 5ms
            sensors_process();
            lastS = Tms;
        }
        can_proc();
        CAN_status stat = CAN_get_status();
        if(stat == CAN_FIFO_OVERRUN){
            SEND("CAN bus fifo overrun occured!\n");
        }else if(stat == CAN_ERROR){
            LED_off(LED1);
            CAN_setup();
            canerror = 1;
        }
        can_messages_proc();
        if(SENS_SLEEPING == sensors_get_state()){ // show temperature @ each sleeping occurence
            if(!gotmeasurement){
                gotmeasurement = 1;
                showtemperature();
            }
        }else{
            gotmeasurement = 0;
        }
        if(usartrx()){ // usart1 received data, store in in buffer
            cmd_parser();
        }
    }
    return 0;
}

