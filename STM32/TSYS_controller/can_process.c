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
#include "can_process.h"
#include "sensors_manage.h"
#include "can.h"
#include "usart.h"

extern volatile uint32_t Tms; // timestamp data

void can_messages_proc(){
    CAN_message *can_mesg = CAN_messagebuf_pop();
    if(!can_mesg) return; // no data in buffer
    uint8_t len = can_mesg->length;
#ifdef EBUG
    SEND("got message, len: "); usart_putchar('0' + len);
    SEND(", data: ");
    uint8_t ctr;
    for(ctr = 0; ctr < len; ++ctr){
        printuhex(can_mesg->data[ctr]);
        usart_putchar(' ');
    }
    newline();
#endif
    uint8_t *data = can_mesg->data, b[2];
    b[0] = data[1];
    if(data[0] == COMMAND_MARK){   // process commands
        if(len < 2) return;
        switch(data[1]){
            case CMD_DUMMY0:
            case CMD_DUMMY1:
                SEND("DUMMY");
                usart_putchar('0' + (data[1]==CMD_DUMMY0 ? 0 : 1));
                newline();
            break;
            case CMD_PING: // pong
                can_send_data(b, 1);
            break;
            case CMD_SENSORS_STATE:
                b[1] = sensors_get_state();
                can_send_data(b, 2);
            break;
            case CMD_START_MEASUREMENT:
                sensors_start();
            break;
            case CMD_START_SCAN:
                sensors_scan_mode = 1;
            break;
            case CMD_STOP_SCAN:
                sensors_scan_mode = 0;
            break;
            case CMD_SENSORS_OFF:
                sensors_off();
            break;
            case CMD_LOWEST_SPEED:
                i2c_setup(VERYLOW_SPEED);
            break;
            case CMD_LOW_SPEED:
                i2c_setup(LOW_SPEED);
            break;
            case CMD_HIGH_SPEED:
                i2c_setup(HIGH_SPEED);
            break;
            case CMD_REINIT_I2C:
                i2c_setup(CURRENT_SPEED);
            break;
        }
    }else if(data[0] == DATA_MARK){ // process received data
        if(len < 3) return;
        switch(data[2]){
            case CMD_PING:
                SEND("PONG");
                usart_putchar('0' + data[1]);
            break;
            case CMD_SENSORS_STATE:
                SEND("SSTATE");
                usart_putchar('0' + data[1]);
                usart_putchar('=');
                printu(data[3]);
            break;
            case CMD_START_MEASUREMENT: // temperature
                if(len != 6) return;
                usart_putchar('T');
                usart_putchar('0' + data[1]);
                usart_putchar('_');
                printu(data[3]);
                usart_putchar('=');
                int16_t t = data[4]<<8 | data[5];
                if(t < 0){
                    t = -t;
                    usart_putchar('-');
                }
                printu(t);
#pragma message("TODO: process received T over USB!")
            break;
            default:
                SEND("Unknown data received");
        }
        newline();
    }
}

// try to send messages, wait no more than 100ms
static CAN_status try2send(uint8_t *buf, uint8_t len, uint16_t id){
    uint32_t Tstart = Tms;
    while(Tms - Tstart < SEND_TIMEOUT_MS){
        if(CAN_OK == can_send(buf, len, id)) return CAN_OK;
    }
    SEND("Bus busy!\n");
    return CAN_BUSY;
}


/**
 * Send command over CAN bus (works only if controller number is 0 - master mode)
 * @param targetID - target identifier
 * @param cmd - command to send
 */
CAN_status can_send_cmd(uint16_t targetID, uint8_t cmd){
    if(Controller_address != 0 && cmd != CMD_DUMMY0 && cmd != CMD_DUMMY1) return CAN_NOTMASTER;
    uint8_t buf[2];
    buf[0] = COMMAND_MARK;
    buf[1] = cmd;
    return try2send(buf, 2, targetID);
}

// send data over CAN bus to MASTER_ID (not more than 6 bytes)
CAN_status can_send_data(uint8_t *data, uint8_t len){
    if(len > 6) return CAN_OK;
    uint8_t buf[8];
    buf[0] = DATA_MARK;
    buf[1] = Controller_address;
    int i;
    for(i = 0; i < len; ++i) buf[i+2] = *data++;
    return try2send(buf, len+2, MASTER_ID);
}

/**
 * send temperature data over CAN bus once per call
 * @return next number or -1 if all data sent
 */
int8_t send_temperatures(int8_t N){
    if(N < 0 || Controller_address == 0) return -1;
    int a, p;
    uint8_t can_data[4];
    int8_t retn = N;
    can_data[0] = CMD_START_MEASUREMENT;
    a = N / 10;
    p = N - a*10;
    if(p == 2){ // next sensor
        if(++a > MUL_MAX_ADDRESS) return -1;
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
    if(a > MUL_MAX_ADDRESS) return -1; // done
    retn = a*10 + p; // current temperature sensor number
    can_data[1] = a*10 + p;
    //char b[] = {'T', a+'0', p+'0', '=', '+'};
    int16_t t = Temperatures[a][p];
    if(t == BAD_TEMPERATURE || t == NO_SENSOR){ // don't send data if it's absent on current measurement
        ++retn;
    }else{
        can_data[2] = t>>8;   // H byte
        can_data[3] = t&0xff; // L byte
        if(CAN_OK == can_send_data(can_data, 4)){ // OK, calculate next address
            ++retn;
        }
    }
    return retn;
}
