CC = g++
ABI_FLAG =
#ABI_FLAG = -DOPENVDB_ABI_VERSION_NUMBER=5
CFLAGS = -march=native -std=c++11 -Wall -w
BUILD_DEBUG   = $(CC) $(CFLAGS) $(ABI_FLAG) -O0 -g
BUILD_RELEASE = $(CC) $(CFLAGS) $(ABI_FLAG) -DNDEBUG -O3
INCLUDES = -I$(HOME)/local/include -I./include -I$(HOME)/private/research/openvdb/include -I../gtest
LIBRARY = -L$(HOME)/local/lib -L$(HOME)/private/research/openvdb/lib -L../gtest
LIBS = -lopenvdb -ltbb -ltbbmalloc_proxy -ltbbmalloc -lHalf -ljsoncpp -lgtest -lpthread -lm
DEP = Makefile src/*.cpp include/*.h

# Avoid associating these build target with files of the same name
.PHONY: all clean
 
.DEFAULT_GOAL := release

all: release debug

release/TestLaplacian.o: $(DEP)
	@mkdir -p release
	$(BUILD_RELEASE) $(INCLUDES) -o release/TestLaplacian.o -c src/TestLaplacian.cpp

debug/TestLaplacian.o: $(DEP)
	@mkdir -p debug
	$(BUILD_DEBUG) $(INCLUDES) -o debug/TestLaplacian.o -c src/TestLaplacian.cpp

release: release/TestLaplacian.o
	$(BUILD_RELEASE) $(INCLUDES) $(LIBRARY) -o release/TestLaplacian release/TestLaplacian.o $(LIBS)

debug: debug/TestLaplacian.o
	$(BUILD_DEBUG)  $(INCLUDES) $(LIBRARY) -o debug/TestLaplacian debug/TestLaplacian.o $(LIBS)

archive:
	@git archive master | gzip > SPGridVDB.tgz

clean:
	@rm -rf debug release SPGridVDB.tgz
