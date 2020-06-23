#Makefile for build blobs test


BUILD_SOURCE :=	\
	main2.cpp \
	qqueue.cpp \
	blobs.cpp \
	blob.cpp \
	calc.cpp \
	colorlut.cpp 

BUILD_LIB := \
	-lSDL \
	-lpthread
	

all:
	g++ $(BUILD_SOURCE)  -o blobs_test $(BUILD_LIB)
clean:
	rm -rf blobs_test
