#include "ch.h"
#include "hal.h"

#include <string.h>
#include "labrador.h"
#include "si446x.h"
#include "s_radio.h"
#include "status.h"
#include "config.h"
#include "gps.h"
#include "logging.h"
#include "measurements.h"


#define TXCODE LDPC_CODE_N256_K128
#define RXCODE LDPC_CODE_N256_K128

static uint8_t labrador_wa[LDPC_SIZE(FAST, TXCODE, MP, RXCODE)];

/* Board configuration.
 * This tells the Si446x driver what our hardware looks like.
 */
struct si446x_board_config brdcfg = {
    .spid = &SPID2,
    .spi_cfg = {
        .end_cb = NULL,
        .ssport = GPIOA,
        .sspad = GPIOA_SR_NSS,
        .cr1 = SPI_CR1_BR_2,
    },
    .sdn = LINE_SR_SDN,
    .nirq = LINE_SR_NIRQ,
    .gpio0 = si446x_gpio_mode_tristate,
    .gpio1 = si446x_gpio_mode_tristate,
    .gpio2 = si446x_gpio_mode_tristate,
    .gpio3 = si446x_gpio_mode_tx_state,
    .clk_out_enable = false,
    .clk_out_div = si446x_clk_out_div_1,
    .tcxo = true,
    .xo_freq = 26000000,
};


/* Labrador configuration.
 * This specifies our Labrador frequency, baud, codes, encoder/decoder, etc.
 */
struct labrador_config labcfg = {
    .freq = 869700000,
    .baud = 2000,
    .tx_code = TXCODE,
    .rx_code = RXCODE,
    .ldpc_none_txlen = 0,
    .ldpc_none_rxlen = 0,
    .ldpc_fast_encoder = true,
    .ldpc_mp_decoder = true,
    .rx_enabled = true,
    .workingarea = labrador_wa,
    .workingarea_size = sizeof(labrador_wa),
};


/* Labrador radio config. Passed between Labrador and Si446x driver.
 */
struct labrador_radio_config labradcfg;


/* Labrador statistics. Updated by Labrador, read by us.
 */
struct labrador_stats labstats;


/* SLAVE Thread Function */
static THD_WORKING_AREA(sr_slave_thd_wa, 1024);
static THD_FUNCTION(sr_slave_thd, arg) {

    (void)arg;
    chRegSetThreadName("SR SLAVE");

    /* Initialise Labrador systems */
    while(labrador_init(&labcfg, &labradcfg, &labstats, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Initialise the Si446x driver */
    while(!si446x_init(&brdcfg, &labradcfg)) {
    
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Backhaul Ranging/Position Data */
    while (true) {
        
        /* Wait for PPS Event */
        if (chBSemWaitTimeout(&pps_event_sem, MS2ST(1000)) == MSG_TIMEOUT) {
            continue;
        }
        
        set_status(COMPONENT_SR, STATUS_GOOD);    
    
        /* Sleep until our TX slot */
        chThdSleepMilliseconds(BACKHAUL_DELAY);       
        
        uint8_t txbuf[16] = {0};
        
        /* Test for Active Telemetry */
        if(telem_activity) {
        
            /* Lock range_pkt */
            chMtxLock(&range_pkt_mutex);
            
            /* Load TX Buffer */
            memcpy(txbuf, &range_pkt, sizeof(ranging_packet));
                           
            /* Send a Ranging Packet */
            labrador_err result = labrador_tx(txbuf);
            
            if(result != LABRADOR_OK) {
                set_status(COMPONENT_SR, STATUS_ERROR);
            } else {
                set_status(COMPONENT_SR, STATUS_ACTIVITY);
            }
        
            /* Log TX */
            log_backhaul_ranging_message(&range_pkt);
            
            /* Relase Mutex */
            chMtxUnlock(&range_pkt_mutex);
        
        } else {
        
            /* Lock pos_pkt */
            chMtxLock(&pos_pkt_mutex);
            
            /* Load TX Buffer */
            memcpy(txbuf, &pos_pkt, sizeof(position_packet));
                           
            /* Send a Position Packet */
            labrador_err result = labrador_tx(txbuf);
                        
            if(result != LABRADOR_OK) {
                set_status(COMPONENT_SR, STATUS_ERROR);
            } else {
                set_status(COMPONENT_SR, STATUS_ACTIVITY);
            }
        
            /* Log TX */
            log_backhaul_position_message(&pos_pkt);
            
            /* Relase Mutex */
            chMtxUnlock(&pos_pkt_mutex);
        }
    }  
}


/* MASTER Thread Function */
static THD_WORKING_AREA(sr_master_thd_wa, 1024);
static THD_FUNCTION(sr_master_thd, arg) {

    (void)arg;
    chRegSetThreadName("SR MASTER");

    /* Initialise Labrador systems */
    while(labrador_init(&labcfg, &labradcfg, &labstats, &labrador_radio_si446x)
          != LABRADOR_OK)
    {
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Initialise the Si446x driver */
    while(!si446x_init(&brdcfg, &labradcfg)) {
    
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(1000);
    }

    /* Reccieve TOAD Network Telemetry */
    while (true) {
        
        // TODO
        set_status(COMPONENT_SR, STATUS_GOOD);
        chThdSleepMilliseconds(500);
        set_status(COMPONENT_SR, STATUS_ERROR);
        chThdSleepMilliseconds(500);
        
    }  
}

/* Start Secondary Radio Thread */
void sr_labrador_init(void) {

    /* Handle Master/Slave Config */
    if(TOAD_ID == TOAD_MASTER) {
    
        chThdCreateStatic(sr_master_thd_wa, sizeof(sr_master_thd_wa), NORMALPRIO, sr_master_thd, NULL);
        
    } else {
    
        chThdCreateStatic(sr_slave_thd_wa, sizeof(sr_slave_thd_wa), NORMALPRIO, sr_slave_thd, NULL);
    }
}

