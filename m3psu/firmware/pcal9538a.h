/*
 * PCAL9538A Driver
 * Cambridge University Spaceflight
 */

#ifndef PCAL9538A_H_
#define PCAL9538A_H_

#include "ch.h"
#include "hal.h"

#define PCAL9538A_INP_INVERTED      1
#define PCAL9538A_INP_NON_INVERTED  0
#define PCAL9538A_DIR_OUTPUT        0
#define PCAL9538A_DIR_INPUT         1
#define PCAL9538A_INP_LATCHING      1
#define PCAL9538A_INP_NON_LATCHING  0
#define PCAL9538A_PULL_ENABLED      1
#define PCAL9538A_PULL_DISABLED     0
#define PCAL9538A_PUD_PULL_UP       1
#define PCAL9538A_PUD_PULL_DOWN     0
#define PCAL9538A_INT_UNMASKED      0
#define PCAL9538A_INT_MASKED        1
#define PCAL9538A_TYPE_PUSH_PULL    0
#define PCAL9538A_TYPE_OPEN_DRAIN   1

typedef struct {
  I2CDriver *i2c;
  i2caddr_t address;
} PCAL9538AConfig;

typedef struct {
  PCAL9538AConfig config;
} PCAL9538A;

uint8_t pcal9538a_init(PCAL9538A *pcal, I2CDriver *i2c, i2caddr_t address);

uint8_t pcal9538a_read_inputs(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_read_outputs(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_read_output_bit(PCAL9538A *pcal, uint8_t bit, uint8_t *result);
uint8_t pcal9538a_write_outputs(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_write_output_bit(PCAL9538A *pcal, uint8_t bit, uint8_t value);
uint8_t pcal9538a_get_input_inversion(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_input_inversion(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_output_dir(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_output_dir(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_input_latch(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_input_latch(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_pull_enabled(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_pull_enabled(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_pull_up_down(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_pull_up_down(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_interrupt_mask(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_interrupt_mask(PCAL9538A *pcal, uint8_t config);
uint8_t pcal9538a_get_interrupt_status(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_get_output_type(PCAL9538A *pcal, uint8_t *result);
uint8_t pcal9538a_set_output_type(PCAL9538A *pcal, uint8_t config);

#endif /* PCAL9538A_H_ */
