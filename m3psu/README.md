# Martlet 3 Power Supply Unit

The Power Supply Unit is designed to have current monitoring and control over every single power line (5V, 3.3V, and VBatt), and all can be controlled via an MCU. It has on-board Li-ion battery charging and balancing capability, and can automatically switch between external power and internal power (or be commanded to do so).

## Work In Progress Notes

### General Notes

- General max working voltage is 16V, get components equal or higher
- 0402s are not available for capacitors >1µF and >=16V
    - Large capacitors will have to use the smallest footprint possible (which varies accordingly)
- All resistors are 0402s unless carrying large currents. Check power rating (in hidden field) if so
    - Be cautious about resistors with values <100Ω since they're likely carrying large enough currents to be concerned about power
- Use `PAGE` command to select channel 0/1/both for control over I2C
- Set `ON_OFF_CONFIG` to `On_off_config_use_pmbus`
- Send `OPERATION` command to turn channel on or off as needed
- RUNn pins should be high using pullups

### DC/DC Converter

- Choose Schottky diode to carry 1A (these are optional in the circuit, but increases efficiency)(choose: NSR20F30NXT5G (2317558(FAR))
- Switching pair of totem pole N-MOSFETs (choose: SiZ340DT (2422226(FAR))
- Boost capacitors are 100x total input (gate) capacitance of topside MOSFETs
- Bigger inductors are better, but it's a compromise on space (choose: TYS5040100M-10 2292532(FAR) -> Iripple = 0.41A)
- Large filtering capacitors give low ripple voltage. (choose: GRM188R60J226MEA0D 2494232(FAR) -> Vripple = 1.2mV)
- Rsense at 50mΩ because Vsense max is 75mV and expected current ~1A (choose: LRCS0603-0R05FT5 1506129(FAR))
- Filter over Isense is 2\*Cp\*Rs ≤ ESL/R (equivalent series inductance of sense resistor est. ~0.5nH)
- Frequency of operation will be 575kHz

#### Software Configurations

- Addresses are 0x40 to 0x45
- Since external temp monitoring is not needed and shorted to GND, set the `UT_FAULT_LIMIT` to –275°C, `IOUT_CAL_GAIN_TC` to zero and the `UT_FAULT_RE-SPONSE` to ignore
- The DC/DC converters start low, and must be configured with their voltages before turning them on.

### Lithium Battery Charger

- Address is 0x12
- Charging current limit is set to 2.7A using the potential divider, with assumption that cells are 2.6Ah
- Switching pair of totem pole N-MOSFETs (choose: SiZ340DT (2422226(FAR)), same as in the DC/DC Converter
- The P-FET (there's only one) needs to have *very low* Rds-on for efficiency (choose: SI7157DP-T1-GE3 2471947(FAR) it's big, but there's nothing smaller)
- The two soft-start N-MOSFETs on the input need to withstand 3A current draw (choose: STL15DN4F5 2098274(FAR) it's also big, but is a pair)
- The reverse polarity protection(?) N-MOSFET can be as small as possible since it carries very little current (choose: SIB452DK-T1-GE3 2364070(FAR))
- Big inductor is needed for lower ripple current for more efficiency (choose: SRP5030T-4R7M 2309887(FAR) -> Iripple = 0.9A)
- Large input and output capacitance to stabilise circuit (choose: GRM32ER61C476ME15L 1735538(FAR))
- ACIN limit is set to 15V using the potential divider, with assumption that normal charging source is 12V

### Lithium Battery Bleeders

- The N-MOSFETs carries only small amounts of current and so can be very small (choose: SIB452DK-T1-GE3 2364070(FAR))
- The P-MOSFET carries only a small amount of current and so can be very small (choose: SIB433EDK-T1-GE3 2335393(FAR))
- The bleed resistors have to take around 0.5W if they are 47Ω (choose: ERJP06F47R0V 1750737(FAR))

### Active Power ORing

- N-MOSFETs need to withstand 3A current draw (choose: STL15DN4F5 2098274(FAR)), same as in Lithium Battery Charger
- Capacitors 10x the gate capacitance (Ciss) of the MOSFET that it is switching (choose: CGA2B3X7R1H223K050BB 2210825(FAR))

### Pyro Channel Monitor and Control

- I2C Monitor used is the LTC4151 (2295457(FAR))
- The P-MOSFETs that control the channel have to carry up to 2A each for short bursts (choose: CSD25310Q2 2501102(FAR))
- The N-MOSFETs that control the P-MOSFETs only carry small amounts of current (choose: SIB452DK-T1-GE3 2364070(FAR)), same as the Li-ion bleeders

#### Software Configurations

- Address of Current Monitor is 0x6F (0b1101 111X)
- The readout for the current is 0-81.92mV (20µV resolution) and must be converted to mA. With Rsense = 0.01Ω, range is 0-8A, with resolution of 2mA
- The readout of the voltage is also the readout of the main line voltage (after the power OR)

## List of Common Parts

### Capacitors (25V)

- 4µ7 - 2426959
- 1µ - 2218855

- 100n - 2496811

- 10n - 2210793

- 4n7 - 2496786
- 1n5 - 2210833

- 220p - 2210772
- 47p - 2210781

### Resistors (±1%)

- 2M - 1458762

- 150k - 2447110
- 100k - 2447094

- 56k - 2447198
- 33k - 2447160
- 30k1 - 2072900
- 24k9 - 1803690
- 23k2 - 2302774
- 15k8 - 2302757
- 10k - 2447096

- 7k32 - 2073231
- 6k2 - 2331559
- 5k76 - 1803113
- 5k1 - 2447201
- 4k99 - 1469715
- 4k7 - 2447187
- 4k32 - 2073037
- 3k57 - 2302693
- 1k96 - 1803065
- 1k - 2447120

- 100 - 2447095

- 50m - 1506129