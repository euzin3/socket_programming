# HW4 - DNS and TCP Half-Close Practice

이 과제는 TCP 소켓 프로그래밍, DNS 해석, Half-Close 통신 구현 과제입니다.  
클라이언트는 실행 인자의 개수에 따라 DNS 정보 조회 또는 파일 전송 모드로 동작하며, 서버는 `test.txt` 파일을 전송한 뒤 클라이언트로부터 다시 데이터를 수신합니다.
<br><br>


## 전체 구조

### Server (`hw4_server.c`)
- 클라이언트와 TCP 연결 형성

- `test.txt` 파일 내용을 클라이언트에게 전송

- Half-Close를 수행하여 쓰기 종료

- 클라이언트가 전송하는 파일 데이터를 수신 후 출력

### Client (`hw4_client.c`)
- 인자 1개: 도메인 이름 → DNS 정보 출력

- 인자 2개: IP 주소 + 포트 → TCP 서버 접속, 파일 수신 후 재전송
<br><br>


## Server 세부 로직 (`hw4_server.c`)

1. 실행 시 포트 번호 인자로 입력
   ```bash
   ./server 8080
   ```
2. TCP 소켓 생성 및 SO_REUSEADDR 설정

3. test.txt 파일 열기

4. 클라이언트 연결 수락 후 파일 전송:

5. fread() → write() 반복

6. Half-Close (shutdown(fd, SHUT_WR)) 수행

7. 클라이언트가 재전송한 데이터 수신 및 출력

8. 소켓과 파일 닫고 종료
<br><br>

## Client 세부 로직 (`hw4_client.c`)

클라이언트는 입력 인자의 개수에 따라 두 가지 모드로 동작


### 1. 도메인 이름만 입력한 경우

```bash
./client google.com
```
- 동작 순서

1. gethostbyname()을 통해 도메인 이름에 대한 공식 이름, 별칭, IP 주소 등 출력

2. IP 주소 리스트 중 첫 번째 IP를 gethostbyaddr()로 역해석하여 다시 도메인 정보를 출력

3. 출력 형식
```bash
<gethostbyname()>
Official name: ...
Aliases 1: ...
Address type: AF_INET
IP addr 0: ...

<gethostbyaddr()>
Official name: ...
Aliases 1: ...
Address type: AF_INET
IP addr 0: ...
```
- 도메인 이름이 올바르지 않거나 DNS 조회 실패 시, gethostbyname() error 출력 후 종료
<br><br>

### 2. IP 주소 + 포트 번호 입력한 경우
```bash
./client 127.0.0.1 9190
```
- 동작 순서
1. TCP 소켓 생성 후 connect()로 서버 연결 요청

2. getsockopt()를 통해 소켓 타입(SOCK_STREAM) 확인 및 출력:

```bash
This socket type is : 1(1)
```

3. 서버로부터 수신한 파일 데이터를 copy.txt로 저장:

    - read()로 수신 → fwrite()로 저장

    - 수신 완료 시 Received file data 메시지 출력

4. copy.txt를 다시 열어 읽고, 같은 내용을 서버에 다시 전송:

    - fread()로 읽고 write()로 전송

    - 마지막에 개행 문자도 함께 전송

5. 파일과 소켓을 닫고 종료
<br><br>


## 예외 처리
- 인자 개수가 1 또는 2가 아닌 경우

    - "Usage : ./client <DomainName> <Port>" 출력 후 종료

- gethostbyname() 또는 gethostbyaddr() 오류 발생 시

    - perror() 출력 후 종료

- getsockopt() 실패 시

    - "getsockopt() error" 출력 후 종료
<br><br>

## 실행 방법
1. DNS 해석 모드
```bash
gcc hw4_client.c -o client
./client google.com
```

2. TCP 파일 수신 모드
```bash
./client 127.0.0.1 9190
```
- 주의사항 : 실행 전 서버(hw4_server.c)가 포트 9190에서 동작 중이어야 하고, 같은 디렉토리에 test.txt 파일이 있어야 함