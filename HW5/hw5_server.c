#define _XOPEN_SOURCE 200
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
#define MAX 1024
void error_handling(char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;

	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;

    char buf[BUF_SIZE];
    char opCount;
    int opResult;
    int operands[MAX];
    char operators[MAX];

	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    int enable = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	state=sigaction(SIGCHLD, &act, 0);

	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
    // bind
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	
	while(1)
	{
		adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		
        // 새 client 요청 받음 
        if(clnt_sock==-1)
			continue;   
		else
			puts("new client connected...");
		
        // fork 
        pid=fork();
		if(pid==-1)
		{
			close(clnt_sock);
			continue;
		}

        // child process
		if(pid==0) 
		{
			close(serv_sock);

            // 연산 서비스 제공
            read(clnt_sock, &opCount, 1);
            if(opCount <= 0) {
                printf("Save file(%d)\n", opCount);
                close(clnt_sock);
                break;
            }

            for(int i=0; i<opCount; i++){
                read(clnt_sock, &operands[i],4);
            }
            
            for(int i=0; i<opCount-1; i++){
                read(clnt_sock, &operators[i], 1);
            }
            
            opResult = operands[0];
            printf("%d: %d", getpid(), operands[0]);
            for(int i=0; i<opCount-1; i++){
                printf("%c%d", operators[i], operands[i+1]);
                switch(operators[i]){
                    case '+':
                        opResult += operands[i+1];
                        break;
                    case '-':
                        opResult -= operands[i+1];
                        break;
                    case '*':
                        opResult *= operands[i+1];
                        break;
                }
            }
            printf("=%d\n", opResult);
            write(clnt_sock, &opResult, 4);
            close(clnt_sock);
        
			return 0;
		}
        //parent process
		else  
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void read_childproc(int sig)
{
	pid_t pid;
	int status;
	pid=waitpid(-1, &status, WNOHANG);
	printf("removed proc id: %d \n", pid);
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}