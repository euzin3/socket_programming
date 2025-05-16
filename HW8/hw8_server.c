// hw8_server.c : 멀티스레드 기반 계산기 서버
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#define BUF_SIZE 1024       // 메시지 버퍼 크기
#define NAME_SIZE 4         // ID 크기
#define MAX_CLNT 100        // 최대 클라이언트 수

int clnt_cnt = 0;                   // 현재 접속한 클라이언트 수
int clnt_socks[MAX_CLNT];          // 클라이언트 소켓 배열
pthread_mutex_t mutx;              // 소켓 공유 보호용 뮤텍스

void* handle_clnt(void* arg);                          // 클라이언트 처리 스레드
void send_msg(const char* msg, int len);               // 모든 클라이언트에 메시지 전송
int calculate_expr(char* req, int* result, char* expr_buf);  // 수식 계산

int main(int argc, char* argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    pthread_t t_id;

    // 포트 인자 확인
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    // 포트 재사용 옵션
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 서버 주소 설정 및 바인딩
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    listen(serv_sock, 5);

    // 클라이언트 연결 반복 수락
    while (1) {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);

        // 클라이언트 목록에 추가 (뮤텍스 보호)
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        // 포트 출력 후 스레드 생성
        printf("Connected client port: %d\n", ntohs(clnt_adr.sin_port));
        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
    }

    close(serv_sock);
    return 0;
}

// 개별 클라이언트 처리 함수
void* handle_clnt(void* arg) {
    int clnt_sock = *((int*)arg);
    struct iovec vec[2];
    char name[NAME_SIZE + 1];                 // 클라이언트 ID
    char req[BUF_SIZE];                       // 수식 요청 문자열
    char msg[BUF_SIZE + NAME_SIZE + 64];      // 최종 전송 메시지
    char expr[BUF_SIZE];                      // 수식 표현용 문자열

    while (1) {
        // readv로 ID + 수식 동시 수신
        vec[0].iov_base = name;
        vec[0].iov_len = NAME_SIZE;
        vec[1].iov_base = req;
        vec[1].iov_len = BUF_SIZE;

        ssize_t str_len = readv(clnt_sock, vec, 2);
        if (str_len <= 0) {
            printf("client close\n");
            break;
        }

        name[NAME_SIZE] = '\0';
        req[BUF_SIZE - 1] = '\0';

        // 수식 계산
        int result;
        if (calculate_expr(req, &result, expr) == -1) {
            int overflow_val = 0;
            sscanf(req, "%d", &overflow_val);  // 피연산자 수 추출
            sprintf(msg, "Overflow value(%d) - client closed\n", overflow_val);
            write(clnt_sock, msg, strlen(msg));
            printf("client close\n");
            break;
        }

        // 결과 메시지 전송
        sprintf(msg, "[%s] %s=%d\n", name, expr, result);
        send_msg(msg, strlen(msg));
    }

    // 클라이언트 목록에서 제거 (뮤텍스 보호)
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_sock == clnt_socks[i]) {
            while (i < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
    printf("client close\n");
    return NULL;
}

// 모든 클라이언트에게 메시지 브로드캐스트
void send_msg(const char* msg, int len) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

// 수식 파싱 및 계산 수행 함수
int calculate_expr(char* req, int* result, char* expr_buf) {
    int count, opnd[100];
    char op[100], temp[16];
    char* token = strtok(req, " ");

    // 피연산자 개수 파싱 및 유효성 검사
    if (sscanf(token, "%d", &count) != 1 || count <= 0 || count > 100)
        return -1;

    // 피연산자 읽기
    for (int i = 0; i < count; i++) {
        token = strtok(NULL, " ");
        if (!token) return -1;
        opnd[i] = atoi(token);
    }

    // 연산자 읽기
    for (int i = 0; i < count - 1; i++) {
        token = strtok(NULL, " ");
        if (!token) return -1;
        op[i] = token[0];
    }

    // 연산 수행 및 문자열 구성
    *result = opnd[0];
    sprintf(expr_buf, "%d", opnd[0]);
    for (int i = 0; i < count - 1; i++) {
        switch (op[i]) {
            case '+': *result += opnd[i + 1]; break;
            case '-': *result -= opnd[i + 1]; break;
            case '*': *result *= opnd[i + 1]; break;
            default: return -1;
        }
        sprintf(temp, "%c%d", op[i], opnd[i + 1]);
        strcat(expr_buf, temp);
    }

    return 0;
}
