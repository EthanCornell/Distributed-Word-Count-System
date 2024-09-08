import os
import random
import string

# Fixed root directory
ROOT_DIR = './directory_big'

def random_string(length=10):
    """Generate a random string of letters."""
    return ''.join(random.choice(string.ascii_letters) for _ in range(length))

def create_files_in_dir(root, depth, breadth, files_per_dir):
    """Recursively create directories and files."""
    if depth == 0:
        return

    for i in range(breadth):
        # Create directory
        dir_name = 'dir_' + random_string()
        dir_path = os.path.join(root, dir_name)
        os.makedirs(dir_path, exist_ok=True)
        print(f"Created directory: {dir_path}")

        # Create files in the directory
        for j in range(files_per_dir):
            file_name = 'file_' + random_string() + '.txt'
            file_path = os.path.join(dir_path, file_name)
            with open(file_path, 'w') as f:
                f.write('The quick brown fox jumps over the lazy dog.\n' * 100)  # Writing 100 lines
            print(f"Created file: {file_path}")

        # Recurse into the new directory to create more directories/files
        create_files_in_dir(dir_path, depth - 1, breadth, files_per_dir)

def main():
    # Get user input for depth, breadth, and files per directory
    depth = int(input("Enter the depth of directories: "))
    breadth = int(input("Enter the breadth (number of subdirectories per directory): "))
    files_per_dir = int(input("Enter the number of files per directory: "))

    # Ensure the root directory exists
    if not os.path.exists(ROOT_DIR):
        os.makedirs(ROOT_DIR)
    
    # Create files and directories
    create_files_in_dir(ROOT_DIR, depth, breadth, files_per_dir)
    print(f"File generation completed in {ROOT_DIR}!")

if __name__ == "__main__":
    main()
