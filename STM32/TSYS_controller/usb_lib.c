/*
 * Copyright 2024 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include "usb.h"
#include "usb_lib.h"
#include "usbhw.h"

ep_t endpoints[STM32ENDPOINTS];

static uint16_t USB_Addr = 0;
static usb_LineCoding lineCoding = {115200, 0, 0, 8};
uint8_t ep0databuf[EP0DATABUF_SIZE], setupdatabuf[EP0DATABUF_SIZE];
config_pack_t *setup_packet = (config_pack_t*) setupdatabuf;

usb_LineCoding getLineCoding(){return lineCoding;}

volatile uint8_t usbON = 0; // device disconnected from terminal

// definition of parts common for USB_DeviceDescriptor & USB_DeviceQualifierDescriptor
#define bcdUSB_L        0x10
#define bcdUSB_H        0x01
#define bDeviceClass    0
#define bDeviceSubClass 0
#define bDeviceProtocol 0
#define bNumConfigurations 1

static const uint8_t USB_DeviceDescriptor[] = {
        18,     // bLength
        0x01,   // bDescriptorType - Device descriptor
        bcdUSB_L,   // bcdUSB_L - 1.10
        bcdUSB_H,   // bcdUSB_H
        bDeviceClass,   // bDeviceClass - USB_COMM
        bDeviceSubClass,   // bDeviceSubClass
        bDeviceProtocol,   // bDeviceProtocol
        USB_EP0_BUFSZ,   // bMaxPacketSize
        0x7b,   // idVendor_L PL2303: VID=0x067b, PID=0x2303
        0x06,   // idVendor_H
        0x03,   // idProduct_L
        0x23,   // idProduct_H
        0x00,   // bcdDevice_Ver_L
        0x03,   // bcdDevice_Ver_H
        iMANUFACTURER_DESCR,   // iManufacturer
        iPRODUCT_DESCR,   // iProduct
        iSERIAL_DESCR,   // iSerialNumber
        bNumConfigurations    // bNumConfigurations
};

static const uint8_t USB_DeviceQualifierDescriptor[] = {
        10,     //bLength
        0x06,   // bDescriptorType - Device qualifier
        bcdUSB_L,   // bcdUSB_L
        bcdUSB_H,   // bcdUSB_H
        bDeviceClass,   // bDeviceClass
        bDeviceSubClass,   // bDeviceSubClass
        bDeviceProtocol,   // bDeviceProtocol
        USB_EP0_BUFSZ,   // bMaxPacketSize0
        bNumConfigurations,   // bNumConfigurations
        0x00    // Reserved
};

static const uint8_t USB_ConfigDescriptor[] = {
        /*Configuration Descriptor*/
        0x09, /* bLength: Configuration Descriptor size */
        0x02, /* bDescriptorType: Configuration */
        39,   /* wTotalLength:no of returned bytes */
        0x00,
        0x01, /* bNumInterfaces: 1 interface */
        0x01, /* bConfigurationValue: Configuration value */
        0x00, /* iConfiguration: Index of string descriptor describing the configuration */
        0xa0, /* bmAttributes - Bus powered, Remote wakeup */
        0x32, /* MaxPower 100 mA */

        /*---------------------------------------------------------------------------*/

        /*Interface Descriptor */
        0x09, /* bLength: Interface Descriptor size */
        0x04, /* bDescriptorType: Interface */
        0x00, /* bInterfaceNumber: Number of Interface */
        0x00, /* bAlternateSetting: Alternate setting */
        0x03, /* bNumEndpoints: 3 endpoints used */
        0xff, /* bInterfaceClass */
        0x00, /* bInterfaceSubClass */
        0x00, /* bInterfaceProtocol */
        iINTERFACE_DESCR, /* iInterface: */
///////////////////////////////////////////////////
        /*Endpoint 1 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x81, /* bEndpointAddress IN1 */
        0x03, /* bmAttributes: Interrupt */
        0x0a, /* wMaxPacketSize LO: */
        0x00, /* wMaxPacketSize HI: */
        0x01, /* bInterval: */

        /*Endpoint OUT2 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x02, /* bEndpointAddress: OUT2 */
        0x02, /* bmAttributes: Bulk */
        (USB_RXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
        (USB_RXBUFSZ >> 8),
        0x00, /* bInterval: ignore for Bulk transfer */

        /*Endpoint IN3 Descriptor*/
        0x07, /* bLength: Endpoint Descriptor size */
        0x05, /* bDescriptorType: Endpoint */
        0x83, /* bEndpointAddress IN3 */
        0x02, /* bmAttributes: Bulk */
        (USB_TXBUFSZ & 0xff), /* wMaxPacketSize: 64 */
        (USB_TXBUFSZ >> 8),
        0x00, /* bInterval: ignore for Bulk transfer */
};

_USB_LANG_ID_(LD, LANG_US);
_USB_STRING_(SD, u"0.0.1");
_USB_STRING_(MD, u"Prolific Technology Inc.");
_USB_STRING_(PD, u"USB-Serial Controller");
_USB_STRING_(ID, u"tsyscontr");
static void const *StringDescriptor[iDESCR_AMOUNT] = {
    [iLANGUAGE_DESCR] = &LD,
    [iMANUFACTURER_DESCR] = &MD,
    [iPRODUCT_DESCR] = &PD,
    [iSERIAL_DESCR] = &SD,
    [iINTERFACE_DESCR] = &ID
};


/*
 * default handlers
 */
// SET_LINE_CODING
void WEAK linecoding_handler(usb_LineCoding __attribute__((unused)) *lc){
}

// SET_CONTROL_LINE_STATE
void WEAK clstate_handler(uint16_t __attribute__((unused)) val){
}

// SEND_BREAK
void WEAK break_handler(){
}

// handler of vendor requests
void WEAK vendor_handler(config_pack_t *packet){
    uint16_t c;
    if(packet->bmRequestType & 0x80){ // read
        switch(packet->wValue){
            case 0x8484:
                c = 2;
            break;
            case 0x0080:
                c = 1;
            break;
            case 0x8686:
                c = 0xaa;
            break;
            default:
                c = 0;
        }
        EP_WriteIRQ(0, (uint8_t*)&c, 1);
    }else{ // write ZLP
        c = 0;
        EP_WriteIRQ(0, (uint8_t *)&c, 0);
    }
}

static void wr0(const uint8_t *buf, uint16_t size){
    if(setup_packet->wLength < size) size = setup_packet->wLength; // shortened request
    if(size < endpoints[0].txbufsz){
        EP_WriteIRQ(0, buf, size);
        return;
    }
    while(size){
        uint16_t l = size;
        if(l > endpoints[0].txbufsz) l = endpoints[0].txbufsz;
        EP_WriteIRQ(0, buf, l);
        buf += l;
        size -= l;
        uint8_t needzlp = (l == endpoints[0].txbufsz) ? 1 : 0;
        if(size || needzlp){ // send last data buffer
            uint16_t status = KEEP_DTOG(USB->EPnR[0]);
            // keep DTOGs, clear CTR_RX,TX, set TX VALID, leave stat_Rx
            USB->EPnR[0] = (status & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX|USB_EPnR_STAT_RX))
                            ^ USB_EPnR_STAT_TX;
            uint32_t ctr = 1000000;
            while(--ctr && (USB->ISTR & USB_ISTR_CTR) == 0){IWDG->KR = IWDG_REFRESH;};
            if((USB->ISTR & USB_ISTR_CTR) == 0){
                return;
            }
            if(needzlp) EP_WriteIRQ(0, (uint8_t*)0, 0);
        }
    }
}

static inline void get_descriptor(){
    uint8_t descrtype = setup_packet->wValue >> 8,
            descridx = setup_packet->wValue & 0xff;
    switch(descrtype){
        case DEVICE_DESCRIPTOR:
            wr0(USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor));
        break;
        case CONFIGURATION_DESCRIPTOR:
            wr0(USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor));
        break;
        case STRING_DESCRIPTOR:
            if(descridx < iDESCR_AMOUNT) wr0((const uint8_t *)StringDescriptor[descridx], *((uint8_t*)StringDescriptor[descridx]));
            else EP_WriteIRQ(0, (uint8_t*)0, 0);
        break;
        case DEVICE_QUALIFIER_DESCRIPTOR:
            wr0(USB_DeviceQualifierDescriptor, USB_DeviceQualifierDescriptor[0]);
        break;
        default:
        break;
    }
}

static uint16_t configuration = 0; // reply for GET_CONFIGURATION (==1 if configured)
static inline void std_d2h_req(){
    uint16_t status = 0; // bus powered
    switch(setup_packet->bRequest){
        case GET_DESCRIPTOR:
            get_descriptor();
        break;
        case GET_STATUS:
            EP_WriteIRQ(0, (uint8_t *)&status, 2); // send status: Bus Powered
        break;
        case GET_CONFIGURATION:
            EP_WriteIRQ(0, (uint8_t*)&configuration, 1);
        break;
        default:
        break;
    }
}

// interrupt IN handler (never used?)
static void EP1_Handler(){
    uint16_t epstatus = KEEP_DTOG(USB->EPnR[1]);
    if(RX_FLAG(epstatus)) epstatus = (epstatus & ~USB_EPnR_STAT_TX) ^ USB_EPnR_STAT_RX; // set valid RX
    else epstatus = epstatus & ~(USB_EPnR_STAT_TX|USB_EPnR_STAT_RX);
    // clear CTR
    epstatus = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX));
    USB->EPnR[1] = epstatus;
}

// data IN/OUT handlers
static void transmit_Handler(){ // EP3IN
    uint16_t epstatus = KEEP_DTOG_STAT(USB->EPnR[3]);
    // clear CTR keep DTOGs & STATs
    USB->EPnR[3] = (epstatus & ~(USB_EPnR_CTR_TX)); // clear TX ctr
    send_next();
}

static uint8_t volatile rcvbuf[USB_RXBUFSZ];
static uint8_t volatile rcvbuflen = 0;

void chkin(){
    if(bufovrfl) return;
    if(!rcvbuflen) return;
    int w = RB_write((ringbuffer*)&rbin, (uint8_t*)rcvbuf, rcvbuflen);
    if(w < 0) return;
    if(w != rcvbuflen) bufovrfl = 1;
    rcvbuflen = 0;
    uint16_t status = KEEP_DTOG(USB->EPnR[2]); // don't change DTOG
    USB->EPnR[2] = status ^ USB_EPnR_STAT_RX;
}

// receiver reads data from local buffer and only then ACK'ed
static void receive_Handler(){ // EP2OUT
    uint16_t status = KEEP_DTOG_STAT(USB->EPnR[2]); // don't change DTOG and NACK
    if(rcvbuflen){
        bufovrfl = 1; // lost last data
        rcvbuflen = 0;
    }
    rcvbuflen = EP_Read(2, (uint8_t*)rcvbuf);
    USB->EPnR[2] = status & ~USB_EPnR_CTR_RX;
}

static inline void std_h2d_req(){
    switch(setup_packet->bRequest){
        case SET_ADDRESS:
            // new address will be assigned later - after  acknowlegement or request to host
            USB_Addr = setup_packet->wValue;
        break;
        case SET_CONFIGURATION:
            // Now device configured
            configuration = setup_packet->wValue;
            EP_Init(1, EP_TYPE_INTERRUPT, USB_EP1BUFSZ, 0, EP1_Handler); // IN1 - transmit
            EP_Init(2, EP_TYPE_BULK, 0, USB_RXBUFSZ, receive_Handler); // OUT2 - receive data
            EP_Init(3, EP_TYPE_BULK, USB_TXBUFSZ, 0, transmit_Handler); // IN3 - transmit data
        break;
        default:
        break;
    }
}

/*
bmRequestType: 76543210
7    direction: 0 - host->device, 1 - device->host
65   type: 0 - standard, 1 - class, 2 - vendor
4..0 getter: 0 - device, 1 - interface, 2 - endpoint, 3 - other
*/
/**
 * Endpoint0 (control) handler
 */
void EP0_Handler(){
    uint16_t epstatus = USB->EPnR[0]; // EP0R on input -> return this value after modifications
    uint8_t reqtype = setup_packet->bmRequestType & 0x7f;
    uint8_t dev2host = (setup_packet->bmRequestType & 0x80) ? 1 : 0;
    int rxflag = RX_FLAG(epstatus);
    if(rxflag && SETUP_FLAG(epstatus)){
        switch(reqtype){
            case STANDARD_DEVICE_REQUEST_TYPE: // standard device request
                if(dev2host){
                    std_d2h_req();
                }else{
                    std_h2d_req();
                    EP_WriteIRQ(0, (uint8_t *)0, 0);
                }
            break;
            case STANDARD_ENDPOINT_REQUEST_TYPE: // standard endpoint request
                if(setup_packet->bRequest == CLEAR_FEATURE){
                    EP_WriteIRQ(0, (uint8_t *)0, 0);
                }
            break;
            case VENDOR_REQUEST_TYPE:
                vendor_handler(setup_packet);
            break;
            case CONTROL_REQUEST_TYPE:
                switch(setup_packet->bRequest){
                    case GET_LINE_CODING:
                        EP_WriteIRQ(0, (uint8_t*)&lineCoding, sizeof(lineCoding));
                    break;
                    case SET_LINE_CODING: // omit this for next stage, when data will come
                    break;
                    case SET_CONTROL_LINE_STATE:
                        usbON = 1;
                        clstate_handler(setup_packet->wValue);
                    break;
                    case SEND_BREAK:
                        usbON = 0;
                        break_handler();
                    break;
                    default:
                    break;
                }
                if(setup_packet->bRequest != GET_LINE_CODING) EP_WriteIRQ(0, (uint8_t *)0, 0); // write acknowledgement
            break;
            default:
                EP_WriteIRQ(0, (uint8_t *)0, 0);
        }
    }else if(rxflag){ // got data over EP0 or host acknowlegement
        if(endpoints[0].rx_cnt){
            if(setup_packet->bRequest == SET_LINE_CODING){
                linecoding_handler((usb_LineCoding*)ep0databuf);
            }
        }
    } else if(TX_FLAG(epstatus)){ // package transmitted
        // now we can change address after enumeration
        if ((USB->DADDR & USB_DADDR_ADD) != USB_Addr){
            USB->DADDR = USB_DADDR_EF | USB_Addr;
            usbON = 0;
        }
    }
    epstatus = KEEP_DTOG(USB->EPnR[0]);
    if(rxflag) epstatus ^= USB_EPnR_STAT_TX; // start ZLP/data transmission
    else epstatus &= ~USB_EPnR_STAT_TX; // or leave unchanged
    // keep DTOGs, clear CTR_RX,TX, set RX VALID
    USB->EPnR[0] = (epstatus & ~(USB_EPnR_CTR_RX|USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_RX;
}

/**
 * Write data to EP buffer (called from IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_WriteIRQ(uint8_t number, const uint8_t *buf, uint16_t size){
    if(size > endpoints[number].txbufsz) size = endpoints[number].txbufsz;
    uint16_t N2 = (size + 1) >> 1;
    // the buffer is 16-bit, so we should copy data as it would be uint16_t
    uint16_t *buf16 = (uint16_t *)buf;
#if defined USB1_16
    // very bad: what if `size` is odd?
    uint32_t *out = (uint32_t *)endpoints[number].tx_buf;
    for(int i = 0; i < N2; ++i, ++out){
        *out = buf16[i];
    }
#elif defined USB2_16
    // use memcpy instead?
    for(int i = 0; i < N2; i++){
        endpoints[number].tx_buf[i] = buf16[i];
    }
#else
#error "Define USB1_16 or USB2_16"
#endif
    USB_BTABLE->EP[number].USB_COUNT_TX = size;
}

/**
 * Write data to EP buffer (called outside IRQ handler)
 * @param number - EP number
 * @param *buf - array with data
 * @param size - its size
 */
void EP_Write(uint8_t number, const uint8_t *buf, uint16_t size){
    EP_WriteIRQ(number, buf, size);
    uint16_t status = KEEP_DTOG(USB->EPnR[number]);
    // keep DTOGs, clear CTR_TX & set TX VALID to start transmission
    USB->EPnR[number] = (status & ~(USB_EPnR_CTR_TX)) ^ USB_EPnR_STAT_TX;
}

/*
 * Copy data from EP buffer into user buffer area
 * @param *buf - user array for data
 * @return amount of data read
 */
int EP_Read(uint8_t number, uint8_t *buf){
    int sz = endpoints[number].rx_cnt;
    if(!sz) return 0;
    endpoints[number].rx_cnt = 0;
#if defined USB1_16
    int n = (sz + 1) >> 1;
    uint32_t *in = (uint32_t*)endpoints[number].rx_buf;
    uint16_t *out = (uint16_t*)buf;
    for(int i = 0; i < n; ++i, ++in)
        out[i] = *(uint16_t*)in;
#elif defined USB2_16
    // use memcpy instead?
    for(int i = 0; i < sz; ++i)
        buf[i] = endpoints[number].rx_buf[i];
#else
#error "Define USB1_16 or USB2_16"
#endif
    return sz;
}


static uint16_t lastaddr = LASTADDR_DEFAULT;
/**
 * Endpoint initialisation
 * @param number - EP num (0...7)
 * @param type - EP type (EP_TYPE_BULK, EP_TYPE_CONTROL, EP_TYPE_ISO, EP_TYPE_INTERRUPT)
 * @param txsz - transmission buffer size @ USB/CAN buffer
 * @param rxsz - reception buffer size @ USB/CAN buffer
 * @param uint16_t (*func)(ep_t *ep) - EP handler function
 * @return 0 if all OK
 */
int EP_Init(uint8_t number, uint8_t type, uint16_t txsz, uint16_t rxsz, void (*func)(ep_t ep)){
    if(number >= STM32ENDPOINTS) return 4; // out of configured amount
    if(txsz > USB_BTABLE_SIZE || rxsz > USB_BTABLE_SIZE) return 1; // buffer too large
    if(lastaddr + txsz + rxsz >= USB_BTABLE_SIZE/ACCESSZ) return 2; // out of btable
    USB->EPnR[number] = (type << 9) | (number & USB_EPnR_EA);
    USB->EPnR[number] ^= USB_EPnR_STAT_RX | USB_EPnR_STAT_TX_1;
    if(rxsz & 1 || rxsz > USB_BTABLE_SIZE) return 3; // wrong rx buffer size
    uint16_t countrx = 0;
    if(rxsz < 64) countrx = rxsz / 2;
    else{
        if(rxsz & 0x1f) return 3; // should be multiple of 32
        countrx = 31 + rxsz / 32;
    }
    USB_BTABLE->EP[number].USB_ADDR_TX = lastaddr;
    endpoints[number].tx_buf = (uint16_t *)(USB_BTABLE_BASE + lastaddr * ACCESSZ);
    endpoints[number].txbufsz = txsz;
    lastaddr += txsz;
    USB_BTABLE->EP[number].USB_COUNT_TX = 0;
    USB_BTABLE->EP[number].USB_ADDR_RX = lastaddr;
    endpoints[number].rx_buf = (uint8_t *)(USB_BTABLE_BASE + lastaddr * ACCESSZ);
    lastaddr += rxsz;
    USB_BTABLE->EP[number].USB_COUNT_RX = countrx << 10;
    endpoints[number].func = func;
    return 0;
}

// standard IRQ handler
void USB_IRQ(){
    if(USB->ISTR & USB_ISTR_RESET){
        usbON = 0;
        // Reinit registers
        USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
        // Endpoint 0 - CONTROL
        // ON USB LS size of EP0 may be 8 bytes, but on FS it should be 64 bytes!
        lastaddr = LASTADDR_DEFAULT;
        // clear address, leave only enable bit
        USB->DADDR = USB_DADDR_EF;
        if(EP_Init(0, EP_TYPE_CONTROL, USB_EP0_BUFSZ, USB_EP0_BUFSZ, EP0_Handler)){
            return;
        }
        USB->ISTR = ~USB_ISTR_RESET;
    }
    if(USB->ISTR & USB_ISTR_CTR){
        // EP number
        uint8_t n = USB->ISTR & USB_ISTR_EPID;
        // copy status register
        uint16_t epstatus = USB->EPnR[n];
        // copy received bytes amount
        endpoints[n].rx_cnt = USB_BTABLE->EP[n].USB_COUNT_RX & 0x3FF; // low 10 bits is counter
        // check direction
        if(USB->ISTR & USB_ISTR_DIR){ // OUT interrupt - receive data, CTR_RX==1 (if CTR_TX == 1 - two pending transactions: receive following by transmit)
            if(n == 0){ // control endpoint
                if(epstatus & USB_EPnR_SETUP){ // setup packet -> copy data to conf_pack
                    EP_Read(0, setupdatabuf);
                    // interrupt handler will be called later
                }else if(epstatus & USB_EPnR_CTR_RX){ // data packet -> push received data to ep0databuf
                    EP_Read(0, ep0databuf);
                }
            }
        }
        // call EP handler
        if(endpoints[n].func) endpoints[n].func(endpoints[n]);
    }
    if(USB->ISTR & USB_ISTR_SUSP){ // suspend -> still no connection, may sleep
        usbON = 0;
#ifndef STM32F0
        USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LP_MODE;
#else
        USB->CNTR |= USB_CNTR_FSUSP | USB_CNTR_LPMODE;
#endif
        USB->ISTR = ~USB_ISTR_SUSP;
    }
    if(USB->ISTR & USB_ISTR_WKUP){ // wakeup
#ifndef STM32F0
        USB->CNTR &= ~(USB_CNTR_FSUSP | USB_CNTR_LP_MODE); // clear suspend flags
#else
        USB->CNTR &= ~(USB_CNTR_FSUSP | USB_CNTR_LPMODE);
#endif
        USB->ISTR = ~USB_ISTR_WKUP;
    }
}

#if defined STM32F3
void usb_lp_isr() __attribute__ ((alias ("USB_IRQ")));
#elif defined STM32F1
void usb_lp_can_rx0_isr() __attribute__ ((alias ("USB_IRQ")));
#elif defined STM32F0
void usb_isr() __attribute__ ((alias ("USB_IRQ")));
#endif
