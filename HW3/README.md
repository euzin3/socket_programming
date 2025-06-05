# HW3 - UDP Advanced Calculator Server

이 과제는 UDP 소켓 프로그래밍을 이용해 클라이언트가 연산 데이터를 서버에 전송하고, 서버는 해당 연산을 수행한 후 결과를 다시 클라이언트로 응답하는 방식으로 동작합니다. TCP와는 달리 연결 지향이 아닌 비연결형 UDP 통신을 사용하여 서버-클라이언트 간 데이터를 송수신합니다.
<br><br>

## 전체 흐름

- **Client**:
  - 사용자에게 피연산자 수(opCount), 각 피연산자, 연산자를 입력받아 `buf[]`에 저장

  - `sendto()`로 서버에 전체 데이터를 전송

  - `recvfrom()`으로 계산 결과를 수신

- **Server**:
  - 클라이언트로부터 `recvfrom()`으로 전체 데이터를 수신

  - 피연산자/연산자를 파싱하여 연산 수행

  - 결과를 `sendto()`로 클라이언트에게 전송
<br><br>


## 데이터 전송 구조

Client는 다음과 같은 구조로 데이터를 `buf[]`에 저장하여 전송합니다:

| 구간 | 내용          | 바이트 수      |
|------|---------------|----------------|
| buf[0] | 피연산자 수(opCount) | 1 Byte         |
| buf[1 ~ 4*n] | 피연산자 n개 (int) | 4 * n Bytes     |
| buf[4*n+1 ~ end] | 연산자 (char) | (n-1) Bytes    |

예: `opCount = 3` → `1 (opCount) + 12 (3 operands) + 2 (2 operators) = 15 bytes`
<br><br>

## Server 세부 로직 (`server.c`)

- `recvfrom()`으로 클라이언트 요청 수신

- 수신된 버퍼에서 순차적으로 다음 추출

  1. `buf[0]`: 피연산자 개수

  2. `memcpy()`를 통해 피연산자 추출

  3. 인덱스를 계산해 연산자 추출

- 연산 수행:
```c
switch (operators[i]) {
    case '+': opResult += operands[i+1]; break;
    case '-': opResult -= operands[i+1]; break;
    case '*': opResult *= operands[i+1]; break;
}
```

- 연산 결과를 sendto()로 전송

- 예외 처리 :

    - opCount <= 0인 경우 종료 메시지 출력 후 서버 종료
<br><br>


## Client 세부 로직 (`client.c`)

1. 사용자로부터 `opCount` 입력 → `buf[0]`에 저장

2. `opCount <= 0` 또는 `opCount >= 128`인 경우 서버에 종료 신호 전송 후 프로그램 종료

3. 피연산자 입력
   - `buf[1 + i*4]`에 각 정수를 4바이트 단위로 저장

4. 연산자 입력
   - `buf[1 + (4 * opCount) + i]` 위치에 1바이트 연산자 저장

5. 전체 데이터를 `sendto()`로 서버에 전송

   - 전송 바이트 수 = `1 + (opCount * 4) + (opCount - 1)`

6. 서버로부터 결과값 4바이트 수신 → `recvfrom()` 사용

7. 결과 출력

- 예외 처리:
  - 수신된 바이트 수가 4보다 작으면 `"Partial result received"` 또는 `"No result received"` 출력

  - `recvfrom()` 오류 발생 시 `perror()`로 출력
<br><br>

## 실행 방법

### 1. Server 실행
```bash
gcc server.c -o server
./server 9190
```

### 2. Client 실행
```bash
gcc client.c -o client
./client 127.0.0.1 9190
```