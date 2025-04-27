#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define MAX 1024

int main(int argc, char** argv) {
    int sfd, cfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buf[MAX];
    char opCount;
    int opResult;
    int operands[MAX];
    char operators[MAX];

    if(argc < 2) {
        printf("usage:./server localPort\n");
        return 1;
    }

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return 1;
    }

    int enable = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(sfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        return 1;
    }       

    if(listen(sfd, 5) < 0) {
        perror("listen error");
        return 1;
    }

    while (1) {
        len = sizeof(cliaddr);
        if((cfd = accept(sfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
            perror("accept error");
            return 1;
        }

        read(cfd, &opCount, 1);
        if (opCount <=0){
            printf("server close(%d)\n" , opCount);
            close(cfd);
            break;
        }

        printf("Operand count: %d\n", opCount);

        if(opCount <= 0) {
            close(cfd);
        }

        for(int i=0; i<opCount; i++) {
            read(cfd,&operands[i],4);
            printf("Operand %d: %d\n", i, operands[i]);
        }

        for (int i =0; i<opCount-1; i++){
            read(cfd,&operators[i],1);
        }
        
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
        write(cfd, &opResult, 4);

        close(cfd);
    }
    close(sfd);
    return 0;
}

