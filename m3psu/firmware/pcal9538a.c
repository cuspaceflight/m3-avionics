/*
 * PCAL9538A Driver
 * Cambridge University Spaceflight
 */

#include "error.h"
#include "smbus.h"
#include "pcal9538a.h"

/* Register Addresses */
#define PCAL9538A_RA_INPUT      0x00
#define PCAL9538A_RA_OUTPUT     0x01
#define PCAL9538A_RA_INVERSION  0x02
#define PCAL9538A_RA_CONFIG     0x03
#define PCAL9538A_RA_LATCH      0x42
#define PCAL9538A_RA_PUD_ENABLE 0x43
#define PCAL9538A_RA_PUD_DIR    0x44
#define PCAL9538A_RA_INT_MASK   0x45
#define PCAL9538A_RA_INT_STATUS 0x46
#define PCAL9538A_RA_OUT_TYPE   0x4F

uint8_t pcal9538a_init(PCAL9538A *pcal, I2CDriver *i2c, i2caddr_t address){
  pcal->config.i2c = i2c;
  pcal->config.address = address;

  return ERR_OK;
}

uint8_t pcal9538a_read_inputs(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INPUT, result);
}

uint8_t pcal9538a_read_outputs(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_OUTPUT, result);
}
uint8_t pcal9538a_read_output_bit(PCAL9538A *pcal, uint8_t bit, uint8_t *result){
  uint8_t curr;
  if(pcal9538a_read_outputs(pcal, &curr) != 0){
    return ERR_COMMS;
  }
  *result = (curr & (1 << bit)) != 0;
  return ERR_OK;
}
uint8_t pcal9538a_write_outputs(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_OUTPUT, config);
}
uint8_t pcal9538a_write_output_bit(PCAL9538A *pcal, uint8_t bit, uint8_t value){
  uint8_t curr;
  if(pcal9538a_read_outputs(pcal, &curr) != 0){
    return ERR_COMMS;
  }
  if(value){
    curr |= (1 << bit);
  }else{
    curr &= ~(1 << bit);
  }
  if(pcal9538a_write_outputs(pcal, curr) != 0){
    return ERR_COMMS;
  }
  return ERR_OK;
}

uint8_t pcal9538a_get_input_inversion(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INVERSION, result);
}
uint8_t pcal9538a_set_input_inversion(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INVERSION, config);
}

uint8_t pcal9538a_get_output_dir(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_CONFIG, result);
}
uint8_t pcal9538a_set_output_dir(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_CONFIG, config);
}

uint8_t pcal9538a_get_input_latch(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_LATCH, result);
}
uint8_t pcal9538a_set_input_latch(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_LATCH, config);
}

uint8_t pcal9538a_get_pull_enabled(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_PUD_ENABLE, result);
}
uint8_t pcal9538a_set_pull_enabled(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_PUD_ENABLE, config);
}

uint8_t pcal9538a_get_pull_up_down(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_PUD_DIR, result);
}
uint8_t pcal9538a_set_pull_up_down(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_PUD_DIR, config);
}

uint8_t pcal9538a_get_interrupt_mask(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INT_MASK, result);
}
uint8_t pcal9538a_set_interrupt_mask(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INT_MASK, config);
}

uint8_t pcal9538a_get_interrupt_status(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_INT_STATUS, result);
}

uint8_t pcal9538a_get_output_type(PCAL9538A *pcal, uint8_t *result){
  return smbus_read_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_OUT_TYPE, result);
}
uint8_t pcal9538a_set_output_type(PCAL9538A *pcal, uint8_t config){
  return smbus_write_byte(pcal->config.i2c, pcal->config.address, PCAL9538A_RA_OUT_TYPE, config);
}
