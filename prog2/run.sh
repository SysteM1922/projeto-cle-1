cc -Wall -O3 $1 -o a.o -lpthread -lm
shift
./a.o $@
