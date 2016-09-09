all:
	mkdir -p build
	gcc -std=c99 -g -march=native -O3 -fno-omit-frame-pointer -Wall -Werror -Wextra ldpc_codes.c ldpc_encoder.c ldpc_decoder.c test.c -lm -o build/test
