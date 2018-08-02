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
    uint8_t gotmeasurement = 0, canerror = 0;
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
            L = usart_getline(&txt);
            char _1st = txt[0];
            if(L == 2 && txt[1] == '\n'){
                L = 0;
                if(_1st > '0' && _1st < '8'){
                    ID = (CAN_ID_PREFIX & CAN_ID_MASK) | (_1st - '0');
                    CANsend(ID, CMD_START_MEASUREMENT, _1st);
                }else switch(_1st){
                    case 'B':
                        CANsend(BCAST_ID, CMD_DUMMY0, _1st);
                    break;
                    case 'c':
                        showcoeffs();
                    break;
                    case 'D':
                        CANsend(MASTER_ID, CMD_DUMMY1, _1st);
                    break;
                    case 'E':
                        CANsend(BCAST_ID, CMD_STOP_SCAN, _1st);
                    case 'e':
                        SEND("End scan mode\n");
                        sensors_scan_mode = 0;
                    break;
                    case 'F':
                        CANsend(BCAST_ID, CMD_SENSORS_OFF, _1st);
                    case 'f':
                        SEND("Turn off sensors\n");
                        sensors_off();
                    break;
                    case 'g':
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'H':
                        CANsend(BCAST_ID, CMD_HIGH_SPEED, _1st);
                    case 'h':
                        i2c_setup(HIGH_SPEED);
                        SEND("High speed\n");
                    break;
                    case 'i':
                        CAN_reinit();
                        SEND("Can address: ");
                        printuhex(getCANID());
                        newline();
                    break;
                    case 'L':
                        CANsend(BCAST_ID, CMD_LOW_SPEED, _1st);
                    case 'l':
                        i2c_setup(LOW_SPEED);
                        SEND("Low speed\n");
                    break;
                    /*case 'o':
                        sensors_on();
                    break;*/
                    case 'P':
                        CANsend(BCAST_ID, CMD_PING, _1st);
                    break;
                    case 'R':
                        CANsend(BCAST_ID, CMD_REINIT_I2C, _1st);
                    case 'r':
                        i2c_setup(CURRENT_SPEED);
                        SEND("Reinit I2C\n");
                    break;
                    case 'S':
                        CANsend(BCAST_ID, CMD_START_SCAN, _1st);
                    case 's':
                        SEND("Start scan mode\n");
                        sensors_scan_mode = 1;
                    break;
                    case 'T':
                        CANsend(BCAST_ID, CMD_START_MEASUREMENT, _1st);
                    case '0':
                    case 't':
                        if(!sensors_scan_mode) sensors_start();
                    break;
                    case 'u':
                        SEND("CANERROR=");
                        if(canerror){
                            canerror = 0;
                            usart_putchar('1');
                        }else usart_putchar('0');
                        newline();
                    break;
                    case 'V':
                        CANsend(BCAST_ID, CMD_LOWEST_SPEED, _1st);
                    case 'v':
                        i2c_setup(VERYLOW_SPEED);
                        SEND("Very low speed\n");
                    break;
                    case 'Z':
                        CANsend(BCAST_ID, CMD_SENSORS_STATE, _1st);
                    break;
                    default: // help
                        SEND(
                        "ALL little letters - without CAN messaging\n"
                        "0..7 - start measurement on given controller (0 - this)\n"
                        "B - send broadcast CAN dummy message\n"
                        "c - show coefficients (current)\n"
                        "D - send CAN dummy message to master\n"
                        "Ee- end themperature scan\n"
                        "Ff- turn oFf sensors\n"
                        "g - get last CAN address\n"
                        "Hh- high I2C speed\n"
                        "i - reinit CAN (with new address)\n"
                        "Ll- low I2C speed\n"
                       // "o - turn On sensors\n"
                        "P - ping everyone over CAN\n"
                        "Rr- reinit I2C\n"
                        "Ss- Start themperature scan\n"
                        "Tt- start temperature measurement\n"
                        "u - check CAN status for errors\n"
                        "Vv- very low I2C speed\n"
                        "Z - get sensors state over CAN\n"
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

