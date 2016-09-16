all:
	mkdir -p build
	gcc -std=c99 -g -march=native -O3 -DNDEBUG -Wall -Wextra -Werror -c ldpc_decoder.c -o build/ldpc_decoder.o
	gcc -std=c99 -g -march=native -O3 -DNDEBUG -Wall -Wextra -Werror -c ldpc_encoder.c -o build/ldpc_encoder.o
	gcc -std=c99 -g -march=native -O3 -DNDEBUG -Wall -Wextra -Werror -c ldpc_codes.c -o build/ldpc_codes.o
	ar -rcs build/libldpc.a build/ldpc_decoder.o build/ldpc_encoder.o build/ldpc_codes.o
	gcc -std=c99 -g -march=native -O3 -Wall -Wextra -Werror -L build test.c -l ldpc -lm -o build/test
	gcc -std=c99 -g -march=native -O3 -Wall -Wextra -Werror -fopenmp -L build ber_trials.c -lldpc -lm -o build/ber_trials
	gcc -g -march=native -O3 -Wall -Wextra -Werror -fopenmp -L build throughput_trials.c -lldpc -lm -o build/throughput_trials
