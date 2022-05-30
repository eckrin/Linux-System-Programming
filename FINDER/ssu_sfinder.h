#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <pwd.h>

#define NAMEMAX 255
#define PATHMAX 4096

#define HASHMAX 33

#define STRMAX 10000 
#define ARGMAX 15

typedef struct fileInfo {
	char path[PATHMAX];
	struct stat statbuf;
	struct fileInfo *next;
} fileInfo;

typedef struct fileList {
	long long filesize;
	char hash[HASHMAX];
	fileInfo *fileInfoList;
	struct fileList *next;
} fileList;

typedef struct dirList {
	char dirpath[PATHMAX];
	struct dirList *next;
} dirList;

typedef struct logInfo {
	char command[15];
	char path[PATHMAX];
	char date[STRMAX];
	char time[STRMAX];
	char username[NAMEMAX];
} logInfo;

typedef struct trashInfo {
	char path[PATHMAX]; //원본 path
	struct stat statbuf;
	char date[STRMAX];
	char time[STRMAX];
	char hash[HASHMAX];
	struct trashInfo* next;
} trashInfo;

#define DIRECTORY 1
#define REGFILE 2

#define KB 1000
#define MB 1000000
#define GB 1000000000
#define KiB 1024
#define MiB 1048576
#define GiB 1073741824
#define SIZE_ERROR -2

char extension[10];
char same_size_files_dir[PATHMAX]; //same size files를 저장하는 파일 경로
char trash_path[PATHMAX];
char log_path[PATHMAX];
char username[STRMAX];
char target_dir[PATHMAX];
long long minbsize;
long long maxbsize;
fileList* dups_list_h = NULL;
trashInfo* trash_list_h = NULL;
int threadnum = 1; //additional thread num
pthread_mutex_t mutex;

dirList* cur;
dirList* subdirs;


void fileinfo_append(fileInfo *head, char *path)
{
	fileInfo *fileinfo_cur;

	fileInfo *newinfo = (fileInfo *)malloc(sizeof(fileInfo));
	memset(newinfo, 0, sizeof(fileInfo));
	strcpy(newinfo->path, path);
	lstat(newinfo->path, &newinfo->statbuf);
	newinfo->next = NULL;

	if (head->next == NULL)
		head->next = newinfo;
	else {
		fileinfo_cur = head->next;
		while (fileinfo_cur->next != NULL)
			fileinfo_cur = fileinfo_cur->next;

		fileinfo_cur->next = newinfo;
	}
	
}

void trashinfo_append(trashInfo* head, char* path, char* date, char* time, char* hash) {

	trashInfo* cur;
//	trashInfo* why = trash_list_h;
	
	trashInfo* newinfo = (trashInfo*)malloc(sizeof(trashInfo));
	memset(newinfo, 0, sizeof(trashInfo));
	strcpy(newinfo->path, path);
	strcpy(newinfo->date, date);
	strcpy(newinfo->time, time);
	strcpy(newinfo->hash, hash);
	if(lstat(newinfo->path, &newinfo->statbuf)<0) {
		fprintf(stderr, "lstat error\n");
		return;
	}
	newinfo->next = NULL;
	
	if (head->next == NULL)
		head->next = newinfo;
	else {
		cur = head->next;
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = newinfo;
	}
	
//	while(why!=NULL) {
//		printf("node> %s\n", why->path);
//		why = why->next;
//	}
	
}

fileInfo *fileinfo_delete_node(fileInfo *head, char *path)
{
	fileInfo *deleted;

	if (!strcmp(head->next->path, path)){
		deleted = head->next;
		head->next = head->next->next;
		return head->next;
	}
	else {
		fileInfo *fileinfo_cur = head->next;

		while (fileinfo_cur->next != NULL){
			if (!strcmp(fileinfo_cur->next->path, path)){
				deleted = fileinfo_cur->next;
				
				fileinfo_cur->next = fileinfo_cur->next->next;
				return fileinfo_cur->next;
			}

			fileinfo_cur = fileinfo_cur->next;
		}
	}
}

int fileinfolist_size(fileInfo *head)
{
	fileInfo *cur = head->next;
	int size = 0;
	
	while (cur != NULL){
		size++;
		cur = cur->next;
	}
	
	return size;
}

int trashinfo_size(trashInfo* head) {

	trashInfo *cur = head->next;
	int size = 0;
	
	while (cur != NULL){
		size++;
		cur = cur->next;
	}
	
	return size;
}

void filelist_append(fileList *head, long long filesize, char *path, char *hash)
{
    fileList *newfile = (fileList *)malloc(sizeof(fileList));
    memset(newfile, 0, sizeof(fileList));

    newfile->filesize = filesize;
    strcpy(newfile->hash, hash);

    newfile->fileInfoList = (fileInfo *)malloc(sizeof(fileInfo));
    memset(newfile->fileInfoList, 0, sizeof(fileInfo));

    fileinfo_append(newfile->fileInfoList, path);
    newfile->next = NULL;

    if (head->next == NULL) {
        head->next = newfile;
    }    
    else {
        fileList *cur_node = head->next, *prev_node = head, *next_node;

        while (cur_node != NULL && cur_node->filesize < newfile->filesize) {
            prev_node = cur_node;
            cur_node = cur_node->next;
        }

        newfile->next = cur_node;
        prev_node->next = newfile;
    }    
}

//hash에 해당하는 filelist노드 삭제
void filelist_delete_node(fileList *head, char *hash)
{
	fileList *deleted;

	if (!strcmp(head->next->hash, hash)){
		deleted = head->next;
		head->next = head->next->next;
	}
	else {
		fileList *filelist_cur = head->next;

		while (filelist_cur->next != NULL){
			if (!strcmp(filelist_cur->next->hash, hash)){
				deleted = filelist_cur->next;
				
				filelist_cur->next = filelist_cur->next->next;

				break;
			}

			filelist_cur = filelist_cur->next;
		}
	}

	free(deleted);
}



int filelist_size(fileList *head)
{
	fileList *cur = head->next;
	int size = 0;
	
	while (cur != NULL){
		size++;
		cur = cur->next;
	}
	
	return size;
}

//
int filelist_search(fileList *head, char *hash)
{
	fileList *cur = head;
	int idx = 0;

	while (cur != NULL){
		if (!strcmp(cur->hash, hash))
			return idx;
		cur = cur->next;
		idx++;
	}

	return 0;
}

void dirlist_append(dirList *head, char *path)
{
	dirList *newFile = (dirList *)malloc(sizeof(dirList));

	strcpy(newFile->dirpath, path);
	newFile->next = NULL;

	if (head->next == NULL)
		head->next = newFile;
	else{
		dirList *cur = head->next;

		while(cur->next != NULL)
			cur = cur->next;
		
		cur->next = newFile;
	}
}

void dirlist_print(dirList *head, int index)
{
	dirList *cur = head->next;
	int i = 1;

	while (cur != NULL){
		if (index) 
			printf("[%d] ", i++);
		printf("%s\n", cur->dirpath);
		cur = cur->next;
	}
}

//dirlist 삭제
void dirlist_delete_all(dirList *head)
{
	dirList *dirlist_cur = head->next;
	dirList *tmp;

	while (dirlist_cur != NULL){
		tmp = dirlist_cur->next;
		free(dirlist_cur);
		dirlist_cur = tmp;
	}

	head->next = NULL;
}

int tokenize(char *input, char *argv[])
{
	char *ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}

	argv[argc-1][strlen(argv[argc-1])-1] = '\0';

	return argc;
}

// ~를 포함하는 절대경로 구하기
void get_path_from_home(char *path, char *path_from_home)
{
	char path_without_home[PATHMAX] = {0,};
	char *home_path;

	home_path = getenv("HOME");

	if (strlen(path) == 1){
		strncpy(path_from_home, home_path, strlen(home_path));
	}
	else {
        	strncpy(path_without_home, path + 1, strlen(path)-1);
        	sprintf(path_from_home, "%s%s", home_path, path_without_home);
	}
}

int is_dir(char *target_dir)
{
    struct stat statbuf;

    if (lstat(target_dir, &statbuf) < 0){
        printf("ERROR: lstat error for %s\n", target_dir);
        return 1;
    }
    return S_ISDIR(statbuf.st_mode) ? DIRECTORY : 0;

}

long long get_size(char *filesize)
{
	double size_bytes = 0;
	char size[STRMAX] = {0, };
	char size_unit[4] = {0, };
	int size_idx = 0;

	if (!strcmp(filesize, "~"))
		size_bytes = -1;
	else {
		for (int i = 0; i < strlen(filesize); i++){
			if (isdigit(filesize[i]) || filesize[i] == '.'){
				size[size_idx++] = filesize[i];
				if (filesize[i] == '.' && !isdigit(filesize[i + 1]))
					return SIZE_ERROR;
			}
			else {
				strcpy(size_unit, filesize + i);
				break;
			}
		}

		size_bytes = atof(size);

		if (strlen(size_unit) != 0){
			if (!strcmp(size_unit, "kb") || !strcmp(size_unit, "KB"))
				size_bytes *= KB;
			else if (!strcmp(size_unit, "mb") || !strcmp(size_unit, "MB"))
				size_bytes *= MB;
			else if (!strcmp(size_unit, "gb") || !strcmp(size_unit, "GB"))
				size_bytes *= GB;
			else if (!strcmp(size_unit, "kib") || !strcmp(size_unit, "KiB"))
				size_bytes *= KiB;
			else if (!strcmp(size_unit, "mib") || !strcmp(size_unit, "MiB"))
				size_bytes *= MiB;
			else if (!strcmp(size_unit, "gib") || !strcmp(size_unit, "GiB"))
				size_bytes *= GiB;
			else
				return SIZE_ERROR;
		}
	}

	return (long long)size_bytes;
}

int get_file_mode(char *target_file, struct stat *statbuf)
{
	if (lstat(target_file, statbuf) < 0){
        printf("ERROR: lstat error for %s\n", target_file);
        return 0;
    }

    if (S_ISREG(statbuf->st_mode))
    	return REGFILE;
    else if(S_ISDIR(statbuf->st_mode))
    	return DIRECTORY;
    else
    	return 0;
}

//경로와 파일명을 이용해서 절대경로 만들어주기
void get_fullpath(char *target_dir, char *target_file, char *fullpath)
{
	strcat(fullpath, target_dir);

	if(fullpath[strlen(target_dir) - 1] != '/')
		strcat(fullpath, "/");

	strcat(fullpath, target_file);
	fullpath[strlen(fullpath)] = '\0';
}

//scandir
int get_dirlist(char *target_dir, struct dirent ***namelist)
{
	int cnt = 0;

	if ((cnt = scandir(target_dir, namelist, NULL, alphasort)) == -1){
		printf("ERROR: scandir error for %s\n", target_dir);
		return -1;
	}

	return cnt;
}

char *get_extension(char *filename)
{
	char *tmp_ext;

	if ((tmp_ext = strstr(filename, ".tar")) != NULL || (tmp_ext = strrchr(filename, '.')) != NULL)
		return tmp_ext + 1;
	else
		return NULL;
}

void get_username() {

	struct passwd *pwd;
	
	if((pwd = getpwuid(getuid())) == NULL) 	
		fprintf(stderr, "getpwuid error\n");
		
	strcpy(username, pwd->pw_name);

}

void get_filename(char *path, char *filename)
{
	char tmp_name[NAMEMAX];
	char *pt = NULL;

	memset(tmp_name, 0, sizeof(tmp_name));
	
	if (strrchr(path, '/') != NULL)
		strcpy(tmp_name, strrchr(path, '/') + 1);
	else
		strcpy(tmp_name, path);
	
	if ((pt = get_extension(tmp_name)) != NULL)
		pt[-1] = '\0';

	if (strchr(path, '/') == NULL && (pt = strrchr(tmp_name, '.')) != NULL)
		pt[0] = '\0';
	
	strcpy(filename, tmp_name);
}

void print_log(logInfo* loginfo) {

	FILE* fp;
	char buf[30000];
	//memset(buf, 0, 15000*sizeof(char));
	if((fp = fopen(log_path, "a+"))<0) {
		fprintf(stderr, "fopen error in print_log");
		return;
	}
	
	sprintf(buf, "%s %s %s %s %s\n", loginfo->command, loginfo->path, loginfo->date, loginfo->time, loginfo->username);
	fwrite(buf, strlen(buf), 1, fp);
	fclose(fp);
}

void get_new_file_name(char *org_filename, char *new_filename)
{
	char new_trash_path[PATHMAX];
	char tmp[NAMEMAX];
	struct dirent **namelist;
	int trashlist_cnt;
	int same_name_cnt = 1;

	get_filename(org_filename, new_filename);
	trashlist_cnt = get_dirlist(trash_path, &namelist);

	for (int i = 0; i < trashlist_cnt; i++){
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		memset(tmp, 0, sizeof(tmp));
		get_filename(namelist[i]->d_name, tmp);

		if (!strcmp(new_filename, tmp))
			same_name_cnt++;
	}
	
	sprintf(new_filename + strlen(new_filename), ".%d", same_name_cnt);

	if (get_extension(org_filename) != NULL)
		sprintf(new_filename + strlen(new_filename), ".%s", get_extension(org_filename));
}

//dir 하위 정규파일 지우기 (디렉토리는 차있으면 삭제x)
void remove_files(char *dir)
{
	struct dirent **namelist;
	int listcnt = get_dirlist(dir, &namelist);

	for (int i = 0; i < listcnt; i++){
		char fullpath[PATHMAX] = {0, };

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		get_fullpath(dir, namelist[i]->d_name, fullpath);

		remove(fullpath);
	}
}

void get_same_size_files_dir(void)
{
	get_path_from_home("~/20200000", same_size_files_dir);

	if (access(same_size_files_dir, F_OK) == 0)
		remove_files(same_size_files_dir);
	else
		mkdir(same_size_files_dir, 0755);
}

void get_trash_path(void)
{
	if (getuid() == 0){ //in root
		get_path_from_home("~/.Trash/files/", trash_path);

		if (access(trash_path, F_OK) == 0)
			remove_files(trash_path);
		else
			mkdir(trash_path, 0755);
	}
	else
		get_path_from_home("~/.Trash/files/", trash_path);
}

void get_log_path(void) {

	if (getuid() == 0) //in root
		get_path_from_home("~/.duplicate_20182593.log", log_path);

	else
		get_path_from_home("~/.duplicate_20182593.log", log_path);
}

//argument 분석해서 전역변수에 값 저장, 정상:0, 비정상:1 리턴
int check_args_fmd5(int argc, char *argv[])
{
	
	extern char* optarg;	
	int num;
	threadnum=1;
	
	optind = 1;

	if (argc!=9 && argc!=11){
		printf("Usage: find [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] [THREAD_NUM]\n");
		return 1;
	}

	while((num = getopt(argc, argv, "e:l:h:d:t:"))!=-1) {

		switch(num) {

			//file extension
			case 'e':				
				if (strchr(optarg, '*') == NULL){
					printf("ERROR: [FILE_EXTENSION] should be '*' or '*.extension'\n");
					return 1;
				}

				if (strchr(optarg, '.') != NULL){
					strcpy(extension, get_extension(optarg));
	
					if (strlen(extension) == 0){
						printf("ERROR: There should be extension\n");
						return 1;
					}
				}
				break;

			//minsize
			case 'l':
				minbsize = get_size(optarg);
				if (minbsize == -1)
					minbsize = 0;
				break;

			//maxsize
			case 'h':
				maxbsize = get_size(optarg);
				if (minbsize == SIZE_ERROR || maxbsize == SIZE_ERROR){
					printf("ERROR: Size wrong -min size : %lld -max size : %lld\n", minbsize, maxbsize);
					return 1;
				}

				if (maxbsize != -1 && minbsize > maxbsize){
					printf("ERROR: [MAXSIZE] should be bigger than [MINSIZE]\n");
					return 1;
				}
				break;

			//target_directory
			case 'd':
				if (strchr(optarg, '~') != NULL)
					get_path_from_home(optarg, target_dir);
				else{
					if (realpath(optarg, target_dir) == NULL){
						printf("ERROR: [TARGET_DIRECTORY] should exist\n");
						return 1;
					}
				}

				if (access(target_dir, F_OK) == -1){
					printf("ERROR: %s directory doesn't exist\n", target_dir);
					return 1;
				}

				if (!is_dir(target_dir)){
					printf("ERROR: [TARGET_DIRECTORY] should be a directory\n");
					return 1;
				}
				break;
				
			//thread_num
			case 't':
				threadnum = atoi(optarg); //추가 쓰레드 개수 = 최대 쓰레드 개수-1
				if(threadnum > 5)
					threadnum = 5; //추가 쓰레드 최대 개수 : 4
				break;
				
		}
	}

	return 0;
}

void filesize_with_comma(long long filesize, char *filesize_w_comma)
{
	char filesize_wo_comma[STRMAX] = {0, };
	int comma;
	int idx = 0;

	sprintf(filesize_wo_comma, "%lld", filesize);
	comma = strlen(filesize_wo_comma)%3;

	for (int i = 0 ; i < strlen(filesize_wo_comma); i++){
		if (i > 0 && (i%3) == comma)
			filesize_w_comma[idx++] = ',';

		filesize_w_comma[idx++] = filesize_wo_comma[i];
	}

	filesize_w_comma[idx] = '\0';
}

void sec_to_ymdt(struct tm *time, char *ymdt)
{
	sprintf(ymdt, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

void sec_to_ymd(struct tm* time, char* ymd) {
	sprintf(ymd, "%04d-%02d-%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday);
}

void sec_to_t(struct tm* time, char* t) {
	sprintf(t, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
}

void filelist_print_format(fileList *head)
{
	fileList *filelist_cur = head->next;
	int set_idx = 1;	

	while (filelist_cur != NULL){
		fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };
		int i = 1;

		filesize_with_comma(filelist_cur->filesize, filesize_w_comma);
		
		if(fileinfolist_size(filelist_cur->fileInfoList)<2) {
			filelist_cur = filelist_cur->next;
			continue;
		}

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, filelist_cur->hash);

		while (fileinfolist_cur != NULL){
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_mtime), mtime);
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_atime), atime);
			printf("[%d] %s (mtime : %s) (atime : %s) (uid : %d) (gid : %d) (mode : %o)\n", i++, fileinfolist_cur->path, mtime, atime, fileinfolist_cur->statbuf.st_uid, fileinfolist_cur->statbuf.st_gid, fileinfolist_cur->statbuf.st_mode);

			fileinfolist_cur = fileinfolist_cur->next;
		}

		printf("\n");

		filelist_cur = filelist_cur->next;
	}	
}

void trash_print_format(trashInfo* head) {

	trashInfo* cur = head->next;
	int idx = 1;
	
	printf("     FILENAME                                   SIZE   DELETION DATE  DELETION TIME\n");
	while(cur!=NULL) {
		printf("[%2d] %s   %ld %s %s\n", idx, cur->path, cur->statbuf.st_size, cur->date, cur->time);
		cur = cur->next;
		idx++;
	}
}

int md5(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	MD5_CTX md5;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	MD5_Init(&md5);

	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		MD5_Update(&md5, buffer, bytes);
	
	MD5_Final(hash, &md5);

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);
	hash_result[HASHMAX-1] = 0;

	fclose(fp);

	return 0;
}


void hash_func(char *path, char *hash)
{
	md5(path, hash);

}


void* list_bfs(void* arg) {
	
	if(cur==NULL)
		return NULL;
	while (cur != NULL) {
//		printf("!\n");
		pthread_mutex_lock(&mutex);
		struct dirent **namelist;
		int listcnt;
		
		if (access(cur->dirpath, F_OK) != 0) break;
		listcnt = get_dirlist(cur->dirpath, &namelist);
//		printf("listcnt:%d\n", listcnt);
		for (int i = 0; i < listcnt; i++) {
			
//			printf("i:%d\n", i);
			char fullpath[PATHMAX] = {0, };
			struct stat statbuf;
			int file_mode;
			long long filesize;

			if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
				continue;
			get_fullpath(cur->dirpath, namelist[i]->d_name, fullpath);
			
			if (!strcmp(fullpath,"/proc") || !strcmp(fullpath, "/run") || !strcmp(fullpath, "/sys") || !strcmp(fullpath, trash_path))
				continue;
//			printf("FULLPATH : %s, pid = %lu\n", fullpath, pthread_self());
			file_mode = get_file_mode(fullpath, &statbuf);

			if ((filesize = (long long)statbuf.st_size) == 0)
				continue;

			if (filesize < minbsize)
				continue;

			if (maxbsize != -1 && filesize > maxbsize)
				continue;

			if (file_mode == DIRECTORY)
				dirlist_append(subdirs, fullpath); //디렉토리일 경우 리스트 추가
			else if (file_mode == REGFILE){ //정규파일일 경우
				FILE *fp;
				char filename[PATHMAX*2];
				char *path_extension;
				char hash[HASHMAX];

				sprintf(filename, "%s/%lld", same_size_files_dir, filesize);

				memset(hash, 0, HASHMAX);
				hash_func(fullpath, hash);

				path_extension = get_extension(fullpath);

				if (strlen(extension) != 0){
					if (path_extension == NULL || strcmp(extension, path_extension))
						continue;
				}

				if ((fp = fopen(filename, "a")) == NULL){
					printf("ERROR: fopen error for %s\n", filename);
					return NULL;
				}

				fprintf(fp, "%s %s\n", hash, fullpath); //별도 파일에 해시값과 경로정보 남기기
				fclose(fp);
			}
		}
		cur = cur->next;
		pthread_mutex_unlock(&mutex);
	}
	pthread_mutex_unlock(&mutex);	
	
	return NULL;
}


void dir_traverse(dirList* dirlist) {

	cur = dirlist->next;
	subdirs = (dirList *)malloc(sizeof(dirList));
	
	memset(subdirs, 0, sizeof(dirList));

	pthread_t thread[5];
//	printf("THREAD NUM>:%d\n", threadnum);
	
	pthread_mutex_init(&mutex, NULL);

	for(int i=0; i<threadnum; i++) {
		pthread_create(&thread[i], NULL, list_bfs, NULL);
	}
//	printf("ended\n");
	for(int i=0; i<threadnum; i++) {
		pthread_join(thread[i], NULL);
	}
	
	pthread_mutex_destroy(&mutex);

	dirlist_delete_all(dirlist);
	
	if (subdirs->next != NULL)
		dir_traverse(subdirs);
			
}


void find_duplicates(void)
{
	struct dirent **namelist;
	int listcnt;
	char hash[HASHMAX];
	FILE *fp;

	listcnt = get_dirlist(same_size_files_dir, &namelist); //duplicate파일 경로

	for (int i = 0; i < listcnt; i++){
		char filename[PATHMAX*2];
		long long filesize;
		char filepath[PATHMAX];
		char hash[HASHMAX];
		char line[STRMAX];

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		filesize = atoll(namelist[i]->d_name);
		sprintf(filename, "%s/%s", same_size_files_dir, namelist[i]->d_name);

		if ((fp = fopen(filename, "r")) == NULL){
			printf("ERROR: fopen error for %s\n", filename);
			continue;
		}

		while (fgets(line, sizeof(line), fp) != NULL){
			int idx;

			strncpy(hash, line, HASHMAX);
			hash[HASHMAX-1] = '\0';

			strcpy(filepath, line+HASHMAX);
			filepath[strlen(filepath)-1] = '\0';

			if ((idx = filelist_search(dups_list_h, hash)) == 0) //이미 리스트에 존재하는 파일이면
				filelist_append(dups_list_h, filesize, filepath, hash); //리스트 추가
			else{
				fileList *filelist_cur = dups_list_h;
				while (idx--){
					filelist_cur = filelist_cur->next;
				}
				fileinfo_append(filelist_cur->fileInfoList, filepath); //아니면 info 추가
			}
		}

		fclose(fp);
	}
}

void remove_no_duplicates(void)
{
	fileList *filelist_cur = dups_list_h->next;

	while (filelist_cur != NULL){
		fileInfo *fileinfo_cur = filelist_cur->fileInfoList;

		if (fileinfolist_size(fileinfo_cur) < 2)
			filelist_delete_node(dups_list_h, filelist_cur->hash);
		
		filelist_cur = filelist_cur->next;
	}
}

time_t get_recent_mtime(fileInfo *head, char *last_filepath)
{
	fileInfo *fileinfo_cur = head->next;
	time_t mtime = 0;

	while (fileinfo_cur != NULL){
		if (fileinfo_cur->statbuf.st_mtime > mtime){
			mtime = fileinfo_cur->statbuf.st_mtime;
			strcpy(last_filepath, fileinfo_cur->path);
		}
		fileinfo_cur = fileinfo_cur->next;
	}
	return mtime;
}


void swap_data(fileInfo* n1, fileInfo* n2) {

	char path[PATHMAX];
	struct stat sb;
	
	strcpy(path, n1->path);
	strcpy(n1->path, n2->path);
	strcpy(n2->path, path);
	
	sb = n1->statbuf;
	n1->statbuf = n2->statbuf;
	n2->statbuf = sb;
}

void swap_list(fileList* n1, fileList* n2) {

	long long tmpsize;
	char tmphash[HASHMAX];
	fileInfo* tmpinfo;
	
	tmpsize = n1->filesize;
	n1->filesize = n2->filesize;
	n2->filesize = tmpsize;
	
	strcpy(tmphash, n1->hash);
	strcpy(n1->hash, n2->hash);
	strcpy(n2->hash, tmphash);
	
	tmpinfo = n1->fileInfoList;
	n1->fileInfoList = n2->fileInfoList;
	n2->fileInfoList = tmpinfo;

}


//옵션 관리
void delete_prompt(void)
{
	while (filelist_size(dups_list_h) > 0){
		char input[STRMAX];
		char last_filepath[PATHMAX];
		char modifiedtime[STRMAX];
		char *argv[ARGMAX];
		int argc;
		int set_idx;
		time_t mtime = 0;
		fileList *target_filelist_p;
		fileInfo *target_infolist_p;
		int num;
		
		optind = 1;
				
		printf(">> ");
		
		fgets(input, sizeof(input), stdin);
		
		if (!strcmp(input, "exit\n")){
			printf(">> Back to Prompt\n");
			break;
		}
		else if(!strcmp(input, "\n"))
			continue;
		
		argc = tokenize(input, argv);

		if (argc!=4 && argc!=5){
			printf("ERROR: >> delete -l [SET_IDX] -? ([LIST_IDX])\n");
			continue;
		}
		if (strcmp(argv[0], "delete") || strcmp(argv[1], "-l")) {
			printf("ERROR: >> delete -l [SET_IDX] -? ([LIST_IDX])\n");
			continue;
		}
		if (atoi(argv[2]) < 0 || atoi(argv[2]) > filelist_size(dups_list_h)){
			printf("ERROR: [SET_INDEX] out of range\n");
			continue;
		}
		
		while((num = getopt(argc, argv, "l:d:tfi"))!=-1) {
			switch(num) {
				case 'l':
					set_idx = atoi(optarg);
			}
		}
		
		target_filelist_p = dups_list_h->next;

		while (--set_idx) {
			if(target_filelist_p->fileInfoList->next->next==NULL) //파일 1개 set(-t)
				set_idx++;
			target_filelist_p = target_filelist_p->next; //set_idx번 이동
		}

		target_infolist_p = target_filelist_p->fileInfoList;

		mtime = get_recent_mtime(target_infolist_p, last_filepath);
		sec_to_ymdt(localtime(&mtime), modifiedtime);

		set_idx = atoi(argv[2]);

		//option f >> set_idx에서 최근수정 파일 제외하고 삭제
		if (!strcmp(argv[3],"-f")) {
		
			fileInfo* tmp;		
			fileInfo *deleted = target_infolist_p->next;
			time_t now = time(NULL);
			char deleteddate[STRMAX];
			char deletedtime[STRMAX];

			while (deleted != NULL){
			
				logInfo* loginfo = (logInfo*)malloc(sizeof(logInfo));
				tmp = deleted->next;
				
				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}
				
				strcpy(loginfo->command, "[DELETE]");
				strcpy(loginfo->path, deleted->path);
				sec_to_ymd(localtime(&now), deleteddate);
				sec_to_t(localtime(&now), deletedtime);
				strcpy(loginfo->date, deleteddate);
				strcpy(loginfo->time, deletedtime);
				strcpy(loginfo->username, username);				
				print_log(loginfo);
								
				remove(deleted->path);
				free(deleted);
				deleted = tmp;
			}

			filelist_delete_node(dups_list_h, target_filelist_p->hash);
			printf("Left file in #%d : %s (%s)\n\n", atoi(argv[0]), last_filepath, modifiedtime);
		}
		//option t >> set_idx에서 최근수정 파일 제외하고 쓰레기통
		else if(!strcmp(argv[3],"-t")) {
		
			fileInfo *tmp;
			fileInfo *deleted = target_infolist_p->next;
			char move_to_trash[PATHMAX];
			char filename[PATHMAX];
			time_t now = time(NULL);
			char deleteddate[STRMAX];
			char deletedtime[STRMAX];

			while (deleted != NULL){
			
				logInfo* loginfo = (logInfo*)malloc(sizeof(logInfo));
				tmp = deleted->next;
				
				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}
				
				strcpy(loginfo->command, "[REMOVE]");
				strcpy(loginfo->path, deleted->path);
				sec_to_ymd(localtime(&now), deleteddate);
				sec_to_t(localtime(&now), deletedtime);
				strcpy(loginfo->date, deleteddate);
				strcpy(loginfo->time, deletedtime);
				strcpy(loginfo->username, username);
				print_log(loginfo);

				trashinfo_append(trash_list_h, deleted->path, deleteddate, deletedtime, target_filelist_p->hash);

				memset(move_to_trash, 0, sizeof(move_to_trash));
				memset(filename, 0, sizeof(filename));
				
				sprintf(move_to_trash, "%s%s", trash_path, strrchr(deleted->path, '/') + 1);

				if (access(move_to_trash, F_OK) == 0){
					get_new_file_name(deleted->path, filename);

					strncpy(strrchr(move_to_trash, '/') + 1, filename, strlen(filename));
				}
				else
					strcpy(filename, strrchr(deleted->path, '/') + 1);
				
				if (rename(deleted->path, move_to_trash) == -1){
					printf("ERROR: Fail to move duplicates to Trash\n");
					printf("target : %s\n", move_to_trash);
					continue;
				}
				
				fileinfo_delete_node(target_infolist_p, deleted->path);
				free(deleted);
				deleted = tmp;
			}

			//filelist_delete_node(dups_list_h, target_filelist_p->hash);
			printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n", atoi(argv[2]), last_filepath, modifiedtime);
		}
		//option i >> set_idx에서 y/n 하나씩
		else if(!strcmp(argv[3],"-i")) {
		
			char ans[STRMAX];
			fileInfo *fileinfo_cur = target_infolist_p->next;
			fileInfo *deleted_list = (fileInfo *)malloc(sizeof(fileInfo));
			fileInfo *tmp;
			int listcnt = fileinfolist_size(target_infolist_p);
			time_t now = time(NULL);
			char deleteddate[STRMAX];
			char deletedtime[STRMAX];


			while (fileinfo_cur != NULL && listcnt--){
				logInfo* loginfo = (logInfo*)malloc(sizeof(logInfo));
				
				printf("Delete \"%s\"? [y/n] ", fileinfo_cur->path);
				memset(ans, 0, sizeof(ans));
				fgets(ans, sizeof(ans), stdin);

				if (!strcmp(ans, "y\n") || !strcmp(ans, "Y\n")){
					strcpy(loginfo->command, "[DELETE]");
					strcpy(loginfo->path, fileinfo_cur->path);
					sec_to_ymd(localtime(&now), deleteddate);
					sec_to_t(localtime(&now), deletedtime);
					strcpy(loginfo->date, deleteddate);
					strcpy(loginfo->time, deletedtime);
					strcpy(loginfo->username, username);
					print_log(loginfo);
					
					remove(fileinfo_cur->path);
					fileinfo_cur = fileinfo_delete_node(target_infolist_p, fileinfo_cur->path);				
				}
				else if (!strcmp(ans, "n\n") || !strcmp(ans, "N\n"))
					fileinfo_cur = fileinfo_cur->next;
				else {
					printf("ERROR: Answer should be 'y/Y' or 'n/N'\n");
					break;
				}
			}
			printf("\n");

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);

		}
		//option d > list_idx 선택해서 1개 제거
		else if(!strcmp(argv[3], "-d")){
			fileInfo *deleted;
			int list_idx;
			
			time_t now = time(NULL);
			char deleteddate[STRMAX];
			char deletedtime[STRMAX];
			logInfo* loginfo = (logInfo*)malloc(sizeof(logInfo));

			if (argv[4] == NULL || (list_idx = atoi(argv[4])) == 0){
				printf("ERROR: There should be an index\n");
				continue;
			}

			if (list_idx < 0 || list_idx > fileinfolist_size(target_infolist_p)){
				printf("ERROR: [LIST_IDX] out of range\n");
				continue;
			}

			deleted = target_infolist_p;

			while (list_idx--)
				deleted = deleted->next;

			printf("\"%s\" has been deleted in #%d\n\n", deleted->path, atoi(argv[2]));
			strcpy(loginfo->command, "[DELETE]");
			strcpy(loginfo->path, deleted->path);
			sec_to_ymd(localtime(&now), deleteddate);
			sec_to_t(localtime(&now), deletedtime);
			strcpy(loginfo->date, deleteddate);
			strcpy(loginfo->time, deletedtime);
			strcpy(loginfo->username, username);				
			print_log(loginfo);
			
			remove(deleted->path);
			fileinfo_delete_node(target_infolist_p, deleted->path);

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);
		}
		else {
			printf("ERROR: Only f, t, i, d options are available\n");
			continue;
		}

		filelist_print_format(dups_list_h);
	}
}


