all:
	mkdir -p build
	gcc -g -march=native -O3 -fno-omit-frame-pointer -Wall -Wextra -Werror -fopenmp ldpc_test.c ldpc_encoder.c ldpc_decoder.c ldpc_parity_check.c ldpc_parity_check_packed.c -lm -o build/ldpc
	gcc -march=native -O3 -DNDEBUG -Wall -Wextra -Werror -c -fpic ldpc_decoder.c -o build/ldpc_decoder.o
	gcc -march=native -O3 -DNDEBUG -Wall -Wextra -Werror -c -fpic ldpc_parity_check.c -o build/ldpc_parity_check.o
	gcc -march=native -shared -o build/libcldpcd.so build/ldpc_decoder.o build/ldpc_parity_check.o -lm
