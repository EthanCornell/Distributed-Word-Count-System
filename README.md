### Distributed Word Count System

This project implements a distributed word count system, comprising two main components: a client program (`client.cpp`) and a server program (`server.cpp`). The system is designed to traverse a specified directory, process files modified after a given timestamp, and count the occurrences of English words in those files. The server employs a **multi-process architecture** to handle high concurrency, ensuring efficient request processing from multiple clients.

---

#### 1. System Overview
The system consists of the following:
- **Client (`client.cpp`)**: Traverses a directory (`./directory_big`), finds files modified after a given timestamp, and sends these file paths to the server for processing.
- **Server (`server.cpp`)**: Receives file paths from clients, processes them to count word occurrences, and outputs the results. The server handles concurrent client requests by spawning multiple worker processes.

---

#### 2. Components
**Client Program (`client.cpp`)**:
- **Purpose**: Traverses the specified directory and identifies files modified after a given timestamp.
- **Functionality**:
  - Accepts a timestamp as a command-line argument.
  - Traverses the directory using Breadth-First Search (BFS) and collects all files modified after the provided timestamp.
  - Establishes a TCP connection to the server and sends file paths for processing.
  - The client closes the connection once all file paths are sent.

**Server Program (`server.cpp`)**:
- **Purpose**: Receives file paths from clients, counts the occurrence of words in those files, and outputs the results.
- **Functionality**:
  - Starts multiple worker processes based on the number of processes specified by the user.
  - Each worker listens for client connections, receives file paths, and processes the files to count words.
  - Outputs the number of files processed, the time taken, and the CPU usage for each process after processing is complete.

---

#### 3. Execution & Testing
**Compilation**:
- Use the provided `Makefile` to compile the client and server programs:
  ```bash
  make
  ```

**Running the Server**:
- Start the server with a specified number of processes:
  ```bash
  ./server 8  # Starts the server with 8 worker processes
  ```

**Running the Client**:
- The client accepts a timestamp and sends file paths to the server:
  ```bash
  ./client 1722181920  # Replace with the appropriate timestamp
  ```

**Testing Scripts**:
- **100 Clients Simulation** (`100clients.sh`)【10†source】:
  ```bash
  for i in {1..100}
  do
    ./client 1722181920 &
  done
  wait
  ```
- **1000 Clients Stress Test** (`stressTest.sh`)【9†source】:
  ```bash
  for i in {1..1000}
  do
    ./client 1722181920 &
  done
  wait
  ```
- **20 Clients Simulation** (`multipleClients.sh`)【11†source】:
  ```bash
  for i in {1..20}
  do
    ./client 1722181920 &
  done
  wait
  ```

---

#### 4. Results Analysis
During testing, each server process handled approximately the same number of files, with minimal variation in processing time. CPU usage across processes was well-balanced, demonstrating effective load distribution.

- **Processing time per file** ranged from 10.103 ms to 10.935 ms.
- **CPU time** in user mode was between 1.5 ms and 1.9 ms, with no significant system mode usage.

---

#### 5. Conclusion
The distributed word count system successfully handles high concurrency by distributing the load across multiple processes. The multi-process architecture ensures that the system remains efficient and stable under heavy loads, making it suitable for real-world applications requiring scalable file processing capabilities.
