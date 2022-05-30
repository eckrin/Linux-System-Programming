#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ssu_sfinder.h"

int split(char *input, char *argv[]);
int command_fmd5(int argc, char *argv[]);
void command_help();
void command_list(int argc, char* argv[]);
void command_trash(int argc, char* argv[]);
void command_restore(int argc, char* argv[]);

int main(void)
{
	while (1) {
		char input[STRMAX];
		char* argv[ARGMAX];

		int argc = 0;
		pid_t pid;

		printf("20182593> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';
		
		argc = split(input, argv);
		argv[argc] = (char *)0;
		
		if (argc == 0)
			continue;

		if (!strcmp(argv[0], "exit")) {
			break;
		}
		else if (!strcmp(argv[0], "fmd5")){
			command_fmd5(argc, argv);
		}
		else if (!strcmp(argv[0], "list")){
			command_list(argc, argv);
		}
		else if (!strcmp(argv[0], "trash")){
			command_trash(argc, argv);
		}
		else if (!strcmp(argv[0], "restore")){
			command_restore(argc, argv);
		}
		else {
			command_help();
		}
	}
	
	printf("Prompt End\n");
	
	return 0;
}

int command_fmd5(int argc, char *argv[])
{
	dirList *dirlist = (dirList *)malloc(sizeof(dirList));
	
	dups_list_h = (fileList *)malloc(sizeof(fileList));
	
	trash_list_h = (trashInfo*)malloc(sizeof(trashInfo));

	if (check_args_fmd5(argc, argv)) //argument 분석
		return 1;

	get_same_size_files_dir(); //duplicate파일 저장할 디렉토리 만들기

	struct timeval begin_t, end_t;

	gettimeofday(&begin_t, NULL);
	dirlist_append(dirlist, target_dir); //리스트 시작노드 만들기
	dir_traverse(dirlist); //디렉토리 탐색(리스트를 이용해서 bfs), duplicate 파일 작성
	find_duplicates(); //duplicate파일 이용해서 동일파일 탐색, 리스트 만들기
	remove_no_duplicates(); //동일X파일 리스트에서 제거

	gettimeofday(&end_t, NULL);

	end_t.tv_sec -= begin_t.tv_sec;

	if (end_t.tv_usec < begin_t.tv_usec){
		end_t.tv_sec--;
		end_t.tv_usec += 1000000;
	}

	end_t.tv_usec -= begin_t.tv_usec;

	if (dups_list_h->next == NULL)
		printf("No duplicates in %s\n", target_dir);
	else 
		filelist_print_format(dups_list_h);

	printf("Searching time: %ld:%06ld(sec:usec)\n\n", end_t.tv_sec, end_t.tv_usec);

	get_trash_path();
	get_log_path();
	get_username();

	delete_prompt(); //delete option

	return 0;
}

void command_restore(int argc, char* argv[]) {

	trashInfo* past = trash_list_h;
	trashInfo* cur = trash_list_h->next;
	int idx = atoi(argv[1]);
	fileList* filelist = dups_list_h->next;
	char trashpath_tmp[PATHMAX];
	char trashpath[PATHMAX];
	char filename[NAMEMAX];
	char* ptr_filename;
	time_t now;
	char deleteddate[STRMAX];
	char deletedtime[STRMAX];
	char* trash_argv[1];
	
	if(strcmp(argv[0], "restore")) {
		fprintf(stderr, "Usage: restore [RESTORE_INDEX]");
		return;
	}
	
	for(int i=0; i<idx-1; i++)  {
		cur = cur->next;
		past = past->next;
	}
	
	//fileList 찾기
	while(strcmp(filelist->hash, cur->hash)) {
		//printf("check\n");
		if(filelist==NULL) {
			fprintf(stderr, "listinfo doesn't exist\n");
			return;
		}
		filelist = filelist->next;
	}
	//실제 파일 이동
	ptr_filename = strrchr(cur->path, '/');
	strcpy(filename, ptr_filename+1);
	sprintf(trashpath_tmp, "%s%s", "~/.Trash/files/", filename);
	get_path_from_home(trashpath_tmp, trashpath);
	if(rename(trashpath, cur->path));
	now = time(NULL);

	//trashInfo에서 제거
	past->next = cur->next;
	
	fileinfo_append(filelist->fileInfoList, cur->path); //fileInfo에 추가
	printf("[RESTORE] success for %s\n", cur->path);
	
	logInfo* loginfo = (logInfo*)malloc(sizeof(logInfo));
	strcpy(loginfo->command, "[RESTORE]");
	strcpy(loginfo->path, cur->path);
	sec_to_ymd(localtime(&now), deleteddate);
	sec_to_t(localtime(&now), deletedtime);
	strcpy(loginfo->date, deleteddate);
	strcpy(loginfo->time, deletedtime);
	strcpy(loginfo->username, username);
	print_log(loginfo);
	
	trash_argv[0] = "trash";
	command_trash(1, trash_argv);
	
	free(cur);
}

void command_trash(int argc, char* argv[]) {

	char category[STRMAX] = "filename";
	char order = 1;
	int num;
	
	optind = 1;
	
	while((num = getopt(argc, argv, "c:o:"))!=-1) {
	
		switch(num) {
		
			case 'c':
				strcpy(category, optarg);
				if(strcmp(category, "filename") && strcmp(category, "size") && strcmp(category, "date") && strcmp(category, "time")) {
					fprintf(stderr, "-c error\n");
					return;
				}
				break;
				
			case 'o':
				order = atoi(optarg);
				if(order!=1 && order!=-1) {
					fprintf(stderr, "-o error\n");
					return;
				}
				break;
		}
	}
	
	if(trash_list_h->next==NULL) {
		printf("No files in trash\n");
		return;
	}
	
	trashInfo* cur = trash_list_h->next;
	trashInfo* past = trash_list_h;
	int trashlist_size = trashinfo_size(trash_list_h);
	
	if(cur==NULL && cur->next==NULL) {
		fprintf(stderr, "trash is NULL\n");
		return;
	}

	for(int i=0; i<trashlist_size; i++) {
		
		if(cur->next==NULL) break;
		for(int j=0; j<trashlist_size-i-1; j++) {
		
			if(cur->next==NULL) {
				//fprintf(stderr, "segm-i:%d, j:%d\n", i, j);
				//fprintf(stderr, "%s, %s, %s, %s\n", trash_list_h->next->path, trash_list_h->next->next->path, trash_list_h->next->next->next->path, trash_list_h->next->next->next->next->path);
				return;
			}
			
			if(!strcmp(category, "filename") && order==1) {
				//filename, 오름차순
				if(strcmp(cur->path, cur->next->path)>0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "filename") && order==-1) {
				//filename, 내림차순
				if(strcmp(cur->path, cur->next->path)<0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "size") && order==1) {
				//size, 오름차순
				if(cur->statbuf.st_size > cur->next->statbuf.st_size) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "size") && order==-1) {
				//size, 내림차순
				if(cur->statbuf.st_size < cur->next->statbuf.st_size) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "date") && order==1) {
				//date, 오름차순
				if(strcmp(cur->date, cur->next->date)>0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "date") && order==-1) {
				//date, 내림차순
				if(strcmp(cur->date, cur->next->date)<0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "time") && order==1) {
				//time, 오름차순
				if(strcmp(cur->time, cur->next->time)>0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			else if(!strcmp(category, "time") && order==-1) {
				//time, 내림차순
				if(strcmp(cur->time, cur->next->time)<0) {
					past->next = cur->next;
					cur->next = cur->next->next;
					past->next->next = cur;
					cur = past->next;
				}
			}
			cur = cur->next;
			past = past->next;
		}
		cur = trash_list_h->next;
		past = trash_list_h;
	}
	trash_print_format(trash_list_h);
	
}


void command_list(int argc, char* argv[]) {

	char list_type[STRMAX] = "fileset";
	char category[STRMAX] = "size";
	int order = 1;
	int num;
	
	optind = 1;
	
	if(dups_list_h==NULL) {
		fprintf(stderr, "command_list before command_fmd5\n");
		return;
	}
	//int setnum = filelist_size(dups_list_h);
	int listnum;
	struct stat* sb;
	
	while((num = getopt(argc, argv, "l:c:o:"))!=-1) {
		
		switch(num) {
		
			case 'l':
				strcpy(list_type, optarg);
				if(strcmp(list_type, "fileset") && strcmp(list_type, "filelist")) {
					fprintf(stderr, "-l error\n");
					return;
				}
				break;
				
			case 'c':
				strcpy(category, optarg);
				if(strcmp(category, "filename") && strcmp(category, "size") && strcmp(category, "uid") && strcmp(category, "gid") && strcmp(category, "mode")) {
					fprintf(stderr, "-c error\n");
					return;
				}
				break;
				
			case 'o':
				order = atoi(optarg);
				if(order!=1 && order!=-1) {
					fprintf(stderr, "-o error\n");
					return;
				}
				break;
		}
	}
	
	//fileset
	if(!strcmp(list_type, "fileset")) {
	
		if(strcmp(category, "size")) {
			return;
		}
		
		int setnum = filelist_size(dups_list_h);
		fileList* curset = dups_list_h->next;
		fileList* start = curset;			
			
		for(int i=0; i<setnum; i++) {
			
			if(curset->next==NULL) break;
				
			for(int j=0; j<setnum-i-1; j++) {				

				//size, 오름차순
				if(!strcmp(category, "size") && order==1) {
					
					if(curset->filesize > curset->next->filesize)
						swap_list(curset, curset->next);
				}
				//size, 내림차순
				else if(!strcmp(category, "size") && order==-1) {
					
					if(curset->filesize < curset->next->filesize)
						swap_list(curset, curset->next);
				}
			
				curset = curset->next;
			}
			curset = start;
		}

		
		filelist_print_format(dups_list_h);
			
		return;
		
	}
	//filelist
	else if(!strcmp(list_type, "filelist")) {
	
		fileList* curset = dups_list_h->next;
		
		while(curset!=NULL) {
			
			listnum = fileinfolist_size(curset->fileInfoList);
			fileInfo* curlist = curset->fileInfoList->next;
			fileInfo* start = curlist;
			//printf("listnum:%d\n", listnum);			
			
			for(int i=0; i<listnum; i++) {
			
				if(curlist->next==NULL) break;
				
				for(int j=0; j<listnum-i-1; j++) {				
//					printf("?>%s %s\n", curlist->path, curlist->next->path);
//					printf("category:%d\n", !strcmp(category, "filename"));
//					printf("order:%d\n", order==-1);
					//filename, 오름차순
					if(!strcmp(category, "filename") && order==1) {
					
						if(strcmp(curlist->path, curlist->next->path)>0)
							swap_data(curlist, curlist->next);
					}
					//filename, 내림차순
					else if(!strcmp(category, "filename") && order==-1) {
					
						if(strcmp(curlist->path, curlist->next->path)<0)
							swap_data(curlist, curlist->next);
					}
					//uid, 오름차순
					else if(!strcmp(category, "uid") && order==1) {
					
						if(curlist->statbuf.st_uid>curlist->next->statbuf.st_uid)
							swap_data(curlist, curlist->next);
					}
					//uid, 내림차순
					else if(!strcmp(category, "uid") && order==-1) {

						if(curlist->statbuf.st_uid<curlist->next->statbuf.st_uid)
							swap_data(curlist, curlist->next);
					}
					//gid, 오름차순
					else if(!strcmp(category, "gid") && order==1) {

						if(curlist->statbuf.st_gid>curlist->next->statbuf.st_gid)
							swap_data(curlist, curlist->next);
					}
					//gid, 내림차순
					else if(!strcmp(category, "gid") && order==-1) {

						if(curlist->statbuf.st_gid<curlist->next->statbuf.st_gid)
							swap_data(curlist, curlist->next);
					}
					//mode, 오름차순
					else if(!strcmp(category, "mode") && order==1) {

						if(curlist->statbuf.st_mode>curlist->next->statbuf.st_mode)
							swap_data(curlist, curlist->next);
					}
					//mode, 내림차순
					else if(!strcmp(category, "mode") && order==-1) {

						if(curlist->statbuf.st_mode<curlist->next->statbuf.st_mode)
							swap_data(curlist, curlist->next);
					}
			
					curlist = curlist->next;
				}
				curlist = start;
			}
			curset = curset->next;
		}
		
		filelist_print_format(dups_list_h);
			
		return;
	}
	else {
		fprintf(stderr, "list_type error\n");
		return;
	}
	
	
}


void command_help() {

	printf("\nUsage : \n > fmd5 -e [FILE_EXTENSION] -l [MINSIZE] -h [MAXSIZE] -d [TARGET_DIRECTORY] -t [THREAD_NUM]\n    >>delete -l [SET_INDEX] -d [OPTARG] -i -f -t\n > list -l [LIST_TYPE] -c [CATEGORY] -o [ORDER]\n > trash -c [CATEGORY] -o [ORDER] \n > restore [RESTORE_INDEX]\n > help\n > exit\n\n");
}


int split(char *input, char *argv[])
{
	char *ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL) {
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}
	
	return argc;
}

