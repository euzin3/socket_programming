#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/uio.h> //writev, readv

#define MODE_SIZE 6      // Mode 문자열 크기 (save, load, quit)
#define ID_SIZE 4        // ID 크기 (4 bytes)
#define REQ_SIZE 100     // 수식 문자열 최대 길이
#define BUF_SIZE 1024    // 버퍼 크기
#define DATA_MAX 100     // 저장 가능한 최대 수식 개수

// 데이터 구조체 (ID + 수식 문자열)
typedef struct data {
    char id[ID_SIZE];     // 사용자 ID
    char req[REQ_SIZE];   // 수식 문자열 "1+2=3"
} data;

data dataArray[DATA_MAX]; // 수식 저장소   
int dataCount = 0;        // 현재 저장된 수식 개수

void error_handling(char *buf);
void read_childproc(int sig);
void store_loop(int fds_a[2], int fds_b[2]);     // child process 실행 함수 
void multiplex_serv(char *port, int fds_a[2], int fds_b[2]); // parent process 실행 함수

int main(int argc, char *argv[])
{
	int fds_a[2], fds_b[2];
    pid_t pid;
    int state;

    // SIGCHLD 핸들러 등록 (zombie process 방지)
    struct sigaction act;
	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	state=sigaction(SIGCHLD, &act, 0);

    if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    // pipe 생성 
    pipe(fds_a); // parent → child
	pipe(fds_b); // child → parent
	pid=fork();

    if (pid==0){ // Child proces
        store_loop(fds_a, fds_b); 
    } 
    else { // Parent proces
        multiplex_serv(argv[1], fds_a, fds_b); 
    }
    return 0;
}

// Child process: save/load 요청 처리
void store_loop(int fds_a[2], int fds_b[2]){
    close(fds_a[1]);    // write close
    close(fds_b[0]);    // read close

    while(1){
        data d;
        int len = read(fds_a[0], &d, sizeof(d));
        if(len<=0) continue;

        // quit 수신 시 종료 
        if (strncmp(d.id, "quit", 4) == 0) exit(0);

        // load 요청 처리
        if (strncmp(d.req, "__load__", 8) == 0) {
            char result[BUF_SIZE] = "";
            for (int i = 0; i < dataCount; i++) {
                if (strncmp(dataArray[i].id, d.id, ID_SIZE) == 0) {
                    strcat(result, dataArray[i].req);
                    strcat(result, "\n");
                }
            }
            if (strlen(result) == 0) strcpy(result, "Not exist");
            // parent로 전송
            write(fds_b[1], result, strlen(result));
        } 
        else {
            // save 요청 처리
            if (dataCount < DATA_MAX) {
                dataArray[dataCount++] = d;
            }
        }
    }
}

// Parent process: Client 접속 및 요청 처리
void multiplex_serv(char *port, int fds_a[2], int fds_b[2]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    struct timeval timeout;
    fd_set reads, cpy_reads;
    int fd_max, str_len, fd_num, i;

    // server socket 생성  
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    // port 재사용 설정 
    int enable = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(port));

    // bind
    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    // pipe 방향 설정
    close(fds_a[0]);
    close(fds_b[1]);

    while(1)
    {
        cpy_reads=reads;
		timeout.tv_sec=5;
		timeout.tv_usec=0;

		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))==-1)
			break;
		
		if(fd_num==0)
			continue; //time-out 처리
        
        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &cpy_reads)) 
            {
                if (i == serv_sock) // connection request
                {
                    // 새 client 연결 수락
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if (fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("connected client: %d\n", clnt_sock);
                }     
                else // read message
                {
                    // client 데이터 수신
                    char mode[MODE_SIZE];
                    char id[ID_SIZE + 1];
                    char calc_data[BUF_SIZE];
                    struct iovec vec[3];

                    vec[0].iov_base = mode;
                    vec[0].iov_len = MODE_SIZE;
                    vec[1].iov_base = id;
                    vec[1].iov_len = ID_SIZE;
                    vec[2].iov_base = calc_data;
                    vec[2].iov_len = BUF_SIZE;

                    str_len = readv(i, vec, 3);
                    if (str_len <= 0) {  // close request
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client: %d\n", i);
                        continue;
                    }

                    mode[MODE_SIZE-1] = '\0';
                    id[ID_SIZE] = '\0';

                    // -------------------------------------------
                    // Mode: SAVE
                    // -------------------------------------------
                    if (strncmp(mode, "save", 4) == 0) 
                    {
                        int opCount = (unsigned char)calc_data[0];
                        int *operands = (int*)&calc_data[1];
                        char *operators = &calc_data[1 + 4 * opCount];

                        printf("Operand count: %d\n", opCount);
                        for (int j = 0; j < opCount; j++) {
                            //printf("Operand %d: %d\n", j, operands[j]);
                            operands[j] = ntohl(operands[j]);
                            printf("Operand %d: %d\n", j, operands[j]);
                        }

                        int result = operands[0];
                        for (int j = 1; j < opCount; j++) {
                            switch (operators[j - 1]) {
                                case '+': result += operands[j]; break;
                                case '-': result -= operands[j]; break;
                                case '*': result *= operands[j]; break;
                            }
                        }

                        printf("Operation result: %d\n", result);

                        // 수식 문자열 생성
                        char expr[REQ_SIZE] = "";
                        char temp[32];
                        sprintf(temp, "%d", operands[0]);
                        strcat(expr, temp);
                        for (int j = 1; j < opCount; j++) {
                            sprintf(temp, "%c%d", operators[j-1], operands[j]);
                            strcat(expr, temp);
                        }
                        sprintf(temp, "=%d", result);
                        strcat(expr, temp);

                        printf("save to %s\n", id);

                        // child에게 저장 요청
                        data d;
                        memcpy(d.id, id, ID_SIZE);
                        memcpy(d.req, expr, REQ_SIZE);
                        write(fds_a[1], &d, sizeof(d));

                        // 결과 전송
                        sprintf(expr, "%d", result);
                        write(i, expr, strlen(expr));
                    }

                    // -------------------------------------------
                    // Mode: LOAD
                    // -------------------------------------------
                    else if (strncmp(mode, "load", 4) == 0) 
                    {
                        printf("load from %s\n", id);
                        data d;
                        memcpy(d.id, id, ID_SIZE);
                        strcpy(d.req, "__load__");
                        write(fds_a[1], &d, sizeof(d)); // child에게 요청

                        usleep(100000); // child 처리 대기
                        int len = read(fds_b[0], calc_data, BUF_SIZE-1);
                        if (len > 0) {
                            calc_data[len] = '\0'; // 널 문자 추가
                            write(i, calc_data, strlen(calc_data)); // client에 전송
                        }
                    }

                    // -------------------------------------------
                    // Mode: QUIT
                    // -------------------------------------------
                    else if (strncmp(mode, "quit", 4) == 0) 
                    {
                        printf("quit\n");
                        // child process에 quit 알림
                        data d;
                        memcpy(d.id, "quit", 4);
                        write(fds_a[1], &d, sizeof(d));

                        // client에 quit 문자열 전송
                        char quit_msg[] = "quit";
                        write(i, quit_msg, strlen(quit_msg));

                        // client 소켓 닫기 로그 출력
                        printf("closed client: %d\n", i);

                        // client 소켓 닫고, server 소켓 닫고 종료
                        close(i);
                        FD_CLR(i, &reads);
                        close(serv_sock);
                        exit(0);
                    }
                }
            }
        }
    }
    close(serv_sock);
}
	
// SIGCHLD 핸들러
void read_childproc(int sig) {
  pid_t pid;
  int status;
  pid = waitpid(-1, &status, WNOHANG);
  printf("removed proc id: %d \n", pid);
}

// 에러 출력 함수
void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}