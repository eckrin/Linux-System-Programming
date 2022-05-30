all : ssu_sfinder

ssu_sfinder : ssu_sfinder.o
	gcc -o ssu_sfinder ssu_sfinder.o -lcrypto -lpthread

ssu_sfinder.o : ssu_sfinder.c
	gcc -c -o ssu_sfinder.o ssu_sfinder.c -lcrypto -lpthread
