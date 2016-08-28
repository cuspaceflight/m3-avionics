/* EZRadioPro Definitions ===================================================*/


/* All constants here are from the EZRadioPRO_REVC2/Si4460/revC2A documents. */

/* EZRadioPRO API Commands --------------------------------------------------*/
#define EZRP_POWER_UP               0x02

#define EZRP_NOP                    0x00
#define EZRP_PART_INFO              0x01
#define EZRP_FUNC_INFO              0x10
#define EZRP_SET_PROPERTY           0x11
#define EZRP_GET_PROPERTY           0x12
#define EZRP_GPIO_PIN_CFG           0x13
#define EZRP_FIFO_INFO              0x15
#define EZRP_GET_INT_STATUS         0x20
#define EZRP_REQUEST_DEVICE_STATE   0x33
#define EZRP_CHANGE_STATE           0x34
#define EZRP_READ_CMD_BUFF          0x44
#define EZRP_FRR_A_READ             0x50
#define EZRP_FRR_B_READ             0x51
#define EZRP_FRR_C_READ             0x53
#define EZRP_FRR_D_READ             0x57

#define EZRP_IRCAL                  0x17
#define EZRP_IRCAL_MANUAL           0x1A

#define EZRP_START_TX               0x31
#define EZRP_TX_HOP                 0x37
#define EZRP_WRITE_TX_FIFO          0x66

#define EZRP_PACKET_INFO            0x16
#define EZRP_GET_MODEM_STATUS       0x22
#define EZRP_START_RX               0x32
#define EZRP_RX_HOP                 0x36
#define EZRP_READ_RX_FIFO           0x77

#define EZRP_GET_ADC_READING        0x14
#define EZRP_GET_PH_STATUS          0x21
#define EZRP_GET_CHIP_STATUS        0x23


/* EZRadioPro Property Groups -----------------------------------------------*/
#define EZRP_PROP_GLOBAL            0x00
#define EZRP_PROP_INT_CTL           0x01
#define EZRP_PROP_FRR_CTL           0x02
#define EZRP_PROP_PREAMBLE          0x10
#define EZRP_PROP_SYNC              0x11
#define EZRP_PROP_PKT               0x12
#define EZRP_PROP_MODEM             0x20
#define EZRP_PROP_MODEM_CHFLT       0x21
#define EZRP_PROP_PA                0x22
#define EZRP_PROP_SYNTH             0x23
#define EZRP_PROP_MATCH             0x30
#define EZRP_PROP_FREQ_CONTROL      0x40
#define EZRP_PROP_RX_HOP            0x50
#define EZRP_PROP_PTI               0xF0


/* EZRadioPro Property Numbers ----------------------------------------------*/
#define EZRP_PROP_GLOBAL_XO_TUNE                    0x00
#define EZRP_PROP_GLOBAL_CLK_CFG                    0x01
#define EZRP_PROP_GLOBAL_LOW_BATT_THRESH            0x02
#define EZRP_PROP_GLOBAL_CONFIG                     0x03
#define EZRP_PROP_GLOBAL_WUT_CONFIG                 0x04
#define EZRP_PROP_GLOBAL_WUT_M0                     0x05
#define EZRP_PROP_GLOBAL_WUT_M1                     0x06
#define EZRP_PROP_GLOBAL_WUT_R                      0x07
#define EZRP_PROP_GLOBAL_WUT_LDC                    0x08
#define EZRP_PROP_GLOBAL_WUT_CAL                    0x09

#define EZRP_PROP_INT_CTL_ENABLE                    0x00
#define EZRP_PROP_INT_CTL_PH_ENABLE                 0x01
#define EZRP_PROP_INT_CTL_MODEM_ENABLE              0x02
#define EZRP_PROP_INT_CTL_CHIP_ENABLE               0x03

#define EZRP_PROP_FRR_CTL_A_MODE                    0x00
#define EZRP_PROP_FRR_CTL_B_MODE                    0x01
#define EZRP_PROP_FRR_CTL_C_MODE                    0x02
#define EZRP_PROP_FRR_CTL_D_MODE                    0x03

#define EZRP_PROP_PREAMBLE_TX_LENGTH                0x00
#define EZRP_PROP_PREAMBLE_CONFIG_STD_1             0x01
#define EZRP_PROP_PREAMBLE_CONFIG_NSTD              0x02
#define EZRP_PROP_PREAMBLE_CONFIG_STD_2             0x03
#define EZRP_PROP_PREAMBLE_CONFIG                   0x04
#define EZRP_PROP_PREAMBLE_PATTERN0                 0x05
#define EZRP_PROP_PREAMBLE_PATTERN1                 0x06
#define EZRP_PROP_PREAMBLE_PATTERN2                 0x07
#define EZRP_PROP_PREAMBLE_PATTERN3                 0x08
#define EZRP_PROP_PREAMBLE_POSTAMBLE_CONFIG         0x09
#define EZRP_PROP_PREAMBLE_POSTAMBLE_PATTERN0       0x0a
#define EZRP_PROP_PREAMBLE_POSTAMBLE_PATTERN1       0x0b
#define EZRP_PROP_PREAMBLE_POSTAMBLE_PATTERN2       0x0c
#define EZRP_PROP_PREAMBLE_POSTAMBLE_PATTERN3       0x0d

#define EZRP_PROP_SYNC_CONFIG                       0x00
#define EZRP_PROP_SYNC_BITS0                        0x01
#define EZRP_PROP_SYNC_BITS1                        0x02
#define EZRP_PROP_SYNC_BITS2                        0x03
#define EZRP_PROP_SYNC_BITS3                        0x04
#define EZRP_PROP_SYNC_CONFIG2                      0x05

#define EZRP_PROP_PKT_CRC_CONFIG                    0x00
#define EZRP_PROP_PKT_WHT_POLY0                     0x01
#define EZRP_PROP_PKT_WHT_POLY1                     0x02
#define EZRP_PROP_PKT_WHT_SEED0                     0x03
#define EZRP_PROP_PKT_WHT_SEED1                     0x04
#define EZRP_PROP_PKT_WHT_BIT_NUM                   0x05
#define EZRP_PROP_PKT_CONFIG1                       0x06
#define EZRP_PROP_PKT_CONFIG2                       0x07
#define EZRP_PROP_PKT_LEN                           0x08
#define EZRP_PROP_PKT_LEN_FIELD_SOURCE              0x09
#define EZRP_PROP_PKT_LEN_ADJUST                    0x0a
#define EZRP_PROP_PKT_TX_THRESHOLD                  0x0b
#define EZRP_PROP_PKT_RX_THRESHOLD                  0x0c
#define EZRP_PROP_PKT_FIELD_1_LENGTH0               0x0d
#define EZRP_PROP_PKT_FIELD_1_LENGTH1               0x0e
#define EZRP_PROP_PKT_FIELD_1_CONFIG                0x0f
#define EZRP_PROP_PKT_FIELD_1_CRC_CONFIG            0x10
#define EZRP_PROP_PKT_FIELD_2_LENGTH0               0x11
#define EZRP_PROP_PKT_FIELD_2_LENGTH1               0x12
#define EZRP_PROP_PKT_FIELD_2_CONFIG                0x13
#define EZRP_PROP_PKT_FIELD_2_CRC_CONFIG            0x14
#define EZRP_PROP_PKT_FIELD_3_LENGTH0               0x15
#define EZRP_PROP_PKT_FIELD_3_LENGTH1               0x16
#define EZRP_PROP_PKT_FIELD_3_CONFIG                0x17
#define EZRP_PROP_PKT_FIELD_3_CRC_CONFIG            0x18
#define EZRP_PROP_PKT_FIELD_4_LENGTH0               0x19
#define EZRP_PROP_PKT_FIELD_4_LENGTH1               0x1a
#define EZRP_PROP_PKT_FIELD_4_CONFIG                0x1b
#define EZRP_PROP_PKT_FIELD_4_CRC_CONFIG            0x1c
#define EZRP_PROP_PKT_FIELD_5_LENGTH0               0x1d
#define EZRP_PROP_PKT_FIELD_5_LENGTH1               0x1e
#define EZRP_PROP_PKT_FIELD_5_CONFIG                0x1f
#define EZRP_PROP_PKT_FIELD_5_CRC_CONFIG            0x20
#define EZRP_PROP_PKT_RX_FIELD_1_LENGTH0            0x21
#define EZRP_PROP_PKT_RX_FIELD_1_LENGTH1            0x22
#define EZRP_PROP_PKT_RX_FIELD_1_CONFIG             0x23
#define EZRP_PROP_PKT_RX_FIELD_1_CRC_CONFIG         0x24
#define EZRP_PROP_PKT_RX_FIELD_2_LENGTH0            0x25
#define EZRP_PROP_PKT_RX_FIELD_2_LENGTH1            0x26
#define EZRP_PROP_PKT_RX_FIELD_2_CONFIG             0x27
#define EZRP_PROP_PKT_RX_FIELD_2_CRC_CONFIG         0x28
#define EZRP_PROP_PKT_RX_FIELD_3_LENGTH0            0x29
#define EZRP_PROP_PKT_RX_FIELD_3_LENGTH1            0x2a
#define EZRP_PROP_PKT_RX_FIELD_3_CONFIG             0x2b
#define EZRP_PROP_PKT_RX_FIELD_3_CRC_CONFIG         0x2c
#define EZRP_PROP_PKT_RX_FIELD_4_LENGTH0            0x2d
#define EZRP_PROP_PKT_RX_FIELD_4_LENGTH1            0x2e
#define EZRP_PROP_PKT_RX_FIELD_4_CONFIG             0x2f
#define EZRP_PROP_PKT_RX_FIELD_4_CRC_CONFIG         0x30
#define EZRP_PROP_PKT_RX_FIELD_5_LENGTH0            0x31
#define EZRP_PROP_PKT_RX_FIELD_5_LENGTH1            0x32
#define EZRP_PROP_PKT_RX_FIELD_5_CONFIG             0x33
#define EZRP_PROP_PKT_RX_FIELD_5_CRC_CONFIG         0x34
#define EZRP_PROP_PKT_CRC_SEED0                     0x36
#define EZRP_PROP_PKT_CRC_SEED1                     0x37
#define EZRP_PROP_PKT_CRC_SEED2                     0x38
#define EZRP_PROP_PKT_CRC_SEED3                     0x39

#define EZRP_PROP_MODEM_MOD_TYPE                    0x00
#define EZRP_PROP_MODEM_MAP_CONTROL                 0x01
#define EZRP_PROP_MODEM_DSM_CTRL                    0x02
#define EZRP_PROP_MODEM_DATA_RATE0                  0x03
#define EZRP_PROP_MODEM_DATA_RATE1                  0x04
#define EZRP_PROP_MODEM_DATA_RATE2                  0x05
#define EZRP_PROP_MODEM_TX_NCO_MODE0                0x06
#define EZRP_PROP_MODEM_TX_NCO_MODE1                0x07
#define EZRP_PROP_MODEM_TX_NCO_MODE2                0x08
#define EZRP_PROP_MODEM_TX_NCO_MODE3                0x09
#define EZRP_PROP_MODEM_FREQ_DEV0                   0x0a
#define EZRP_PROP_MODEM_FREQ_DEV1                   0x0b
#define EZRP_PROP_MODEM_FREQ_DEV2                   0x0c
#define EZRP_PROP_MODEM_FREQ_OFFSET0                0x0d
#define EZRP_PROP_MODEM_FREQ_OFFSET1                0x0e
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_8           0x0f
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_7           0x10
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_6           0x11
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_5           0x12
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_4           0x13
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_3           0x14
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_2           0x15
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_1           0x16
#define EZRP_PROP_MODEM_TX_FILTER_COEFF_0           0x17
#define EZRP_PROP_MODEM_TX_RAMP_DELAY               0x18
#define EZRP_PROP_MODEM_MDM_CTRL                    0x19
#define EZRP_PROP_MODEM_IF_CONTROL                  0x1a
#define EZRP_PROP_MODEM_IF_FREQ0                    0x1b
#define EZRP_PROP_MODEM_IF_FREQ1                    0x1c
#define EZRP_PROP_MODEM_IF_FREQ2                    0x1d
#define EZRP_PROP_MODEM_DECIMATION_CFG1             0x1e
#define EZRP_PROP_MODEM_DECIMATION_CFG0             0x1f
#define EZRP_PROP_MODEM_DECIMATION_CFG2             0x20
#define EZRP_PROP_MODEM_IFPKD_THRESHOLDS            0x21
#define EZRP_PROP_MODEM_BCR_OSR0                    0x22
#define EZRP_PROP_MODEM_BCR_OSR1                    0x23
#define EZRP_PROP_MODEM_BCR_NCO_OFFSET0             0x24
#define EZRP_PROP_MODEM_BCR_NCO_OFFSET1             0x25
#define EZRP_PROP_MODEM_BCR_NCO_OFFSET2             0x26
#define EZRP_PROP_MODEM_BCR_GAIN0                   0x27
#define EZRP_PROP_MODEM_BCR_GAIN1                   0x28
#define EZRP_PROP_MODEM_BCR_GEAR                    0x29
#define EZRP_PROP_MODEM_BCR_MISC1                   0x2a
#define EZRP_PROP_MODEM_BCR_MISC0                   0x2b
#define EZRP_PROP_MODEM_AFC_GEAR                    0x2c
#define EZRP_PROP_MODEM_AFC_WAIT                    0x2d
#define EZRP_PROP_MODEM_AFC_GAIN0                   0x2e
#define EZRP_PROP_MODEM_AFC_GAIN1                   0x2f
#define EZRP_PROP_MODEM_AFC_LIMITER0                0x30
#define EZRP_PROP_MODEM_AFC_LIMITER1                0x31
#define EZRP_PROP_MODEM_AFC_MISC                    0x32
#define EZRP_PROP_MODEM_AFC_ZIFOFF                  0x33
#define EZRP_PROP_MODEM_ADC_CTRL                    0x34
#define EZRP_PROP_MODEM_AGC_CONTROL                 0x35
#define EZRP_PROP_MODEM_AGC_WINDOW_SIZE             0x38
#define EZRP_PROP_MODEM_AGC_RFPD_DECAY              0x39
#define EZRP_PROP_MODEM_AGC_IFPD_DECAY              0x3a
#define EZRP_PROP_MODEM_FSK4_GAIN1                  0x3b
#define EZRP_PROP_MODEM_FSK4_GAIN0                  0x3c
#define EZRP_PROP_MODEM_FSK4_TH0                    0x3d
#define EZRP_PROP_MODEM_FSK4_TH1                    0x3e
#define EZRP_PROP_MODEM_FSK4_MAP                    0x3f
#define EZRP_PROP_MODEM_OOK_PDTC                    0x40
#define EZRP_PROP_MODEM_OOK_BLOPK                   0x41
#define EZRP_PROP_MODEM_OOK_CNT1                    0x42
#define EZRP_PROP_MODEM_OOK_MISC                    0x43
#define EZRP_PROP_MODEM_RAW_CONTROL                 0x45
#define EZRP_PROP_MODEM_RAW_EYE0                    0x46
#define EZRP_PROP_MODEM_RAW_EYE1                    0x47
#define EZRP_PROP_MODEM_ANT_DIV_MODE                0x48
#define EZRP_PROP_MODEM_ANT_DIV_CONTROL             0x49
#define EZRP_PROP_MODEM_RSSI_THRESH                 0x4a
#define EZRP_PROP_MODEM_RSSI_JUMP_THRESH            0x4b
#define EZRP_PROP_MODEM_RSSI_CONTROL                0x4c
#define EZRP_PROP_MODEM_RSSI_CONTROL2               0x4d
#define EZRP_PROP_MODEM_RSSI_COMP                   0x4e
#define EZRP_PROP_MODEM_RAW_SEARCH2                 0x50
#define EZRP_PROP_MODEM_CLKGEN_BAND                 0x51
#define EZRP_PROP_MODEM_SPIKE_DET                   0x54
#define EZRP_PROP_MODEM_ONE_SHOT_AFC                0x55
#define EZRP_PROP_MODEM_RSSI_HYSTERESIS             0x56
#define EZRP_PROP_MODEM_RSSI_MUTE                   0x57
#define EZRP_PROP_MODEM_FAST_RSSI_DELAY             0x58
#define EZRP_PROP_MODEM_PSM0                        0x59
#define EZRP_PROP_MODEM_PSM1                        0x5a
#define EZRP_PROP_MODEM_DSA_CTRL1                   0x5b
#define EZRP_PROP_MODEM_DSA_CTRL2                   0x5c
#define EZRP_PROP_MODEM_DSA_QUAL                    0x5d
#define EZRP_PROP_MODEM_DSA_RSSI                    0x5e
#define EZRP_PROP_MODEM_DSA_MISC                    0x5f

#define EZRP_PROP_PA_MODE                           0x00
#define EZRP_PROP_PA_PWR_LVL                        0x01
#define EZRP_PROP_PA_BIAS_CLKDUTY                   0x02
#define EZRP_PROP_PA_TC                             0x03
#define EZRP_PROP_PA_RAMP_EX                        0x04
#define EZRP_PROP_PA_RAMP_DOWN_DELAY                0x05
#define EZRP_PROP_PA_DIG_PWR_SEQ_CONFIG             0x06

#define EZRP_PROP_SYNTH_PFDCP_CPFF                  0x00
#define EZRP_PROP_SYNTH_PFDCP_CPINT                 0x01
#define EZRP_PROP_SYNTH_VCO_KV                      0x02
#define EZRP_PROP_SYNTH_LPFILT3                     0x03
#define EZRP_PROP_SYNTH_LPFILT2                     0x04
#define EZRP_PROP_SYNTH_LPFILT1                     0x05
#define EZRP_PROP_SYNTH_LPFILT0                     0x06
#define EZRP_PROP_SYNTH_VCO_KVCAL                   0x07

#define EZRP_PROP_MATCH_VALUE_1                     0x00
#define EZRP_PROP_MATCH_MASK_1                      0x01
#define EZRP_PROP_MATCH_CTRL_1                      0x02
#define EZRP_PROP_MATCH_VALUE_2                     0x03
#define EZRP_PROP_MATCH_MASK_2                      0x04
#define EZRP_PROP_MATCH_CTRL_2                      0x05
#define EZRP_PROP_MATCH_VALUE_3                     0x06
#define EZRP_PROP_MATCH_MASK_3                      0x07
#define EZRP_PROP_MATCH_CTRL_3                      0x08
#define EZRP_PROP_MATCH_VALUE_4                     0x09
#define EZRP_PROP_MATCH_MASK_4                      0x0a
#define EZRP_PROP_MATCH_CTRL_4                      0x0b

#define EZRP_PROP_FREQ_CONTROL_INTE                 0x00
#define EZRP_PROP_FREQ_CONTROL_FRAC0                0x01
#define EZRP_PROP_FREQ_CONTROL_FRAC1                0x02
#define EZRP_PROP_FREQ_CONTROL_FRAC2                0x03
#define EZRP_PROP_FREQ_CONTROL_CHANNEL_STEP_SIZE0   0x04
#define EZRP_PROP_FREQ_CONTROL_CHANNEL_STEP_SIZE1   0x05
#define EZRP_PROP_FREQ_CONTROL_W_SIZE               0x06
#define EZRP_PROP_FREQ_CONTROL_VCOCNT_RX_ADJ        0x07

#define EZRP_PROP_RX_HOP_CONTROL                    0x00
#define EZRP_PROP_RX_HOP_TABLE_SIZE                 0x01

#define EZRP_PROP_PTI_CTL                           0x00
#define EZRP_PROP_PTI_BAUD0                         0x01
#define EZRP_PROP_PTI_BAUD1                         0x02
#define EZRP_PROP_PTI_LOG_EN                        0x03

/* States -------------------------------------------------------------------*/
#define EZRP_STATE_SLEEP                            0x01
#define EZRP_STATE_SPI_ACTIVE                       0x02
#define EZRP_STATE_READY                            0x03
#define EZRP_STATE_READY2                           0x04
#define EZRP_STATE_TX_TUNE                          0x05
#define EZRP_STATE_RX_TUNE                          0x06
#define EZRP_STATE_TX                               0x07
#define EZRP_STATE_RX                               0x08
