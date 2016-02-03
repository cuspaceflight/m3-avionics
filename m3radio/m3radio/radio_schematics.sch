EESchema Schematic File Version 2
LIBS:agg-kicad
LIBS:m3radio-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 2
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 1750 1150 0    59   ~ 0
SI4460 (radio) 
Wire Notes Line
	4330 500  4330 4340
Wire Notes Line
	4330 4340 510  4340
Wire Notes Line
	8270 520  8270 2780
Wire Notes Line
	8270 2780 4330 2780
Text Notes 5200 720  0    118  ~ 0
RF amplifier and antenna
Text Notes 5750 1000 0    59   ~ 0
M2PA - radio amplifier\n
Text Notes 1870 710  0    118  ~ 0
Radio
$Comp
L Si4460 IC?
U 1 1 56B20B0C
P 2100 2000
F 0 "IC?" H 1800 2700 50  0000 L CNN
F 1 "Si4460" H 1800 1300 50  0000 L CNN
F 2 "agg:QFN-20-EP-SI" H 1800 1200 50  0001 L CNN
F 3 "" H 1700 2600 50  0001 C CNN
F 4 "2462635" H 1800 1100 50  0001 L CNN "Farnell"
	1    2100 2000
	1    0    0    -1  
$EndComp
$EndSCHEMATC
