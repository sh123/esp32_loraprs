EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L lora_tracker:E22-400M30S U2
U 1 1 6194CACC
P 7600 2850
F 0 "U2" H 7600 4150 85  0000 C CNN
F 1 "E22-400M30S" H 7600 2850 50  0001 C CNN
F 2 "lora_tracker:E22-400M30S" H 7600 2850 50  0001 C CNN
F 3 "" H 7600 2850 50  0001 C CNN
	1    7600 2850
	1    0    0    -1  
$EndComp
Text GLabel 2050 3750 0    50   Input ~ 0
VCC
Text GLabel 5800 3750 0    50   Input ~ 0
VCC
Wire Wire Line
	6600 3750 6500 3750
Wire Wire Line
	6600 3550 6500 3550
Wire Wire Line
	6500 3550 6500 3750
Wire Wire Line
	2250 3750 2050 3750
Text GLabel 5800 3950 0    50   Input ~ 0
GND
Wire Wire Line
	6500 3950 6500 4250
Wire Wire Line
	6500 4250 7150 4250
Wire Wire Line
	8750 4250 8750 3950
Wire Wire Line
	8750 3950 8600 3950
Wire Wire Line
	6500 3950 6600 3950
Text GLabel 6350 2150 0    50   Input ~ 0
GND
Wire Wire Line
	6600 2150 6450 2150
Wire Wire Line
	6600 1750 6450 1750
Wire Wire Line
	6450 1750 6450 1950
Connection ~ 6450 2150
Wire Wire Line
	6450 2150 6350 2150
Wire Wire Line
	6600 1950 6450 1950
Connection ~ 6450 1950
Wire Wire Line
	6450 1950 6450 2150
Wire Wire Line
	6450 1750 6450 1500
Wire Wire Line
	6450 1500 8700 1500
Wire Wire Line
	8700 1500 8700 1750
Wire Wire Line
	8700 1750 8600 1750
Connection ~ 6450 1750
Wire Wire Line
	8600 2150 8700 2150
Wire Wire Line
	8700 2150 8700 1750
Connection ~ 8700 1750
Text GLabel 3400 1950 2    50   Input ~ 0
GND
Text GLabel 850  3250 0    50   Input ~ 0
GND
Wire Wire Line
	8600 1950 8800 1950
Text GLabel 8900 2550 2    50   Input ~ 0
NSS
Text GLabel 8900 2750 2    50   Input ~ 0
SCK
Text GLabel 8900 2950 2    50   Input ~ 0
MOSI
Text GLabel 8900 3150 2    50   Input ~ 0
MISO
Text GLabel 8900 3350 2    50   Input ~ 0
NRST
Text GLabel 8900 3550 2    50   Input ~ 0
BUSY
Text GLabel 8900 3750 2    50   Input ~ 0
DIO1
Wire Wire Line
	8900 2550 8600 2550
Wire Wire Line
	8900 2750 8600 2750
Wire Wire Line
	8600 2950 8900 2950
Wire Wire Line
	8600 3150 8900 3150
Wire Wire Line
	8600 3350 8900 3350
Wire Wire Line
	8600 3550 8900 3550
Wire Wire Line
	8600 3750 8900 3750
Text GLabel 6350 2750 0    50   Input ~ 0
GND
Wire Wire Line
	6350 2750 6450 2750
Wire Wire Line
	6600 2550 6450 2550
Wire Wire Line
	6450 2550 6450 2750
Connection ~ 6450 2750
Wire Wire Line
	6450 2750 6600 2750
Text GLabel 6350 2950 0    50   Input ~ 0
RXEN
Text GLabel 6350 3150 0    50   Input ~ 0
TXEN
Wire Wire Line
	6600 2950 6350 2950
Wire Wire Line
	6600 3150 6350 3150
Text GLabel 3400 2050 2    50   Input ~ 0
MOSI
Wire Wire Line
	3400 2050 3200 2050
Text GLabel 2050 3150 0    50   Input ~ 0
DIO1
Text GLabel 2050 3050 0    50   Input ~ 0
BUSY
Text GLabel 2050 2950 0    50   Input ~ 0
NRST
Wire Wire Line
	2050 3150 2250 3150
Wire Wire Line
	2050 3050 2250 3050
Wire Wire Line
	2050 2950 2250 2950
Text GLabel 2050 2550 0    50   Input ~ 0
RXEN
Text GLabel 2050 2650 0    50   Input ~ 0
TXEN
Wire Wire Line
	2250 2550 2050 2550
Wire Wire Line
	2250 2650 2050 2650
NoConn ~ 6600 3350
$Comp
L Connector:Conn_Coaxial J1
U 1 1 619A9181
P 9000 1950
F 0 "J1" H 9099 1926 50  0000 L CNN
F 1 "Conn_Coaxial" H 9099 1835 50  0000 L CNN
F 2 "Connector_Coaxial:SMA_Samtec_SMA-J-P-H-ST-EM1_EdgeMount" H 9000 1950 50  0001 C CNN
F 3 " ~" H 9000 1950 50  0001 C CNN
	1    9000 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	9000 2150 9000 2300
Wire Wire Line
	9000 2300 8700 2300
Wire Wire Line
	8700 2300 8700 2150
Connection ~ 8700 2150
$Comp
L Jumper:SolderJumper_3_Open JP1
U 1 1 619A9CAA
P 4500 2500
F 0 "JP1" H 4500 2705 50  0000 C CNN
F 1 "SolderJumper_3_Open" H 4500 2614 50  0000 C CNN
F 2 "Jumper:SolderJumper-3_P1.3mm_Open_Pad1.0x1.5mm" H 4500 2500 50  0001 C CNN
F 3 "~" H 4500 2500 50  0001 C CNN
	1    4500 2500
	1    0    0    -1  
$EndComp
Text GLabel 5050 2650 2    50   Input ~ 0
MISO
Text GLabel 5050 2750 2    50   Input ~ 0
SCK
Text GLabel 5050 2850 2    50   Input ~ 0
NSS
$Comp
L Jumper:SolderJumper_3_Open JP2
U 1 1 619AAF54
P 4500 2950
F 0 "JP2" H 4500 3155 50  0000 C CNN
F 1 "SolderJumper_3_Open" H 4500 3064 50  0000 C CNN
F 2 "Jumper:SolderJumper-3_P1.3mm_Open_Pad1.0x1.5mm" H 4500 2950 50  0001 C CNN
F 3 "~" H 4500 2950 50  0001 C CNN
	1    4500 2950
	1    0    0    -1  
$EndComp
$Comp
L Jumper:SolderJumper_3_Open JP3
U 1 1 619AC0BC
P 4500 3400
F 0 "JP3" H 4500 3605 50  0000 C CNN
F 1 "SolderJumper_3_Open" H 4500 3514 50  0000 C CNN
F 2 "Jumper:SolderJumper-3_P1.3mm_Open_Pad1.0x1.5mm" H 4500 3400 50  0001 C CNN
F 3 "~" H 4500 3400 50  0001 C CNN
	1    4500 3400
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 1950 3400 1950
Wire Wire Line
	5050 2650 4500 2650
Wire Wire Line
	5050 2750 4800 2750
Wire Wire Line
	4800 2750 4800 3100
Wire Wire Line
	4800 3100 4500 3100
Wire Wire Line
	5050 2850 4900 2850
Wire Wire Line
	4900 2850 4900 3550
Wire Wire Line
	4900 3550 4500 3550
Wire Wire Line
	4300 3400 3900 3400
Wire Wire Line
	3900 3400 3900 2850
Wire Wire Line
	4300 2950 4150 2950
Wire Wire Line
	4000 2950 4000 2750
Wire Wire Line
	4300 2500 4150 2500
Wire Wire Line
	4000 2500 4000 2650
Wire Wire Line
	4700 3400 4700 3150
Wire Wire Line
	4700 3150 4150 3150
Wire Wire Line
	4150 3150 4150 2950
Connection ~ 4150 2950
Wire Wire Line
	4150 2950 4000 2950
Wire Wire Line
	4700 2950 4700 2750
Wire Wire Line
	4700 2750 4150 2750
Wire Wire Line
	4150 2750 4150 2500
Connection ~ 4150 2500
Wire Wire Line
	4150 2500 4000 2500
Wire Wire Line
	4700 2500 4700 2350
Wire Wire Line
	4700 2350 3900 2350
Wire Wire Line
	3900 2350 3900 2550
Text GLabel 850  3650 0    50   Input ~ 0
GND
$Comp
L Jumper:SolderJumper_2_Open JP4
U 1 1 619C59F2
P 1200 3250
F 0 "JP4" H 1200 3455 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 1200 3364 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 1200 3250 50  0001 C CNN
F 3 "~" H 1200 3250 50  0001 C CNN
	1    1200 3250
	1    0    0    -1  
$EndComp
$Comp
L Jumper:SolderJumper_2_Open JP5
U 1 1 619C9C44
P 1200 3650
F 0 "JP5" H 1200 3855 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 1200 3764 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 1200 3650 50  0001 C CNN
F 3 "~" H 1200 3650 50  0001 C CNN
	1    1200 3650
	1    0    0    -1  
$EndComp
Wire Wire Line
	850  3250 1050 3250
Wire Wire Line
	850  3650 1050 3650
$Comp
L Jumper:SolderJumper_2_Open JP6
U 1 1 619D6D42
P 4500 2150
F 0 "JP6" H 4500 2355 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 4500 2264 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 4500 2150 50  0001 C CNN
F 3 "~" H 4500 2150 50  0001 C CNN
	1    4500 2150
	1    0    0    -1  
$EndComp
Text GLabel 5050 2150 2    50   Input ~ 0
GND
Wire Wire Line
	4650 2150 5050 2150
Wire Wire Line
	4350 2150 3900 2150
Wire Wire Line
	3900 2150 3900 2350
Connection ~ 3900 2350
Wire Wire Line
	5800 3750 5900 3750
Connection ~ 6500 3750
Wire Wire Line
	5800 3950 6500 3950
Connection ~ 6500 3950
$Comp
L Device:CP1 C2
U 1 1 619EF8E3
P 5900 4300
F 0 "C2" H 6015 4346 50  0000 L CNN
F 1 "100uF" H 6015 4255 50  0000 L CNN
F 2 "Capacitor_SMD:CP_Elec_3x5.3" H 5900 4300 50  0001 C CNN
F 3 "~" H 5900 4300 50  0001 C CNN
	1    5900 4300
	1    0    0    -1  
$EndComp
$Comp
L Device:C C1
U 1 1 619EF996
P 6250 4550
F 0 "C1" H 6365 4596 50  0000 L CNN
F 1 "100nF" H 6365 4505 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 6288 4400 50  0001 C CNN
F 3 "~" H 6250 4550 50  0001 C CNN
	1    6250 4550
	1    0    0    -1  
$EndComp
Wire Wire Line
	5900 4150 5900 3750
Connection ~ 5900 3750
Wire Wire Line
	5900 3750 6250 3750
Wire Wire Line
	6250 4400 6250 3750
Connection ~ 6250 3750
Wire Wire Line
	6250 3750 6500 3750
Wire Wire Line
	5900 4450 5900 5000
Wire Wire Line
	5900 5000 6250 5000
Wire Wire Line
	7150 5000 7150 4250
Connection ~ 7150 4250
Wire Wire Line
	7150 4250 8750 4250
Wire Wire Line
	6250 4700 6250 5000
Connection ~ 6250 5000
Wire Wire Line
	6250 5000 7150 5000
$Comp
L RF_Module:Ai-Thinker-Ra-01 U3
U 1 1 61A0738A
P 2650 5850
F 0 "U3" H 2650 6828 50  0000 C CNN
F 1 "Ai-Thinker-Ra-01" H 2650 6737 50  0000 C CNN
F 2 "RF_Module:Ai-Thinker-Ra-01-LoRa" H 3650 5450 50  0001 C CNN
F 3 "https://mikroelectron.com/Product/10KM-433M-LORA-LONG-RANGE-WIRELESS-MODULE-RA-01" H 2750 6550 50  0001 C CNN
	1    2650 5850
	1    0    0    -1  
$EndComp
Text GLabel 3400 6600 2    50   Input ~ 0
GND
Wire Wire Line
	2650 6550 2650 6600
Wire Wire Line
	2650 6600 3200 6600
Text GLabel 3400 4900 2    50   Input ~ 0
3v3
Wire Wire Line
	2650 4900 3250 4900
Text GLabel 850  1950 0    50   Input ~ 0
3v3
Text GLabel 5050 3750 2    50   Input ~ 0
3v3
Wire Wire Line
	2650 4900 2650 5050
Wire Wire Line
	2150 5350 2150 4300
Wire Wire Line
	2150 4300 5300 4300
Wire Wire Line
	5300 4300 5300 1100
Wire Wire Line
	5300 1100 8800 1100
Wire Wire Line
	8800 1100 8800 1950
Connection ~ 8800 1950
Text GLabel 1800 5550 0    50   Input ~ 0
NRST
Wire Wire Line
	1800 5550 2150 5550
Text GLabel 1800 5750 0    50   Input ~ 0
NSS
Text GLabel 1800 5950 0    50   Input ~ 0
MISO
Text GLabel 1800 6050 0    50   Input ~ 0
MOSI
Text GLabel 1800 5850 0    50   Input ~ 0
SCK
Wire Wire Line
	1800 5750 2150 5750
Wire Wire Line
	1800 5850 2150 5850
Wire Wire Line
	1800 5950 2150 5950
Text GLabel 3400 5550 2    50   Input ~ 0
DIO1
Text GLabel 3400 5450 2    50   Input ~ 0
BUSY
Wire Wire Line
	3400 5450 3150 5450
Wire Wire Line
	3400 5550 3150 5550
Wire Wire Line
	1800 6050 2150 6050
$Comp
L Jumper:SolderJumper_2_Open JP8
U 1 1 61A555B2
P 4700 3750
F 0 "JP8" H 4700 3955 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 4700 3864 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 4700 3750 50  0001 C CNN
F 3 "~" H 4700 3750 50  0001 C CNN
	1    4700 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	4850 3750 5050 3750
$Comp
L Jumper:SolderJumper_2_Open JP7
U 1 1 61A74681
P 1150 1950
F 0 "JP7" H 1150 2155 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 1150 2064 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_Pad1.0x1.5mm" H 1150 1950 50  0001 C CNN
F 3 "~" H 1150 1950 50  0001 C CNN
	1    1150 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	850  1950 1000 1950
$Comp
L Device:C C3
U 1 1 61A98B18
P 3250 5150
F 0 "C3" H 3365 5196 50  0000 L CNN
F 1 "100nF" H 3365 5105 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 3288 5000 50  0001 C CNN
F 3 "~" H 3250 5150 50  0001 C CNN
	1    3250 5150
	1    0    0    -1  
$EndComp
Wire Wire Line
	3250 5000 3250 4900
Connection ~ 3250 4900
Wire Wire Line
	3250 4900 3400 4900
Wire Wire Line
	3250 5300 3800 5300
Wire Wire Line
	3800 5300 3800 6300
Wire Wire Line
	3800 6300 3200 6300
Wire Wire Line
	3200 6300 3200 6600
Connection ~ 3200 6600
Wire Wire Line
	3200 6600 3400 6600
$Comp
L Connector:Conn_01x10_Female J2
U 1 1 61AA7D88
P 4750 6600
F 0 "J2" H 4644 5875 50  0000 C CNN
F 1 "Conn_01x10_Female" H 4644 5966 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x10_P2.54mm_Vertical" H 4750 6600 50  0001 C CNN
F 3 "~" H 4750 6600 50  0001 C CNN
	1    4750 6600
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x12_Female J3
U 1 1 61AA8097
P 5800 6700
F 0 "J3" H 5694 5875 50  0000 C CNN
F 1 "Conn_01x12_Female" H 5694 5966 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x12_P2.54mm_Vertical" H 5800 6700 50  0001 C CNN
F 3 "~" H 5800 6700 50  0001 C CNN
	1    5800 6700
	-1   0    0    1   
$EndComp
Text GLabel 2050 2050 0    50   Input ~ 0
EN
Text GLabel 2050 2150 0    50   Input ~ 0
GPIO36
Text GLabel 2050 2250 0    50   Input ~ 0
GPIO39
Text GLabel 2050 2350 0    50   Input ~ 0
GPIO34
Text GLabel 2050 2450 0    50   Input ~ 0
GPIO35
$Comp
L lora_tracker:ESP32 U1
U 1 1 61978D9D
P 2650 1850
F 0 "U1" H 2725 2015 50  0000 C CNN
F 1 "ESP32" H 2725 1924 50  0000 C CNN
F 2 "lora_tracker:esp32-devkit-wide" H 2700 2000 50  0001 C CNN
F 3 "" H 2700 2000 50  0001 C CNN
	1    2650 1850
	1    0    0    -1  
$EndComp
Wire Wire Line
	2050 2050 2250 2050
Wire Wire Line
	2050 2150 2250 2150
Wire Wire Line
	2050 2250 2250 2250
Wire Wire Line
	2050 2350 2250 2350
Wire Wire Line
	2050 2450 2250 2450
Wire Wire Line
	3200 2550 3900 2550
Wire Wire Line
	3200 2650 4000 2650
Wire Wire Line
	3200 2750 4000 2750
Wire Wire Line
	3200 2850 3900 2850
Wire Wire Line
	3200 3750 4550 3750
Text GLabel 2050 2750 0    50   Input ~ 0
GPIO25
Text GLabel 2050 2850 0    50   Input ~ 0
GPIO26
Wire Wire Line
	2050 2750 2250 2750
Wire Wire Line
	2050 2850 2250 2850
Text GLabel 2050 3350 0    50   Input ~ 0
GPIO13
Text GLabel 2050 3450 0    50   Input ~ 0
GPIO9
Text GLabel 2050 3550 0    50   Input ~ 0
GPIO10
Wire Wire Line
	1350 3250 2250 3250
Wire Wire Line
	1350 3650 2250 3650
Wire Wire Line
	2250 3350 2050 3350
Wire Wire Line
	2250 3450 2050 3450
Wire Wire Line
	2250 3550 2050 3550
Text GLabel 5150 6100 2    50   Input ~ 0
EN
Text GLabel 5150 6200 2    50   Input ~ 0
GPIO36
Text GLabel 5150 6300 2    50   Input ~ 0
GPIO39
Text GLabel 5150 6400 2    50   Input ~ 0
GPIO34
Text GLabel 5150 6500 2    50   Input ~ 0
GPIO35
Text GLabel 5150 6600 2    50   Input ~ 0
GPIO25
Text GLabel 5150 6700 2    50   Input ~ 0
GPIO26
Text GLabel 5150 6800 2    50   Input ~ 0
GPIO13
Text GLabel 5150 6900 2    50   Input ~ 0
GPIO9
Text GLabel 5150 7000 2    50   Input ~ 0
GPIO10
Wire Wire Line
	5150 6100 4950 6100
Wire Wire Line
	4950 6200 5150 6200
Wire Wire Line
	4950 6300 5150 6300
Wire Wire Line
	4950 6400 5150 6400
Wire Wire Line
	4950 6500 5150 6500
Wire Wire Line
	4950 6600 5150 6600
Wire Wire Line
	4950 6700 5150 6700
Wire Wire Line
	4950 6800 5150 6800
Wire Wire Line
	4950 6900 5150 6900
Wire Wire Line
	4950 7000 5150 7000
Text GLabel 3400 2150 2    50   Input ~ 0
GPIO22
Text GLabel 3400 2250 2    50   Input ~ 0
GPIO1
Text GLabel 3400 2350 2    50   Input ~ 0
GPIO3
Text GLabel 3400 2450 2    50   Input ~ 0
GPIO21
Text GLabel 3400 2950 2    50   Input ~ 0
GPIO17
Text GLabel 3400 3050 2    50   Input ~ 0
GPIO16
Text GLabel 3400 3150 2    50   Input ~ 0
GPIO4
Text GLabel 3400 3250 2    50   Input ~ 0
GPIO0
Text GLabel 3400 3350 2    50   Input ~ 0
GPIO2
Text GLabel 3400 3450 2    50   Input ~ 0
GPIO15
Text GLabel 3400 3550 2    50   Input ~ 0
GPIO8
Text GLabel 3400 3650 2    50   Input ~ 0
GPIO7
Wire Wire Line
	3200 2150 3400 2150
Wire Wire Line
	3200 2250 3400 2250
Wire Wire Line
	3200 2350 3400 2350
Wire Wire Line
	3200 2450 3400 2450
Wire Wire Line
	3200 2950 3400 2950
Wire Wire Line
	3200 3050 3400 3050
Wire Wire Line
	3200 3150 3400 3150
Wire Wire Line
	3200 3250 3400 3250
Wire Wire Line
	3200 3350 3400 3350
Wire Wire Line
	3200 3450 3400 3450
Wire Wire Line
	3200 3550 3400 3550
Wire Wire Line
	3200 3650 3400 3650
Text GLabel 6200 6100 2    50   Input ~ 0
GPIO22
Text GLabel 6200 6200 2    50   Input ~ 0
GPIO1
Text GLabel 6200 6300 2    50   Input ~ 0
GPIO3
Text GLabel 6200 6400 2    50   Input ~ 0
GPIO21
Wire Wire Line
	6000 6100 6200 6100
Wire Wire Line
	6000 6200 6200 6200
Wire Wire Line
	6000 6300 6200 6300
Wire Wire Line
	6000 6400 6200 6400
Text GLabel 6200 6500 2    50   Input ~ 0
GPIO17
Text GLabel 6200 6600 2    50   Input ~ 0
GPIO16
Text GLabel 6200 6700 2    50   Input ~ 0
GPIO4
Text GLabel 6200 6800 2    50   Input ~ 0
GPIO0
Text GLabel 6200 6900 2    50   Input ~ 0
GPIO2
Text GLabel 6200 7000 2    50   Input ~ 0
GPIO15
Text GLabel 6200 7100 2    50   Input ~ 0
GPIO8
Text GLabel 6200 7200 2    50   Input ~ 0
GPIO7
Wire Wire Line
	6000 6500 6200 6500
Wire Wire Line
	6000 6600 6200 6600
Wire Wire Line
	6000 6700 6200 6700
Wire Wire Line
	6000 6800 6200 6800
Wire Wire Line
	6000 6900 6200 6900
Wire Wire Line
	6000 7000 6200 7000
Wire Wire Line
	6000 7100 6200 7100
Wire Wire Line
	6000 7200 6200 7200
NoConn ~ 3150 5650
NoConn ~ 3150 5750
NoConn ~ 3150 5850
NoConn ~ 3150 5950
$Comp
L Connector:Conn_01x03_Female J4
U 1 1 61D04C33
P 7650 5850
F 0 "J4" H 7544 5525 50  0000 C CNN
F 1 "Conn_01x03_Female" H 7544 5616 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x03_P2.54mm_Vertical" H 7650 5850 50  0001 C CNN
F 3 "~" H 7650 5850 50  0001 C CNN
	1    7650 5850
	-1   0    0    1   
$EndComp
Text GLabel 8150 5750 2    50   Input ~ 0
VCC
Text GLabel 8150 5850 2    50   Input ~ 0
3v3
Text GLabel 8150 5950 2    50   Input ~ 0
GND
Wire Wire Line
	7850 5750 8150 5750
Wire Wire Line
	7850 5850 8150 5850
Wire Wire Line
	7850 5950 8150 5950
Wire Notes Line
	3850 1850 3850 3950
Wire Notes Line
	3850 3950 5000 3950
Wire Notes Line
	5000 3950 5000 1850
Wire Notes Line
	5000 1850 3850 1850
Text Notes 3850 1800 0    50   ~ 0
ESP32 SPI flavours
Wire Wire Line
	1300 1950 2250 1950
Wire Notes Line
	900  3900 1600 3900
Wire Notes Line
	1600 3900 1600 1600
Wire Notes Line
	1600 1600 900  1600
Wire Notes Line
	900  1600 900  3900
Text Notes 950  1550 0    50   ~ 0
ESP32 options
$EndSCHEMATC
