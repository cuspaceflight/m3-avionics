EESchema Schematic File Version 2
LIBS:agg-kicad
LIBS:m3psu-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 4 13
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 950  2400 1250 550 
U 56D5BD87
F0 "Regulator 1" 60
F1 "file56D5BD86.sch" 60
F2 "SDA" B L 950 2750 60 
F3 "SCL" B L 950 2850 60 
F4 "VOUT1" U R 2200 2600 60 
F5 "VOUT0" U R 2200 2500 60 
F6 "~SMB_ALERT" O L 950 2500 60 
$EndSheet
Text Notes 950  1050 0    60   ~ 0
Cameras (5V @ 1A)\nPyro MCU (3.3V @ 0.1A)\nAddress 0x40
Text Notes 950  2300 0    60   ~ 0
Radio (5V @ 0.35A)\nRadio MCU (3.3V @ 0.25A)\nAddress 0x41
$Sheet
S 950  1150 1250 550 
U 56B76640
F0 "Regulator 0" 60
F1 "file56B7663F.sch" 60
F2 "SDA" B L 950 1500 60 
F3 "SCL" B L 950 1600 60 
F4 "VOUT0" U R 2200 1250 60 
F5 "VOUT1" U R 2200 1350 60 
F6 "~SMB_ALERT" O L 950 1250 60 
$EndSheet
Text Notes 600  650  0    120  ~ 0
BOARD 1
Text Notes 3950 1050 0    60   ~ 0
AUX 1 (5V @ 1A)\nCAN transceivers (5V @ 0.1A)\nAddress 0x42
Text Notes 3950 2300 0    60   ~ 0
IMU ADIS (5V @ 0.3A)\nIMU Other (3.3V @ 0.3A)\nAddress 0x43
Text Notes 3600 650  0    120  ~ 0
BOARD 2
$Sheet
S 3950 1150 1250 550 
U 56D37487
F0 "Regulator 2" 60
F1 "file56D37486.sch" 60
F2 "SDA" B L 3950 1500 60 
F3 "SCL" B L 3950 1600 60 
F4 "VOUT1" U R 5200 1350 60 
F5 "VOUT0" U R 5200 1250 60 
F6 "~SMB_ALERT" O L 3950 1250 60 
$EndSheet
$Sheet
S 3950 2400 1250 550 
U 56D4B3D6
F0 "Regulator 3" 60
F1 "file56D4B3D5.sch" 60
F2 "SDA" B L 3950 2750 60 
F3 "SCL" B L 3950 2850 60 
F4 "VOUT1" U R 5200 2600 60 
F5 "VOUT0" U R 5200 2500 60 
F6 "~SMB_ALERT" O L 3950 2500 60 
$EndSheet
Text Notes 900  4150 0    60   ~ 0
AUX 2 (5V @ 1A)\nFC MCU (3.3V @ 0.15A)\nAddress 0x44
Text Notes 900  5400 0    60   ~ 0
DL Sensors (5V @ 0.1A)\nDL MCU (3.3V @ 0.3A)\nAddress 0x45
Text Notes 550  3750 0    120  ~ 0
BOARD 3
$Sheet
S 900  5500 1250 550 
U 56D569DB
F0 "Regulator 5" 60
F1 "file56D569DA.sch" 60
F2 "SDA" B L 900 5850 60 
F3 "SCL" B L 900 5950 60 
F4 "VOUT1" U R 2150 5700 60 
F5 "VOUT0" U R 2150 5600 60 
F6 "~SMB_ALERT" O L 900 5600 60 
$EndSheet
Text HLabel 4800 4200 2    60   BiDi ~ 0
PMBUS_SDA
Text Label 4350 4200 2    60   ~ 0
SDA
Text HLabel 4800 4300 2    60   BiDi ~ 0
PMBUS_SCL
Text Label 4350 4300 2    60   ~ 0
SCL
Text Label 850  1600 2    60   ~ 0
SCL
Text Label 850  1500 2    60   ~ 0
SDA
Text Label 3850 1600 2    60   ~ 0
SCL
Text Label 3850 1500 2    60   ~ 0
SDA
Text Notes 3700 3750 0    120  ~ 0
Off-sheet connections
Text Label 2250 1250 0    60   ~ 0
V_CAMERAS
Text Label 2250 1350 0    60   ~ 0
V_PYRO_MCU
Text Label 2250 2500 0    60   ~ 0
V_RADIO_AMP
Text Label 2250 2600 0    60   ~ 0
V_RADIO_MCU
Text Label 2200 4350 0    60   ~ 0
V_AUX_2
Text Label 2200 4450 0    60   ~ 0
V_FC_MCU
Text Label 2200 5600 0    60   ~ 0
V_DL_SENSORS
Text Label 2200 5700 0    60   ~ 0
V_DL_MCU
Text Label 5250 2500 0    60   ~ 0
V_IMU_ADIS
Text Label 5250 2600 0    60   ~ 0
V_IMU_OTHER
Text Label 5250 1250 0    60   ~ 0
V_AUX_1
Text Label 5250 1350 0    60   ~ 0
V_CAN
Text HLabel 4800 4450 2    60   UnSpc ~ 0
V_CAMERAS
Text HLabel 4800 4550 2    60   UnSpc ~ 0
V_PYRO_MCU
Text HLabel 4800 4650 2    60   UnSpc ~ 0
V_RADIO_AMP
Text HLabel 4800 4750 2    60   UnSpc ~ 0
V_RADIO_MCU
Text HLabel 4800 4900 2    60   UnSpc ~ 0
V_AUX_1
Text HLabel 4800 5000 2    60   UnSpc ~ 0
V_CAN
Text HLabel 4800 5100 2    60   UnSpc ~ 0
V_IMU_ADIS
Text HLabel 4800 5200 2    60   UnSpc ~ 0
V_IMU_OTHER
Text HLabel 4800 5350 2    60   UnSpc ~ 0
V_AUX_2
Text HLabel 4800 5450 2    60   UnSpc ~ 0
V_FC_MCU
Text HLabel 4800 5550 2    60   UnSpc ~ 0
V_DL_SENSORS
Text HLabel 4800 5650 2    60   UnSpc ~ 0
V_DL_MCU
Wire Wire Line
	850  1500 950  1500
Wire Wire Line
	950  1600 850  1600
Wire Notes Line
	600  700  2950 700 
Wire Notes Line
	2950 700  2950 3200
Wire Notes Line
	2950 3200 600  3200
Wire Notes Line
	600  3200 600  700 
Wire Wire Line
	3850 1500 3950 1500
Wire Wire Line
	3950 1600 3850 1600
Wire Notes Line
	3600 700  5950 700 
Wire Notes Line
	5950 700  5950 3200
Wire Notes Line
	5950 3200 3600 3200
Wire Notes Line
	3600 3200 3600 700 
Wire Notes Line
	550  3800 2950 3800
Wire Notes Line
	2950 3800 2950 6300
Wire Notes Line
	2950 6300 550  6300
Wire Notes Line
	550  6300 550  3800
Wire Wire Line
	4350 4200 4800 4200
Wire Wire Line
	4350 4300 4800 4300
Wire Notes Line
	3900 3800 5650 3800
Wire Notes Line
	5650 3800 5650 5950
Wire Notes Line
	5650 5950 3900 5950
Wire Notes Line
	3900 5950 3900 3800
Wire Wire Line
	2200 1250 2250 1250
Wire Wire Line
	2200 1350 2250 1350
Wire Wire Line
	2250 2600 2200 2600
Wire Wire Line
	2250 2500 2200 2500
Wire Wire Line
	2150 4350 2200 4350
Wire Wire Line
	2150 4450 2200 4450
Wire Wire Line
	2150 5600 2200 5600
Wire Wire Line
	2150 5700 2200 5700
Wire Wire Line
	5200 1250 5250 1250
Wire Wire Line
	5200 1350 5250 1350
Wire Wire Line
	5200 2500 5250 2500
Wire Wire Line
	5200 2600 5250 2600
Wire Wire Line
	4800 4450 4650 4450
Wire Wire Line
	4650 4550 4800 4550
Wire Wire Line
	4800 4650 4650 4650
Wire Wire Line
	4800 4750 4650 4750
Wire Wire Line
	4800 4900 4650 4900
Wire Wire Line
	4800 5000 4650 5000
Wire Wire Line
	4650 5100 4800 5100
Wire Wire Line
	4800 5200 4650 5200
Wire Wire Line
	4650 5350 4800 5350
Wire Wire Line
	4800 5450 4650 5450
Wire Wire Line
	4650 5550 4800 5550
Wire Wire Line
	4650 5650 4800 5650
Text Label 4650 4450 2    60   ~ 0
V_CAMERAS
Text Label 4650 4550 2    60   ~ 0
V_PYRO_MCU
Text Label 4650 4650 2    60   ~ 0
V_RADIO_AMP
Text Label 4650 4750 2    60   ~ 0
V_RADIO_MCU
Text Label 4650 4900 2    60   ~ 0
V_AUX_1
Text Label 4650 5000 2    60   ~ 0
V_CAN
Text Label 4650 5100 2    60   ~ 0
V_IMU_ADIS
Text Label 4650 5200 2    60   ~ 0
V_IMU_OTHER
Text Label 4650 5350 2    60   ~ 0
V_AUX_2
Text Label 4650 5450 2    60   ~ 0
V_FC_MCU
Text Label 4650 5550 2    60   ~ 0
V_DL_SENSORS
Text Label 4650 5650 2    60   ~ 0
V_DL_MCU
$Comp
L R R?
U 1 1 56D55B46
P 4700 4050
F 0 "R?" H 4750 4100 50  0000 C CNN
F 1 "4k7" H 4750 4000 50  0000 C CNN
F 2 "agg:0402" H 4700 4050 50  0001 C CNN
F 3 "" H 4700 4050 50  0001 C CNN
F 4 "2447187" H 4700 4050 60  0001 C CNN "Farnell"
	1    4700 4050
	0    1    1    0   
$EndComp
$Comp
L 3v3 #PWR?
U 1 1 56D59DE5
P 4600 3950
F 0 "#PWR?" H 4600 4060 50  0001 L CNN
F 1 "3v3" H 4600 4040 50  0000 C CNN
F 2 "" H 4600 3950 60  0000 C CNN
F 3 "" H 4600 3950 60  0000 C CNN
	1    4600 3950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4700 4150 4700 4200
Connection ~ 4700 4200
$Comp
L R R?
U 1 1 56D5D5A6
P 4500 4050
F 0 "R?" H 4550 4100 50  0000 C CNN
F 1 "4k7" H 4550 4000 50  0000 C CNN
F 2 "agg:0402" H 4500 4050 50  0001 C CNN
F 3 "" H 4500 4050 50  0001 C CNN
F 4 "2447187" H 4500 4050 60  0001 C CNN "Farnell"
	1    4500 4050
	0    1    1    0   
$EndComp
Wire Wire Line
	4500 4000 4700 4000
Wire Wire Line
	4500 4000 4500 4050
Wire Wire Line
	4500 4150 4500 4300
Connection ~ 4500 4300
Wire Wire Line
	4700 4000 4700 4050
Wire Wire Line
	4600 3950 4600 4000
Connection ~ 4600 4000
Text Label 850  2850 2    60   ~ 0
SCL
Text Label 850  2750 2    60   ~ 0
SDA
Wire Wire Line
	850  2750 950  2750
Wire Wire Line
	950  2850 850  2850
Text Label 3850 2850 2    60   ~ 0
SCL
Text Label 3850 2750 2    60   ~ 0
SDA
Wire Wire Line
	3850 2750 3950 2750
Wire Wire Line
	3950 2850 3850 2850
$Sheet
S 900  4250 1250 550 
U 56D55B4D
F0 "Regulator 4" 60
F1 "file56D55B4C.sch" 60
F2 "SDA" B L 900 4600 60 
F3 "SCL" B L 900 4700 60 
F4 "VOUT1" U R 2150 4450 60 
F5 "VOUT0" U R 2150 4350 60 
F6 "~SMB_ALERT" O L 900 4350 60 
$EndSheet
Text Label 800  5950 2    60   ~ 0
SCL
Text Label 800  5850 2    60   ~ 0
SDA
Wire Wire Line
	800  5850 900  5850
Wire Wire Line
	900  5950 800  5950
Text Label 800  4700 2    60   ~ 0
SCL
Text Label 800  4600 2    60   ~ 0
SDA
Wire Wire Line
	800  4600 900  4600
Wire Wire Line
	900  4700 800  4700
Text HLabel 4650 5850 0    60   UnSpc ~ 0
VCC
$Comp
L VCC #PWR?
U 1 1 56DE6AB6
P 4800 5850
F 0 "#PWR?" H 4800 5960 50  0001 L CNN
F 1 "VCC" H 4800 5940 50  0000 C CNN
F 2 "" H 4800 5850 60  0000 C CNN
F 3 "" H 4800 5850 60  0000 C CNN
	1    4800 5850
	0    1    1    0   
$EndComp
Wire Wire Line
	4650 5850 4800 5850
$EndSCHEMATC
