
// Author: I-Hsuan Huang
// email: ethan.huang.ih@gmail.com

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <signal.h>
#include <sys/time.h> 
#include <sys/resource.h>

// Function to count words in a file and update the word count map
void count_words_in_file(const std::string& file_path, std::map<std::string, int>& word_count) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    std::string word;
    while (file >> word) {
        ++word_count[word];
    }
}

// Function to process client requests
void process_client(int client_socket) {
    struct timeval start, end;
    gettimeofday(&start, NULL); // Record the start time

    struct rusage usage_before, usage_after;
    getrusage(RUSAGE_SELF, &usage_before); // Get CPU usage before processing

    static int file_count = 0;  // Static variable to track files processed by this process

    char buffer[1024];
    int read_size;
    std::string partial_path;
    std::map<std::string, int> word_count;

    while ((read_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[read_size] = '\0';  // Null-terminate the received data
        std::string received_data(buffer);

        // Handle potential partial path from previous recv call
        if (!partial_path.empty()) {
            received_data = partial_path + received_data;
            partial_path.clear();
        }

        size_t pos;
        while ((pos = received_data.find('\n')) != std::string::npos) {
            std::string file_path = received_data.substr(0, pos);
            received_data.erase(0, pos + 1);

            // Increment the file count
            file_count++;
            std::cout << "[INFO] Process " << getpid() << " processed file: " << file_path << std::endl;

            count_words_in_file(file_path, word_count);
        }

        // If there is any leftover data that doesn't end with a newline, save it for the next recv
        if (!received_data.empty()) {
            partial_path = received_data;
        }
    }

    // Print the word counts
    std::cout << "[INFO] Word counts for the current batch:" << std::endl;
    for (const auto& [word, count] : word_count) {
        std::cout << word << ": " << count << std::endl;
    }

    // Record the end time and get CPU usage after processing
    gettimeofday(&end, NULL);
    getrusage(RUSAGE_SELF, &usage_after);

    // double processing_time = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0; // Convert to milliseconds
    // double user_cpu_time = (usage_after.ru_utime.tv_sec - usage_before.ru_utime.tv_sec) * 1000.0 + (usage_after.ru_utime.tv_usec - usage_before.ru_utime.tv_usec) / 1000.0;
    // double system_cpu_time = (usage_after.ru_stime.tv_sec - usage_before.ru_stime.tv_sec) * 1000.0 + (usage_after.ru_stime.tv_usec - usage_before.ru_stime.tv_usec) / 1000.0;

        // Convert to seconds
    double processing_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0; // Convert to seconds
    double user_cpu_time = (usage_after.ru_utime.tv_sec - usage_before.ru_utime.tv_sec) + (usage_after.ru_utime.tv_usec - usage_before.ru_utime.tv_usec) / 1000000.0;
    double system_cpu_time = (usage_after.ru_stime.tv_sec - usage_before.ru_stime.tv_sec) + (usage_after.ru_stime.tv_usec - usage_before.ru_stime.tv_usec) / 1000000.0;

    // std::cout << "[INFO] Process " << getpid() << " processed " << file_count << " files in " << processing_time << " ms." << std::endl;
    // std::cout << "[INFO] Process " << getpid() << " used " << user_cpu_time << " ms in user mode and " << system_cpu_time << " ms in system mode." << std::endl;
    std::cout << "[INFO] Process " << getpid() << " processed " << file_count << " files in " << processing_time << " seconds." << std::endl;
    std::cout << "[INFO] Process " << getpid() << " used " << user_cpu_time << " seconds in user mode and " << system_cpu_time << " seconds in system mode." << std::endl;

    close(client_socket);
    exit(0);  // Terminate the child process
}

// Signal handler to reap zombie processes
void sigchld_handler(int sig) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <num_processes>" << std::endl;
        return 1;
    }

    int num_processes = std::atoi(argv[1]);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }
    std::cout << "[INFO] Server socket created." << std::endl;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }
    std::cout << "[INFO] Socket successfully bound to port 8080." << std::endl;

    listen(server_socket, 3);
    std::cout << "[INFO] Server is listening for connections..." << std::endl;

    // Set up the signal handler to avoid zombie processes
    signal(SIGCHLD, sigchld_handler);

    // Fork the specified number of worker processes
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return 1;
        }
        if (pid == 0) {  // Child process
            while (true) {
                int client_socket = accept(server_socket, nullptr, nullptr);
                if (client_socket < 0) {
                    std::cerr << "Accept failed" << std::endl;
                    continue;
                }
                std::cout << "[INFO] Accepted new client connection." << std::endl;
                process_client(client_socket);
            }
            // The child process should exit if the loop breaks, but it should never actually break in this case.
        }
        // Parent process continues to fork the next worker
    }

    // Parent process remains running indefinitely, reaping child processes as needed
    while (true) {
        pause();  // Wait for signals (like SIGCHLD)
    }

    close(server_socket);
    return 0;
}
