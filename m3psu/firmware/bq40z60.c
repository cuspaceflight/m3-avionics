/*
 * BQ40Z60 Driver
 * Cambridge University Spaceflight
 */

#include "error.h"
#include "smbus.h"
#include "bq40z60.h"

/////////// CONFIGURE THESE IN DATA MEMORY
// Disable temperature sensing
// Set Minimum output voltage
// Set voltage resolution
// Set charging voltage
// Set charging current

//// Mfg Status Init
// FET_EN -> 1
// LED_EN -> 0 (default)
// CHGR_EN -> 0 (default)
// GAUGE_EN -> 1
//// DA Configuration
// NR (non-removable) -> 1
// CC (cell count) -> 1 (value is num cells - 1)
//// Temperature Enable
// TS{0-4} Enable -> 0 (Disabled)
// TSInt Enable -> 1 (Enabled)
//// Temperature Mode
// TSInt Mode -> 1 (FET)
//// Balancing Configuration
// CB -> 1 (Enable balancing)
// CBR -> 1 (Balancing at rest)
// CBM -> 0 (Internal balancing) (*** CHECK ***)
// Min Start Balance Delta -> ?
// Relax Balance Interval -> ?
// Min RSOC for Balancing -> ?
// (See section 8.5 to configure balancing at rest)

//// Impedence track config:
// Load Mode -> 1 (Constant Power)
// Load Select -> 7 (Max Avg P Last Run) (apparently best for variable loads)
// Term Voltage ?
// Term Min Cell V ?
// ReserveCap-mWh ?
// QMax Initial Values ?
// Design Capacity -> 2200
// Design Voltage -> 7400
// Update Status ?

//// Alert Interrupt
// BTP_EN -> 1
// BTP_POL -> 


/////////// REGISTERS TO USE
// MAC DAStatus1 gives cell voltages, power etc.
// RelativeStateOfCharge (RSOC)
// FullChargeCapacity (FCC)
// RemainingCapacity (RemCap)


#define BQ40Z60_CMD_MANUFACTURER_ACCESS     0x44

#define BQ40Z60_MAC_OPERATION_STATUS        0x0054
#define BQ40Z60_MAC_MANUFACTURING_STATUS    0x0057
#define BQ40Z60_MAC_CHGR_EN_TOGGLE          0x00C0

#define BQ40Z60_MAC_OPERATION_STATUS_W3_CB_MASK             (1 << 4)
#define BQ40Z60_MAC_MANUFACTURING_STATUS_HI_CHGR_EN_MASK    (1 << 2)

uint8_t bq40z60_mac_write(BQ40Z60 *bq, uint16_t mac_address, uint8_t *txbuf, uint8_t txbuflen){
    uint8_t txdat[64];
    
    chDbgAssert(datalen <= 62, "datalen > 62");
    
    txdat[0] = mac_address & 0xff;
    txdat[1] = (mac_address >> 8) & 0xff;
    
    // copy rest of the data to transmit
    memcpy(txdat+2, txbuf, txbuflen);
    
    return smbus_write_block(bq->config.i2c, bq->config.address, BQ40Z60_CMD_MANUFACTURER_ACCESS, txdat, txbuflen+2);
}

uint8_t bq40z60_mac_read(BQ40Z60 *bq, uint16_t mac_address, uint8_t *rxbuf, uint8_t rxbuflen){
    uint8_t txdat[2];
    txdat[0] = mac_address & 0xff;
    txdat[1] = (mac_address >> 8) & 0xff;
    return smbus_read_block(bq->config.i2c, bq->config.address, BQ40Z60_CMD_MANUFACTURER_ACCESS, txdat, sizeof(txdat), rxbuf, rxbuflen);
}

uint8_t bq40z60_set_charger_enabled(BQ40Z60 *bq, uint8_t enabled){
    // Check if charger is already enabled and toggle it if necessary
    uint8_t is_enabled = 0;
    if(bq40z60_is_charging_enabled(bq, &is_enabled) != ERR_OK){
        return ERR_COMMS;
    }
    if(!is_enabled){
        return bq40z60_mac_write(bq, BQ40Z60_MAC_CHGR_EN_TOGGLE, NULL, 0);
    }
    return ERR_OK;
}

uint8_t bq40z60_is_charger_enabled(BQ40Z60 *bq, uint8_t *enabled){
    uint8_t rxdat[2];
    if(bq40z60_mac_read(bq, BQ40Z60_MAC_MANUFACTURING_STATUS, rxdat, sizeof(rxdat))!=ERR_OK){
        return ERR_COMMS;
    }
    
    if((rxdat[1] & BQ40Z60_MAC_MANUFACTURING_STATUS_HI_CHGR_EN_MASK) != 0){
        *enabled = 1;
    }else{
        *enabled = 0;
    }
    
    return ERR_OK;
}

uint8_t bq40z60_set_balancing_enabled(BQ40Z60 *bq, uint8_t enabled){
    // Check if charger is already enabled and toggle it if necessary
    uint8_t is_enabled = 0;
    if(bq40z60_is_balancing_enabled(bq, &is_enabled) != ERR_OK){
        return ERR_COMMS;
    }
    if(!is_enabled){
        return bq40z60_mac_write(bq, BQ40Z60_MAC_CHGR_EN_TOGGLE, NULL, 0);
    }
    return ERR_OK;
}

uint8_t bq40z60_is_balancing_enabled(BQ40Z60 *bq, uint8_t *enabled){
    uint8_t rxdat[4];
    if(bq40z60_mac_read(bq, BQ40Z60_MAC_OPERATION_STATUS, rxdat, sizeof(rxdat))!=ERR_OK){
        return ERR_COMMS;
    }
    
    if((rxdat[3] & BQ40Z60_MAC_OPERATION_STATUS_W3_CB_MASK) != 0){
        *enabled = 1;
    }else{
        *enabled = 0;
    }
    
    return ERR_OK;
}

uint8_t bq40z60_init(BQ40Z60 *bq, I2CDriver *i2c, i2caddr_t address){
  bq->config.i2c = i2c;
  bq->config.address = address;

  return ERR_OK;
}

