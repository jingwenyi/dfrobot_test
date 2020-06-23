#Makefile for build blobs test


BUILD_SOURCE :=	\
	main.cpp \
	blob.cpp \
	blobs.cpp \
	colorlut.cpp \
	calc.cpp  \
	qqueue.cpp

BUILD_LIB := \
	-lSDL \
	-lpthread
	

all:
	g++ $(BUILD_SOURCE)  -o blobs $(BUILD_LIB)
clean:
	rm -rf blobs
