/*us
 * usart.c
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
#include "stm32f0.h"
#include "usart.h"
#include <string.h>

extern volatile uint32_t Tms;
static int datalen[2] = {0,0}; // received data line length (including '\n')

int linerdy = 0,        // received data ready
    dlen = 0,           // length of data (including '\n') in current buffer
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;


int rbufno = 0; // current rbuf number
static char rbuf[UARTBUFSZ][2], tbuf[UARTBUFSZ]; // receive & transmit buffers
static char *recvdata = NULL;

/**
 * return length of received data (without trailing zero
 */
int usart2_getline(char **line){
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    *line = recvdata;
    linerdy = 0;
    return dlen;
}

TXstatus usart2_send(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    if(len > UARTBUFSZ) return STR_TOO_LONG;
    txrdy = 0;
    memcpy(tbuf, str, len);
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CNDTR = len;
    DMA1_Channel4->CCR |= DMA_CCR_EN; // start transmission
    return ALL_OK;
}

TXstatus usart2_send_blocking(const char *str, int len){
    if(!txrdy) return LINE_BUSY;
    int i;
    bufovr = 0;
    for(i = 0; i < len; ++i){
        USART2->TDR = *str++;
        while(!(USART2->ISR & USART_ISR_TXE));
    }
    txrdy = 1;
    return ALL_OK;
}


// Nucleo's USART2 connected to VCP proxy of st-link
void usart2_setup(){
    // setup pins: PA2 (Tx - AF1), PA15 (Rx - AF1)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;
    // AF mode (AF1)
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER2|GPIO_MODER_MODER15))\
                | (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER15_1);
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~GPIO_AFRH_AFRH2) | 1 << (2 * 4); // PA2
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~GPIO_AFRH_AFRH7) | 1 << (7 * 4); // PA15
    // DMA: Tx - Ch4
    DMA1_Channel4->CPAR = (uint32_t) &USART2->TDR; // periph
    DMA1_Channel4->CMAR = (uint32_t) tbuf; // mem
    DMA1_Channel4->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    // Tx CNDTR set @ each transmission due to data size
    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
    NVIC_SetPriority(USART2_IRQn, 0);
    // setup usart2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // clock
    // oversampling by16, 115200bps (fck=48mHz)
    //USART2_BRR = 0x1a1; // 48000000 / 115200
    USART2->BRR = 480000 / 1152;
    USART2->CR3 = USART_CR3_DMAT; // enable DMA Tx
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // 1start,8data,nstop; enable Rx,Tx,USART
    while(!(USART2->ISR & USART_ISR_TC)); // polling idle frame Transmission
    USART2->ICR |= USART_ICR_TCCF; // clear TC flag
    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART2_IRQn);
}


void dma1_channel4_5_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF4){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF4; // clear TC flag
        txrdy = 1;
    }
}

void usart2_isr(){
    #ifdef CHECK_TMOUT
    static uint32_t tmout = 0;
    #endif
    if(USART2->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout && Tms >= tmout){ // set overflow flag
            bufovr = 1;
            datalen[rbufno] = 0;
        }
        tmout = Tms + TIMEOUT_MS;
        if(!tmout) tmout = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USART2->RDR;
        if(datalen[rbufno] < UARTBUFSZ){ // put next char into buf
            rbuf[rbufno][datalen[rbufno]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy = 1;
                dlen = datalen[rbufno];
                recvdata = rbuf[rbufno];
                // prepare other buffer
                rbufno = !rbufno;
                datalen[rbufno] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr = 1;
            datalen[rbufno] = 0;
            #ifdef CHECK_TMOUT
            tmout = 0;
            #endif
        }
    }
}
