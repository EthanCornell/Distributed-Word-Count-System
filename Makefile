# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -fopenmp

# Targets
TARGETS = server client

# Directories
DIR = ./directory_big

# Object files
SERVER_OBJS = server.o
CLIENT_OBJS = client.o

# Rules
all: $(TARGETS) $(DIR)

# Build the server
server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS) $(LDFLAGS)

# Build the client
client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS) $(LDFLAGS)

# Rule to compile server.o
server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c server.cpp

# Rule to compile client.o
client.o: client.cpp
	$(CXX) $(CXXFLAGS) -c client.cpp

# Ensure the directory exists
$(DIR):
	mkdir -p $(DIR)

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(TARGETS)

.PHONY: all clean