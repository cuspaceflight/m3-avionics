/*
 * MS5611 Driver
 * 2014, 2016 Adam Greig, Cambridge University Spaceflight
 */
#ifndef MS5611_H
#define MS5611_H

/*
 * Initialise the MS5611 and start a thread that will collect samples from it.
 */
void ms5611_init(SPIDriver* spid, ioportid_t ssport, uint16_t sspad);

#endif
