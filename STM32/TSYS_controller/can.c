/*
 *                                                                                                  geany_encoding=koi8-r
 * can.c
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
#include "can.h"

static uint8_t CAN_addr = 0;

// get CAN address data from GPIO pins
void readCANaddr(){
    CAN_addr = READ_CAN_INV_ADDR();
    CAN_addr = ~CAN_addr & 0x7;
}

uint8_t getCANaddr(){
    return CAN_addr;
}
