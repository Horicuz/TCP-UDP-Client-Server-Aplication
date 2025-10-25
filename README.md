# Tema 2 – Communication Protocols  
**TCP & UDP Client–Server Application for Message Management**  
**Author:** Potop Horia-Ioan  

---

## 1. INTRODUCTION  
This project implements a **publish–subscribe message broker** with two main components:  
- **Server (broker)** – receives UDP messages from publishers and forwards them to TCP subscribers.  
- **TCP subscribers** – send subscribe/unsubscribe commands and receive messages from the server.  

---

## 2. COMPILATION AND EXECUTION  
The project includes a simple Makefile with the following commands:  
- `make server` – compiles the server (`./server`)  
- `make subscriber` – compiles the TCP client (`./subscriber`)  
- `make clean` – removes object files and executables  

### Launch commands:  
- **Server:** `./server <PORT>`  
- **Subscriber:** `./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>`  

---

## 3. FUNCTIONALITY  

- At startup, the server opens both a UDP socket (for receiving publisher messages) and a TCP socket (for accepting subscriber connections).  
- Uses `select()` to wait on both sockets and on stdin.  
- The only keyboard command accepted by the server is **"exit"**, which closes the server and all connected TCP clients.  

### a) Message Redistribution Component  
- Once the server and clients are running, and some clients have subscribed to topics using the `subscribe`/`unsubscribe` commands, the system handles receiving UDP packets and redistributing them only to subscribers of the corresponding topics.  
- This is managed by the **`contains_topic`** function, which checks for each client if the received topic matches their subscriptions.  
- Wildcard `"*"` replaces zero or more topic levels, while `"+"` replaces exactly one level. Each message reaches a subscriber only once, even if multiple subscriptions match the same topic.  
- The **`create_pck`** function decodes messages according to their format (topic, type, and payload – INT, SHORT_REAL, FLOAT, STRING), packages them into a TCP structure, and forwards them to all online subscribers for that topic.  
- Messages are sent on each socket individually, and clients display them immediately upon reception.  

### b) Connection Management Component  
- New TCP connection requests are verified by the server:  
  - A **new client** is added along with its file descriptor into a map structure, awaiting commands. Initially, it has no active topics.  
  - A **returning client** (reconnecting with the same ID) is marked as online again, reactivating its previously stored topics.  
  - If a **duplicate connection** attempt is made (same ID already connected), the server notifies the client that the connection cannot be established.  

### c) Subscription, Unsubscription, and Disconnection Component  
- For each `subscribe <topic>` / `unsubscribe <topic>` command, the server updates its internal data structures and confirms the action to the subscriber via stdout.  
- Invalid operations (e.g., unsubscribe from a non-existing topic) are handled gracefully.  
- When disconnecting, the server closes the client socket and marks it as offline in its internal state to allow future reconnection with the same ID.  

### TCP Subscriber Functionality  
a) **Connection** – The client connects to the server, sends its ID, and awaits confirmation.  
b) **Command Handling** – Supports `subscribe`, `unsubscribe`, and `exit` commands. Input is parsed via the **`create_req`** function and sent to the server. Upon `exit`, the client closes the TCP socket.  
c) **Receiving and Displaying Messages** – The TCP client receives messages for subscribed topics and displays them neatly on stdout.  

---

## 4. ARCHITECTURE AND DATA STRUCTURES  
The server maintains:  
- A mapping between **CLIENT_ID → TCP file descriptor → online/offline state**  
- A mapping between **client → set of subscribed topics**  
- Functions for **pattern/topic matching with wildcard support**  

The application-level protocol (`tcp_pck`) is implemented as a compact, packed structure containing the UDP origin IP and port, topic name, data type, and ASCII value.  
`serv_pck` is the structure for client commands and topic names.  

---

## 5. VALIDATION AND ERROR HANDLING  
- All system calls (`socket`, `bind`, `listen`, `accept`, `select`, `recvfrom`, `recv`, `send`, `setsockopt`) are checked for errors using the **DIE** macro, printing an error message to stderr and exiting.  
- **stdout buffering** is disabled (`_IONBF`) to ensure immediate display of logs and feedback.  
- **Nagle’s algorithm** is disabled (`TCP_NODELAY`) on all TCP connections to minimize latency.  
- Commands are validated: exactly two tokens, topic ≤ 50 characters, and no extra arguments; invalid inputs trigger error messages.  
- UDP packets are decoded carefully with proper **byte-order handling** (`ntohs` / `ntohl`) and null-terminated topics.  

---

## 6. OTHER INFORMATION  
- Clients retain their subscribed topics across disconnections and reconnections.  
- The implementation fully meets requirements for: TCP+UDP multiplexing, wildcard topics, efficient application-level protocol, robust error handling, and clean disconnection logic.  
- The code is modular, well-documented, and easy to compile using the provided Makefile.  
