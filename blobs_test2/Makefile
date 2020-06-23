##
## author jingwenyi create 2016.04.08 for test yuv
##


CC = arm-none-linux-gnueabi-g++
INCLUDES = -I ./blobs/include
BUILD_FLAG = 
SOURCE_FILE := \
	./blobs/blob.cpp  \
	./blobs/blobs.cpp  \
	./blobs/calc.cpp \
	./blobs/colorlut.cpp \
	./blobs/qqueue.cpp \
	./main.cpp



	
TARGET := blobs_test

all:
	${CC} ${SOURCE_FILE}  -o ${TARGET} ${INCLUDES} ${BUILD_FLAG}


clean:
	rm -rf ${TARGET}
