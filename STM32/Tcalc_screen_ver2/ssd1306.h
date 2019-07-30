/*
 *                                                                                                  geany_encoding=koi8-r
 * ssd1306.h
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
#pragma once
#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "ssd1306_fonts.h"

// SSD1306 OLED height in pixels
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT          64
#endif

// SSD1306 width in pixels
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH           128
#endif

void ssd1306_process();
uint8_t ssd1306_UpdateScreen();
void ssd1306_Reset();

void ssd1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
char ssd1306_WriteChar(char ch, FontDef Font, uint8_t color);
void ssd1306_Fill(uint8_t colr);

char ssd1306_WriteString(char* str, FontDef Font, uint8_t color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);

#endif // __SSD1306_H__
