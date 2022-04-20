#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<time.h>
#include<openssl/md5.h>


#define NAMEMAX 255
#define PATHMAX 4096
#define BUFSIZE	1024*16

unsigned char hash[NAMEMAX] = "appple";
char comma_str[100];

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
 
    ptr = strrchr(ext, '.');
    if (ptr == NULL)
        return NULL;
 
    strcpy(buf, ptr + 1);
 
    return buf;
}

double getByteSize(char* str) {

	char copystr[200];
	int i=0;
	int flag;
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

char* addComma(long n) {

	char str[32];
	int len;
	int c = 0;
	int mod; 
	
	sprintf(str, "%ld", n);
	len = strlen(str);
	mod = len % 3; 
	for(int i = 0; i < len; i++) {
		if(i!=0 && i%3==mod) {
			comma_str[c++] = ','; 
		} 
		comma_str[c++] = str[i];
	} 
	comma_str[c] = 0; 
	return comma_str; 
}

int getCharCount(char* str) {

	int cnt=0;
	for(int i=0; i<strlen(str); i++) {
		if(str[i]=='/') cnt++;
	}
	
	return cnt;
}
	


int main()
{
    rename("/home/youngbin/testingfile/finddir/dididir/movethisfilelel", "/home/youngbin/.local/share/Trash/files/movethisfilelel");
}







