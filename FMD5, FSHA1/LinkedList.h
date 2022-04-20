#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<time.h>
#include<pwd.h>
#include<unistd.h>
#include"FileInfo.h"
#include <utime.h> 
#include <errno.h> 
#include <fcntl.h>



typedef struct Node {
	Fileinfo file;
	struct Node* nextNode;	//for next set
	struct Node* nextSameNode; //for same set
} Node;



void createList(Fileinfo f);
void addList(Fileinfo f);
char* addComma(long n);
int printList();
void funcD(int setIdx, int listIdx);
void funcI(int setIdx);
void funcF(int setIdx);
int moveToTrash(char* path);
void freeList();

#endif
