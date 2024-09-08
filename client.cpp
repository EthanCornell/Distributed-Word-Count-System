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

// Function to traverse the directory using BFS and collect files modified after the cutoff time
void bfs_traverse_directory(const std::string& root_dir, time_t cutoff_time, std::vector<std::string>& modified_files) {
    std::queue<std::string> dirs_to_visit;
    dirs_to_visit.push(root_dir);

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
            }
        }
        closedir(dir);
    }
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

    // Traverse the directory starting from root using BFS
    bfs_traverse_directory("./directory_big", cutoff_time, modified_files);

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

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    // Send the modified file paths to the server
    for (const auto& file : modified_files) {
        send(sock, file.c_str(), file.length(), 0);
        send(sock, "\n", 1, 0);
    }

    // Close the socket
    close(sock);

    return 0;
}


// date +%s -d "2024-08-25 19:36:45" to find timestamp

