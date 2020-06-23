


all:
#	arm-none-linux-gnueabi-gcc  main2345_test.c  -o  test2345 -ldl -L. -lcurl -lssl -lcrypto -lz -I../curl-7.49.1/include/  -I/source/anyka/openssl/include  -I/source/anyka/zlib/include
	arm-none-linux-gnueabi-gcc  main2345_test.c  -o  test2345 -ldl  ./libcurl.a  ./libssl.a  ./libcrypto.a  ./libz.a  -I../curl-7.49.1/include/   -I/source/anyka/openssl/include  -I/source/anyka/zlib/include


clean:
	rm -rf test2345
