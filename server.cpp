/*
 * This file is part of word count distributed system project.
 *
 * word count distributed system project is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * word count distributed system project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with word count distributed system project.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Author: I-Hsuan Huang
 * Email: ethan.huang.ih@gmail.com
 * File: server.cpp
 */

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <omp.h> 

// Global data structures
std::queue<std::string> job_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
bool done = false;
std::unordered_map<std::string, int> global_word_count;
std::mutex global_count_mutex; // Mutex for global word count

// Memory-mapped word counting for large files
void count_words_in_file_mmap(const std::string& file_path, std::unordered_map<std::string, int>& local_word_count) {
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "[ERROR] Failed to open file: " << file_path << std::endl;
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "[ERROR] Failed to stat file: " << file_path << std::endl;
        close(fd);
        return;
    }

    if (sb.st_size == 0) {
        std::cerr << "[INFO] File is empty: " << file_path << std::endl;
        close(fd);
        return;
    }

    char *file_data = static_cast<char*>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_data == MAP_FAILED) {
        std::cerr << "[ERROR] Failed to mmap file: " << file_path << std::endl;
        close(fd);
        return;
    }

    std::string word;
    for (off_t i = 0; i < sb.st_size; ++i) {
        if (isalnum(file_data[i])) {
            word += file_data[i];
        } else if (!word.empty()) {
            ++local_word_count[word];
            word.clear();
        }
    }

    munmap(file_data, sb.st_size);
    close(fd);
}

// Worker function with parallelism using OpenMP (Map Phase)
void mapper_function() {
    while (true) {
        std::string file_data;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [] { return !job_queue.empty() || done; });

            if (job_queue.empty() && done) {
                break;
            }

            file_data = job_queue.front();
            job_queue.pop();
        }

        auto start_time = std::chrono::high_resolution_clock::now(); // Start processing timer

        std::istringstream file_stream(file_data);
        std::string file_path;
        std::unordered_map<std::string, int> local_word_count; // Intermediate word count (local to the worker)

        std::vector<std::string> file_paths;
        // Collect file paths from the batch
        while (std::getline(file_stream, file_path)) {
            if (!file_path.empty()) {
                file_paths.push_back(file_path);
            }
        }

        // Use OpenMP parallel for to process each file in parallel
        #pragma omp parallel for
        for (long unsigned int i = 0; i < file_paths.size(); ++i) {
            std::unordered_map<std::string, int> thread_word_count;
            count_words_in_file_mmap(file_paths[i], thread_word_count);

            // Shuffle Phase: Merge thread word counts into the local word count
            std::lock_guard<std::mutex> lock(global_count_mutex);
            for (const auto& entry : thread_word_count) {
                local_word_count[entry.first] += entry.second;
            }
        }

        // Log the word counts for the current batch
        if (!local_word_count.empty()) {
            std::cout << "[INFO] Word counts for the current batch:" << std::endl;
            for (const auto& entry : local_word_count) {
                std::cout << entry.first << ": " << entry.second << std::endl;
            }
        } else {
            std::cout << "[INFO] No valid files were processed in this batch." << std::endl;
        }

        // Shuffle Phase: Merge local word counts into the global word count
        {
            std::lock_guard<std::mutex> lock(global_count_mutex);
            for (const auto& entry : local_word_count) {
                global_word_count[entry.first] += entry.second;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now(); // End processing timer
        std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "[INFO] Time taken to process batch: " << elapsed.count() << " seconds" << std::endl;
    }
}

// Handle incoming connections from clients (each connection spawns a thread)
void handle_client(int client_socket) {
    char buffer[1024];
    int read_size;
    std::string data;

    // Receive data from client
    auto start_time = std::chrono::high_resolution_clock::now();
    while ((read_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        data += buffer;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "[INFO] Time taken to receive data from client: " << elapsed.count() << " seconds" << std::endl;

    if (data.empty()) {
        std::cerr << "[ERROR] Received empty data from client." << std::endl;
        close(client_socket);
        return;
    }

    // Add received data (file paths) to the job queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        job_queue.push(data);
    }
    queue_cv.notify_one();

    close(client_socket);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <num_workers>" << std::endl;
        return 1;
    }

    int num_workers = std::atoi(argv[1]);

    // Start worker threads (mappers)
    std::vector<std::thread> workers;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(mapper_function);
    }

    // Set up server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "[ERROR] Failed to create socket, errno: " << errno << std::endl;
        return 1;
    }
    std::cout << "[INFO] Server socket created." << std::endl;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ERROR] Bind failed, errno: " << errno << std::endl;
        return 1;
    }
    std::cout << "[INFO] Socket successfully bound to port 8080." << std::endl;

    if (listen(server_socket, 100) < 0) {
        std::cerr << "[ERROR] Listen failed, errno: " << errno << std::endl;
        return 1;
    }
    std::cout << "[INFO] Server is listening for connections..." << std::endl;

    // Accept clients in a loop
    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "[ERROR] Accept failed, errno: " << errno << std::endl;
            continue;
        }
        std::cout << "[INFO] Accepted new client connection." << std::endl;

        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);

    // Graceful shutdown
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        done = true;
    }
    queue_cv.notify_all();

    for (auto& worker : workers) {
        worker.join();
    }

    // Reduce Phase: Print final aggregated word count
    std::cout << "[INFO] Final word counts: " << std::endl;
    for (const auto& entry : global_word_count) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }

    return 0;
}
