cc -Wall -O3 prog2/main.c prog2/bitonicSort.c prog2/distributor.c prog2/sharedMemory.c prog2/worker.c -o a.o -lpthread -lm
shift 5
./a.o $@
