/*
 *                                                                                                  geany_encoding=koi8-r
 * ssd1306.c
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
#include "spi.h"
#include "ssd1306.h"
//#include "usart.h"
#include "proto.h"
#include <string.h> // memset

#define RST_PAUSE       (10)
#define BOOT_PAUSE      (100)

extern volatile uint32_t Tms;
static uint32_t Tloc;
static uint8_t curx, cury;

typedef enum{
    ST_UNINITIALIZED,
    ST_RSTSTART,
    ST_RESETED,
    ST_NEED4UPDATE,
    ST_IDLE
} SSD1306_STATE;

static SSD1306_STATE state = ST_UNINITIALIZED;

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];


static void ssd1306_WriteCommand(uint8_t byte){
    CS_LO(); DC_LO();
    spi_write_byte(byte);
    CS_HI();
}

static void ssd1306_WriteData(uint8_t* buffer, uint16_t len){
    CS_LO(); DC_HI();
    spiWrite(buffer, len);
    CS_HI();
}

// colr == 0 for black or 0xff for white or other values for vertical patterns
void ssd1306_Fill(uint8_t colr){
    memset(SSD1306_Buffer, colr, sizeof(SSD1306_Buffer));
    curx = 0; cury = 0;
}

// Write ful; screenbuffer to the screen
static void ssd1306_UpdateScreen_(){
    uint8_t i;
    for(i = 0; i < SSD1306_HEIGHT/8; i++){
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i], SSD1306_WIDTH);
    }
}

// change status to need4update, return 1 if can't
uint8_t ssd1306_UpdateScreen(){
    if(state != ST_IDLE) return 1;
    state = ST_NEED4UPDATE;
    return 0;
}

static void ssd1306_init(){
    const uint8_t cmds[] = {
        0xAE,   // display off
        0x20,   // Set memory address
        0x10,   // 0x00: horizontal addressing mode, 0x01: vertical addressing mode
        // 0x10: Page addressing mode(RESET), 0x11: invalid
        0xB0,   // Set page start address for page addressing mode: 0 ~ 7
        0xC8,   // Set COM output scan direction
        0x00,   // Set low column address
        0x10,   // Set height column address
        0x40,   // Set start line address
        0x81,   // Set contrast control register
        0xFF,
        0xA1,   // Set segment re-map 0 to 127
        0xA6,   // Set normal display
        0xA8,   // Set multiplex ratio(1 to 64)
        0x3F,
        0xA4,   // 0xa4: ouput follows RAM content, 0xa5: ouput ignores RAM content
        0xD3,   // Set display offset
        0x00,   // Not offset
        0xD5,   // Set display clock divide ratio/oscillator frequency
        0xF0,   // Set divide ration
        0xD9,   // Set pre-charge period
        0x22,
        0xDA,   // Set COM pins hardware configuration
        0x12,
        0xDB,   // Set VCOMH
        0x20,   // 0x20: 0.77*Vcc
        0x8D,   // Set DC-DC enable
        0x14,
        0xAF,   // turn on SSD1306panel
    };
    uint8_t idx;
    for (idx = 0; idx < sizeof(cmds); ++idx){
        ssd1306_WriteCommand(cmds[idx]);
    }
}

void ssd1306_Reset(){
    state = ST_UNINITIALIZED;
}

void ssd1306_process(){
    switch (state){
        case ST_UNINITIALIZED: // reset screen
            //SEND("ST_UNINITIALIZED\n");
            state = ST_RSTSTART;
            Tloc = Tms;
            // high CS, low RST
            CS_HI(); RST_LO();
        break;
        case ST_RSTSTART: // reset procedure is over
            //SEND("ST_RSTSTART\n");
            if(Tms - Tloc > RST_PAUSE){
                state = ST_RESETED;
                RST_HI();
                Tloc = Tms;
            }
        break;
        case ST_RESETED: // initialize screen
            //SEND("ST_RESETED\n");
            if(Tms - Tloc > BOOT_PAUSE){
                ssd1306_init();
                state = ST_IDLE;
            }
        break;
        case ST_NEED4UPDATE:
            //SEND("ST_NEED4UPDATE\n");
            ssd1306_UpdateScreen_();
            state = ST_IDLE;
        break;
        case ST_IDLE:
        default:
        break;
    }
    ;
}

/**
 * Draw one pixel in the screenbuffer
 * @parameter x, y - coordinate (Y from top to bottom)
 * @parameter color == 0 for black
 */
void ssd1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color){
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    // Check if pixel should be inverted
    if(color){
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    }else{
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/**
 * Draw 1 char to the screen buffer
 * @parameter ch - given char
 * @parameter Font - selected font
 * @parameter color == 0 for black
 * @return 1 if all OK
 */
char ssd1306_WriteChar(char ch, FontDef Font, uint8_t color){
    uint32_t i, b, j;
    // Check remaining space on current line
    if(SSD1306_WIDTH <= (curx + Font.FontWidth)){
        if(SSD1306_HEIGHT <= (cury + 2*Font.FontHeight)) return 0;
        cury += Font.FontHeight;
        curx = 0;
    }
    if(SSD1306_HEIGHT <= (cury + Font.FontHeight)) return 0;
    if(ch == '\n'){
        cury += Font.FontHeight;
        curx = 0;
    }
    if(ch < ' ' || ch > '~') return 0; // wrong symbol
    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++){
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(curx + j, (cury + i), color);
            } else {
                ssd1306_DrawPixel(curx + j, (cury + i), !color);
            }
        }
    }
    curx += Font.FontWidth;
    return ch;
}

// Write full string to screenbuffer
char ssd1306_WriteString(char* str, FontDef Font, uint8_t color){
    // Write until null-byte
    while (*str){
        if(!ssd1306_WriteChar(*str, Font, color)){
            // Char could not be written
            return *str;
        }
        // Next char
        str++;
    }
    // Everything ok - return 0
    return 0;
}

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y){
    if(x >= SSD1306_WIDTH || y > SSD1306_HEIGHT) return;
    curx = x;
    cury = y;
}
