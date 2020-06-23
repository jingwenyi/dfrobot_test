#Makefile for build blobs test


BUILD_SOURCE :=	\
	main_client.cpp \
	dfrobot_client.cpp \
	osal_linux.cpp

BUILD_LIB := \
	-lpthread
	

all:
	g++ $(BUILD_SOURCE)  -o socket_client  $(BUILD_LIB)
clean:
	rm -rf socket_client
