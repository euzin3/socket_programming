// hw8_client.c : 멀티스레드 기반 계산기 클라이언트
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/uio.h>

#define BUF_SIZE 1024       // 계산 수식 버퍼 크기
#define NAME_SIZE 4         // 고정된 ID 길이

char name[NAME_SIZE + 1];   // 사용자 ID
char req[BUF_SIZE];         // 수식 요청 문자열

void* send_msg(void* arg);      // 서버로 요청 전송 스레드
void* recv_msg(void* arg);      // 서버 응답 수신 스레드

int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in serv_adr;
    pthread_t snd_thread, rcv_thread;

    // 인자 검증: IP, PORT, ID(4글자)
    if (argc != 4) {
        printf("Usage: %s <IP> <PORT> <ID>\n", argv[0]);
        exit(1);
    }

    if (strlen(argv[3]) != NAME_SIZE) {
        printf("ID have to be 4\n");
        exit(1);
    }

    // ID 복사
    strncpy(name, argv[3], NAME_SIZE);
    name[NAME_SIZE] = '\0';

    // 서버 연결
    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        perror("connect() error");
        exit(1);
    }

    // 송/수신 스레드 생성 및 대기
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(snd_thread, NULL);
    pthread_join(rcv_thread, NULL);

    close(sock);
    return 0;
}

// 사용자 입력을 받아 서버로 계산 요청 전송
void* send_msg(void* arg) {
    int sock = *((int*)arg);
    struct iovec vec[2];

    while (1) {
        fgets(req, BUF_SIZE, stdin);                 // 수식 입력
        req[strcspn(req, "\n")] = '\0';              // 개행 제거

        int tmp_count;
        if (sscanf(req, "%d", &tmp_count) != 1) {
            printf("Invalid input\n");
            continue;
        }

        char signed_count = (char)tmp_count;

        // char 범위 초과 또는 0일 경우 종료
        if ((int)signed_count != tmp_count || signed_count == 0) {
            printf("Overflow value(%d) - client closed\n", signed_count);
            close(sock);
            exit(1);
        }

        // ID와 수식 함께 전송
        vec[0].iov_base = name;
        vec[0].iov_len = NAME_SIZE;
        vec[1].iov_base = req;
        vec[1].iov_len = BUF_SIZE;
        writev(sock, vec, 2);
    }
    return NULL;
}

// 서버로부터 계산 결과 수신 및 출력
void* recv_msg(void* arg) {
    int sock = *((int*)arg);
    char buf[BUF_SIZE + 64];
    int str_len;

    while ((str_len = read(sock, buf, sizeof(buf) - 1)) > 0) {
        buf[str_len] = '\0';
        fputs(buf, stdout);     // 결과 출력
    }
    return NULL;
}
