#ifndef FILEINFO_H
#define FILEINFO_H

#include<stdio.h>
#include<stdlib.h>

#define PATHMAX 4096
#define NAMEMAX 255
#define BUFSIZE 1024*16
#define FILEMAX 4096 //탐색 파일의 최대 개수


#define REGFILE 1
#define DIRECTORY 2

int setNum;
int optT;

typedef struct fileinfo {
	char extension[NAMEMAX];
	char path[PATHMAX];
	double size;
	int type;
	char hash[NAMEMAX];
	
} Fileinfo;


#endif
