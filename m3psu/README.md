# Martlet 3 Power Supply Unit

The Power Supply Unit is designed to have current monitoring and control over every single power line (5V, 3.3V, and VBatt), and all can be controlled via an MCU. It has on-board Li-ion battery charging and balancing capability, and can automatically switch between external power and internal power (or be commanded to do so).

## Work In Progress Notes

### General Notes

- Everything is designed to assume shore power of 11.5V to 14.5V, i.e. a 12V lead-acid (car) battery.
- Internal power supply is assumed to be 2 Li-ion cells, 3.5V-4.2V per cell giving 7V-8.4V internal supply.
- All components are designed to function at max working voltage of 16V, or higher (usually 25V where possible).
- 0402s are not available for capacitors >1µF and ≥16V.
    - Large capacitors will have to use the smallest footprint possible (which varies accordingly).
- All resistors are 0402s unless carrying large currents. Check power rating (in hidden field) if so.
    - Be cautious about resistors with values <100Ω since they're likely carrying large enough currents to be concerned about power.
- Assembled stack has to have charger board and controller board next to each other, followed by DC/DC converters as furthest.
- The I2C Bus is pulled up at the microcontroller unit.

### DC/DC Converter

- Choose: LTC3887 2475658(FAR).
- Max current provisioned: 1A @5V or @3.3V
- Operating Frequency: 575kHz.
- Choose Schottky diode to carry 1A (these are optional in the circuit, but increases efficiency)(choose: NSR20F30NXT5G (2317558(FAR)).
- Switching pair of totem pole N-MOSFETs (choose: SiZ340DT (2422226(FAR)).
- Boost capacitors are 100x total input (gate) capacitance of topside MOSFETs.
- Bigger inductors are better, but it's a compromise on space (choose: TYS5040100M-10 2292532(FAR) -> Iripple = 0.41A).
- Large filtering capacitors give low ripple voltage. (choose: GRM21BR61E226ME44L 1907510(FAR) -> Vripple = 1.2mV).
- Rsense at 50mΩ because Vsense max is 75mV and expected current ~1A (choose: LRCS0603-0R05FT5 1506129(FAR)).
- Filter over Isense is 2\*Cp\*Rs ≤ ESL/R (equivalent series inductance of sense resistor est. ~0.5nH).

#### Software Configurations

- Addresses are 0x40 to 0x45 (0b100 0000 X to 0b100 0101 X), note that the EEPROM is supposed to have the 3 most significant bits as 100 by default.
    - Address 0x40:
        - V0 = 5V Cameras (1A)
        - V1 = 3.3V Pyro µC (0.1A)
    - Address 0x41:
        - V0 = 5V Radio AMP (0.35A)
        - V1 = 3.3V Radio µC + etc (0.25A)
    - Address 0x42:
        - V0 = 5V AUX1 (1A)
        - V1 = 5V CAN transceivers (0.1A)
    - Address 0x43:
        - V0 = 5V IMU ADIS (0.3A)
        - V1 = 3.3V IMU µC + etc (0.3A)
    - Address 0x44:
        - V0 = 5V AUX2 (1A)
        - V1 = 3.3V Flight Computer µC + etc (0.15A)
    - Address 0x45:
        - V0 = 3.3V AUX1 (1A)
        - V1 = 3.3V Datalogger µC + etc (0.3A)
- Since external temp monitoring is not needed and shorted to GND, set the `UT_FAULT_LIMIT` to –275°C, `IOUT_CAL_GAIN_TC` to zero and the `UT_FAULT_RE-SPONSE` to ignore.
- The DC/DC converters start low, and must be configured with their voltages before turning them on.
- Use `PAGE` command to select channel 0/1/both for control over I2C.
- Set `ON_OFF_CONFIG` to `On_off_config_use_pmbus`.
- Send `OPERATION` command to turn channel on or off as needed.

### Lithium Battery Charger

- Choose: MAX17435 2516688(FAR).
- Address is 0x09 (0b000 1001 X).
- Charging current limit is set to 2.7A using the potential divider, with assumption that cells are 2.6Ah.
- Switching pair of totem pole N-MOSFETs (choose: SiZ340DT (2422226(FAR)), same as in the DC/DC Converter.
- The P-FET from the battery needs to have *very low* Rds-on for efficiency (choose: Si7157DP 2471947(FAR) it's big, but there's nothing smaller).
- P-FET from main power just needs to have low enough Rds-on to handle ~4A (choose: Si7101DN 2364059(FAR)). This is used to control whether the charger is actually charging.
- The two soft-start N-MOSFETs on the input need to withstand 3A current draw (choose: STL15DN4F5 2098274(FAR) it's also big, but is a pair).
- The reverse polarity protection(?) N-MOSFET can be as small as possible since it carries very little current (choose: PMZB150UNE 2498598(FAR)).
- Big inductor is needed for lower ripple current for more efficiency (choose: SRP5030T-4R7M 2309887(FAR) -> Iripple = 0.9A).
- Large input and output capacitance to stabilise circuit (choose: GRM32ER61C476ME15L 1735538(FAR)).
- ACIN limits are 11.5-14.5V using the potential divider, with assumption of a nominal source charging of 12V (lead-acid battery).

#### Software Considerations

- The charger has two "switches", one for enabling charging and monitoring of discharging, and the other to control if the charger is connected to the external power supply.
- By default, both are pulled down, and so must be set high for charging to begin.
- When switching from shore power to internal power before launch, disable the external power supply from the charger first, and then switch the whole stack using the dedicated power OR.
    - The reason is because the Lithium Charger does its *own* power OR-ing but isn't as fast, so just switching from the dedicated power OR causes the lithium charger to switch at the same time, causing the switchover time to be the slower of the 2 which is undesirable, and pointless.
- All digital inputs and outputs (apart from the Bus) are funneled through the port expander (see port expander for details).

##### Port Expander (for Lithium Charger)

- Choose: PCAL9538A 2428172(FAR).
- Address is 0x70 (0b111 0000 X).
- Ports P4-P7 are grounded to prevent floating, but datasheet advises to change them to outputs after bootup.

### Lithium Battery Bleeders

- The N-MOSFETs carries only small amounts of current and so can be very small (choose: PMZB150UNE 2498598(FAR)).
- The P-MOSFET carries only a small amount of current and so can be very small (choose: PMZB320UPE 2498601(FAR)).
- The bleed resistors have to take around 0.5W if they are 47Ω (choose: ERJP06F47R0V 1750737(FAR)).

### Lithium Battery Module

- This will use one of the global reserve lines for the middle battery tap, but will not be linked to the external lines. Note that this will break the global interconnection.

### Active Power ORing

- Choose: LTC4353 2115909(FAR).
- N-MOSFETs need to withstand 3A current draw (choose: STL15DN4F5 2098274(FAR)), same as in Lithium Battery Charger.
- Capacitors 10x the gate capacitance (Ciss) of the MOSFET that it is switching (choose: CGA2B3X7R1H223K050BB 2210825(FAR)).

### Pyro Channel Monitor and Control

- Choose: LTC4151 (2295457(FAR)).
- The P-MOSFETs that control the channel have to carry up to 2A each for short bursts (choose: CSD25310Q2 2501102(FAR)).
- The N-MOSFET that control the P-MOSFET only carries a small amount of current (choose: PMZB150UNE 2498598(FAR)), same as the Li-ion bleeders.

#### Software Configurations

- Address of Current Monitor is 0x6F (0b110 1111 X).
- The readout for the current is 0-81.92mV (20µV resolution) and must be converted to mA. With Rsense = 0.01Ω, range is 0-8A, with resolution of 2mA.
- The readout of the voltage is also the readout of the main line voltage (after the power OR).

### PSU for the PSU

- Choose: TPS61252 2382918(FAR).
- Max current provisioned: 0.2A @3.3V.
- Operatiing frequency: 1.25MHz.
- Choose smallest footprint inductor possible which meets requirements (choose: VLS201610HBX-3R3M-1 2455353(FAR)).
- Input-Output Capacitors are large for low ripple (choose: GRM21BR61E226ME44L 1907510(FAR)).

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