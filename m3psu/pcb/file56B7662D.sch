EESchema Schematic File Version 2
LIBS:agg-kicad
LIBS:m3psu-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 4 12
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
S 3450 2650 1250 550 
U 56D5BD87
F0 "Regulator 1" 60
F1 "file56D5BD86.sch" 60
F2 "SDA" B L 3450 3000 60 
F3 "SCL" B L 3450 3100 60 
F4 "VOUT1" U R 4700 2850 60 
F5 "VOUT0" U R 4700 2750 60 
F6 "~SMB_ALERT" O L 3450 2750 60 
$EndSheet
Text Notes 3450 1300 0    60   ~ 0
Cameras (5V @ 1A)\nPyro MCU (3.3V @ 0.1A)\nAddress 0x40
Text Notes 3450 2550 0    60   ~ 0
Radio (5V @ 0.35A)\nRadio MCU (3.3V @ 0.25A)\nAddress 0x41
$Sheet
S 3450 1400 1250 550 
U 56B76640
F0 "Regulator 0" 60
F1 "file56B7663F.sch" 60
F2 "SDA" B L 3450 1750 60 
F3 "SCL" B L 3450 1850 60 
F4 "VOUT0" U R 4700 1500 60 
F5 "VOUT1" U R 4700 1600 60 
F6 "~SMB_ALERT" O L 3450 1500 60 
$EndSheet
Text Notes 3100 900  0    120  ~ 0
BOARD 1
Text Notes 6450 1300 0    60   ~ 0
AUX 1 (5V @ 1A)\nCAN transceivers (5V @ 0.1A)\nAddress 0x42
Text Notes 6450 2550 0    60   ~ 0
IMU ADIS (5V @ 0.3A)\nIMU Other (3.3V @ 0.3A)\nAddress 0x43
Text Notes 6100 900  0    120  ~ 0
BOARD 2
$Sheet
S 6450 1400 1250 550 
U 56D37487
F0 "Regulator 2" 60
F1 "file56D37486.sch" 60
F2 "SDA" B L 6450 1750 60 
F3 "SCL" B L 6450 1850 60 
F4 "VOUT1" U R 7700 1600 60 
F5 "VOUT0" U R 7700 1500 60 
F6 "~SMB_ALERT" O L 6450 1500 60 
$EndSheet
$Sheet
S 6450 2650 1250 550 
U 56D4B3D6
F0 "Regulator 3" 60
F1 "file56D4B3D5.sch" 60
F2 "SDA" B L 6450 3000 60 
F3 "SCL" B L 6450 3100 60 
F4 "VOUT1" U R 7700 2850 60 
F5 "VOUT0" U R 7700 2750 60 
F6 "~SMB_ALERT" O L 6450 2750 60 
$EndSheet
Text Notes 3400 4400 0    60   ~ 0
AUX 2 (5V @ 1A)\nFC MCU (3.3V @ 0.15A)\nAddress 0x44
Text Notes 3400 5650 0    60   ~ 0
DL Sensors (5V @ 0.1A)\nDL MCU (3.3V @ 0.3A)\nAddress 0x45
Text Notes 3050 4000 0    120  ~ 0
BOARD 3
$Sheet
S 3400 5750 1250 550 
U 56D569DB
F0 "Regulator 5" 60
F1 "file56D569DA.sch" 60
F2 "SDA" B L 3400 6100 60 
F3 "SCL" B L 3400 6200 60 
F4 "VOUT1" U R 4650 5950 60 
F5 "VOUT0" U R 4650 5850 60 
F6 "~SMB_ALERT" O L 3400 5850 60 
$EndSheet
Text HLabel 7300 4800 2    60   BiDi ~ 0
PMBUS_SDA
Text Label 6850 4800 2    60   ~ 0
SDA
Text HLabel 7300 4900 2    60   BiDi ~ 0
PMBUS_SCL
Text Label 6850 4900 2    60   ~ 0
SCL
Text Label 3350 1850 2    60   ~ 0
SCL
Text Label 3350 1750 2    60   ~ 0
SDA
Text Label 6350 1850 2    60   ~ 0
SCL
Text Label 6350 1750 2    60   ~ 0
SDA
Text Notes 6200 4350 0    120  ~ 0
Off-sheet connections
Text Label 4750 1500 0    60   ~ 0
V_CAMERAS
Text Label 4750 1600 0    60   ~ 0
V_PYRO_MCU
Text Label 4750 2750 0    60   ~ 0
V_RADIO_AMP
Text Label 4750 2850 0    60   ~ 0
V_RADIO_MCU
Text Label 4700 4600 0    60   ~ 0
V_AUX_2
Text Label 4700 4700 0    60   ~ 0
V_FC_MCU
Text Label 4700 5850 0    60   ~ 0
V_DL_SENSORS
Text Label 4700 5950 0    60   ~ 0
V_DL_MCU
Text Label 7750 2750 0    60   ~ 0
V_IMU_ADIS
Text Label 7750 2850 0    60   ~ 0
V_IMU_OTHER
Text Label 7750 1500 0    60   ~ 0
V_AUX_1
Text Label 7750 1600 0    60   ~ 0
V_CAN
Text HLabel 7300 5050 2    60   UnSpc ~ 0
V_CAMERAS
Text HLabel 7300 5150 2    60   UnSpc ~ 0
V_PYRO_MCU
Text HLabel 7300 5250 2    60   UnSpc ~ 0
V_RADIO_AMP
Text HLabel 7300 5350 2    60   UnSpc ~ 0
V_RADIO_MCU
Text HLabel 7300 5500 2    60   UnSpc ~ 0
V_AUX_1
Text HLabel 7300 5600 2    60   UnSpc ~ 0
V_CAN
Text HLabel 7300 5700 2    60   UnSpc ~ 0
V_IMU_ADIS
Text HLabel 7300 5800 2    60   UnSpc ~ 0
V_IMU_OTHER
Text HLabel 7300 5950 2    60   UnSpc ~ 0
V_AUX_2
Text HLabel 7300 6050 2    60   UnSpc ~ 0
V_FC_MCU
Text HLabel 7300 6150 2    60   UnSpc ~ 0
V_DL_SENSORS
Text HLabel 7300 6250 2    60   UnSpc ~ 0
V_DL_MCU
Wire Wire Line
	3350 1750 3450 1750
Wire Wire Line
	3450 1850 3350 1850
Wire Notes Line
	3100 950  5450 950 
Wire Notes Line
	5450 950  5450 3450
Wire Notes Line
	5450 3450 3100 3450
Wire Notes Line
	3100 3450 3100 950 
Wire Wire Line
	6350 1750 6450 1750
Wire Wire Line
	6450 1850 6350 1850
Wire Notes Line
	6100 950  8450 950 
Wire Notes Line
	8450 950  8450 3450
Wire Notes Line
	8450 3450 6100 3450
Wire Notes Line
	6100 3450 6100 950 
Wire Notes Line
	3050 4050 5450 4050
Wire Notes Line
	5450 4050 5450 6550
Wire Notes Line
	5450 6550 3050 6550
Wire Notes Line
	3050 6550 3050 4050
Wire Wire Line
	6850 4800 7300 4800
Wire Wire Line
	6850 4900 7300 4900
Wire Notes Line
	6400 4400 8150 4400
Wire Notes Line
	8150 4400 8150 6350
Wire Notes Line
	8150 6350 6400 6350
Wire Notes Line
	6400 6350 6400 4400
Wire Wire Line
	4700 1500 4750 1500
Wire Wire Line
	4700 1600 4750 1600
Wire Wire Line
	4750 2850 4700 2850
Wire Wire Line
	4750 2750 4700 2750
Wire Wire Line
	4650 4600 4700 4600
Wire Wire Line
	4650 4700 4700 4700
Wire Wire Line
	4650 5850 4700 5850
Wire Wire Line
	4650 5950 4700 5950
Wire Wire Line
	7700 1500 7750 1500
Wire Wire Line
	7700 1600 7750 1600
Wire Wire Line
	7700 2750 7750 2750
Wire Wire Line
	7700 2850 7750 2850
Wire Wire Line
	7300 5050 7150 5050
Wire Wire Line
	7150 5150 7300 5150
Wire Wire Line
	7300 5250 7150 5250
Wire Wire Line
	7300 5350 7150 5350
Wire Wire Line
	7300 5500 7150 5500
Wire Wire Line
	7300 5600 7150 5600
Wire Wire Line
	7150 5700 7300 5700
Wire Wire Line
	7300 5800 7150 5800
Wire Wire Line
	7150 5950 7300 5950
Wire Wire Line
	7300 6050 7150 6050
Wire Wire Line
	7150 6150 7300 6150
Wire Wire Line
	7150 6250 7300 6250
Text Label 7150 5050 2    60   ~ 0
V_CAMERAS
Text Label 7150 5150 2    60   ~ 0
V_PYRO_MCU
Text Label 7150 5250 2    60   ~ 0
V_RADIO_AMP
Text Label 7150 5350 2    60   ~ 0
V_RADIO_MCU
Text Label 7150 5500 2    60   ~ 0
V_AUX_1
Text Label 7150 5600 2    60   ~ 0
V_CAN
Text Label 7150 5700 2    60   ~ 0
V_IMU_ADIS
Text Label 7150 5800 2    60   ~ 0
V_IMU_OTHER
Text Label 7150 5950 2    60   ~ 0
V_AUX_2
Text Label 7150 6050 2    60   ~ 0
V_FC_MCU
Text Label 7150 6150 2    60   ~ 0
V_DL_SENSORS
Text Label 7150 6250 2    60   ~ 0
V_DL_MCU
$Comp
L R R?
U 1 1 56D55B46
P 7200 4650
F 0 "R?" H 7250 4700 50  0000 C CNN
F 1 "4k7" H 7250 4600 50  0000 C CNN
F 2 "agg:0402" H 7200 4650 50  0001 C CNN
F 3 "" H 7200 4650 50  0001 C CNN
F 4 "2447187" H 7200 4650 60  0001 C CNN "Farnell"
	1    7200 4650
	0    1    1    0   
$EndComp
$Comp
L 3v3 #PWR?
U 1 1 56D59DE5
P 7100 4550
F 0 "#PWR?" H 7100 4660 50  0001 L CNN
F 1 "3v3" H 7100 4640 50  0000 C CNN
F 2 "" H 7100 4550 60  0000 C CNN
F 3 "" H 7100 4550 60  0000 C CNN
	1    7100 4550
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 4750 7200 4800
Connection ~ 7200 4800
$Comp
L R R?
U 1 1 56D5D5A6
P 7000 4650
F 0 "R?" H 7050 4700 50  0000 C CNN
F 1 "4k7" H 7050 4600 50  0000 C CNN
F 2 "agg:0402" H 7000 4650 50  0001 C CNN
F 3 "" H 7000 4650 50  0001 C CNN
F 4 "2447187" H 7000 4650 60  0001 C CNN "Farnell"
	1    7000 4650
	0    1    1    0   
$EndComp
Wire Wire Line
	7000 4600 7200 4600
Wire Wire Line
	7000 4600 7000 4650
Wire Wire Line
	7000 4750 7000 4900
Connection ~ 7000 4900
Wire Wire Line
	7200 4600 7200 4650
Wire Wire Line
	7100 4550 7100 4600
Connection ~ 7100 4600
Text Label 3350 3100 2    60   ~ 0
SCL
Text Label 3350 3000 2    60   ~ 0
SDA
Wire Wire Line
	3350 3000 3450 3000
Wire Wire Line
	3450 3100 3350 3100
Text Label 6350 3100 2    60   ~ 0
SCL
Text Label 6350 3000 2    60   ~ 0
SDA
Wire Wire Line
	6350 3000 6450 3000
Wire Wire Line
	6450 3100 6350 3100
$Sheet
S 3400 4500 1250 550 
U 56D55B4D
F0 "Regulator 4" 60
F1 "file56D55B4C.sch" 60
F2 "SDA" B L 3400 4850 60 
F3 "SCL" B L 3400 4950 60 
F4 "VOUT1" U R 4650 4700 60 
F5 "VOUT0" U R 4650 4600 60 
F6 "~SMB_ALERT" O L 3400 4600 60 
$EndSheet
Text Label 3300 6200 2    60   ~ 0
SCL
Text Label 3300 6100 2    60   ~ 0
SDA
Wire Wire Line
	3300 6100 3400 6100
Wire Wire Line
	3400 6200 3300 6200
Text Label 3300 4950 2    60   ~ 0
SCL
Text Label 3300 4850 2    60   ~ 0
SDA
Wire Wire Line
	3300 4850 3400 4850
Wire Wire Line
	3400 4950 3300 4950
$EndSCHEMATC
