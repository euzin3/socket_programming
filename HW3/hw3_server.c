#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define MAX 1024

int main(int argc, char** argv) {
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t len;
    char buf[MAX];
    char opCount;
    int opResult;
    int operands[MAX];
    char operators[MAX];

    if(argc < 2) {
        printf("usage: ./server PORT");
        return 1;
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return 1;
    }

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        return 1;
    }       

    while (1) {
        
        // 1. 전체 요청 수신
        len = sizeof(cli_addr);
        int recv_len = recvfrom(sockfd, buf, MAX, 0, (struct sockaddr *)&cli_addr, &len);

        // 2. opCount 추출
        opCount = buf[0];
        if (opCount <= 0) {
            printf("server close(%d)\n", opCount);
            break;
        }
        printf("Operand count: %d\n", opCount);

        // 3. 피연산자 추출
        for (int i = 0; i < opCount; i++) {
            memcpy(&operands[i], &buf[1 + (i * 4)], 4);  // 4바이트씩 복사
            printf("Operand %d: %d\n", i, operands[i]);
        }

        // 4. 연산자 추출
        for (int i = 0; i < opCount - 1; i++) {
            operators[i] = buf[1 + (opCount * 4) + i];
            printf("Operator %d: %c\n", i, operators[i]);
        }

        //5. 계산 수행
        opResult = operands[0];
        for(int i=0; i<opCount-1; i++) {
            switch (operators[i]) {
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

        printf("Operation result: %d\n", opResult);
        
        //5. 결과 전송 
        sendto(sockfd, &opResult, 4, 0, (struct sockaddr *)&cli_addr, len);
    }
    close(sockfd);
    return 0;
}

