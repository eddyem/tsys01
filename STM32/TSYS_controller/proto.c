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

#include <string.h>

#include "adc.h"
#include "can.h"
#include "can_process.h"
#include "hardware.h"
#include "proto.h"
#include "sensors_manage.h"
#include "strfunc.h"
#include "usb.h"
#include "version.inc"

extern volatile uint8_t canerror;
extern volatile uint32_t Tms;

static uint8_t debugmode = 0;
// LEDs are OFF by default
uint8_t noLED =
#ifdef EBUG
        0
#else
        1
#endif
;

static void CANsend(uint16_t targetID, uint8_t cmd, char echo){
    if(targetID == 0){
        USB_sendstr("Point number of controller (1..7), broadcast messages are deprecated!\n");
        return;
    }
    if(CAN_OK == can_send_cmd(targetID, cmd) && echo){
        USB_putbyte(echo);
        newline();
    }
}

// show all ADC values
static inline void showADCvals(){
    char msg[] = "ADCn=";
    for(int n = 0; n < NUMBER_OF_ADC_CHANNELS; ++n){
        msg[3] = n + '0';
        USB_sendstr(msg);
        printu(getADCval(n));
        newline();
    }
}

static inline void printmcut(){
    USB_sendstr("TMCU=");
    int32_t T = getMCUtemp();
    if(T < 0){
        USB_putbyte('-');
        T = -T;
    }
    printu(T);
    newline();
}

static inline void showUIvals(){
    uint16_t *vals = getUval();
    USB_sendstr("V12="); printu(vals[0]);
    USB_sendstr("\nV5="); printu(vals[1]);
    USB_sendstr("\nV33="); printu(vals[3]);
    USB_sendstr("\nI12="); printu(vals[2]);
    newline();
}

static inline void setCANbrate(char *str){
    if(!str || !*str) return;
    uint32_t spd = 0;
    str = omit_spaces(str);
    char *e = getnum(str, &spd);
    if(e == str){
        USB_sendstr("BAUDRATE=");
        printu(curcanspeed);
        newline();
        return;
    }
    if(spd < CAN_SPEED_MIN || spd > CAN_SPEED_MAX){
        USB_sendstr("Wrong speed\n");
        return;
    }
    CAN_setup(spd);
    USB_sendstr("OK\n");
}

// parse `txt` to CAN_message
static CAN_message *parseCANmsg(char *txt){
    static CAN_message canmsg;
    uint32_t N;
    char *n;
    int ctr = -1;
    canmsg.ID = 0xffff;
    do{
        txt = omit_spaces(txt);
        n = getnum(txt, &N);
        if(txt == n) break;
        txt = n;
        if(ctr == -1){
            if(N > 0x7ff){
                USB_sendstr("ID should be 11-bit number!\n");
                return NULL;
            }
            canmsg.ID = (uint16_t)(N&0x7ff);
            ctr = 0;
            continue;
        }
        if(ctr > 7){
            USB_sendstr("ONLY 8 data bytes allowed!\n");
            return NULL;
        }
        if(N > 0xff){
            USB_sendstr("Every data portion is a byte!\n");
            return NULL;
        }
        canmsg.data[ctr++] = (uint8_t)(N&0xff);
    }while(1);
    if(canmsg.ID == 0xffff){
        USB_sendstr("NO ID given, send nothing!\n");
        return NULL;
    }
    USB_sendstr("Message parsed OK\n");
    canmsg.length = (uint8_t) ctr;
    return &canmsg;
}

// send command, format: ID (hex/bin/dec) data bytes (up to 8 bytes, space-delimeted)
static void sendCANcommand(char *txt){
    CAN_message *msg = parseCANmsg(txt);
    if(!msg) return;
    uint32_t N = 1000;
    while(CAN_BUSY == can_send(msg->data, msg->length, msg->ID)){
        if(--N == 0) break;
    }
}

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 * @param isUSB - == 1 if data got from USB
 */
void cmd_parser(char *txt){
    int16_t L = strlen(txt), ID = 0;
    char _1st = txt[0];
    if(_1st >= '0' && _1st < '8'){ // send command to Nth controller, not broadcast
        if(L == 3){ // with '\n' at end!
            ID = CAN_ID_PREFIX | (_1st - '0');
            _1st = txt[1];
        }else{
            _1st = '?'; // show help
        }
    }
    switch(_1st){
        case '@':
            debugmode = !debugmode;
            USB_sendstr("DEBUG mode ");
            if(debugmode) USB_sendstr("ON");
            else USB_sendstr("OFF");
            newline();
        break;
        case 'A':
            CANsend(ID, CMD_SPEAK, _1st);
        break;
        case 'a':
            showADCvals();
        break;
        case 'B':
            CANsend(ID, CMD_DUMMY0, _1st);
        break;
        case 'b':
            setCANbrate(txt + 1);
        break;
        case 'c':
            showcoeffs();
        break;
        case 'D':
            CANsend(CAN_ID_PREFIX, CMD_DUMMY1, _1st);
        break;
        case 'd':
            USB_sendstr("CAN_ID=");
            printuhex(CANID);
            newline();
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
            USB_sendstr("Sniffer CAN mode\n");
            CAN_listenall();
        break;
        case 'H':
            CANsend(ID, CMD_HIGH_SPEED, _1st);
        break;
        case 'h':
            i2c_setup(HIGH_SPEED);
        break;
        case 'I':
            CANsend(ID, CMD_REINIT_SENSORS, _1st);
        break;
        case 'i':
            sensors_init();
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
        case 'N':
            CANsend(ID, CMD_GETBUILDNO, _1st);
        break;
        case 'O':
            noLED = 0;
            USB_sendstr("LED on\n");
        break;
        case 'o':
            noLED = 1;
            LED_off(LED0);
            LED_off(LED1);
            USB_sendstr("LED off\n");
        break;
        case 'P':
            CANsend(ID, CMD_PING, _1st);
        break;
        case 'Q':
            CANsend(ID, CMD_SYSTIME, _1st);
        break;
        case 'q':
            USB_sendstr("SYSTIME="); printu(Tms); newline();
        break;
        case 'R':
            CANsend(ID, CMD_REINIT_I2C, _1st);
        break;
        case 'r':
            i2c_setup(CURRENT_SPEED);
        break;
        case 'S':
            CANsend(ID, CMD_SHUTUP, _1st);
        break;
        case 's':
            sendCANcommand(txt+1);
        break;
        case 'T':
            CANsend(ID, CMD_START_MEASUREMENT, _1st);
        break;
        case 't':
            if(!sensors_scan_mode) sensors_start();
        break;
        case 'U':
            CANsend(ID, CMD_USBSTATUS, _1st);
        break;
        case 'u':
            USB_sendstr("Unique ID CAN mode\n");
            CAN_listenone();
        break;
        case 'V':
            CANsend(ID, CMD_LOWEST_SPEED, _1st);
        break;
        case 'v':
            i2c_setup(VERYLOW_SPEED);
        break;
        case 'X':
            CANsend(ID, CMD_START_SCAN, _1st);
        break;
        case 'x':
            sensors_scan_mode = 1;
        break;
        case 'Y':
            CANsend(ID, CMD_SENSORS_STATE, _1st);
        break;
        case 'y':
            USB_sendstr("SSTATE=");
            USB_sendstr(sensors_get_statename(Sstate));
            USB_sendstr("\nNSENS=");
            printu(Nsens_present);
            USB_sendstr("\nSENSPRESENT=");
            printuhex(sens_present[0] | (sens_present[1]<<8));
            USB_sendstr("\nNTEMP=");
            printu(Ntemp_measured);
            newline();
        break;
        case 'z':
            USB_sendstr("CANERROR=");
            if(canerror){
                canerror = 0;
                USB_putbyte('1');
            }else USB_putbyte('0');
            newline();
        break;
        default: // help
            USB_sendstr("https://github.com/eddyem/tsys01/tree/master/STM32/TSYS_controller " RLSDBG " build #" BUILD_NUMBER " @ " BUILD_DATE "\n");
            USB_sendstr(
            "ALL little letters - without CAN messaging\n"
            "0..7 - send command to given controller (0 - master) instead of broadcast\n"
            "@ - set/reset debug mode\n"
            "a - get raw ADC values\n"
            "A - allow given node to speak\n"
            "B - send CAN dummy message\n"
            "b - get/set CAN bus baudrate\n"
            "c - show coefficients (current)\n"
            "d - get last CAN address\n"
            "D - send CAN dummy message to master\n"
            "Ee- end themperature scan\n"
            "Ff- turn oFf sensors\n"
            "g - sniffer CAN mode\n"
            "Hh- high I2C speed\n"
            "Ii- (re)init sensors\n"
            "Jj- get MCU temperature\n"
            "Kk- get U/I values\n"
            "Ll- low I2C speed\n"
            "N - get build number\n"
            "Oo- turn onboard diagnostic LEDs *O*n or *o*ff (both commands are local)\n"
            "P - ping given node\n"
            "Qq- get system time\n"
            "Rr- reinit I2C\n"
            "s - send CAN message\n"
            "S - shut up given node\n"
            "Tt- start temperature measurement\n"
            "U - USB status of given slave (0 - off)\n"
            "u - unique ID (default) CAN mode (only for slaves)\n"
            "Vv- very low I2C speed\n"
            "Xx- Start themperature scan\n"
            "Yy- get sensors' state\n"
            "z - check CAN status for errors\n"
            );
        break;
    }
}

// show message in debug mode
void mesg(const char *txt){
    if(!debugmode) return;
    USB_sendstr("[DBG] ");
    USB_sendstr(txt);
    newline();
}
