#include "sp100.h"
#include "transmit.h"
#include <string.h>

/* Function Prototypes */
void transmit_SP100(struct SP100 sp100res[]);
void uart_send_buffer(void* inbuf, size_t n);
static uint16_t compute_crc(uint8_t *buf, size_t len);


/* Transmit Pressure Readings */
void transmit_SP100(struct SP100 sp100res[]){
    
    /* 8 Byte Results Buffer */
    uint16_t txres[4] = {0};
    
    /* Populate Result Tranmission Buffer */
    txres[0] = sp100res[0].pressure;
    txres[1] = sp100res[1].pressure;
    txres[2] = sp100res[2].pressure;
    txres[3] = sp100res[3].pressure;
    
    /* Send Over UART1 */
    uart_send_buffer(txres, 8);  
}


/* Frame Results and Transmit */
void uart_send_buffer(void* inbuf, size_t n){
  
    size_t i;
    uint8_t* buf = (uint8_t*)inbuf;
    uint16_t chk = compute_crc(buf, n);
    uint8_t txbuf[20] = {0};
    size_t p = sizeof(txbuf);
    uint8_t j = 0;
    
    /* Start Frame with 0x7E */
    txbuf[j] = 0x7E;
    
    /* Escape all 0x7E and 0x7D to 0x7D 0x5E and 0x7D 0x5D */
    for(i=0; i<n; i++) {
        j += 1;
        if(buf[i] == 0x7D || buf[i] == 0x7E) {
             txbuf[j] = 0x7D;
            j += 1;
             txbuf[j] = (buf[i] ^ 0x20);
        } else {
             txbuf[j] = buf[i];
        }
    }
    
    /* Append CRC16 Checksum */
    txbuf[j + 1] = (chk & 0xFF);
    txbuf[j + 2] = ((chk & 0xFF00) >> 8);
    
    /* Transmit Buffer */
    uartSendTimeout(&UARTD1, &p, &txbuf, TIME_INFINITE);
}


/* Compute CRC16 */
static uint16_t compute_crc(uint8_t *buf, size_t len){
    size_t i, j;
    uint16_t crc = 0xFFFF;
    for(i=0; i<len; i++) {
        crc ^= (uint16_t) buf[i] << 8;
        for(j=0; j<8; j++) {
            if(crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc ^ 0xFFFF;
}
