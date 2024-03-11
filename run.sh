cc -Wall -O3 $1 -o a.o -lm
shift
./a.o $@
