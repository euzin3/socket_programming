#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int serv_sd, clnt_sd;
    FILE * fp;
    char buf[BUF_SIZE];
    int read_cnt;
    
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    if(argc!=2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // client에게 보낼 파일 열기
    fp = fopen("test.txt", "rb");
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

    // 1. 클라이언트에게 파일 전송
    while((read_cnt = fread(buf, 1, BUF_SIZE, fp)) > 0)
        write(clnt_sd, buf, read_cnt);
    fclose(fp);
    shutdown(clnt_sd, SHUT_WR);

    // 2. Client가 재전송한 데이터 출력
    printf("Message from client:\n");
    while((read_cnt = read(clnt_sd, buf, BUF_SIZE)) > 0){
        fwrite(buf, 1, read_cnt, stdout);
    }

    close(clnt_sd);
    close(serv_sd);
    return 0;
}