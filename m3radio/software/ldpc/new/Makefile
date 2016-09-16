all:
	mkdir -p build
	gcc -std=c99 -g -march=native -O3 -fno-omit-frame-pointer -Wall -Werror -Wextra ldpc_codes.c ldpc_encoder.c ldpc_decoder.c test.c -lm -o build/test
	gcc -std=c99 -g -march=native -O3 -fno-omit-frame-pointer -fopenmp -Wall -Werror -Wextra ldpc_codes.c ldpc_encoder.c ldpc_decoder.c bench.c -lm -o build/bench

debug:
	gcc -std=c99 -g -march=native -O0 -fno-omit-frame-pointer -Wall -Werror -Wextra ldpc_codes.c ldpc_encoder.c ldpc_decoder.c test.c -lm -o build/test
	gdb ./build/test

profile:
	gcc -std=c99 -g -march=native -O3 -fno-omit-frame-pointer -Wall -Werror -Wextra ldpc_codes.c ldpc_encoder.c ldpc_decoder.c test.c -lm -o build/test
	perf record -g -- ./build/test
	perf script | gprof2dot -f perf | dot -Tpng -o build/profile.png
