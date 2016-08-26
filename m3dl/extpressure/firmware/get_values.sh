arm-none-eabi-gdb --batch --quiet \
                    -ex 'target extended-remote /dev/ttyACM0' \
                    -ex 'file build/extpressurefw.elf' \
                    -ex 'monitor swdp_scan' \
                    -ex 'attach 1' \
                    -ex 'printf "ID=%d T=%dC B=%.02fV A=%+.01fg P=%.01fkPa\n", sp100s[0].id, sp100s[0].temperature - 50, (double)sp100s[0].battery * 0.0184 + 1.73, (double)sp100s[0].acceleration * 0.5 - 12, (double)sp100s[0].pressure * 1.25'\
                    -ex 'printf "ID=%d T=%dC B=%.02fV A=%+.01fg P=%.01fkPa\n", sp100s[1].id, sp100s[1].temperature - 50, (double)sp100s[1].battery * 0.0184 + 1.73, (double)sp100s[1].acceleration * 0.5 - 12, (double)sp100s[1].pressure * 1.25'\
                    -ex 'printf "ID=%d T=%dC B=%.02fV A=%+.01fg P=%.01fkPa\n", sp100s[2].id, sp100s[2].temperature - 50, (double)sp100s[2].battery * 0.0184 + 1.73, (double)sp100s[2].acceleration * 0.5 - 12, (double)sp100s[2].pressure * 1.25'\
                    -ex 'printf "ID=%d T=%dC B=%.02fV A=%+.01fg P=%.01fkPa\n", sp100s[3].id, sp100s[3].temperature - 50, (double)sp100s[3].battery * 0.0184 + 1.73, (double)sp100s[3].acceleration * 0.5 - 12, (double)sp100s[3].pressure * 1.25'\
