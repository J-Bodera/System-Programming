CC = gcc -Wall -lpthread #-lrt
FILE = ex1

all:
	make clean
	make compile
	make run

run:
	./$(FILE)

compile:
	$(CC) $(FILE).c -o $(FILE)

clean:
	rm -f *.o *.so *.a $(FILE)
