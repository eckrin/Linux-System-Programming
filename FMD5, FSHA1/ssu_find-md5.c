#include<openssl/md5.h>
#include"headers.h"


/**
  * ssu_sdup 내장명령어. linked list, queue 사용
  * argv[1]> [FILE_EXTENSION]: 파일 확장자. *으로 들어오면 모든 확장자 처리
  * argv[2]> [MINSIZE]: 탐색 최소크기. ~으로 들어오면 최소크기X
  * argv[3]> [MAXSIZE]: 탐색 최대크기. ~으로 들어오면 최대크기X
  * argv[4]> [TARGET_DIRECTORY]: 탐색 시작 디렉토리. ~은 홈디렉토리로 입력받음
  **/
void main(int argc, char* argv[]) {

	char FILE_EXTENSION[NAMEMAX];
	double minSize;
	double maxSize;
	char TARGET_DIRECTORY[PATHMAX];
	char path[PATHMAX];
	char tmp[PATHMAX];
	struct timeval startTime;
	struct timeval endTime;
	struct passwd *result;
	struct passwd pwd;
	
	char searchingPath[PATHMAX]; //탐색 진행경로
	struct stat statBuf;
	
	if(argc!=5) {
		fprintf(stderr, "command argument error\n");
		return;
	}
	//홈디렉토리(~) 처리
	strcpy(path, argv[4]);
	strcpy(tmp, argv[4]);
	if((result = getpwnam(getlogin()))==NULL) {
		fprintf(stderr, "getpwnam error\n");
		return;
	}
	
	memcpy(&pwd, result, sizeof(struct passwd));
	
	int homeLen = strlen(pwd.pw_dir);
	if(tmp[0]=='~') {		
		for(int i=1; i<PATHMAX-homeLen-1; i++) {
			path[homeLen+i-1] = tmp[i]; // /home/youngbin + /testingfile
		}
		strncpy(path, pwd.pw_dir, homeLen);		
	}
	
	
	//절대경로 변환
	if(realpath(path, TARGET_DIRECTORY) == NULL) {
		fprintf(stderr, "realpath error\n");
		return;
	}
	
	minSize = getByteSize(argv[2]);
	maxSize = getByteSize(argv[3]);
	
	//FILE_EXTENSION
	if(!strcmp(getFileExtension(argv[1]), "error")) return;
	strcpy(FILE_EXTENSION, getFileExtension(argv[1]));
	
	
	if(lstat(TARGET_DIRECTORY, &statBuf)<0) {
		fprintf(stderr, "lstat error\n");
	}
	
	//fmd5
	gettimeofday(&startTime, NULL);
	//search
	search_fmd5(FILE_EXTENSION, minSize, maxSize, TARGET_DIRECTORY);
	//print
	if(printList()==-1) {
		printf("No duplicates in %s\n", TARGET_DIRECTORY);
		gettimeofday(&endTime, NULL);
	
		long sec = endTime.tv_sec-startTime.tv_sec;
		long usec = endTime.tv_usec-startTime.tv_usec;
		if(usec<0) {
			sec--;
			usec+=1000000;
		}
		printf("Searching time: %ld:%ld(sec:usec)\n\n", sec, usec);
		freeList();
		return;
	}
	gettimeofday(&endTime, NULL);
	
	long sec = endTime.tv_sec-startTime.tv_sec;
	long usec = endTime.tv_usec-startTime.tv_usec;
	if(usec<0) {
		sec--;
		usec+=1000000;
	}
	printf("Searching time: %ld:%ld(sec:usec)\n\n", sec, usec);
	
	
	//[INDEX]
	while(1) {
		char input[BUFSIZE];
		char* arr[10];
		int cnt;
		int set_idx;
		int list_idx;
		optT=0;
		
		
		printf("\n>> ");
		fgets(input, sizeof(input), stdin);
		//exit\n 입력시 프롬프트로
		if(!strcmp(input, "exit\n")) {
			printf(">> Back to Prompt\n");
			freeList();
			return;
		}
		
		if((cnt = split(input, " ", arr))<2) {
			fprintf(stderr, "not enough index1\n");
			continue;
		}
		
		if(atoi(arr[0])<0 || atoi(arr[0])>setNum) {
			fprintf(stderr, "[SET_INDEX] error\n");
			continue;
		}
		set_idx = atoi(arr[0]);
		
		//옵션
		if(arr[1][0]=='d') {
			if(cnt<3) {
				fprintf(stderr, "not enough index2\n");
				continue;
			}
			list_idx = atoi(arr[2]);
			funcD(set_idx, list_idx);
			if(printList()==-1) break;			
		}
		else if(arr[1][0]=='i') {
			funcI(set_idx);
			if(printList()==-1) break;
		}
		else if(arr[1][0]=='f') {
			funcF(set_idx);
			if(printList()==-1) break;
		}
		else if(arr[1][0]=='t') {
			optT=1;
			funcF(set_idx);
			if(printList()==-1) break;
		}
		else {
			fprintf(stderr, "option error\n");
			continue;
		}
		
		
	}
	
	
	freeList();
}


//BFS를 이용한 search
//탐색을 진행하며 리스트에 sort+add
void search_fmd5(char* extension, double min, double max, char* path) {

	struct dirent** fileList;
	struct stat statbuf1;
	struct stat statbuf2;
	char pathbuf[PATHMAX*2];
	int cnt;
	char fileName[PATHMAX];
	char backupName[PATHMAX]; //확장자 구하는중 기본문자열이 변형되어 사용
	
	Queue rq;
	Queue* queue = &rq;
	queueInit(queue);
	
	Fileinfo sample;
	strcpy(sample.path, path);
	sample.type = DIRECTORY;
	enQueue(queue, sample);


	while(!isEmptyQueue(queue)) {
		//큐에서 제거
		sample = deQueue(queue);
		strcpy(fileName, sample.path);
		
		if(lstat(fileName, &statbuf1)==-1) {
			fprintf(stderr, "lstat error1\n");
			return;
		}
		
		//디렉토리라면 scandir후 바로 하위 파일들을 큐에 넣어주기
		if(S_ISDIR(statbuf1.st_mode)) {
		
			if(access(fileName, F_OK)!=0) {
				if(errno==13) return;
				fprintf(stderr, "directory access error\n");
				exit(1);
			}
		
			if((cnt = scandir(fileName, &fileList, NULL, alphasort))==-1) {
				fprintf(stderr, "scandir error\n");
				return;
			}

			for(int i=0; i<cnt; i++) {
	
				//특정 디렉토리 제외
				if (!strcmp(fileList[i]->d_name, ".") || !strcmp(fileList[i]->d_name, "..") || !strcmp(fileList[i]->d_name, "proc") || !strcmp(fileList[i]->d_name, "run") || !strcmp(fileList[i]->d_name, "sys"))
					continue;

				//d_name으로부터 절대경로 구하기
				if (strcmp(fileName, "/")==0)
					sprintf(pathbuf, "%s%s", fileName, fileList[i]->d_name);
				else
					sprintf(pathbuf, "%s/%s", fileName, fileList[i]->d_name);			
				
				if(lstat(pathbuf, &statbuf2)==-1) {
					fprintf(stderr, "lstat error2\n");
					return;
				}
				
				//정규파일, 디렉토리 모두 아닌것은 처리X
				//읽을 수 없는 파일은 아예 처리X
				if(!S_ISREG(statbuf2.st_mode) && !S_ISDIR(statbuf2.st_mode)) continue;
//				if((statbuf2.st_mode & S_IRUSR)==0 || (statbuf2.st_mode & S_IRGRP)==0 || (statbuf2.st_mode & S_IROTH)==0) continue;
				if(access(pathbuf, F_OK)<0) continue;
				

				strcpy(sample.path, pathbuf);
				strcpy(sample.extension, getFileExtension(pathbuf));
				sample.type = (S_ISDIR(statbuf2.st_mode))?DIRECTORY:REGFILE;
				if(S_ISREG(statbuf2.st_mode)) { //정규파일일 경우에만. 해시, 사이즈 구하기
					sample.size = statbuf2.st_size; 
					strcpy(sample.hash, getMD5Hash(pathbuf));
				}
				
				enQueue(queue, sample);
			}

		}
		//size범위, 지정 확장자의 정규파일이라면 리스트에 넣어주기
		else if(S_ISREG(statbuf1.st_mode)) {
			
			//확장자 조건 검사
			if(strcmp(sample.extension, extension) && strcmp(extension, "all")) {
				continue;
			}
		
			//크기 조건 검사			
			if(min==-1 && max==-1) {
				if(0<statbuf1.st_size)
					addList(sample);
			}
			else if(min==-1) {
				if(0<statbuf1.st_size && statbuf1.st_size<=max)
					addList(sample);
			}
			else if(max==-1) {
				if(min<=statbuf1.st_size) {
					addList(sample);
				}
			}
			else {
				if(min<=statbuf1.st_size && statbuf1.st_size<=max) 
					addList(sample);

			}
		}
	}
	
	
}

//md5 (32자리)
char* getMD5Hash(char* path) {

	FILE* fp;
	MD5_CTX ctx;
	unsigned char md[MD5_DIGEST_LENGTH];
	unsigned char buf[BUFSIZE];

	if((fp = fopen(path, "r"))==NULL) {
		fprintf(stderr, "fopen error 1, errno:%d-in %s\n", errno, path);
		exit(1);
	}
	
	int fd = fileno(fp);
	
	MD5_Init(&ctx);
	while(1) {
		int len = read(fd, buf, BUFSIZE);
		if(len<=0) break;
		MD5_Update(&ctx, buf, (unsigned long)len);
	}
	MD5_Final(&(md[0]), &ctx);
	
	sprintf(hash, "%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x", md[0], md[1], md[2], md[3], md[4], md[5], md[6], md[7], md[8], md[9], md[10], md[11], md[12], md[13], md[14], md[15]);
	
	fclose(fp);
	return hash;
	
}	











