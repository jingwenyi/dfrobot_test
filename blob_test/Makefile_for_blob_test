#Makefile for build blobs test


BUILD_SOURCE :=	\
	blob_test.cpp \
	blob.cpp 

BUILD_LIB := \
	-lSDL \
	-lpthread
	

all:
	g++ $(BUILD_SOURCE)  -o blob_test $(BUILD_LIB)
clean:
	rm -rf blob_test
