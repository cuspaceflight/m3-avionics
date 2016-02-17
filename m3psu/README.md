# Martlet 3 Power Supply Unit

The Power Supply Unit is designed to have current monitoring and control over every single power line (5V, 3.3V, and VBatt), and all can be controlled via an MCU. It has on board Li-ion battery charging and balancing capability, and can automatically switch between external power and internal power (or be commanded to do so).

##Work In Progress Notes
###DC/DC Converter
- Since external temp monitoring is not needed and shorted to GND, set the `UT_FAULT_LIMIT` to –275°C, `IOUT_CAL_GAIN_TC` set to zero and the `UT_FAULT_RE-SPONSE` to ignore.
- Choose Schottky diode to carry 1A
- Boost capacitors are 100x total input (gate) capacitance of topside MOSFETs
- Low ESR capacitors are needed (particularly the 100uF one. Get this: 1735535(FAR))
- Inductors need to carry above 4A of peak current, pref 5A without saturating
- Rsense at 10mΩ are still a shot in the dark
- Frequency of operation will be 575kHz