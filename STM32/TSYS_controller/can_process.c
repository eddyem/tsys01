/*
 *                                                                                                  geany_encoding=koi8-r
 * can_process.c
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
#include "proto.h"
#include "sensors_manage.h"
#include "strfunc.h"
#include "usb.h"
#include "usb_lib.h" // usbON
#include "version.inc"

static CAN_status can_send_data(uint16_t targetID, uint8_t *data, uint8_t len);

extern volatile uint32_t Tms; // timestamp data
static uint16_t TprocID = 0xffff; // last ID asked for T


static inline void sendmcut(uint16_t targetID){
    uint8_t t[3];
    uint16_t T = getMCUtemp();
    t[0] = CMD_GETMCUTEMP; // command itself
    t[1] = (T >> 8) & 0xff; // H
    t[2] = T & 0xff;        // L
    can_send_data(targetID, t, 3);
}

static inline void senduival(uint16_t targetID){
    uint8_t buf[5];
    uint16_t *vals = getUval();
    buf[0] = CMD_GETUIVAL0; // V12 and V5
    buf[1] = vals[0] >> 8;  // H
    buf[2] = vals[0] & 0xff;// L
    buf[3] = vals[1] >> 8; // -//-
    buf[4] = vals[1] & 0xff;
    can_send_data(targetID, buf, 5);
    buf[0] = CMD_GETUIVAL1; // I12 and V3.3
    buf[1] = vals[2] >> 8;  // H
    buf[2] = vals[2] & 0xff;// L
    buf[3] = vals[3] >> 8; // -//-
    buf[4] = vals[3] & 0xff;
    can_send_data(targetID, buf, 5);
}

static inline void showui(char *v1, char *v2, uint8_t *data){
    char N = '0' + data[1];
    USB_sendstr(v1);
    USB_putbyte(N);
    USB_putbyte('=');
    uint16_t v = data[3]<<8 | data[4];
    printu(v);
    newline();
    USB_sendstr(v2);
    USB_putbyte(N);
    USB_putbyte('=');
    v = data[5]<<8 | data[6];
    printu(v);
}

void  can_messages_proc(){
    CAN_message *can_mesg = CAN_messagebuf_pop();
    if(!can_mesg) return; // no data in buffer
    uint8_t len = can_mesg->length;
    IWDG->KR = IWDG_REFRESH;
    mesg(u2str(can_mesg->ID));
#ifdef EBUG
    USB_sendstr("got message, len: "); USB_putbyte('0' + len);
    USB_sendstr(", data: ");
    for(uint8_t ctr = 0; ctr < len; ++ctr){
        printuhex(can_mesg->data[ctr]);
        USB_putbyte(' ');
    }
    newline();
#endif

    // show received message in sniffer mode
    if(cansniffer){
        mesg("SNIF:");
        printu(Tms);
        USB_sendstr(" #");
        printuhex(can_mesg->ID);
        for(uint8_t ctr = 0; ctr < len; ++ctr){
            USB_sendstr(" ");
            printuhex(can_mesg->data[ctr]);
        }
        newline();
    }
    if(can_mesg->ID != CANID) return; // don't process alien messages

    uint8_t *data = can_mesg->data, b[6]; // b - rest 6 bytes of data messages
    b[0] = data[2]; // command code
    int16_t t, targetID = CAN_ID_PREFIX | data[1];
    uint32_t U32;
    if(data[0] == COMMAND_MARK){   // process commands
#define OK()    do{b[0] = CMD_ANSOK; can_send_data(targetID, b, 1);}while(0)
        if(len < 3) return; // minimal command length: MARK-Number-CMDcode
        switch(data[2]){
            case CMD_DUMMY0:
            case CMD_DUMMY1:
                USB_sendstr("DUMMY");
                USB_putbyte('0' + (data[1]==CMD_DUMMY0 ? 0 : 1));
                newline();
            break;
            case CMD_PING: // pong
                USB_sendstr("PING\n");
                can_send_data(targetID, b, 1);
            break;
            case CMD_SENSORS_STATE:
                b[1] = Sstate;
                b[2] = sens_present[0];
                b[3] = sens_present[1];
                b[4] = Nsens_present;
                b[5] = Ntemp_measured;
                can_send_data(targetID, b, 6);
            break;
            case CMD_START_MEASUREMENT:
                TprocID = targetID;
                sensors_start();
                OK();
            break;
            case CMD_START_SCAN:
                TprocID = targetID;
                sensors_scan_mode = 1;
                OK();
            break;
            case CMD_STOP_SCAN:
                sensors_scan_mode = 0;
                OK();
            break;
            case CMD_SENSORS_OFF:
                sensors_off();
                OK();
            break;
            case CMD_LOWEST_SPEED:
                i2c_setup(VERYLOW_SPEED);
                OK();
            break;
            case CMD_LOW_SPEED:
                i2c_setup(LOW_SPEED);
                OK();
            break;
            case CMD_HIGH_SPEED:
                i2c_setup(HIGH_SPEED);
                OK();
            break;
            case CMD_REINIT_I2C:
                i2c_setup(CURRENT_SPEED);
                OK();
            break;
            case CMD_GETMCUTEMP:
                sendmcut(targetID);
            break;
            case CMD_GETUIVAL:
                senduival(targetID);
            break;
            case CMD_REINIT_SENSORS:
                sensors_init();
                OK();
            break;
            case CMD_GETBUILDNO:
                b[1] = 0;
                *((uint32_t*)&b[2]) = BUILDNO;
                can_send_data(targetID, b, 6);
            break;
            case CMD_SYSTIME:
                b[1] = 0;
                *((uint32_t*)&b[2]) = Tms;
                can_send_data(targetID, b, 6);
            break;
            case CMD_USBSTATUS:
                b[1] = usbON;
                can_send_data(targetID, b, 2);
            break;
            case CMD_SHUTUP:
                OK();
                CANshutup = 1;
            break;
            case CMD_SPEAK:
                CANshutup = 0;
                OK();
            break;
        }
#undef OK
    }else if(data[0] == DATA_MARK){ // process received data
        char Ns = '0' + data[1]; // controller number
        if(len < 3) return; // no data in packet (even command code)
        switch(data[2]){
            case CMD_PING:
                USB_sendstr("PONG");
                USB_putbyte(Ns);
            break;
            case CMD_SENSORS_STATE:
                USB_sendstr("SSTATE");
                USB_putbyte(Ns);
                USB_putbyte('=');
                USB_sendstr(sensors_get_statename(data[3]));
                USB_sendstr("\nNSENS");
                USB_putbyte(Ns);
                USB_putbyte('=');
                printu(data[6]);
                USB_sendstr("\nSENSPRESENT");
                USB_putbyte(Ns);
                USB_putbyte('=');
                printuhex(data[4] | (data[5]<<8));
                USB_sendstr("\nNTEMP");
                USB_putbyte(Ns);
                USB_putbyte('=');
                printu(data[7]);
            break;
            case CMD_START_MEASUREMENT: // temperature
                if(len != 6) return;
                USB_putbyte('T');
                USB_putbyte(Ns);
                USB_putbyte('_');
                printu(data[3]);
                USB_putbyte('=');
                t = data[4]<<8 | data[5];
                if(t < 0){
                    t = -t;
                    USB_putbyte('-');
                }
                printu(t);
            break;
            case CMD_GETMCUTEMP:
                USB_sendstr("TMCU");
                USB_putbyte(Ns);
                USB_putbyte('=');
                t = data[3]<<8 | data[4];
                if(t < 0){
                    USB_putbyte('-');
                    t = -t;
                }
                printu(t);
            break;
            case CMD_GETUIVAL0: // V12 and V5
                showui("V12_", "V5_", data);
            break;
            case CMD_GETUIVAL1: // I12 and V3.3
                showui("I12_", "V33_", data);
            break;
            case CMD_GETBUILDNO:
                USB_sendstr("BUILDNO");
                USB_putbyte(Ns);
                USB_putbyte('=');
                U32 = *((uint32_t*)&data[4]);
                printu(U32);
            break;
            case CMD_SYSTIME:
                USB_sendstr("SYSTIME");
                USB_putbyte(Ns);
                USB_putbyte('=');
                U32 = *((uint32_t*)&data[4]);
                printu(U32);
            break;
            case CMD_USBSTATUS:
                USB_sendstr("USB");
                USB_putbyte(Ns);
                USB_putbyte('=');
                printu(data[3]);
            break;
                ;
            case CMD_ANSOK:
                USB_sendstr("OK");
                USB_putbyte(Ns);
            break;
            default:
                USB_sendstr("UNKNOWN_DATA");
        }
        newline();
    }
}

// try to send messages, wait no more than 100ms
static CAN_status try2send(uint8_t *buf, uint8_t len, uint16_t id){
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == can_send(buf, len, id)) return CAN_OK;
        IWDG->KR = IWDG_REFRESH;
    }
    USB_sendstr("CAN_BUSY\n");
    return CAN_BUSY;
}

/**
 * Send command over CAN bus (works only if controller number is 0 - master mode)
 * @param targetID - target identifier
 * @param cmd - command to send
 */
CAN_status can_send_cmd(uint16_t targetID, uint8_t cmd){
    //if(Controller_address != 0 && cmd != CMD_DUMMY0 && cmd != CMD_DUMMY1) return CAN_NOTMASTER;
    uint8_t buf[3];
    buf[0] = COMMAND_MARK;
    buf[1] = Controller_address;
    buf[2] = cmd;
    return try2send(buf, 3, targetID);
}

// send data over CAN bus with MY ID (not more than 6 bytes)
static CAN_status can_send_data(uint16_t targetID, uint8_t *data, uint8_t len){
    if(len > 6) return CAN_OK;
    uint8_t buf[8];
    buf[0] = DATA_MARK;
    buf[1] = Controller_address;
    int i;
    for(i = 0; i < len; ++i) buf[i+2] = *data++;
    return try2send(buf, len+2, targetID);
}

/**
 * send temperature data over CAN bus once per call
 * @return next number or -1 if all data sent
 */
int8_t send_temperatures(int8_t N){
    if(N < 0){ TprocID = 0xffff; return -1; } // data sent
    if(TprocID == 0xffff) return -1; // there was no CAN requests
    int a, p;
    uint8_t can_data[4];
    int8_t retn = N;
    can_data[0] = CMD_START_MEASUREMENT;
    a = N / 10;
    p = N - a*10;
    if(p == 2){ // next sensor
        if(++a > MUL_MAX_ADDRESS){
            if(!sensors_scan_mode) TprocID = 0xffff; // forget target ID
            return -1;
        }
        p = 0;
    }
    do{
        if(!(sens_present[p] & (1<<a))){
            if(p == 0) p = 1;
            else{
                p = 0;
                ++a;
            }
        } else break;
    }while(a <= MUL_MAX_ADDRESS);
    if(a > MUL_MAX_ADDRESS){
        if(!sensors_scan_mode) TprocID = 0xffff;
        return -1; // done
    }
    retn = a*10 + p; // current temperature sensor number
    can_data[1] = a*10 + p;
    //char b[] = {'T', a+'0', p+'0', '=', '+'};
    int16_t t = Temperatures[a][p];
    if(t == BAD_TEMPERATURE || t == NO_SENSOR){ // don't send data if it's absent on current measurement
        ++retn;
    }else{
        can_data[2] = t>>8;   // H byte
        can_data[3] = t&0xff; // L byte
        if(CAN_OK == can_send_data(TprocID, can_data, 4)){ // OK, calculate next address
            ++retn;
        }
    }
    return retn;
}
