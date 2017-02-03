#include "ch.h"
#include "hal.h"

#include "sp100.h"
#include "transmit.h"

struct SP100 sp100s[4] = {{0}};
ioline_t sp100_ss[4] = {LINE_CS1, LINE_CS2, LINE_CS3, LINE_CS4};


static const UARTConfig uart_cfg = {
    .txend1_cb = NULL,
    .txend2_cb = NULL,
    .rxend_cb  = NULL,
    .rxchar_cb = NULL,
    .rxerr_cb  = NULL,
    .speed     = 38400,
    .cr1       = 0,
    .cr2       = USART_CR2_LINEN,
    .cr3       = 0,
};

static THD_WORKING_AREA(heartbeat_wa, 128);
static THD_FUNCTION(heartbeat_thd, arg) {
    (void)arg;
    while(true) {
        palSetLine(LINE_LED_GRN);
        chThdSleepMilliseconds(100);
        palClearLine(LINE_LED_GRN);
        chThdSleepMilliseconds(900);
    }
}

int main(void) {
    halInit();
    chSysInit();    
    
    chThdCreateStatic(heartbeat_wa, sizeof(heartbeat_wa),
                      NORMALPRIO, heartbeat_thd, NULL); 
                      
    SYSCFG->CFGR1 |= (SYSCFG_CFGR1_USART1TX_DMA_RMP | SYSCFG_CFGR1_USART1RX_DMA_RMP);
                      
    uartStart(&UARTD1, &uart_cfg);
    sp100_init(&SPID1, 4, sp100_ss, sp100s);
    
    while(true) {
    }
}
