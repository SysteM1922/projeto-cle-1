cc -Wall -O3 main.c bitonicSort.c distributor.c sharedMemory.c worker.c -o a.o -lpthread -lm
shift 5
./a.o $@
