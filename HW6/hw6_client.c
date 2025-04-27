#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h> //writev, readv

#define MODE_SIZE_IN 10
#define ID_SIZE_IN 10
#define MODE_SIZE 6
#define ID_SIZE 4
#define BUF_SIZE 1024

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;
	struct sockaddr_in serv_adr;
    struct iovec vec[3];

    char mode_in[MODE_SIZE_IN];
    char id_in[ID_SIZE_IN];
    char mode[MODE_SIZE];       // 송신용
    char id[ID_SIZE];           // 송신용
    char buf[BUF_SIZE]; 

	if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
    // 1. Mode 입력 
    fputs("Mode: ", stdout);
    fgets(mode_in, sizeof(mode_in), stdin);
    mode_in[strcspn(mode_in, "\n")] = '\0'; // 개행 제거


    // 유효하지 않은 mode 입력 시 처리
    if (strcmp(mode_in, "save") && strcmp(mode_in, "load") && strcmp(mode_in, "quit")) {
        printf("supported mode: save load quit\n");
        exit(1);
    }

    // 송신용 mode 초기화 및 복사
    memset(mode, 0, sizeof(mode));
    strncpy(mode, mode_in, MODE_SIZE);

    // 2. 소켓 연결
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
    else
        puts("Connected....");  
	
    // 3. ID 입력 (save/load만 해당)
    if (strcmp(mode_in, "quit") != 0) {
        fputs("ID: ", stdout);
        fgets(id_in, sizeof(id_in), stdin);
        id_in[strcspn(id_in, "\n")] = '\0';

        // ID 길이가 4(bytes) 아닐 시 처리
        if (strlen(id_in) != 4) {
            printf("Error: ID length must be 4\n");
            close(sock);
            exit(1);
        }

        // 송신용 id 초기화 및 복사
        memset(id, 0, sizeof(id));
        strncpy(id, id_in, ID_SIZE);
    }

    // 4. 계산 데이터 입력 (save만 해당)
    if(strcmp(mode_in, "save") == 0){
        int opCount;
        // 피연산자 개수 입력
        printf("Operand count: ");
        scanf("%d", &opCount);

        buf[0] = (char)opCount;

        // Operand count가 overflow 일어나는 값 (0 이하) 받을 시 처리
        if(opCount <= 0) {
            printf("Overflow will happen (%d)\n", opCount);
            close(sock);
            exit(1);
        }

        // 피연산자 입력 
        for(int i=0; i<opCount; i++) {
            int operand;
            printf("Operand %d: ", i);
            scanf("%d", &operand);
            operand = htonl(operand);
            memcpy(&buf[(i*4)+1], &operand, 4); //4byte 연속 메모리에 복사
        }

        // 연산자 입력 
        for (int i=0; i<opCount-1; i++) {
            char op;
            printf("Operator %d: ", i);
            scanf(" %c", &op);
            buf[(opCount*4)+i+1] = op; // 1byte씩 저장
        }
    }

	// 5. writev로 전송 
    vec[0].iov_base = mode;
    vec[0].iov_len = MODE_SIZE;
    vec[1].iov_base = id;
    vec[1].iov_len = ID_SIZE;
    vec[2].iov_base = buf;
    vec[2].iov_len = BUF_SIZE;

    writev(sock, vec, 3);

    // 6. 서버 응답 수신 (계산 결과 출력)
    str_len = read(sock, buf, BUF_SIZE-1);
    if(str_len > 0) {
        buf[str_len] = 0;

        if(strcmp(buf, "Not exist")==0) {
            printf("Not exist\n");
        }
        else if(strcmp(buf, "quit")==0) {
            printf("quit\n");
        }
        else {
            // load mode
            if (strcmp(mode_in, "load") == 0) {
                char *line = strtok(buf, "\n");
                while (line != NULL) {
                    printf("%s: %s\n", id_in, line);  
                    line = strtok(NULL, "\n");
                }
            } 
            // save mode
            else {
                printf("Operation result: %s\n", buf);
            }
        }
    }
	
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}