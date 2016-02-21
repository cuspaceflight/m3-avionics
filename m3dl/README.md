# Martlet 3 Data Logger Stack

x1 Temperature Sensor Board:
- LTC2983 10 Channel fancy box
- 9 Thermocouples w/ Cold junction compensation
- SPI over data logger stack interconnect (x4 Pins)
- On board 18-pin connector (2 Pins per Thermocouple) connects to a wiring harness which passes through cutouts in below boards and breaks out to 9x 2-Pin in line connectors for thermocouples.

x2 Strain Gauge Boards:
- Analogue front end w/ AD8226    
- 3 Strain Gauges per PCB – 6 Total
- Connection to ADC Inputs over data logger stack interconnect (x6 Pins)
- Per PCB an on board 12-pin connector (4 Pins per Gauge) connects to a wiring harness which passes through cutouts in below boards and breaks out to 6x 4-Pin in line connectors for strain gauges

x1 Pressure Sensor Board:
- Absolute pressure sensors with mV Output to be placed externally
- Simple RC filter connects sensor outputs to ADC over data logger stack interconnect (x5 Pins)
- Maximum 5 external Pressure Sensors 
- On board 10-pin connector to a cable which passes through cutouts in below boards and attaches to the external module

x1 Main Board:
- 64 Pin STM32F405
- uSD Card
- CAN Controller
- Surface mount USB port if space allows for debugging

x1 Connector Board:
- STM32F4 doing JTAG to USB and CAN to USB simultaneous
- JST-PA Connectors for USB, Pyro, Power/Charge


20 Pin Data Logger Interconnect: 
 - x5 SPI 
 - x5 ADC Pressure 
 - x6 ADC Strain 
 - x4 GND
