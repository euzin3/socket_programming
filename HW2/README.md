# HW2 - Advanced TCP Calculator Server

이 과제는 TCP 소켓 프로그래밍을 통해 클라이언트가 사용자로부터 연산 데이터를 입력받아 서버로 전송하고, 서버가 이를 처리하여 연산 결과를 클라이언트로 반환하는 구조로 구성되어 있습니다.
클라이언트는 피연산자와 연산자 정보를 전송하고, 서버는 순차적으로 `+`, `-`, `*` 연산을 수행하여 결과를 반환합니다.
<br><br>


## 전체 흐름

- **Client**:
  - 사용자에게 피연산자 수(opCount), 각 피연산자, 연산자를 입력받아 `buf[]`에 저장

  - 서버에 `(1 + 4*n + (n-1))` 바이트의 데이터를 전송

  - 연산 결과를 서버로부터 수신하여 출력

- **Server**:
  - 클라이언트로부터 데이터 수신 후, 피연산자와 연산자를 파싱

  - 수식을 계산하여 결과를 클라이언트에 전송\
<br><br>


## 데이터 전송 구조


| 구간 | 내용          | 바이트 수      |
|------|---------------|----------------|
| buf[0] | 피연산자 수(opCount) | 1 Byte         |
| buf[1 ~ 4*n] | 피연산자 n개 (int) | 4 * n Bytes     |
| buf[4*n+1 ~ end] | 연산자 (char) | (n-1) Bytes    |

예: `opCount = 3` → `1 (opCount) + 12 (3 operands) + 2 (2 operators) = 15 bytes`
<br><br>


## Server 세부 로직 (`server.c`)

- `accept()` 이후 다음 순서로 데이터 수신

  1. `read()`로 1바이트 opCount 수신

  2. 피연산자 n개(각 4바이트), 연산자 n-1개(각 1바이트) 수신

  3. `switch` 문을 이용해 연산 수행

  4. 결과를 4바이트로 `write()`하여 전송


```c
switch (operators[i]) {
    case '+': opResult += operands[i+1]; break;
    case '-': opResult -= operands[i+1]; break;
    case '*': opResult *= operands[i+1]; break;
}
```
- 예외 처리 : opCount <= 0인 경우 연결 종료
<br><br>

## client 세부 로직 (`client.c`)
1. 사용자로부터 opCount 입력 → buf[0] 저장

2. 피연산자 입력 → buf[1 + i*4]에 4바이트 정수 저장

3. 연산자 입력 → buf[4*n + i]에 문자 저장

4. write()로 전체 데이터 전송

5. read()로 서버 연산 결과 수신 후 출력

- 예외 처리 : 
    - opCount <= 0일 경우 서버에 종료 신호 전송 후 즉시 종료      

    -  서버 응답 미수신 또는 불완전 수신 시 경고 출력
<br><br>

## 실행 방법

### 1. 서버 실행
```bash
gcc server.c -o server
./server 9190
```

### 2. 클라이언트 실행
```bash
gcc client.c -o client
./client 127.0.0.1 9190
```
