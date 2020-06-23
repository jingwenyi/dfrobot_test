##
## author jingwenyi create 2016.04.08 for test yuv
##
INCLUDES = -I include
BUILD_FLAG = -L./ -lcurl  -I../curl-7.49.1/include/
#BUILD_FLAG = ./libcurl.a -I../curl-7.49.1/include/
SOURCE_FILE := \
	maintuling_test.cpp \
	json_reader.cpp \
	json_value.cpp \
	json_writer.cpp \
	json_api.cpp \
	dfrobot_tuling.cpp \
	json_api_for_tuling.cpp 
	
TARGET := testtuling

all:
	g++ ${SOURCE_FILE}  -o ${TARGET} ${INCLUDES} ${BUILD_FLAG}


clean:
	rm -rf ${TARGET}
