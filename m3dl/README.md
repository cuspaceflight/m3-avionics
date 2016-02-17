# Martlet 3 Data Logger

17/02/16 - Schematics Completed

Main - STM32F4 & uSD Card that will carry out processing using the on board ADCs

Strain - Analogue front ends for 6x Strain Gauges that are then passed to the ADC on the main board

Pressure - Pressure sensors will not be mounted in the stack, so this consists of a simple RC filter passing 
           the mV output to the ADc on the main board 

Temperature - External Thermocouple controller that supports 9x K-Type Thermocouples over SPI

Base - The final board in the avionics stack that consists of external connectors & an STM32F4 that handles CAN to USB
        and the JTAG interface for the over processors on board the rocket
