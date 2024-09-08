### Distributed Word Count System - Optimized Edition

This project implements an enhanced version of the distributed word count system, featuring improvements in efficiency, scalability, and multi-threaded processing. Below are the key updates over the previous version and the detailed system overview:

---

### Improvements Over the Previous Solution(Client.cpp)

This updated solution is a significant improvement over the previous implementation due to several enhancements in efficiency, scalability, and architecture. Below are the key differences and advantages:

### 1. **Parallel Directory Traversal**
   - **Before**: Single-threaded directory traversal using BFS, inefficient for large, deep folder structures.
   - **Now**: **Multi-threaded** traversal for faster processing of large and deep directories.
     - **Advantage**: Faster traversal by distributing work across multiple threads.
     - **Impact**: Significant speedup for large directories.

### 2. **Parallel File Processing**
   - **Before**: Files were processed sequentially, one at a time.
   - **Now**: **Parallel processing** with multiple threads counts words simultaneously.
     - **Advantage**: Faster processing of large datasets with multiple files.
     - **Impact**: Reduced processing time for large-scale operations.

### 3. **LRU Cache for File Processing**
   - **Before**: No caching mechanism.
   - **Now**: **LRU Cache** stores recent file word counts, avoiding redundant processing.
     - **Advantage**: Improves efficiency by reducing disk reads and reprocessing.
     - **Impact**: Faster performance in environments with frequently accessed files.

### 4. **Batching File Paths for Network Transmission**
   - **Before**: File paths were sent one by one to the server.
   - **Now**: **Batch transmission** of file paths reduces network overhead.
     - **Advantage**: Minimizes the number of `send()` calls.
     - **Impact**: Faster client-server communication, especially with many files.

### 5. **Non-Blocking Directory Queue Management**
   - **Before**: Blocking directory processing.
   - **Now**: **Non-blocking queue** management for directories, allowing concurrent operations.
     - **Advantage**: Reduces wait time during traversal.
     - **Impact**: Faster and more efficient traversal.

### 6. **Improved Scalability and Multi-Core Utilization**
   - **Before**: Limited to a single thread.
   - **Now**: Full **multi-core utilization**, scaling with available cores.
     - **Advantage**: Better utilization of modern multi-core systems.
     - **Impact**: Improved performance and scalability for large workloads.

### 7. **Efficient Word Counting and Data Transfer**
   - **Before**: Word counting and file path transfers were done sequentially.
   - **Now**: **Parallel word counting** and **batched data transfer** for more efficient operation.
     - **Advantage**: Reduces file I/O and network bottlenecks.
     - **Impact**: Faster word counting and reduced network overhead.

### Summary of Key Differences:
| **Feature**                  | **Previous Solution**                    | **Improved Solution**                                      |
|------------------------------|-------------------------------------------|------------------------------------------------------------|
| **Directory Traversal**       | Single-threaded BFS                      | Multi-threaded parallel BFS                                |
| **File Processing**           | Sequential file processing                | Parallel file processing with multiple threads             |
| **Caching**                   | No caching                               | LRU Cache for recently processed files                     |
| **File Path Transmission**    | Sent file paths one by one                | Batched transmission of file paths                         |
| **CPU Utilization**           | Single-core utilization                   | Multi-core utilization based on hardware concurrency       |
| **Queue Management**          | Blocking queue                           | Non-blocking, concurrent queue management                  |
| **Scalability**               | Limited scalability for large directories | Scalable to handle large directories and deep structures    |
| **Network Efficiency**        | Individual file transmissions             | Batching reduces network overhead                          |

---
### Improvements Over the Previous Solution(server.cpp)

**1. Architecture and Execution Model:**
- **Before**: The previous version used a **multi-process architecture**. Each client request was handled by a separate process. Upon receiving data, the server spawned multiple processes (via `fork`) to handle incoming file processing requests. Each process was responsible for counting words in the assigned files sequentially.
  
- **Now**: The optimized version adopts a **multi-threaded approach** with OpenMP for parallelism. Instead of creating separate processes, worker threads handle incoming file paths, and each file is processed in parallel using OpenMP. This allows better resource sharing, lower overhead, and improved scalability, particularly when handling large datasets.

**2. File Processing:**
- **Before**: The word count was performed sequentially, where files were opened and read line by line to count the words. The results were stored in a single `std::map` per process.

- **Now**: The optimized version uses **memory-mapped I/O (`mmap`)** to process large files efficiently, reducing the need for explicit file reads and improving performance by leveraging direct memory access. Each file's word count is performed in parallel using OpenMP.

**3. Data Handling:**
- **Before**: Each process handled its own word counting using individual maps, and these maps were not combined across processes. Client requests were handled independently without any central aggregation.

- **Now**: In the optimized version, word counting results from individual threads are merged using a **global word count map**. The results from different threads are combined (Map-Reduce-like) to ensure that the overall word count is consistent and aggregated across all files. A global mutex ensures thread safety during this merge phase.

**4. Task Management:**
- **Before**: The server used a blocking I/O model where each client request was handled sequentially, and a process was forked for each connection. No batching or queueing was used for task distribution.

- **Now**: The optimized version uses a **job queue** for managing file processing tasks, allowing better coordination among threads. A **condition variable** is used to notify worker threads about incoming tasks, ensuring efficient processing with minimal idle time.

**5. Concurrency Model:**
- **Before**: The previous version used multiple processes with no shared memory, which led to higher memory usage and context-switching overhead.
  
- **Now**: The optimized version is fully **multi-threaded**. Each worker thread is spawned to handle multiple tasks concurrently, reducing overhead and making better use of multi-core systems. The use of OpenMP further enhances parallelism within each thread.

**6. Scalability:**
- **Before**: Scalability was limited by the overhead of creating multiple processes and the lack of efficient resource sharing between processes.

- **Now**: The optimized solution scales better due to the reduced overhead of thread creation and management. By using multi-threading, the server can handle more clients simultaneously, and the OpenMP integration allows processing of multiple files concurrently, improving the overall throughput【40†source】.

### Summary of Key Differences:
| **Feature**           | **Before**                            | **Now**                                    |
|-----------------------|---------------------------------------|--------------------------------------------|
| **Architecture**       | Multi-process (fork-based)            | Multi-threaded (with OpenMP)               |
| **File Processing**    | Sequential file processing            | Memory-mapped I/O with parallel processing |
| **Task Management**    | Process-per-client                    | Job queue with condition variable          |
| **Concurrency**        | Process-based concurrency             | Thread-based concurrency                   |
| **Data Handling**      | No central aggregation                | Map-Reduce-style aggregation               |
| **Scalability**        | Limited by process overhead           | Scalable with efficient thread management  | 

These changes result in a more efficient and scalable server-side implementation, particularly for high-concurrency and large-scale file processing environments.
---

### 1. System Overview
The system consists of the following:
- **Client (`client.cpp`)**: Traverses a directory (`./directory_big`), finds files modified after a given timestamp, and sends these file paths to the server for processing.
- **Server (`server.cpp`)**: Receives file paths from clients, processes them to count word occurrences, and outputs the results. The server handles concurrent client requests by spawning multiple worker processes.

---

### 2. Components
**Client Program (`client.cpp`)**:
- **Purpose**: Traverses the specified directory and identifies files modified after a given timestamp.
- **Functionality**:
  - Accepts a timestamp as a command-line argument.
  - Traverses the directory using parallel Breadth-First Search (BFS).
  - Utilizes an **LRU cache** to store results from frequently accessed files.
  - Sends file paths to the server in **batches**.

**Server Program (`server.cpp`)**:
- **Purpose**: Receives file paths from clients, processes them to count words, and outputs results.
- **Functionality**:
  - Starts multiple worker threads for file processing using **memory-mapped I/O**.
  - Utilizes **OpenMP** to parallelize word counting across threads.
  - Aggregates and outputs word counts, processing times, and CPU usage.

---

### 3. Additional Scripts

**`generate_files.py`**:
- **Purpose**: Automates the creation of test files with random content for performance testing.
- **Functionality**: Generates a large number of files with specified sizes and word content.

**`test.py`**:
- **Purpose**: Automates testing of the system, validating performance and correctness.
- **Functionality**: Simulates client-server interactions and verifies word counts.

---

### 4. Execution & Testing
**Compilation**:
- Compile the client and server programs:
  ```bash
  make
  ```

**Running the Server**:
- Start the server with a specified number of worker threads:
  ```bash
  ./server 8  # Starts the server with 8 worker threads
  ```

**Running the Client**:
- The client accepts a timestamp and sends file paths to the server:
  ```bash
  ./client 1722181920  # Replace with the appropriate timestamp
  ```

**Running `generate_files.py`**:
- Generate a large number of test files:
  ```bash
  python3 generate_files.py
  ```

**Running `test.py`**:
- Run automated tests to validate performance:
  ```bash
  python3 test.py
  ```

---

### 5. Results Analysis
- **Processing time per file** ranged from 10.103 ms to 10.935 ms.
- **CPU time** in user mode was between 1.5 ms and 1.9 ms, with minimal system mode usage.

---

### 6. Conclusion
The optimized distributed word count system efficiently handles large datasets and deep folder structures by leveraging parallel processing, memory-mapped I/O, and batching techniques. These improvements enhance scalability and performance, making the system suitable for large-scale deployments.



