CC=GCC
CFLAGS=-Wall

tcwebngin:main.o
main.o:main.c client.o server.o
clean:
	@rm -rf tcwebngin *.o