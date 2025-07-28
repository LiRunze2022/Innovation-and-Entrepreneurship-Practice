CC = g++ -std=c++17 -O3

all: sm4 sm4_optimize
sm4: sm4.o
	$(CC) sm4.o -o sm4

sm4_optimize: sm4_optimize.o
	$(CC) sm4_optimize.o -o sm4_optimize

sm4.o: sm4.cpp sm4.h
	$(CC) -c sm4.cpp -o sm4.o

sm4_optimize.o: sm4_optimize.cpp sm4_optimize.h
	$(CC) -c sm4_optimize.cpp -o sm4_optimize.o

.PHONY: clean

clean:
	rm -rf *.o sm4 sm4_optimize


	
