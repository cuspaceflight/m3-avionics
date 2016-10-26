all:
	gcc -Wall -Werror -Wextra -O0 -fPIC -I . -I ../firmware -I ../../shared/m3status -I ../../shared/m3can -o m3fc_mission.o -c ../firmware/m3fc_mission.c
	gcc -Wall -Werror -Wextra -O0 -fPIC -I . -I ../firmware -I ../../shared/m3status -I ../../shared/m3can -o m3fc_mock.o -c m3fc_mock.c
	gcc -shared -o libm3fc_mission.so m3fc_mission.o m3fc_mock.o
