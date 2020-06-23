#Makefile for build blobs test


BUILD_SOURCE :=	\
	main.cpp \
	socket_transfer_server.cpp \
	socket_transfer_client.cpp \
	osal_linux.cpp

BUILD_LIB := \
	-lpthread
	

all:
	arm-none-linux-gnueabi-g++ $(BUILD_SOURCE)  -o socket_server  $(BUILD_LIB)
clean:
	rm -rf socket_server
