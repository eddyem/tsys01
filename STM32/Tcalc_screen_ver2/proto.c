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
#include "i2c.h"
#include "proto.h"
#include "ssd1306.h"
//#include "usart.h"
#include "usb.h"
#include <string.h> // strlen, strcpy(

static char buff[UARTBUFSZ+1], *bptr = buff;
static uint8_t blen = 0;
// LEDs are OFF by default
uint8_t noLED = 1;

void sendbuf(){
    IWDG->KR = IWDG_REFRESH;
    if(blen == 0) return;
    *bptr = 0;
    USB_send(buff);
    bptr = buff;
    blen = 0;
}

void addtobuf(const char *txt){
    IWDG->KR = IWDG_REFRESH;
    int l = strlen(txt);
    if(l > UARTBUFSZ){
        sendbuf();
        USB_send(txt);
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

/**
 * @brief cmd_parser - command parsing
 * @param txt   - buffer with commands & data
 */
void cmd_parser(char *txt){
    char _1st = txt[0];
    sendbuf();
    switch(_1st){
        case '0':
            LEDT_blink(0);
        break;
        case '1':
            LEDT_blink(1);
        break;
        case 'b':
            ssd1306_Fill(0);
            ssd1306_UpdateScreen();
            SEND("Clear screen\n");
        break;
        case 'c':
            ssd1306_Fill(0xff);
            ssd1306_UpdateScreen();
            SEND("Fill screen white\n");
        break;
        case 'D':
            if(write_i2c(TSYS01_ADDR0, TSYS01_RESET)) SEND("Found 0\n");
            if(write_i2c(TSYS01_ADDR1, TSYS01_RESET)) SEND("Found 1\n");
        break;
        case 'h':
            i2c_setup(HIGH_SPEED);
            SEND("Set I2C speed to high\n");
        break;
        case 'l':
            i2c_setup(LOW_SPEED);
            SEND("Set I2C speed to low\n");
        break;
        case 'r':
            i2c_setup(CURRENT_SPEED);
            SEND("Reinit I2C with current speed\n");
        break;
        case 'R': // 'R' - reset both
            SEND("Reset\n");
            write_i2c(TSYS01_ADDR0, TSYS01_RESET);
            write_i2c(TSYS01_ADDR1, TSYS01_RESET);
        break;
        case 'v':
            i2c_setup(VERYLOW_SPEED);
            SEND("Set I2C speed to very low\n");
        break;
        default: // help
            SEND(
            "Commands:\n"
            "0/1 - invert LEDT0/1\n"
            "b - clear screen\n"
            "c - fill screen white\n"
            "D - discovery sensors\n"
            "h - high I2C speed\n"
            "l - low I2C speed\n"
            "r - reinit I2C\n"
            "R - reset sensors\n"
            "v - very low I2C speed\n"
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
