#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define MAX 1024

int main(int argc, char** argv) {
    int sockfd;
    char buf[MAX];
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    int opCount,opResult;

    if(argc < 3) {
        printf("usage: ./client IP PORT");
        return -1;
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));


    memset(buf,0,MAX);

    printf("Operand count: ");
    scanf("%d", &opCount);
    buf[0] = (char)opCount; // 첫 byte에 operand count 저장

    // 서버 종료 조건 처리
    if(opCount <= 0 || opCount >= 128){
        sendto(sockfd, buf, 1, 0, (struct sockaddr *)&serv_addr, addr_len);
        close(sockfd);
        return 0;
    }

    // 피연산자 입력
    for(int i=0; i<opCount; i++) {
        printf("Operand %d: ", i);
        scanf("%d", (int*)&buf[(i*4)+1]); // 4byte 정수로 저장
    }

    // 연산자 입력 
    for (int i=0; i<opCount-1; i++){
        printf("Operator %d: ", i);
        scanf(" %c" ,&buf[(opCount*4)+i+1]); // 1byte 연산자
    }
    
    // 전체 데이터 sendto (opCount + 피연산자 + 연산자 포함)
    int total_len = 1 + (opCount*4) + (opCount-1);
    sendto(sockfd, buf, total_len, 0, (struct sockaddr *)&serv_addr, addr_len);

    // 결과 수신 
    int recv_len = recvfrom(sockfd, &opResult, 4, 0, NULL, NULL);

    if (recv_len == 4) {
        printf("Operaion reslut: %d\n" , opResult);
    } else if (recv_len == 0) {
        printf("No result received.\n");
    } else if (recv_len < 0) {
        perror("recvfrom error");
    } else {
        printf("Partial result received (%d bytes)\n", recv_len);
    }

    close(sockfd);
    return 0;
}