# NSP Homework Projects

This repository contains homework assignments for the "Network Programming" course.  
Each folder contains both server and client code implementing various network programming concepts.
<br><br>


## Folder Structure

- `HW1/` : Week 1 - Basic TCP Socket Programming (Simple client-server communication)
- `HW2/` : Week 2 - Advanced TCP Calculator Server (Multi-operand, multi-operator calculation)
- `HW3/` : Week 3 - UDP Advanced Calculator Server (Calculator implementation over UDP)
- `HW4/` : Week 4 - DNS and TCP Half-Close Practice (Domain resolution and file transmission)
- `HW5/` : Week 5 - Multi-Process Calculator Server (Zombie process handling with pipes)
- `HW6/` : Week 6 - Multiplexing Calculator Server (Multiplexed TCP server using `select()`, `writev()`, `readv()`)

Each folder contains:
- `hwX_server.c` : Server-side implementation
- `hwX_client.c` : Client-side implementation

(*X is the homework number.)
<br><br>

## Environment

- Language: C
- Platform: Ubuntu 24.04 (WSL 2)
- Compiler: gcc 13.3.0
<br><br>

## How to Run

Each homework consists of server and client programs.
Basic running steps:

1. Move to the desired week's folder:
   ```bash
   cd HW1
   ```
   <br>

2. Compile the server and client:
   ```bash
   gcc hw1_server.c -o server
   gcc hw1_client.c -o client
   ```
   <br>

3. Open two terminals:
   - In Terminal 1 (server side):
   ```bash
   ./server [port] 
   ```
   - In Terminal 2 (client side):
   ```bash
   ./client [server_ip] [port]
   ```
