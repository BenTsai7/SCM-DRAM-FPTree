CXX = g++

HEADER_DIR = ../include

FPTREE_HEADER_DIR = $(HEADER_DIR)/fptree

CXXFLAGS = -std=c++11 -g

CFLAGS = -fPIC -g -std=c99 -O3 -msse4.2 -mpclmul -march=native -funroll-loops -Wstrict-overflow -Wstrict-aliasing -Wall -Wextra -pedantic -Wshadow

MAIN = ./bin/main

YCSB = ./bin/ycsb

LYCSB = ./bin/lycsb

BIN = bin

FPTREE = fptree.o

UTILITY = utility.o

PALLOCATOR = p_allocator.o

CLHASH = clhash.o

ALL :=  $(LYCSB) $(MAIN) $(YCSB)
#============================TODO: all below============================

all : $(ALL)

.PHONY:mount  
mount :
	@mkdir -p bin/data
	-sudo umount /dev/pmem0
	#mount NVM to the data directory
	sudo mkfs.ext4 /dev/pmem0
	sudo mount -o dax /dev/pmem0 ./bin/data
	sudo chmod -R 0777 ./bin/data

$(MAIN) : $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH) $(BIN) mount
	$(CXX) $(CXXFLAGS) -c main.cpp -I $(HEADER_DIR) -o main.o
	$(CXX) $(CXXFLAGS) -I$(HEADER_DIR)  $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH) main.o -o  $@  -lpmem

$(FPTREE) : fptree.cpp p_allocator.cpp
	@$(CXX) $(CXXFLAGS) -c fptree.cpp -I $(HEADER_DIR) -o $@ -lpmem

$(UTILITY) : utility.cpp
	@$(CXX) $(CXXFLAGS) -c utility.cpp -I $(HEADER_DIR) -o $@

$(PALLOCATOR) : p_allocator.cpp $(HEADER_DIR)/utility/p_allocator.h $(HEADER_DIR)/utility/utility.h
	@$(CXX) $(CXXFLAGS) -c p_allocator.cpp -I $(HEADER_DIR) -o $@ -lpmem

$(LYCSB) : $(BIN) lycsb.cpp
	@$(CXX) $(CXXFLAGS) lycsb.cpp -o $@ -lleveldb -lpthread

$(YCSB) : ycsb.cpp $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH) $(MAIN) mount
	@mkdir -p bin/data
	@$(CXX) $(CXXFLAGS) -c ycsb.cpp -I$(HEADER_DIR) -o ycsb.o -lpmem -lleveldb -lpthread
	@$(CXX) $(CXXFLAGS) -I$(HEADER_DIR) ycsb.o  $(FPTREE) $(UTILITY) $(PALLOCATOR) $(CLHASH) -o  $@ -lpmem -lleveldb -lpthread
	@-rm -rf *.o

$(CLHASH) : clhash.c $(HEADER_DIR)/utility/clhash.h
	@$(CC) $(CFLAGS) -c clhash.c -I $(HEADER_DIR) -g -o $@

$(BIN):
	@mkdir -p bin

clean :
	-sudo umount /dev/pmem0
	@-rm -rf *.o ./bin/* ../data/*
	@-rm -rf bin

cleand : 
	@-rm -rf ../data/*

