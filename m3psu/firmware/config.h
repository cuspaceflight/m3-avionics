/*
 * config.h
 *
 *  Created on: 28 Apr 2016
 *      Author: Jamie
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// Which devices we're using
#define ADC_DRIVER      ADCD1
#define I2C_DRIVER      I2CD3
#define CAN_DRIVER      CAND1

// Whether to read the calculated power value from the LTC3887, or
//  calculate it ourselves (faster, less accurate)
#define LTC3887_READ_CALCULATED_POWER   TRUE

#endif /* CONFIG_H_ */
