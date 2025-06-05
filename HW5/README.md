# HW5 - Multi-Process Calculator Server

이 과제는 서버가 여러 클라이언트의 요청을 `fork()`를 통해 동시에 처리할 수 있도록 구현한 멀티프로세스 기반의 계산기 구현 과제입니다. `SIGCHLD` 시그널 핸들러를 활용하여 좀비 프로세스를 방지합니다.
<br><br>

## 전체 구조

### Server (`hw5_server.c`)
- 클라이언트 요청을 `accept()`하고, `fork()`를 통해 자식 프로세스를 생성하여 각각 처리

- 자식 프로세스는 클라이언트의 계산 요청을 받아 결과를 반환

- 부모 프로세스는 `SIGCHLD` 시그널 핸들러(`read_childproc()`)를 통해 자식 종료 시 자원 회수

### Client (`hw5_client.c`)
- 사용자로부터 피연산자 개수, 피연산자 값, 연산자를 입력받아 서버에 전송

- 서버로부터 계산 결과를 수신하여 출력
<br><br>

## Server 세부 로직 (`hw5_server.c`)

1. 실행 시 포트 번호를 인자로 받음
   ```bash
   ./server 9190
   ```

2. SO_REUSEADDR 소켓 옵션 설정

3. SIGCHLD 시그널 등록 : 자식 프로세스 종료 시 waitpid() 호출

4. 서버 소켓 생성 후 bind() 및 listen()

5. 메인 루프에서 accept() 

    -  클라이언트가 접속하면 fork()로 자식 프로세스 생성

    - 부모는 close(clnt_sock) 후 루프로 복귀

    - 자식은 다음 작업 수행 

        - read()로 opCount, operands, operators 수신

        - 계산 수행 및 결과 출력

        - 결과를 write()로 클라이언트에 전송 후 종료

6. 계산 예시 출력 (pid 포함) 
    ```bash
   34567: 3+2*5=25
    ```

7. 자식 종료 시
    ```bash
    removed proc id: 34567
    ```
<br><br>

## Client 세부 로직 (hw5_client.c)
1. 사용자 입력:

    - Operand count (정수)

    - 각 피연산자 값 (int)

    - 연산자 입력 (+, -, *)

2. buf[] 구성:

    | 구간 | 내용             | 크기         |
    |------|------------------|--------------|
    | [0]  | opCount = 3      | 1 byte       |
    | [1~12] | operands (3개) | 4 bytes × 3  |
    | [13~14] | operators (2개) | 1 byte × 2  |
    | 총합  |                 | 1 + 12 + 2 = 15 bytes |

3. 서버에 write()로 전송:

    - 총 전송 바이트 수 = 1 + (opCount * 4) + (opCount - 1)

4. 서버로부터 4바이트 결과 수신 → 출력:   
    ```bash
    Operaion reslut: 25
    ```
5. opCount <= 0 입력 시 서버에 종료 신호만 전송 후 종료
<br><br>

## 실행 방법

### 1. Server 실행
```bash
gcc server.c -o server
./server 9190
```

### 2. Client 실행
```bash
gcc hw5_client.c -o client
./client 127.0.0.1 9190
```