/**
  * LinkedList for Fileinfo
  *
  *
  **/
#include"LinkedList.h"

Node* head = NULL;
Node* tail = NULL;
Node* cur = NULL;

Node* listNode = NULL;

//int nodeNum = 0;
char amTime[NAMEMAX];
char comma_str[100];

char* getMTime(struct stat *buf) { 
	amTime;
        struct tm *t; 
        t=localtime(&buf->st_mtime); 
        sprintf(amTime, "%02d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); 
        return amTime;
}


char* getATime(struct stat *buf) {
	amTime; 
        struct tm *t; 
        t=localtime(&buf->st_atime); 
        sprintf(amTime, "%02d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); 
        return amTime;
}

int getSlashCount(char* str) {

	int cnt=0;
	for(int i=0; i<strlen(str); i++) {
		if(str[i]=='/') cnt++;
	}
	
	return cnt;
}

char* addComma(long n) {

	char str[64];
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





//첫 노드 생성
void createList(Fileinfo f) {

	listNode = (Node*)malloc(sizeof(Node));
	listNode->file = f;
	listNode->nextNode = NULL;
	listNode->nextSameNode = NULL;
	
	head = listNode;
	cur = listNode;
	tail = listNode;
}


//노드 추가
void addList(Fileinfo f) {

	Node* tmp;

	if(head==NULL) {
		createList(f);
		return;
	}

	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->file = f;
	newNode->nextNode = NULL;
	newNode->nextSameNode = NULL;
	
	//노드 초기화
	cur = head;
	
	while(1) {
		//내용이 동일한 파일이 이미 존재하는 경우 (정렬은 bfs를 통해 들어오므로 depth가 작은것부터, 큐에서 scandir의 인자로 alphasort를 주었으므로 자동으로 아스키코드순으로 정렬된다)
		if(!strcmp(newNode->file.hash, cur->file.hash)) {
			tmp = cur;
			while(tmp->nextSameNode!=NULL) {
				tmp = tmp->nextSameNode;
			}
			tmp->nextSameNode = newNode;
			break;
		}
		
		//이미 끝에 다다른경우
		if(cur->nextNode==NULL) {
			cur->nextNode = newNode;
			tail = newNode;
			break;
		}
		//크기 오름차순-경로 오름차순 정렬
		if(cur==head && newNode->file.size < cur->file.size) {
			newNode->nextNode = cur;
			head = newNode;
			break;
		}
		else if(newNode->file.size < cur->nextNode->file.size) {
			newNode->nextNode = cur->nextNode;
			cur->nextNode = newNode;
			break;
		}
		else if (newNode->file.size == cur->nextNode->file.size) {
			if(getSlashCount(newNode->file.path) < getSlashCount(cur->nextNode->file.path)) {
				newNode->nextNode = cur->nextNode;
				cur->nextNode = newNode;
				break;
			}
		}
		cur = cur->nextNode;

	}
}


//리스트 탐색하며 동일파일 출력
//동일파일이 없으면 -1 return
int printList() {

	cur = head;
	Node* tmp;
	int idSetNum=0;
	int idFileNum;
	struct stat statbuf;
	int flag=-1;
	
	if(head==NULL) {
		return flag;
	}
	while(1) {
		//내용이 동일한 파일이 이미 존재하는 경우
		if(cur->nextSameNode!=NULL) {
			flag=0;
			idSetNum++;
			idFileNum=1;
			tmp = cur;
			if(lstat(tmp->file.path, &statbuf)==-1) {
				fprintf(stderr, "lstat error1\n");
				exit(1);
			}
			printf("---- Identical files #%d (%s bytes - %s) ----\n", idSetNum, addComma((double)tmp->file.size), tmp->file.hash);
			printf("[%d] %s (mtime : %s) (atime : %s)\n",  idFileNum, tmp->file.path, getMTime(&statbuf), getATime(&statbuf));
			while(tmp->nextSameNode!=NULL) {
				idFileNum++;
				tmp = tmp->nextSameNode;
				if(lstat(tmp->file.path, &statbuf)==-1) {
					fprintf(stderr, "lstat error1\n");
					exit(1);
				}				
				printf("[%d] %s (mtime : %s) (atime : %s)\n",  idFileNum, tmp->file.path, getMTime(&statbuf), getATime(&statbuf));
			}
			printf("\n");
		}
	
		//이미 끝에 다다른경우
		if(cur==tail) {
			setNum = idSetNum;
			break;
		}
				
		cur = cur->nextNode;
	}
	
	return flag;
	
}



//중복된 리스트에서 삭제 (option:d)
void funcD(int setIdx, int listIdx) {

	cur = head;
	Node* pastcur = NULL;
	int i=1;
	
	//Node* cur 이동
	while(1) {
		if(cur->nextSameNode!=NULL) {
			if(i==setIdx) 
				break;
			i++;
		}
		
		pastcur = cur;
		if(cur->nextNode==NULL) {
			fprintf(stderr, "[SET_INDEX] error\n");
			return;
		}
		cur = cur->nextNode;
	}
	for(int i=1; i<listIdx; i++) {
		pastcur = cur;
		if(cur->nextSameNode==NULL) {
			fprintf(stderr, "[LIST_IDX] error\n");
			return;	
		}
		cur = cur->nextSameNode;
	}
	//파일 제거
	remove(cur->file.path);
	printf("%s has been deleted in #%d\n\n", cur->file.path, setIdx);
	
	//Node 제거
	if(listIdx==1) {
		if(cur==head) {
			cur->nextSameNode->nextNode = cur->nextNode;
			head = cur->nextSameNode;
		}
		else if(cur==tail) {
			pastcur->nextNode = cur->nextSameNode;
			tail = cur->nextSameNode;
		}
		else {
			pastcur->nextNode = cur->nextSameNode;
			cur->nextSameNode->nextNode = cur->nextNode;
		}
	}
	else {
		if(cur->nextSameNode!=NULL) {
			pastcur->nextSameNode = cur->nextSameNode;
		}
		else {
			pastcur->nextSameNode = NULL;
		}
	}
	//메모리 해제
	free(cur);
}


//중복된 리스트에서 한개씩 삭제여부 확인 (option:i)
void funcI(int setIdx) {

	cur = head;
	Node* pastcur = NULL;
	Node* tmp;
	int i=1;
	
	//Node* cur 이동
	while(1) {
		if(cur->nextSameNode!=NULL) {
			if(i==setIdx) 
				break;
			i++;
		}
		
		pastcur = cur;
		if(cur->nextNode==NULL) {
			fprintf(stderr, "[SET_INDEX] error\n");
			return;
		}
		cur = cur->nextNode;
	}
	//진행 탐색
	while(cur!=NULL) {
		char resp;
		printf("Delete \"%s\"? [y/n] ", cur->file.path);
		resp = getchar(); getchar();
		//노드 제거
		if(resp=='y'||resp=='Y') {
			
			if(cur->nextNode!=NULL) {
				if(cur==head) {
					if(cur->nextSameNode!=NULL) {
						cur->nextSameNode->nextNode = cur->nextNode;
						head = cur->nextSameNode;
					}
					else {
						head = cur->nextNode;
					}
				}
				else if(cur==tail) {
					if(cur->nextSameNode!=NULL) {
						pastcur->nextNode = cur->nextSameNode;
						tail = cur->nextSameNode;
					}
					else {
						tail = pastcur;
					}
				}
				else {
					if(cur->nextSameNode!=NULL) {
						pastcur->nextNode = cur->nextSameNode;
						cur->nextSameNode->nextNode = cur->nextNode;
					}
					else {
						pastcur->nextNode = cur->nextNode;
					}
				}
			}
			else {
				if(cur->nextSameNode!=NULL) {
					pastcur->nextSameNode = cur->nextSameNode;
				}
				else {
					pastcur->nextSameNode = NULL;
				}
			}
			tmp = cur;
			cur = cur->nextSameNode;
			//파일 제거
			remove(tmp->file.path);
			//printf("====DELETE %s====\n", tmp->file.path);

			//메모리 해제
			free(tmp);
			
		}
		else if (resp=='n'||resp=='N') {
			pastcur = cur;
			cur = cur->nextSameNode;
		}
		else {
			fprintf(stderr, "input option error\n");
			return;
		}
		i++;
	}
	printf("\n");
	
}


void funcF(int setIdx) {

	Node* pastset = NULL;
	Node* sethead;
	Node* tmp;
	struct stat statbuf;
	struct stat recentstat;
	int i=1;
	time_t recent=0;
	int recentIdx;
	
	//set으로 cur 이동
	cur = head;
	while(1) {
		if(cur->nextSameNode!=NULL) {
			if(i==setIdx) 
				break;
			i++;
		}
		
		pastset = cur;
		if(cur->nextNode==NULL) {
			fprintf(stderr, "[SET_INDEX] error\n");
			return;
		}
		cur = cur->nextNode;
	}
	sethead = cur;
	//탐색하면서 가장 최근 수정 검사. mtime이 같다면 아스키코드 순서
	i=1;
	while(cur!=NULL) {
		if(lstat(cur->file.path, &statbuf)==-1) {
			fprintf(stderr, "lstat error\n");
			return;
		}
		if(recent < statbuf.st_mtime) {
			recent = statbuf.st_mtime;
			recentIdx = i;
			recentstat = statbuf;
		}
		cur = cur->nextSameNode;
		i++;
	}
	
	//최근수정 노드에 리스트 nextNode 연결
	cur = sethead;
	for(int i=1; i<recentIdx; i++) {
		cur = cur->nextSameNode;
	}
	if(pastset!=NULL)
		pastset->nextNode = cur;
	cur->nextNode = sethead->nextNode;
	if(recentIdx!=1)
		sethead->nextNode=NULL;
	//최근수정 노드를 제외하고 삭제or휴지통 이동
	cur = sethead;
	while(cur!=NULL) {
		if(cur->nextNode==NULL) {
			tmp = cur;
			cur = cur->nextSameNode;
			//파일 제거(f)
			if(!optT) {
				remove(tmp->file.path);
				//printf("====DELETE %s====\n", tmp->file.path);
			}
			//파일 이동(t)
			else {
				moveToTrash(tmp->file.path);
			}
			//메모리 해제
			free(tmp);
		}
		else if(cur->nextNode!=NULL || cur==tail) {
			tmp = cur;
			if(!optT)
				printf("Left file in #%d : %s (%s)\n\n", setIdx, tmp->file.path, getMTime(&recentstat));
			else
				printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n", setIdx, tmp->file.path, getMTime(&recentstat));
			cur = cur->nextSameNode;
			tmp->nextSameNode = NULL;
		}
	}
	
	
}

//성공시 0 return, 실패시 1 return
int moveToTrash(char* path) {

	FILE* fp1;
	FILE* fp2;
	
	char fileName[NAMEMAX]; //파일명 저장
	char trash[PATHMAX]; //쓰레기통에 옮길 경로
	char buf[1024];
	char* ptr = NULL;
	struct stat statbuf;
	struct utimbuf timebuf;
	struct passwd *result;
	struct passwd pwd;

	//홈 경로 구하기
	if((result = getpwnam(getlogin()))==NULL) {
		fprintf(stderr, "getpwnam error\n");
		return -1;
	}
	memcpy(&pwd, result, sizeof(struct passwd));
	
	//파일명 구하기
	ptr = strrchr(path, '/');
	if(ptr==NULL) {
		fprintf(stderr, "name error\n");
		return -1;
	}
	strcpy(fileName, ptr+1);
	
	// ~/.local/share/Trash/files 밑으로 이동
	sprintf(trash, "%s/%s/%s", pwd.pw_dir, ".local/share/Trash/files", fileName); //trash : 이동경로

	//rename 이용하여 이동
	if(rename(path, trash)<0) {
		fprintf(stderr, "rename error\n");
		exit(1);
	}
//	printf("TRASH %s To %s\n", path, trash);
	return 0;
}


//동적할당 해제, 전역변수 초기화
void freeList() {

	Node* root = head;
	Node* dummy;
	Node* dummys;

	while(root!=NULL) {
		dummy = root;
		root = root->nextNode;
		while(dummy->nextSameNode!=NULL) {
			dummys = dummy;
			dummy = dummy->nextSameNode;
			free(dummys);
		}
		
		free(dummy);
	}
	
	head = NULL;
	tail = NULL;
	cur = NULL;
	Node* listNode = NULL;


}


