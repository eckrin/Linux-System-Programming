all : ssu_sdup md5 sha1 help

ssu_sdup : ssu_sdup.o
	gcc -o ssu_sdup ssu_sdup.o

md5 : ssu_find-md5.o LinkedList.o Queue.o
	gcc -o md5 ssu_find-md5.o LinkedList.o Queue.o -lcrypto

sha1 : ssu_find-sha1.o LinkedList.o Queue.o
	gcc -o sha1 ssu_find-sha1.o LinkedList.o Queue.o -lcrypto

help : ssu_help.o
	gcc -o help ssu_help.o




ssu_sdup.o : ssu_sdup.c
	gcc -c -o ssu_sdup.o ssu_sdup.c

LinkedList.o : LinkedList.c
	gcc -c -o LinkedList.o LinkedList.c
	
Queue.o : Queue.c
	gcc -c -o Queue.o Queue.c
	
ssu_help.o : ssu_help.c
	gcc -c -o ssu_help.o ssu_help.c
	
ssu_find-md5.o : ssu_find-md5.c
	gcc -c -o ssu_find-md5.o ssu_find-md5.c -lcrpyto
	
ssu_find-sha1.o : ssu_find-sha1.c
	gcc -c -o ssu_find-sha1.o ssu_find-sha1.c -lcrypto

clean : 
	rm *.o ssu_sdup md5 sha1 help
