#ifndef HEADERS_H
#define HEADERS_H

#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<time.h>
#include"Queue.h"
#include"LinkedList.h"
#include"FileInfo.h"
#include<errno.h>
#include<pwd.h>




//전역변수
Fileinfo samplefile;
unsigned char hash[NAMEMAX];



//headers
double getByteSize(char* str);
int split(char* string, char* seperator, char* argv[]);
char* getFileExtension(char* ext);


//md5
void search_fmd5(char* extension, double min, double max, char* path);
char* getMD5Hash(char* path);

//sha1
void search_fsha1(char* extension, double min, double max, char* path);
char* getSHA1Hash(char* path);


//help


/**
  * 인자로 들어온 문자열을 실수(바이트)로 변환.
  * @param str: 실수로 변환할 문자열
  * @return 바이트 크기
  *
  **/
double getByteSize(char* str) {

	char copystr[200];
	int i=0;
	int flag=0;
	int dotflag;
	
	if(str[0]=='~')
		return -1;
	
	while(i<strlen(str)) {
		//정수부분
		if(!dotflag) {
			if(str[i]=='k'||str[i]=='K') {
				flag=1;
				break;
			}
			else if(str[i]=='m'||str[i]=='M') {
				flag=2;
				break;
			}
			else if(str[i]=='g'||str[i]=='G') {
				flag=3;
				break;
			}
			else if(str[i]=='.') {
				copystr[i]=str[i];
				dotflag=1;
				continue;
			}
		}
		//소수뒷부분
		else {
			if(str[i]=='k'||str[i]=='K') {
				flag=1;
				break;		
			}
			else if(str[i]=='m'||str[i]=='M') {
				flag=2;
				break;
			}
			else if(str[i]=='g'||str[i]=='G') {
				flag=3;
				break;
			}
		}	
		copystr[i]=str[i];
		i++;
	}
	
	double f = atof(copystr);
	if(flag==1) f*=1000;
	else if(flag==2) f*=1000000;
	else if(flag==3) f*=1000000000;
	return f;
}


// 입력 문자열 토크나이징 함수
int split(char* string, char* seperator, char* argv[]) {
	int argc = 0;
	char* ptr = NULL;

	ptr = strtok(string, seperator);
	while (ptr != NULL) {
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}

	return argc;
}


char* getFileExtension(char* ext) {

	static char buf[PATHMAX] = "";
	char* ptr = NULL;
	
	if(!strcmp(ext, "*"))
		return "all";
 
	ptr = strrchr(ext, '.');
	if (ptr == NULL)
		return "error";
 
	strcpy(buf, ptr+1);
	return buf;
}





#endif
