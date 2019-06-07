/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
 *
 */
#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "hardware.h"
#include "proto.h"
#include "sensors_manage.h"
#include "usart.h"
#include "usb.h"
#include <string.h> // strlen, strcpy(

extern volatile uint8_t canerror;

static char buff[UARTBUFSZ+1], *bptr = buff;
static uint8_t blen = 0, USBcmd = 0;
// LEDs are OFF by default
uint8_t noLED = 1;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    if(USBcmd) USB_send(buff);
    else while(LINE_BUSY == usart_send(buff, blen)){IWDG->KR = IWDG_REFRESH;}
    bptr = buff;
    blen = 0;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    int l = strlen(txt);
    if(l > UARTBUFSZ){
        sendbuf();
        if(USBcmd) USB_send(txt);
        else while(LINE_BUSY == usart_send_blocking(txt, l)){IWDG->KR = IWDG_REFRESH;}
    }else{
        if(blen+l > UARTBUFSZ){
            sendbuf();
        }
        strcpy(bptr, txt);
        bptr += l;
    }
    *bptr = 0;
    blen += l;
}

void bufputchar(char ch){
    if(blen > UARTBUFSZ-1){
        sendbuf();
    }
    *bptr++ = ch;
    ++blen;
}

static void CANsend(uint16_t targetID, uint8_t cmd, char echo){
    if(CAN_OK == can_send_cmd(targetID, cmd)){
        bufputchar(echo);
        bufputchar('\n');
    }
}

// show all ADC values
static inline void showADCvals(){
    char msg[] = "ADCn=";
    for(int n = 0; n < NUMBER_OF_ADC_CHANNELS; ++n){
        msg[3] = n + '0';
        addtobuf(msg);
        printu(getADCval(n));
        newline();
    }
}

static inline void printmcut(){
    SEND("MCUT=");
    int32_t T = getMCUtemp();
    if(T < 0){
        bufputchar('-');
        T = -T;
    }
    printu(T);
    newline();
}

static inline void showUIvals(){
    uint16_t *vals = getUval();
    SEND("V12="); printu(vals[0]);
    SEND("\nV5="); printu(vals[1]);
    SEND("\nV33="); printu(vals[3]);
    SEND("\nI12="); printu(vals[2]);
    newline();
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt, uint8_t isUSB){
    USBcmd = isUSB;
    int16_t L = strlen(txt), ID = BCAST_ID;
    char _1st = txt[0];
    sendbuf();
    if(_1st >= '0' && _1st < '8'){ // send command to Nth controller, not broadcast
        if(L == 3){ // with '\n' at end!
            /*if(_1st == '0'){
                bufputchar(txt[1]);
                _1st = txt[1] + 'a' - 'A'; // change network command to local
                bufputchar('\n');
            }else */
            {
                ID = (CAN_ID_PREFIX & CAN_ID_MASK) | (_1st - '0');
                _1st = txt[1];
            }
        }else{
            _1st = '?'; // show help
        }
    }else if(L != 2) _1st = '?';
    switch(_1st){
        case 'a':
            showADCvals();
        break;
        case 'B':
            CANsend(ID, CMD_DUMMY0, _1st);
        break;
        case 'c':
            showcoeffs();
        break;
        case 'D':
            CANsend(MASTER_ID, CMD_DUMMY1, _1st);
        break;
        case 'E':
            CANsend(ID, CMD_STOP_SCAN, _1st);
        break;
        case 'e':
            sensors_scan_mode = 0;
        break;
        case 'F':
            CANsend(ID, CMD_SENSORS_OFF, _1st);
        break;
        case 'f':
            sensors_off();
        break;
        case 'g':
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'H':
            CANsend(ID, CMD_HIGH_SPEED, _1st);
        break;
        case 'h':
            i2c_setup(HIGH_SPEED);
        break;
        case 'i':
            CAN_reinit();
            SEND("Can address: ");
            printuhex(getCANID());
            newline();
        break;
        case 'J':
            CANsend(ID, CMD_GETMCUTEMP, _1st);
        break;
        case 'j':
            printmcut();
        break;
        case 'K':
            CANsend(ID, CMD_GETUIVAL, _1st);
        break;
        case 'k':
            showUIvals();
        break;
        case 'L':
            CANsend(ID, CMD_LOW_SPEED, _1st);
        break;
        case 'l':
            i2c_setup(LOW_SPEED);
        break;
        case 'M':
            CANsend(ID, CMD_CHANGE_MASTER_B, _1st);
        break;
        case 'm':
            CANsend(ID, CMD_CHANGE_MASTER, _1st);
        break;
        case 'O':
            noLED = 0;
            SEND("LED on\n");
        break;
        case 'o':
            noLED = 1;
            LED_off(LED0);
            LED_off(LED1);
            SEND("LED off\n");
        break;
        case 'P':
            CANsend(ID, CMD_PING, _1st);
        break;
        case 'R':
            CANsend(ID, CMD_REINIT_I2C, _1st);
        break;
        case 'r':
            i2c_setup(CURRENT_SPEED);
        break;
        case 'S':
            CANsend(ID, CMD_START_SCAN, _1st);
        break;
        case 's':
            sensors_scan_mode = 1;
        break;
        case 'T':
            CANsend(ID, CMD_START_MEASUREMENT, _1st);
        break;
        case 't':
            if(!sensors_scan_mode) sensors_start();
        break;
        break;
        case 'u':
            SEND("CANERROR=");
            if(canerror){
                canerror = 0;
                bufputchar('1');
            }else bufputchar('0');
            newline();
        break;
        case 'V':
            CANsend(ID, CMD_LOWEST_SPEED, _1st);
        break;
        case 'v':
            i2c_setup(VERYLOW_SPEED);
        break;
        case 'Z':
            CANsend(ID, CMD_SENSORS_STATE, _1st);
        break;
        case 'z':
            SEND("SSTATE0=");
            printu(sensors_get_state());
            newline();
        break;
        default: // help
            SEND(
            "ALL little letters - without CAN messaging\n"
            "0..7 - send command to given controller (0 - this) instead of broadcast\n"
            "a - get raw ADC values\n"
            "B - send broadcast CAN dummy message\n"
            "c - show coefficients (current)\n"
            "D - send CAN dummy message to master\n"
            "Ee- end themperature scan\n"
            "Ff- turn oFf sensors\n"
            "g - get last CAN address\n"
            "Hh- high I2C speed\n"
            "i - reinit CAN (with new address)\n"
            "Jj- get MCU temperature\n"
            "Kk- get U/I values\n"
            "Ll- low I2C speed\n"
            "Mm- change master id to 0 (m) / broadcast (M)\n"
            "Oo- turn onboard diagnostic LEDs *O*n or *o*ff (both commands are local)\n"
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
    sendbuf();
}

// print 32bit unsigned int
void printu(uint32_t val){
    char buf[11], *bufptr = &buf[10];
    *bufptr = 0;
    if(!val){
        *(--bufptr) = '0';
    }else{
        while(val){
            *(--bufptr) = val % 10 + '0';
            val /= 10;
        }
    }
    addtobuf(bufptr);
}

// print 32bit unsigned int as hex
void printuhex(uint32_t val){
    addtobuf("0x");
    uint8_t *ptr = (uint8_t*)&val + 3;
    int i, j;
    for(i = 0; i < 4; ++i, --ptr){
        for(j = 1; j > -1; --j){
            uint8_t half = (*ptr >> (4*j)) & 0x0f;
            if(half < 10) bufputchar(half + '0');
            else bufputchar(half - 10 + 'a');
        }
    }
}
