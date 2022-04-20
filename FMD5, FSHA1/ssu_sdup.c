#include"headers.h"


int main() {
		
	int argc;
	char input[1024];
	char* argv[6]; //fmd5 * ~ ~ file
	pid_t pid;
	
	while(1) {
		printf("20182593> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1] = '\0';
		argc = split(input, " ", argv);
		
		argv[5] = (char*)0;
		
		if(argc==0)
			continue;
			
		if(!strcmp(argv[0], "exit")) {
			printf("Prompt End\n");
			break;
		}
		
		if((pid = fork())<0) {
			fprintf(stderr, "fork error\n");
			exit(1);
		}
		//자식 프로세스일 경우
		else if(pid==0) {
			
			if(!strcmp(argv[0], "fmd5")) {
				execv("./md5", argv);
				fprintf(stderr, "fmd5 execute error\n");
			}
			else if(!strcmp(argv[0], "fsha1")) {
				execv("./sha1", argv);
				fprintf(stderr, "fsha1 execute error\n");	
			}
			else {
				execl("./help", "./help", NULL);
				fprintf(stderr, "help execute error\n");	
			}
			fprintf(stderr, "command error\n");
			exit(1);
			
		}
		while(wait((int *)0)!=-1);
		
	}
	
	return 0;
}
