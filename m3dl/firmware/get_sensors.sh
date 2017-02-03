arm-none-eabi-gdb --batch --quiet \
                    -ex 'target extended-remote /dev/ttyACM0' \
				    -ex 'monitor version' \
      			    -ex 'monitor jtag_scan' \
                    -ex 'attach 1' \
                    -ex "file build/m3dl.elf" \
                    -ex 'break LTC2983.c:167' \
                    -ex 'run' \
                    -ex 'printf "\n\nTEMPERATURE READINGS:  "' \
                    -ex 'printf "T2=%.1fC ", (double)(((TEMP_1_2[5] << 16) | (TEMP_1_2[6] << 8) | (TEMP_1_2[7])) / 1024)' \
                    -ex 'printf "T4=%.1fC ", (double)(((TEMP_3_4[5] << 16) | (TEMP_3_4[6] << 8) | (TEMP_3_4[7])) / 1024)' \
                    -ex 'printf "T5=%.1fC ", (double)(((TEMP_5_6[1] << 16) | (TEMP_5_6[2] << 8) | (TEMP_5_6[3])) / 1024)' \
                    -ex 'printf "T6=%.1fC ", (double)(((TEMP_5_6[5] << 16) | (TEMP_5_6[6] << 8) | (TEMP_5_6[7])) / 1024)' \
                    -ex 'printf "T7=%.1fC ", (double)(((TEMP_7_8[1] << 16) | (TEMP_7_8[2] << 8) | (TEMP_7_8[3])) / 1024)' \
                    -ex 'printf "T8=%.1fC\n\n", (double)(((TEMP_7_8[5] << 16) | (TEMP_7_8[6] << 8) | (TEMP_7_8[7])) / 1024)' \
                    -ex 'delete 1' \
                    -ex 'break pressure.c:112'  \
                    -ex 'continue' \
                    -ex 'printf "\n\nPRESSURE READINGS:  "' \
                    -ex 'printf "P1=%.1fkPa ", (double)((res[1] << 8) | res[0]) * 1.25'\
                    -ex 'printf "P2=%.1fkPa ", (double)((res[3] << 8) | res[2])  * 1.25'\
                    -ex 'printf "P3=%.1fkPa ", (double)((res[5] << 8) | res[4])  * 1.25'\
                    -ex 'printf "P4=%.1fkPa\n\n", (double)((res[7] << 8) | res[6])  * 1.25'\
                    

