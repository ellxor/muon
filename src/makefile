CFLAGS=-O3 -march=native -flto -Wl,-O1

main:
	clang -o main $(CFLAGS) main.c -fprofile-generate
	./main
	llvm-profdata merge *.profraw -o default.profdata
	clang -o main $(CFLAGS) main.c -fprofile-use -g
	strip main
