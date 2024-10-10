PARENT_DIR := $(realpath ..)
INC_LIB := $(PARENT_DIR)/lib/include
LIB := $(PARENT_DIR)/lib

main: client.c server.c
	gcc -c client.c
	gcc -c package.c
	gcc -o client client.o package.o
	gcc -o server server.c

clean:
	rm -f client server