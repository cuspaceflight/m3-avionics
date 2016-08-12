/*
 * ADXL345 Driver
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */

#ifndef ADXL345_H
#define ADXL345_H

/*
 * Initialise the ADXL345 and start a thread that will collect samples from it.
 */
void adxl345_init(SPIDriver* spid, ioportid_t ssport, uint16_t sspad);

/*
 * Interrupt callbacks for EXTI. Register against the ADXL interrupt pin.
 */
void adxl345_interrupt(EXTDriver *extp, expchannel_t channel);

#endif
