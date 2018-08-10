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
#include "hardware.h"
#include "usart.h"
#include "can.h"
#include "can_process.h"
#include "sensors_manage.h"

extern volatile uint8_t canerror;

static void CANsend(uint16_t targetID, uint8_t cmd, char echo){
    if(CAN_OK == can_send_cmd(targetID, cmd)){
        usart_putchar(echo);
        newline();
    }
}

void cmd_parser(){
    char *txt = NULL;
    int16_t L = 0, ID = BCAST_ID;
    L = usart_getline(&txt);
    char _1st = txt[0];
    if(_1st >= '0' && _1st < '8'){ // send command to Nth controller, not broadcast
        if(L == 3){ // with '\n' at end!
            if(_1st == '0'){
                usart_putchar(txt[1]);
                _1st = txt[1] + 'a' - 'A'; // change network command to local
                newline();
            }else{
                ID = (CAN_ID_PREFIX & CAN_ID_MASK) | (_1st - '0');
                _1st = txt[1];
            }
        }else{
            _1st = '?'; // show help
        }
    }else if(L != 2) _1st = '?';
    switch(_1st){
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
        case 'L':
            CANsend(ID, CMD_LOW_SPEED, _1st);
        break;
        case 'l':
            i2c_setup(LOW_SPEED);
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
        case 'u':
            SEND("CANERROR=");
            if(canerror){
                canerror = 0;
                usart_putchar('1');
            }else usart_putchar('0');
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
            usart_putchar(sensors_get_state());
            newline();
        break;
        default: // help
            SEND(
            "ALL little letters - without CAN messaging\n"
            "0..7 - send command to given controller (0 - this) instead of broadcast\n"
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
