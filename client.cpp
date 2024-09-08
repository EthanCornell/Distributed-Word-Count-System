
// Author: I-Hsuan Huang
// email: ethan.huang.ih@gmail.com

#include <iostream>
#include <queue>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <string>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>
#include <chrono>
// Function to traverse the directory using BFS and collect files modified after the cutoff time
void bfs_traverse_directory(const std::string& root_dir, time_t cutoff_time, std::vector<std::string>& modified_files) {
    std::queue<std::string> dirs_to_visit;
    dirs_to_visit.push(root_dir);

    // std::cout << "[INFO] Using  threads for parallel directory traversal." << std::endl;  // Log message

    while (!dirs_to_visit.empty()) {
        std::string current_dir = dirs_to_visit.front();
        dirs_to_visit.pop();

        DIR *dir = opendir(current_dir.c_str());
        if (dir == nullptr) continue;

        struct dirent *entry;
        struct stat file_stat;

        while ((entry = readdir(dir)) != nullptr) {
            std::string path = current_dir + "/" + entry->d_name;

            // Get the file or directory status
            if (stat(path.c_str(), &file_stat) == -1) continue;

            // If it's a directory, add it to the queue to visit next
            if (S_ISDIR(file_stat.st_mode)) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    dirs_to_visit.push(path);
                }
            } 
            // If it's a file, check the modification time
            else if (S_ISREG(file_stat.st_mode) && difftime(file_stat.st_mtime, cutoff_time) > 0) {
                modified_files.push_back(path);
                std::cout << "[INFO] Added modified file: " << path << std::endl;  // Log message
            }
        }
        closedir(dir);
    }

    std::cout << "[INFO] Directory traversal complete." << std::endl;  // Log message
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./client <timestamp>" << std::endl;
        return 1;
    }

    // Convert the argument to a time_t value
    time_t cutoff_time = atol(argv[1]);

    // Vector to hold paths of modified files
    std::vector<std::string> modified_files;

    
    
    // Start timing the directory traversal
    auto start_time = std::chrono::high_resolution_clock::now();
    bfs_traverse_directory("./directory_big", cutoff_time, modified_files);

    // End timing the directory traversal
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
 
    std::cout << "[INFO] Time taken to complete directory traversal: " << elapsed.count() << " seconds" << std::endl;

    // Establish a connection to the server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "[INFO] Client socket created." << std::endl;  // Log message

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    std::cout << "[INFO] Successfully connected to the server." << std::endl;  // Log message

    // Start timing for sending file paths
    start_time = std::chrono::high_resolution_clock::now();
    
    // Send the modified file paths to the server
    for (const auto& file : modified_files) {
        // std::cout << "[INFO] Sending modified file: " << file << std::endl;  // Log message for sending files
        send(sock, file.c_str(), file.length(), 0);
        send(sock, "\n", 1, 0);
    }

    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> send_time = end_time - start_time;
    std::cout << "[INFO] Time taken to send file paths to server: " << send_time.count() << " seconds" << std::endl;

    std::cout << "[INFO] Finished sending file paths to the server." << std::endl;

    // Close the socket
    close(sock);
    std::cout << "[INFO] Client socket closed." << std::endl;

    return 0;
}
