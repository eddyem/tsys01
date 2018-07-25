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

void CANsend(uint16_t targetID, uint8_t cmd, char echo){
    if(CAN_OK == can_send_cmd(targetID, cmd)){
        usart_putchar(echo);
        newline();
    }
}

int main(void){
    uint32_t lastT = 0, lastS = 0;
    int16_t L = 0, ID;
    uint8_t gotmeasurement = 0;
    char *txt;
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
        if(CAN_get_status() == CAN_FIFO_OVERRUN){
            SEND("CAN bus fifo overrun occured!\n");
        }
        can_messages_proc();
        //if(sensors_scan_mode){
            if(SENS_SLEEPING == sensors_get_state()){ // show temperature @ each sleeping occurence
                if(!gotmeasurement){
                    //SEND("\nTIME=");
                    //printu(Tms);
                    //usart_putchar('\t');
                    //newline();
                    gotmeasurement = 1;
                    showtemperature();
                }
            }else{
                gotmeasurement = 0;
            }
        //}
        if(usartrx()){ // usart1 received data, store in in buffer
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                if(_1st > '0' && _1st < '8'){
                    ID = (CAN_ID_PREFIX & CAN_ID_MASK) | (_1st - '0');
                    CANsend(ID, CMD_START_MEASUREMENT, _1st);
                }else switch(_1st){
                    case 'A':
                        CANsend(BCAST_ID, CMD_START_MEASUREMENT, _1st);
                        if(!sensors_scan_mode) sensors_start();
                    break;
                    case 'B':
                        CANsend(BCAST_ID, CMD_DUMMY0, _1st);
                    break;
                    case 'C': // 'C' - show coefficients
                        showcoeffs();
                    break;
                    case 'D':
                        CANsend(MASTER_ID, CMD_DUMMY1, _1st);
                    break;
                    case 'E':
                        SEND("End scan mode\n");
                        sensors_scan_mode = 0;
                    break;
                    case 'F':
                        sensors_off();
                    break;
                    case 'G':
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'H':
                        i2c_setup(HIGH_SPEED);
                        SEND("High speed\n");
                    break;
                    case 'I':
                        CAN_reinit();
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'L':
                        i2c_setup(LOW_SPEED);
                        SEND("Low speed\n");
                    break;
                    case 'O':
                        sensors_on();
                    break;
                    case 'P':
                        CANsend(BCAST_ID, CMD_PING, _1st);
                    break;
                    case 'R':
                        i2c_setup(CURRENT_SPEED);
                        SEND("Reinit I2C\n");
                    break;
                    case 'S':
                        SEND("Start scan mode\n");
                        sensors_scan_mode = 1;
                    break;
                    case '0':
                    case 'T': // 'T' - get temperature
                        if(!sensors_scan_mode) sensors_start();
                    break;
                    case 'V':
                        i2c_setup(VERYLOW_SPEED);
                        SEND("Very low speed\n");
                    break;
                    case 'Z':
                        CANsend(BCAST_ID, CMD_SENSORS_STATE, _1st);
                    break;
#if 0
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
                        SEND(
                        "0..7 - start measurement on given controller\n"
                        "A - start measurement on all controllers\n"
                        "B - send broadcast CAN dummy message\n"
                        "C - show coefficients\n"
                        "D - send CAN dummy message to master\n"
                        "E - end themperature scan\n"
                        "F - turn oFf sensors\n"
                        "G - get CAN address\n"
                        "H - high speed\n"
                        "I - reinit CAN\n"
                        "L - low speed\n"
                        "O - turn On sensors\n"
                        "P - ping everyone over CAN\n"
                        "R - reinit I2C\n"
                        "S - Start themperature scan\n"
                        "T - start temperature measurement\n"
                        "V - very low speed\n"
                        "Z - get sensors state over CAN\n"
#if 0
                        "\t\tTEST OPTIONS\n"
                        "d - discovery\n"
                        "g - get coeff\n"
                        "t - measure temper\n"
                        "s - show temper measured\n"
                        "p - sensors_process()\n"
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

