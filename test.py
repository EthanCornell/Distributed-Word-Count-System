import os
import shutil
import subprocess
import random
import string
import math
import errno

# Define the root directory for tests
ROOT_DIR = "./directory_big"

# Function to create a random string
def random_string(length=10):
    return ''.join(random.choice(string.ascii_letters) for _ in range(length))

# Function to calculate the total number of directories and files
def calculate_total_files_and_dirs(depth, breadth, files_per_dir):
    total_dirs = sum(breadth ** i for i in range(depth))  # Geometric series for directory count
    total_files = total_dirs * files_per_dir
    return total_dirs, total_files

# Function to create a nested directory structure
def create_directory_structure(root, depth, breadth, files_per_dir):
    if depth == 0:
        return

    for i in range(breadth):
        dir_name = f"{random_string()}_{i}"
        dir_path = os.path.join(root, dir_name)
        os.makedirs(dir_path, exist_ok=True)
        
        # Create files in this directory
        for j in range(files_per_dir):
            file_name = f"file_{random_string()}_{j}.txt"
            file_path = os.path.join(dir_path, file_name)
            try:
                with open(file_path, 'w') as f:
                    f.write("The quick brown fox, dressed in a top hat and monocle, jumps over the lazy dog who’s napping on a sunlit hammock while eating a giant slice of pepperoni pizza and dreaming of becoming the next big rock star.\n" * 100)  # Writing 100 lines to the file
            except OSError as e:
                print(f"[ERROR] Failed to create file {file_path}: {e}")

        # Recursively create more directories
        create_directory_structure(dir_path, depth - 1, breadth, files_per_dir)

# Function to run the test based on different configurations
def run_test(test_name, depth, breadth, files_per_dir):
    # Cleanup any previous test runs
    print(f"Cleaning up previous test files for {test_name}...")
    if os.path.exists(ROOT_DIR):
        try:
            shutil.rmtree(ROOT_DIR)
        except OSError as e:
            print(f"[ERROR] Failed to remove directory {ROOT_DIR}: {e}")
    print("Cleanup completed.")

    # Create the directory structure
    print(f"Creating directory structure for {test_name} with depth {depth} and breadth {breadth}...")
    create_directory_structure(ROOT_DIR, depth, breadth, files_per_dir)
    print("Directory structure created.")

    # Calculate total directories and files
    total_dirs, total_files = calculate_total_files_and_dirs(depth, breadth, files_per_dir)
    print(f"Total directories: {total_dirs}, Total files: {total_files}")

    # Dynamically set the number of server worker processes
    server_workers = max(2, math.ceil(total_files / 100))
    print(f"Using {server_workers} server worker(s) for this test.")

    # Compile the client and server if necessary
    print("Compiling client and server...")
    subprocess.run(["make"], check=True)

    # Run the server in the background
    server_process = subprocess.Popen(["./server", str(server_workers)])

    # Generate a timestamp for testing - Using a fixed timestamp here for consistency
    cutoff_time = subprocess.run(["date", "+%s", "-d", "1 day ago"], capture_output=True, text=True).stdout.strip()

    # Run the client with the generated timestamp
    print(f"Running client for {test_name}...")
    subprocess.run(["./client", cutoff_time])

    # Cleanup: Stop the server process
    print(f"Stopping the server for {test_name}...")
    server_process.terminate()
    server_process.wait()

    print(f"Test {test_name} completed.")

    # After test completion, clean up the folder
    print(f"Cleaning up test files for {test_name}...")
    if os.path.exists(ROOT_DIR):
        try:
            shutil.rmtree(ROOT_DIR)
        except OSError as e:
            print(f"[ERROR] Failed to remove directory {ROOT_DIR}: {e}")
    print(f"Cleanup after {test_name} completed.")


# Run correctness test
print("Running Correctness Test: Small files and few directories")
run_test("Correctness Test", depth=1, breadth=1, files_per_dir=20)

# Run tiny test
print("Running Tiny Test: Small files and few directories")
run_test("Tiny Test", depth=2, breadth=1, files_per_dir=100)

# Run small test
print("Running Small Test: Small files and few directories")
run_test("Small Test", depth=2, breadth=2, files_per_dir=5)

# Run medium test
print("Running Medium Test: Medium number of files and directories")
run_test("Medium Test", depth=5, breadth=3, files_per_dir=10)

# Run large test
print("Running Large Test: Large size files and directories")
run_test("Medium Large Test", depth=6, breadth=4, files_per_dir=15)
# run_test("Large Test", depth=10, breadth=5, files_per_dir=20)

print("All tests completed.")


# pkill -f './client 1722181920'
# find ./directory_big/ -type f | wc -l