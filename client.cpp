
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
 */


#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <string>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <sstream>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <list>
#include <unordered_map>
#include <queue>

// LRU Cache Data Structure
class LRUCache {
    size_t cache_size_limit;
    std::list<std::string> lru_list; // Cache order
    std::unordered_map<std::string, std::map<std::string, int>> cache; // Maps file path to word count

public:
    LRUCache(size_t limit) : cache_size_limit(limit) {}

    bool get_from_cache(const std::string& file_path, std::map<std::string, int>& word_count) {
        if (cache.find(file_path) != cache.end()) {
            // Move the accessed file to the front of the list
            lru_list.remove(file_path);
            lru_list.push_front(file_path);
            word_count = cache[file_path];
            return true;
        }
        return false;
    }

    void add_to_cache(const std::string& file_path, const std::map<std::string, int>& word_count) {
        if (cache.find(file_path) == cache.end()) {
            if (lru_list.size() >= cache_size_limit) {
                // Evict the least recently used item
                std::string least_recent = lru_list.back();
                lru_list.pop_back();
                cache.erase(least_recent);
            }
            lru_list.push_front(file_path);
            cache[file_path] = word_count;
        }
    }
};

// Global LRU cache
LRUCache lru_cache(10000); // Set cache limit to 100 files

// Thread pool management
std::queue<std::string> dirs_to_visit;
std::mutex queue_mutex;
std::condition_variable queue_cv;
bool done_traversal = false;

// Word counting utility
void count_words_in_file(const std::string& file_path, std::map<std::string, int>& word_count) {
    // Check if the result is already cached
    if (lru_cache.get_from_cache(file_path, word_count)) {
        std::cout << "[INFO] Cache hit for file: " << file_path << std::endl;
        return;
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: " << file_path << std::endl;
        return;
    }
    
    std::string word;
    while (file >> word) {
        ++word_count[word];
    }

    // Add the result to the cache
    lru_cache.add_to_cache(file_path, word_count);
}

// Parallel directory traversal worker function
void bfs_traverse_directory_worker(time_t cutoff_time, std::vector<std::string>& modified_files) {
    std::cout << "[INFO] Worker thread started." << std::endl;

    while (true) {
        std::string current_dir;

        // Fetch directory to process
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [] { 
                return !dirs_to_visit.empty() || done_traversal; 
            });

            if (dirs_to_visit.empty() && done_traversal) {
                std::cout << "[INFO] Worker thread is exiting as queue is empty and traversal is done." << std::endl;
                break;
            }

            if (!dirs_to_visit.empty()) {
                current_dir = dirs_to_visit.front();
                dirs_to_visit.pop();
                std::cout << "[INFO] Processing directory: " << current_dir << std::endl;
            }
        }

        if (current_dir.empty()) continue;

        DIR* dir = opendir(current_dir.c_str());
        if (dir == nullptr) {
            std::cerr << "[ERROR] Failed to open directory: " << current_dir << std::endl;
            continue;
        }

        struct dirent* entry;
        struct stat file_stat;

        while ((entry = readdir(dir)) != nullptr) {
            std::string path = current_dir + "/" + entry->d_name;
            if (stat(path.c_str(), &file_stat) == -1) continue;

            // Skip unmodified directories
            if (S_ISDIR(file_stat.st_mode)) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    if (difftime(file_stat.st_mtime, cutoff_time) > 0) {
                        std::lock_guard<std::mutex> lock(queue_mutex);
                        dirs_to_visit.push(path);
                        queue_cv.notify_one();
                        std::cout << "[INFO] Added directory to queue: " << path << std::endl;
                    }
                }
            } else if (S_ISREG(file_stat.st_mode) && difftime(file_stat.st_mtime, cutoff_time) > 0) {
                std::lock_guard<std::mutex> lock(queue_mutex);
                modified_files.push_back(path);
                std::cout << "[INFO] Added modified file: " << path << std::endl;
            }
        }
        closedir(dir);
    }
}

// Parallel directory traversal
void parallel_bfs_traverse_directory(const std::string& root_dir, time_t cutoff_time, std::vector<std::string>& modified_files, int num_threads) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        dirs_to_visit.push(root_dir);
    }
    queue_cv.notify_all();

    // Create worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(bfs_traverse_directory_worker, cutoff_time, std::ref(modified_files));
    }

    // Wait for the traversal to finish
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        done_traversal = true;
    }
    queue_cv.notify_all();

    for (auto& worker : workers) {
        worker.join();
    }
}

// Parallel file processing
void process_files_parallel(const std::vector<std::string>& modified_files, int num_threads, std::map<std::string, int>& total_word_count) {
    std::mutex word_count_mutex;
    size_t file_index = 0;
    std::mutex file_index_mutex;

    auto process_file_worker = [&]() {
        while (true) {
            std::string file_path;

            // Fetch the next file to process
            {
                std::lock_guard<std::mutex> lock(file_index_mutex);
                if (file_index >= modified_files.size()) {
                    break;
                }
                file_path = modified_files[file_index++];
            }

            std::map<std::string, int> word_count;
            count_words_in_file(file_path, word_count);

            // Merge local word count into global total_word_count
            {
                std::lock_guard<std::mutex> lock(word_count_mutex);
                for (const auto& entry : word_count) {
                    total_word_count[entry.first] += entry.second;
                }
            }
        }
    };

    // Start worker threads to process files in parallel
    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(process_file_worker);
    }

    // Join all worker threads
    for (auto& worker : workers) {
        worker.join();
    }
}

// Batch sending file paths to the server
void send_file_paths_to_server(int sock, const std::vector<std::string>& modified_files) {
    std::ostringstream oss;
    for (const auto& file : modified_files) {
        oss << file << "\n";
    }

    std::string data = oss.str();
    auto start_time = std::chrono::high_resolution_clock::now();
    send(sock, data.c_str(), data.size(), 0);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "[INFO] Time taken to send file paths to server: " << elapsed.count() << " seconds" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./client <timestamp>" << std::endl;
        return 1;
    }

    time_t cutoff_time = atol(argv[1]);
    std::vector<std::string> modified_files;

    // Get the number of hardware-supported threads
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 2; // Fallback to 2 threads if hardware_concurrency() returns 0
    }

    // Start timing the directory traversal
    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform parallel traversal using the number of hardware-supported threads
    parallel_bfs_traverse_directory("./directory_big", cutoff_time, modified_files, num_threads);

    // End timing the directory traversal
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    // Log the time taken for directory traversal
    std::cout << "[INFO] Directory traversal complete." << std::endl;
    std::cout << "[INFO] Using " << num_threads << " threads for parallel directory traversal." << std::endl;
    std::cout << "[INFO] Time taken to complete directory traversal: " << elapsed.count() << " seconds" << std::endl;

    // Create socket and connect to server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "[ERROR] Failed to create socket, errno: " << errno << std::endl;
        return 1;
    }
    std::cout << "[INFO] Client socket created." << std::endl;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Loopback address

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ERROR] Connection to server failed, errno: " << errno << std::endl;
        return 1;
    }
    std::cout << "[INFO] Successfully connected to the server." << std::endl;

    // Send file paths to the server
    send_file_paths_to_server(sock, modified_files);

    std::cout << "[INFO] Finished sending file paths to the server." << std::endl;

    close(sock);
    std::cout << "[INFO] Client socket closed." << std::endl;
    return 0;
}
// ./client $(date +%s -d "1 day ago")