#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<time.h>


//파일 절대경로 배열
char pathArr[100][400];
int idx = 0;
int flag;
int optQ; int optS; int optI; int optR; int optC;


//파일 비교용 배열
char f1str[1030][1030];
char f2str[1030][1030];

//기타 전역변수
char getMod[100]; // getMode함수 리턴을 위해




/**
  * 파일 리스트에 경로 추가
  * @param path: 추가할 파일의 경로
  **/
void savePath(char* path) {
	
	strcpy(pathArr[idx], path);
	idx++;
}

/**
  * stat구조체를 받아서 Access Time 출력
  **/
void printATime(struct stat *buf){ 
        struct tm *t; 
        t=localtime(&buf->st_atime); 
        printf("%02d-%02d-%02d %02d:%02d ", 
                t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min); 
}

/**
  * stat구조체를 받아서 Change Time 출력
  **/
void printCTime(struct stat *buf){ 
        struct tm *t; 
        t=localtime(&buf->st_ctime); 
        printf("%02d-%02d-%02d %02d:%02d ", 
                t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min); 
}

/**
  * stat구조체를 받아서 Modify Time 출력
  **/
void printMTime(struct stat *buf){ 
        struct tm *t; 
        t=localtime(&buf->st_mtime); 
        printf("%02d-%02d-%02d %02d:%02d ", 
                t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min); 
}

/**
  * mode값 변환
  *
  **/
char* getMode(unsigned long mode) {

	char* di = ((S_ISDIR(mode))?"d":"_");
	char* ru = ((mode&S_IRUSR)?"r":"_");
	char* wu = ((mode&S_IWUSR)?"w":"_");
	char* xu = ((mode&S_IXUSR)?"x":"_");
	char* rg = ((mode&S_IRGRP)?"r":"_");
	char* wg = ((mode&S_IWGRP)?"w":"_");
	char* xg = ((mode&S_IXGRP)?"x":"_");
	char* ro = ((mode&S_IROTH)?"r":"_");
	char* wo = ((mode&S_IWOTH)?"w":"_");
	char* xo = ((mode&S_IXOTH)?"x":"_");

	sprintf(getMod, "%s%s%s%s%s%s%s%s%s%s", di, ru, wu, xu, rg, wg, xg, ro, wo, xo);
	return getMod;
}

/**
  * 정규파일의 정보 출력
  * @param targetStat: 출력할 파일의 정보를 가지는 stat구조체
  * @param fileRealPath: 파일의 절대경로
  *
  **/
void printRegFileInfo(struct stat targetStat, char* fileRealPath) {

	printf("%d %lld %s %ld %ld %ld %ld ", idx, (long long)targetStat.st_size, getMode((unsigned long)targetStat.st_mode), (long)targetStat.st_blocks, (long)targetStat.st_nlink, (long)targetStat.st_uid, (long)targetStat.st_gid);
	printATime(&targetStat);
	printCTime(&targetStat);
	printMTime(&targetStat);
	printf("%s\n", fileRealPath);
}


/**
  * 디렉토리 정보 출력
  * @param targetStat: 출력할 파일의 정보를 가지는 stat구조체
  * @param fileRealPath: 파일의 절대경로
  * @param dirSize: 디렉토리의 크기
  *
  **/
void printDirInfo(struct stat targetStat, char* fileRealPath, long long dirSize) {

	printf("%d %lld %s %ld %ld %ld %ld ", idx, dirSize, getMode((unsigned long)targetStat.st_mode), (long)targetStat.st_blocks, (long)targetStat.st_nlink, (long)targetStat.st_uid, (long)targetStat.st_gid);
	printATime(&targetStat);
	printCTime(&targetStat);
	printMTime(&targetStat);
	printf("%s\n", fileRealPath);
}


/**
  * scandir filtering method - for ".",".."
  * @param entry: file path
  * @return 제외하려면 0, 포함하려면 1
  */
int fileFilter(const struct dirent* entry) {
	
	if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0)
		return 0;
	return 1;
}




/**
  * 디렉토리 하위 파일들의 크기 합을 구하는 메소드
  * @param dirRealPath: 하위 디렉토리 내 모든 파일의 크기를 구할 디렉토리의 절대경로
  * @return 하위 모든 파일의 크기 합. 에러 발생시 음수값 리턴
  **/
long long getDirectorySize(char* dirRealPath) {

	DIR* dirptr;
	struct stat fileStat;
	struct dirent* file;
	long long size=0;
	char searchingPath[400];
	 
	//백트래킹용 path
	strcpy(searchingPath, dirRealPath);
	
	//get directory path	
	if((dirptr = opendir(dirRealPath)) == NULL) {
		printf("DIRECTORY OPEN ERROR, ");
		printf("ERRNO: %d\n", errno);
		return -1;
	}		
	
	//재귀적 탐색하며 size 합 구하기
	while((file = readdir(dirptr))!=NULL) {
	
		//".", ".." 제외
		if(strcmp(file->d_name, ".")==0 || strcmp(file->d_name, "..")==0) continue;
		
		//경로 설정, 파일 stat 확인
		if(strcmp(dirRealPath, "/")==0)
			sprintf(searchingPath, "%s%s", dirRealPath, file->d_name);
		else
			sprintf(searchingPath, "%s/%s", dirRealPath, file->d_name);
		if(lstat(searchingPath, &fileStat)<0) continue;
		
		//파일or디렉토리 크기 더하기
		if(S_ISDIR(fileStat.st_mode)) {
			size += getDirectorySize(searchingPath);
		}
		else {
			size += (long long)fileStat.st_size;
		}
	}
	
	closedir(dirptr);
	return size;
	
}





/**
  * 두 파일의 내용을 비교하는 메소드
  * @param filePath1: 비교할 파일 절대경로
  * @param filePath2: 비교대상 파일 절대경로
  * @param ifPrint: 0이면 비교내용 출력X, 1이면 출력 (리턴값만 얻으려면 0을 넣으면 된다)
  * @return 같으면 0, 다르면 1
  *
  **/
int compareRegFiles(char* filePath1, char* filePath2, int ifPrint) {
		

	//기준파일 open
	FILE* fp1 = fopen(filePath1, "r");
	if(fp1==NULL)
		printf("FOPEN ERROR");
	
	//비교대상파일 open
	FILE* fp2 = fopen(filePath2, "r");
	if(fp2==NULL)
		printf("FOPEN ERROR");
		
	
	int f1Size; int f2Size;
	//파일 입력받기
	f1Size=1;
	while(fgets(f1str[f1Size], 1029, fp1)!=NULL) {
		f1Size++;
		continue;
	}
	f2Size=1;
	while(fgets(f2str[f2Size], 1029, fp2)!=NULL) {
		f2Size++;
		continue;
	}
	f1Size--;
	f2Size--;
	fclose(fp1);
	fclose(fp2);

	//마지막 행의 개행문자 제거
	f1str[f1Size][strlen(f1str[f1Size])-1] = '\0';
	f2str[f2Size][strlen(f2str[f2Size])-1] = '\0';
	
	//파일 특정행 비교 (optC)
	int l1, l2;
	if(optC && ifPrint==1) {
		printf("stdFile's line num : "); scanf("%d", &l1);
		printf("compare File's line num : "); scanf("%d", &l2);
		getchar();
		if(l1>f1Size || l2>f2Size) {
			printf("LINE RANGE ERROR\n");
			return -1;
		}
		if(!optI) {
		if(strcmp(f1str[l1], f2str[l2])!=0) {
			printf("selected lines are different : \n");
			printf("file [%s] line %d : '%s'\n", filePath1, l1, f1str[l1]);
			printf("file [%s] line %d : '%s'\n", filePath2, l2, f2str[l2]);
		}
		else {
			printf("selected lines are identical : \n");
			printf("selected line : '%s'\n", f1str[l1]);
		}
		}
		else if(optI) {
		if(strcasecmp(f1str[l1], f2str[l2])!=0) {
			printf("selected lines are different : \n");
			printf("file [%s] line %d : '%s'\n", filePath1, l1, f1str[l1]);
			printf("file [%s] line %d : '%s'\n", filePath2, l2, f2str[l2]);
		}
		else {
			printf("selected lines are identical : \n");
			printf("file [%s] line %d : '%s'\n", filePath1, l1, f1str[l1]);
			printf("file [%s] line %d : '%s'\n", filePath2, l2, f2str[l2]);
		}
		}
		
	}
	
	//한줄씩 비교 [for standard, cursor]
	int o1=1; int o2=1; //FILENAME
	int n1=1; int n2=1; //INDEX
	
	flag=0;
	while(1) {
		//set o1, o2, n1, n2
		for(n2=n1; n2<=f2Size; n2++) { //INDEX
			for(o2=o1; o2<=f1Size; o2++) { //FILENAME
			
//printf("FOR- o1:%d, o2:%d, n1:%d, n2:%d\n", o1, o2, n1, n2); 
				//두 파일의 행이 같을때 && 대소문자 구분 비교 (optI==0)
				if(!optI) {
				if(strcmp(f1str[o2], f2str[n2])==0 && f1str[o2]!=NULL && f2str[n2]!=NULL) {


					//두 파일의 행이 연속으로 같을때
					if(o1==o2 && n1==n2) {
						if(o1<=f1Size) o1++;
						if(n1<=f2Size) n1++;
						break;
					}
					
					//빈 줄일 경우 다음줄도 같아야 인정
					if((strcmp(f1str[o2],"\n")==0 || strcmp(f2str[n2],"\n")==0)) {
						if(strcmp(f1str[o2+1], f2str[n2+1])!=0)
							continue;
					}

					
					goto EXIT;
					
				}
				}
				//두 파일의 행이 같을때 && 대소문자 구분하지 않음 (optI==1)
				else {
				if(strcasecmp(f1str[o2], f2str[n2])==0 && f1str[o2]!=NULL && f2str[n2]!=NULL) {


					//두 파일의 행이 연속으로 같을때
					if(o1==o2 && n1==n2) {
						if(o1<=f1Size) o1++;
						if(n1<=f2Size) n1++;
						break;
					}
					
					//빈 줄일 경우 다음줄도 같아야 인정
					if((strcasecmp(f1str[o2],"\n")==0 || strcasecmp(f2str[n2],"\n")==0)) {
						if(strcasecmp(f1str[o2+1], f2str[n2+1])!=0)
							continue;
					}

					
					goto EXIT;
					
				}
				}

				//두 파일의 행이 다를경우
				flag = 1;
				//다른 행이 파일의 끝이면
				if((o2==f1Size) && (n2==f2Size)) {
					o2++; n2++;
					goto EXIT;
				}

			}
			if((o2==f1Size) && (n2==f2Size)) goto EXIT; //마지막이면 n2++생략
		}

		
	EXIT:
//printf("EXIT- o1:%d, o2:%d, n1:%d, n2:%d\n", o1, o2, n1, n2); 
		//파일 내용 전체가 같은 케이스
		if(!flag) {
			if(optS && ifPrint) printf("Files %s and %s are identical\n", filePath1, filePath2);	
			break;
		}
		
		//한쪽이 끝남
		if(o1>f1Size) { 
			break;
		} 
		if(n1>f2Size) {
			break;
		}
		
		//optQ값이 1인경우 달라도 출력하지 않음
		if(ifPrint==1 && !optQ) {
//printf("EXIT- o1:%d, o2:%d, n1:%d, n2:%d\n", o1, o2, n1, n2); 

		//changed
		if(o1!=o2 && n1!=n2) {
			if(n1==n2-1 && o1==o2-1)
				printf("%dc%d\n", o1, n1);
			else if(n1==n2-1)
				printf("%d,%dc%d\n", o1, o2-1, n1);
			else if(o1==o2-1)
				printf("%dc%d,%d\n", o1, n1, n2-1);
			else {
				printf("%d,%dc%d,%d\n", o1, o2-1, n1, n2-1);
			}
				
			for(int k=o1; k<o2; k++) {
				if(k==f1Size) {
					printf("< %s\n", f1str[k]);
					printf("/ No newline at end of file\n");
				}
				else
					printf("< %s", f1str[k]);
			}
//if(o2>=f1Size || n2>=f2Size) printf("/ No newline at end of file\n");
			printf("---\n");
			for(int k=n1; k<n2; k++) {
				if(k==f2Size) {
					printf("> %s\n", f2str[k]);
					printf("/ No newline at end of file\n");
				}
				else
					printf("> %s", f2str[k]);
			}
		}
		//add
		else if(o1==o2 && n1!=n2) {
//printf("%d %d %d %d\n", o1,o2,n1,n2);
			if(n1==n2-1)
				printf("%da%d\n", o1-1, n1);
			else
				printf("%da%d,%d\n", o1-1, n1, n2-1);
			for(int k=n1; k<n2; k++) {
				if(k==f2Size) {
					printf("> %s\n", f2str[k]);
					printf("/ No newline at end of file\n");
				}
				else
					printf("> %s", f2str[k]);
			}
		}
		//delete
		else if(o1!=o2 && n1==n2) {
			if(o1==o2-1)
				printf("%dd%d\n", o1, n1-1);
			else
				printf("%d,%dd%d\n", o1, o2-1, n1-1);
			for(int k=o1; k<o2; k++)
				if(k==f1Size) {
					printf("> %s\n", f1str[k]);
					printf("/ No newline at end of file\n");
				}
				else
					printf("> %s", f1str[k]);
		}
		
		}
		
		//한쪽이 끝남
		if(o1>=f1Size) { 
			break;
		} 
		if(n1>=f2Size) {
			break;
		}
		o1=o2; n1=n2;
	
		
	}
	
	//파일내용이 다르고 optQ
	if(ifPrint==1 && optQ && flag) printf("Files %s and %s are differ\n", filePath1, filePath2);

	return flag;
	
}



/**
  * 두 디렉토리를 비교하는 메소드
  * @param dirPath1: 비교할 파일 절대경로
  * @param dirPath2: 비교대상 파일 절대경로
  * @param ifPrint: 0이면 비교내용 출력X, 1이면 출력 (리턴값만 얻으려면 0을 넣으면 된다)
  * @return 같으면 0, 다르면 1
  **/
int compareDirs(char* dirPath1, char* dirPath2, int ifPrint) {

	int notSame=0;
	int cnt1; //file num in fileList1
	int cnt2; // 
	struct dirent** fileList1;
	struct dirent** fileList2;
	
	struct stat targetStat1;
	struct stat targetStat2;
	
	//절대경로
	char searchingPath1[1024];
	char searchingPath2[1024];
	
	//아스키코드순 정렬
	cnt1 = scandir(dirPath1, &fileList1, fileFilter, alphasort);
	cnt2 = scandir(dirPath2, &fileList2, fileFilter, alphasort);
	
	int idx1=0;
	int idx2=0;
	
	while(1) {
			
		//list1, list2에 이름이 동일한 파일 존재
		if(idx1<cnt1 && idx2<cnt2 && strcmp(fileList1[idx1]->d_name, fileList2[idx2]->d_name)==0) {
							
			//파일 절대경로 구하기
			sprintf(searchingPath1, "%s/%s", dirPath1, fileList1[idx1]->d_name);
			sprintf(searchingPath2, "%s/%s", dirPath2, fileList2[idx2]->d_name);
			
			
			
			if(lstat(searchingPath1, &targetStat1)<0) {
				printf("STAT ERROR\n");
				return -1;
			}
			if(lstat(searchingPath2, &targetStat2)<0) {
				printf("STAT ERROR\n");
				return -1;
			}
			
			
			//파일 종류가 다른 경우
			if(targetStat1.st_mode != targetStat2.st_mode) {
				notSame=1;
				if(!optQ && ifPrint)
					printf("File %s is a %s while File %s is a %s\n", searchingPath1, ((S_ISREG(targetStat1.st_mode))?"regular file":"directory"), searchingPath2, ((S_ISDIR(targetStat2.st_mode))?"directory":"regular file"));
			}
			//둘 다 정규파일인 경우 diff+차이점 출력
			else if(S_ISREG(targetStat1.st_mode)&&S_ISREG(targetStat2.st_mode)) {
			
				int regrst;
				regrst = compareRegFiles(searchingPath1, searchingPath2, 0);
				if(regrst==1) {
					notSame=1;					
					if(!optQ && ifPrint) {
						printf("diff ");
						if(optQ) printf("-q ");
						if(optS) printf("-s ");
						if(optI) printf("-i ");
						if(optR) printf("-r ");
						if(optC) printf("-c "); //호출되면 안됨
						printf("%s %s\n", searchingPath1, searchingPath2);
					}
					if(!optQ && ifPrint)
						compareRegFiles(searchingPath1, searchingPath2, 1);
					
				}
			}
			//둘 다 디렉토리인 경우 
			else if(S_ISDIR(targetStat1.st_mode)&&S_ISDIR(targetStat2.st_mode)) { //재귀적으로 하위 디렉토리 비교
			
				if(!optQ && ifPrint && !optS && !optR)
					printf("Common subdirectories: %s and %s\n", searchingPath1, searchingPath2);
				//재귀 탐색
				if(optR) {
		
					int dirrst;
					if(!optQ && ifPrint)
						dirrst = compareDirs(searchingPath1, searchingPath2, 1);
					else
						dirrst = compareDirs(searchingPath1, searchingPath2, 0);
						
					if(dirrst==1) notSame=1;
					else if(dirrst==0) printf("Common subdirectories: %s and %s\n", searchingPath1, searchingPath2);
				}
			}
			idx1++;
			idx2++;
					
		}
					
		//list1에만 파일이 존재
		else if(idx1<cnt1 && idx2<cnt2 && strcmp(fileList1[idx1]->d_name, fileList2[idx2]->d_name)<0) {
			
			notSame=1;
			if(!optQ && ifPrint)
				printf("Only in %s: %s\n", dirPath1, fileList1[idx1]->d_name);
			idx1++;
			continue;
		}
		//list2에만 파일이 존재
		else if(idx1<cnt1 && idx2<cnt2 && strcmp(fileList1[idx1]->d_name, fileList2[idx2]->d_name)>0) {

			notSame=1;
			if(!optQ && ifPrint)
				printf("Only in %s: %s\n", dirPath2, fileList2[idx2]->d_name);
			idx2++;
			continue;
		}
		else {
			break;
		}
	
		
					
	}
	
	if(idx1<cnt1 && idx2>=cnt2) {
		while(idx1<cnt1) {
			notSame=1;
			if(!optQ && ifPrint)
				printf("Only in %s: %s\n", dirPath1, fileList1[idx1]->d_name);
			idx1++;
		}
	}
	else if(idx1>=cnt1 && idx2<cnt2) {
		while(idx2<cnt2) {
			notSame=1;
			if(!optQ && ifPrint)
				printf("Only in %s: %s\n", dirPath2, fileList2[idx2]->d_name);
			idx2++;
		}
	}
	
	if(notSame && ifPrint && optQ) printf("Files %s and %s are differ\n", dirPath1, dirPath2);
	if(!notSame && ifPrint && optS) printf("Files %s and %s are identical\n", dirPath1, dirPath2);
	return notSame;
}




/**
  * 조건에 맞는 파일들을 백트래킹을 통해 찾아 출력하는 메소드
  * @param fileName: 찾고자 하는 순수 파일명
  * @param fileRealPath: 찾고자 하는 파일의 절대경로
  * @param fileSize: 찾고자 하는 파일 사이즈
  * @param realPath: 탐색을 시작할 디렉토리 절대경로
  **/
void findAllFiles(char* fileName, char* fileRealPath, long long fileSize, char* realPath) {
	
	DIR* dirptr;
	struct stat findFileStat;
	struct dirent* file;
	char searchingPath[1024];


	//백트래킹용 path
	strcpy(searchingPath, realPath);

	//get directory path
	if((dirptr = opendir(realPath)) == NULL) {
		printf("DIRECTORY OPEN ERROR, ");
		printf("ERRNO: %d\n", errno);
		return;
	}

	//directory 탐색
	while((file = readdir(dirptr)) != NULL) {
				
		//".", ".." 제외
		if(strcmp(file->d_name, ".")==0 || strcmp(file->d_name, "..")==0) continue;
		
		//경로 설정, 파일 stat 확인
		if(strcmp(realPath, "/")==0)
			sprintf(searchingPath, "%s%s", realPath, file->d_name);
		else
			sprintf(searchingPath, "%s/%s", realPath, file->d_name);
			
		if(lstat(searchingPath, &findFileStat)<0) continue;

		//파일이 directory라면 DFS
		if(S_ISDIR(findFileStat.st_mode)) {
			findAllFiles(fileName, fileRealPath, fileSize, searchingPath);
		}
		//파일이 regular file이라면
		else if(S_ISREG(findFileStat.st_mode)) {
		
			//파일명과 크기가 fileName과 동일하다면
			if(fileSize==findFileStat.st_size && strcmp(fileName, file->d_name)==0 && strcmp(searchingPath, fileRealPath)!=0) {
				flag=1;
				printRegFileInfo(findFileStat, searchingPath);
				savePath(searchingPath);
			}

		}
	}
	closedir(dirptr);
	

}


/**
  * 조건에 맞는 디렉토리들을 백트래킹을 통해 찾아 출력하는 메소드
  * @param fileName: 찾고자 하는 순수 디렉토리명
  * @param fileRealPath: 찾고자 하는 디렉토리의 절대경로
  * @param fileSize: 찾고자 하는 디렉토리 사이즈
  * @param realPath: 탐색을 시작할 디렉토리 절대경로
  **/
void findAllDirectories(char* fileName, char* fileRealPath, long long fileSize, char* realPath) {
	
	DIR* dirptr;
	struct stat findFileStat;
	struct dirent* file;
	char searchingPath[1024];


	//백트래킹용 path 
	strcpy(searchingPath, realPath);

	//get directory path
	if((dirptr = opendir(realPath)) == NULL) {
		printf("DIRECTORY OPEN ERROR,");
		printf("ERRNO: %d\n", errno);
		return;
	}

	//directory 탐색
	while((file = readdir(dirptr)) != NULL) {
				
		//".", ".." 제외
		if(strcmp(file->d_name, ".")==0 || strcmp(file->d_name, "..")==0) continue;
		
		//경로 설정, 파일 stat 확인
		if(strcmp(realPath, "/")==0)
			sprintf(searchingPath, "%s%s", realPath, file->d_name);
		else
			sprintf(searchingPath, "%s/%s", realPath, file->d_name);
			
		if(lstat(searchingPath, &findFileStat)<0) continue;

		//파일이 directory라면 찾는 디렉토리인지 확인/출력 후 DFS
		if(S_ISDIR(findFileStat.st_mode)) {
		
			//이름과 크기가 동일하면
			if(strcmp(fileName, file->d_name)==0 && strcmp(searchingPath, fileRealPath)!=0 && fileSize==getDirectorySize(searchingPath)) {
				flag=1;
				printDirInfo(findFileStat, searchingPath, fileSize);
				savePath(searchingPath);
			}
			findAllDirectories(fileName, fileRealPath, fileSize, searchingPath);
		}
		//파일이 directory가 아니라면 별도 처리 X
	}
	closedir(dirptr);
	

}


/** 
  * ssu_sindex 내장 기본 명령어
  * @param fileName: 찾고자 하는 파일명(디렉토리명) 혹은 절대/상대경로
  * @param path: 탐색을 시작할 디렉토리 경로 (절대/상대경로 모두 가능)
  **/
void find(char* fileName, char* path) {


	flag = 0;

	//파일의 절대경로 구하기 
	char fileRealPath[200];
	if(realpath(fileName, fileRealPath)==NULL) {
		printf("FILE REALPATH ERROR\n");
		return;
	}
//printf("파일 절대경로 : %s\n", fileRealPath);
	
	//순수 파일명 구하기
	char* filePureName;
	if((filePureName = strrchr(fileRealPath, '/')+1)==NULL) {
		printf("FILE PURENAME ERROR\n");
		return;
	}
//printf("PURENAME : '%s'\n", filePureName);


	//탐색경로를 절대경로로 변환
	char realPath[200];
	if(realpath(path, realPath)==NULL) { 
		printf("REALPATH ERROR\n");
		return;
	}
//printf("탐색 절대경로 : %s\n", realPath);

	//파일 상태 얻기
	struct stat targetStat;
	if(lstat(fileRealPath, &targetStat)<0) {
		printf("STAT ERROR\n");   
		return;
	}
	
	
	//파일일 경우
	if(S_ISREG(targetStat.st_mode)) {
	
		//기준 파일 정보출력
		printf("Index Size Mode Blocks Links UID GID Access Change Modify Path\n");	
		printRegFileInfo(targetStat, fileRealPath);
		
		savePath(fileRealPath);

		//reg파일 dfs
		findAllFiles(filePureName, fileRealPath, (long long)targetStat.st_size, realPath);
	}
	//디렉토리일 경우 - size 별도계산 필요
	else if(S_ISDIR(targetStat.st_mode)) {
	
		long long dirSize = getDirectorySize(fileRealPath);
		//기준 디렉토리 정보 출력
		printf("Index Size Mode Blocks Links UID GID Access Change Modify Path\n");	
		printDirInfo(targetStat, fileRealPath, dirSize);
		
		savePath(fileRealPath);
		
		//디렉토리 dfs
		findAllDirectories(filePureName, fileRealPath, dirSize, realPath);
	
	}
	
	//이름과 크기가 같은 파일 존재X
	if(!flag) {
		printf("(None)\n"); 
		return;
	}
	
	//[INDEX][OPTION]
	//OPTION q : 파일 내용이 다른 경우 차이점 출력 없이 알림
	//OPTION s : 두 파일이 동일한 경우 별도 알림
	//OPTION i : 대소문자 구분 없이 비교
	//OPTION r : (디렉토리 비교시) 하위 디렉토리를 전부 재귀적 탐색
	//OPTION c : (정규파일 비교시) 특정 행 비교
	printf(">> ");
	
	//[INDEX]
	int index;	
	
	//[OPTION]
	optQ=0; optS=0; optI=0; optR=0; optC=0;
	char option[30];
	
	fgets(option, sizeof(option), stdin);
	option[strlen(option)-1] = ' ';
	
	char* cptr = strtok(option, " ");
	if(cptr==NULL) return; //INDEX가 존재하지 않을 경우 에러처리
	index = atoi(cptr);
	cptr = strtok(NULL, " ");
	
	while(cptr!=NULL) {
		if(strcmp(cptr, "q")==0) optQ=1;
		else if(strcmp(cptr, "s")==0) optS=1;
		else if(strcmp(cptr, "i")==0) optI=1;
		else if(strcmp(cptr, "r")==0) optR=1;
		else if(strcmp(cptr, "c")==0) optC=1;
		else return;
		cptr = strtok(NULL, " ");
	}
	
//printf("q:%d, s:%d, i:%d, r:%d\n", optQ,optS,optI,optR);
	
	if(S_ISREG(targetStat.st_mode)) {
		compareRegFiles(pathArr[0], pathArr[index], 1);
	}
	else if(S_ISDIR(targetStat.st_mode)) {
		if(optC==1) { 
			printf("ERROR : you can't use OPTION C while comparing directories\n");
			return;
		}
		if(compareDirs(pathArr[0], pathArr[index], 0)==0) return;
		compareDirs(pathArr[0], pathArr[index], 1);
	}
	
	
}



int main() {
	
	char fileName[1024];
	char path[1024];
	char command[1024];
	struct timeval startTime, endTime;

	printf("20182593>> ");
	gettimeofday(&startTime, NULL);
	
	while(1) {
		idx=0;

		//개행문자 입력 체크		
		char command[1000];
		fgets(command, sizeof(command), stdin);
		if(command[0]=='\n') {
			printf("\n20182593>>: ");
			continue;
		}
		command[strlen(command)-1]=' ';
		char* cptr = strtok(command, " ");
	
		//find()
		if(strcmp(cptr, "find")==0) {
		
			int flag1 = 0;
			int flag2 = 0;
			int flag3 = 0;
			char input[1024];		
			
			//띄어쓰기 기준 입력받기
			cptr = strtok(NULL, " ");
			if(cptr!=NULL) {
				flag1 = 1;
    				strcpy(fileName, cptr);          
   				cptr = strtok(NULL, " "); 
   			} 
   			if(cptr!=NULL) {
   				flag2 = 1;
				strcpy(path, cptr);          
   				cptr = strtok(NULL, " ");
   			}
   			if(cptr!=NULL) {
   				flag3 = 1;
   				cptr = strtok(NULL, " ");
   			}

			//끝 개행문자 제거
   			if(fileName[strlen(fileName)-1]=='\n') {
   				fileName[strlen(fileName)-1]='\0';
   			}
   			if(path[strlen(path)-1]=='\n') {	
   				path[strlen(path)-1]='\0';
   			}

			//입력형식 오류 검증
			if(flag1 && flag2 && !flag3) 
				find(fileName, path);

//			else if(!flag1 && !flag2);
			else
				printf("INPUT ERROR - in [command] [FILENAME] [PATH] \n");
		}
		//exit()
		else if(strcmp(command, "exit")==0) {
			gettimeofday(&endTime, NULL);
			long sec = endTime.tv_sec-startTime.tv_sec;
			long usec = endTime.tv_usec-startTime.tv_usec;
			if(usec<0) {
				sec--;
				usec+=1000000;
			}
			printf("Prompt End\nRuntime: %ld:%ld(sec:usec)\n", sec, usec);
			break;
		}
		//help()
		else {
			printf("========================================================\n");
			printf("\n<< HELP:Usage of SSU_INDEX program >>\n\n");
			printf(" THIS FILE IS USED FOR COMPARE AND PRINT THE DIFFERENCES BETWEEN FILES\n\n");
			printf("<FUNCTION>\n\n");
			printf("> find [FILENAME] [PATH]\n");
			printf("  - [FILENAME] : name(or path) of file already exists\n");
			printf("  - [PATH] : path to start searching\n");
			printf(" >> [INDEX] [OPTION]\n");
			printf("    - [INDEX] : compare Index 0 file with file you choose\n");
			printf("    - [OPTION] \n");
			printf("        q : don't print detail when files are different\n");
			printf("        s : notice when files are identical\n");
			printf("        i : don't classify cases (UPPER/lower) (only in regfile)\n");
			printf("        r : recursive search (only in directory)\n");
			printf("        c : line selective compare (only in regfile)\n\n");
			printf("> help\n");
			printf("  - input 'help' to see guide of this program\n\n");
			printf("> exit\n");
			printf("  - show the program runtime and terminate the program\n\n");
			printf("========================================================\n");
			
		}
		printf("20182593>> ");
		
	}

}
