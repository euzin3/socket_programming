#include <stdio.h>       // print(), fputs() 등 표준 입출력 함수 
#include <stdlib.h>      // exit() 등 일반적인 유틸리티 함수들
#include <string.h>
#include <unistd.h>      // read(), write(), close()등 시스템 호출 함수
#include <arpa/inet.h>   // inet_ntoa(), inet_addr() 등 IP 다루는 함수들 
#include <netdb.h>       // gethostbyname() 등 DNS 관련 함수와 구조체들

#define BUF_SIZE 1024
void handle_domain_only(char *domain);
void handle_ip_and_port(char *ip, char *port);

int main(int argc, char *argv[])
{
    if (argc == 2) {
        // 도메인만 입력받는 경우
        handle_domain_only(argv[1]);
    }
    else if (argc == 3) {
        // IP + Port 입력받는 경우 
        handle_ip_and_port(argv[1], argv[2]);
    }
    else {
        // 도메인 이름 안 줬을 경우
        printf("Usage : %s <DomainName> <RemoteAddress>\n", argv[0]);
        return 1;
    }

    return 0;
}


// 매개 변수로 도메인이름 하나만 입력받는 경우 
void handle_domain_only(char *domain)
{
    int i;
    struct hostent *host;
    struct sockaddr_in addr;

    /*gethostbyname*/ 
    host = gethostbyname(domain);
    if(!host){
        perror("gethostbyname() error");
        exit(1);
    }
    printf("gethostbyname()\n");

    // 1. Official name 
    printf("Official name: %s \n", host->h_name);
    // 2. Aliases
    for(i=0; host->h_aliases[i]; i++)
        printf("Aliases %d: %s \n", i+1, host->h_aliases[i]);
    // 3. Address type 
    printf("Address type: %s \n", (host->h_addrtype==AF_INET)?"AF_INET":"AF_INET6");
    // 4. IP address 
    for(i=0; host->h_addr_list[i]; i++)
        printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));


    /*gethostbyaddr*/
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr=inet_addr(inet_ntoa(*(struct in_addr*)host->h_addr_list[1])); //IP 주소 문자열을 숫자로 바꿔서 addr에 저장
    host = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);
    if(!host) {
        perror("gethostbyaddr() error");
        exit(1);
    }
    printf("\ngethostbyaddr()\n");

    // 1. Official name 
    printf("Official name: %s \n", host->h_name);
    // 2. Aliases
    for(i=0; host->h_aliases[i]; i++){
        printf("Aliases %d: %s \n", i, host->h_aliases[i]);
    }
    // 3. Address type
    printf("Address type: %s \n", (host->h_addrtype==AF_INET)? "AF_INET" : "AF_INET6");
    // 4. IP address
    for(i=0; host->h_addr_list[i]; i++) {
        printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }

    exit(0);
}

// 매개 변수로 포트 번호와 IP 주소를 입력받는 경우 
void handle_ip_and_port(char *ip, char *port)
{
    int sd; 
    int sock_type;
    socklen_t optlen;
    int state;

    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;
    struct sockaddr_in serv_adr;

    sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(ip);
    serv_adr.sin_port = htons(atoi(port));

    connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    // 1. socket type 출력
    optlen = sizeof(sock_type);
    state = getsockopt(sd, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen);
    if(state) {
        perror("getsockopt() error");
        exit(1);
    }
    printf("This socket is : %d(%d)\n", sock_type, SOCK_STREAM);

    // 2. 서버로부터 받은 파일 데이터를 'copy.txt' 파일에 저장
    fp = fopen("copy.txt", "wb");
    while((read_cnt=read(sd, buf, BUF_SIZE))!=0)
        fwrite((void*)buf, 1, read_cnt, fp);

    fclose(fp);
    puts("Received file data");
    
    // 3. 저장된 'copy.txt' 파일을 다시 읽어 서버에게 재전송
    fp = fopen("copy.txt", "rb");
    while((read_cnt=fread(buf, 1, BUF_SIZE, fp))!=0)
        write(sd, buf, read_cnt);
    write(sd, "\n", 1);
    fclose(fp);
    close(sd);

    exit(0);
}