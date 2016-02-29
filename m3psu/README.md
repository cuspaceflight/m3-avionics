# Martlet 3 Power Supply Unit

The Power Supply Unit is designed to have current monitoring and control over every single power line (5V, 3.3V, and VBatt), and all can be controlled via an MCU. It has on-board Li-ion battery charging and balancing capability, and can automatically switch between external power and internal power (or be commanded to do so).

## Work In Progress Notes

### DC/DC Converter

- Choose Schottky diode to carry 1A (these are optional in the circuit, but increases efficiency) choose: NSR20F30NXT5G (2317558(FAR))
- N Channel MOSFETs will be the TrenchFET® SiZ340DT (2422226(FAR))
- Boost capacitors are 100x total input (gate) capacitance of topside MOSFETs
- Bigger inductors are better, but it's a compromise on space (choose: TYS5040100M-10 2292532(FAR) -> Iripple = 0.41A)
- Large filtering capacitors give low ripple voltage. (choose: GRM188R60J226MEA0D 2494232(FAR) -> Vripple = 1.2mV)
- Rsense at 50mΩ because Vsense max is 75mV and expected current ~1A (choose: LRCS0603-0R05FT5 1506129(FAR))
- Filter over Isense is 2\*Cp\*Rs ≤ ESL/R (equivalent series inductance of sense resistor ~0.5nH)
- Frequency of operation will be 575kHz
- Addresses of Converters and expanders are:
    - 0x40 to 0x45 and 0x50, 0x52, 0x54
    - 0x40 & 0x41 together with bus expander 0x50, etc.

#### Software Configurations

- Since external temp monitoring is not needed and shorted to GND, set the `UT_FAULT_LIMIT` to –275°C, `IOUT_CAL_GAIN_TC` to zero and the `UT_FAULT_RE-SPONSE` to ignore.
- Addresses of Converters and expanders are:
    - 0x40 to 0x45 and 0x50, 0x52, 0x54
    - 0x40 & 0x41 together with bus expander 0x50, etc.

### Lithium Battery Charger

- Charging current limit is set to 2.7A using the potential divider, with assumption that cells are 2.6Ah
- The pair of NFETs connected to each other are TrenchFET® SiZ340DT (2422226(FAR)), same as in the DC/DC Converter
- Big inductor is needed for lower ripple current for more efficiency (choose: SRP5030T-4R7M 2309887(FAR) -> Iripple = 0.9A)
- Large input and output capacitance to stabilise circuit (choose: GRM32ER61C476ME15L 1735538(FAR))
- ACIN limit is set to 15V using the potential divider, with assumption that normal charging source is 12V
- Unsure what input N-FETs do... along with their resistors 2M and 150k.