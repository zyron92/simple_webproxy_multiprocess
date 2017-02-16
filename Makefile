# Project directories
SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj

# Compilation options
CC = gcc
CFLAGS = -I$(INC_DIR) -Wextra #-Wall
LDFLAGS = -lm -lpthread

# List of object file
OBJ_FILES = $(OBJ_DIR)/csapp.o $(OBJ_DIR)/socket_helper.o $(OBJ_DIR)/http_handler.o

################################################################################

all : webproxy

# Generation of main object file for proxy program
$(OBJ_DIR)/webproxy.o: $(SRC_DIR)/webproxy.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Generation of an object file from a source file and its specification file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/%.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Linking of all object files for proxy program
webproxy: $(OBJ_DIR)/webproxy.o $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

################################################################################

# Clean Up
.PHONY: clean
clean: clean_temp
	rm -f webproxy $(OBJ_DIR)/*.o proxy.log source.tar.gz

.PHONY: clean_temp
clean_temp:
	rm -f ./*/*~
	rm -f ./*~

################################################################################

.PHONY: test
test: webproxy
	valgrind --leak-check=full --show-leak-kinds=all ./webproxy

################################################################################

.PHONY: run
run: webproxy
	./webproxy

################################################################################

.PHONY: compress
compress: clean
	tar -czvf source.tar.gz include/ obj/ src/ Makefile README.txt

################################################################################

# Help
.PHONY: help
help:
	@echo "Options :-"
	@echo "1) make / make all - to compile everything"
	@echo "2) make clean - clean up the object files, executables and proxy.log"
	@echo "3) make clean_temp - clean up temporary files"
	@echo "4) make test - run the executable of web proxy with valgrind to check memory"
	@echo "5) make run - run the executable of web proxy"
	@echo "6) make compress - compress the source code, readme and makefile"
	@echo "7) make help - help menu"
