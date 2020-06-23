//author jingwenyi create 2016.4.13 for test socket transfer 

#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "socket_transfer_server.h"

#define YUV420P_1280X720  1280*720*3/2  //1382400


TransferServer *myServer;


int main()
{
	uint8_t* one_frame;

	one_frame = new uint8_t[YUV420P_1280X720+9];

	one_frame[0] = 0x55;
	one_frame[1] = 0xaa;
	one_frame[2] = 0x10;
	

	//printf("%x,%x,%x,%x,%x,%x,%x\r\n",one_frame[0],one_frame[1],one_frame[2],one_frame[3],one_frame[4],one_frame[5],one_frame[6]);

	sleep(2);
#if 0
	//long fd = open("DC000237.yuv",O_RDONLY);
	long fd = open("DC000237.jpeg",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	ssize_t readcnt = 0;
	readcnt = read(fd,&one_frame[7],YUV420P_1280X720);
	close(fd);
	printf("readcnt=%d\r\n",readcnt);
	one_frame[3] = readcnt & 0xff;
	one_frame[4] = (readcnt >> 8) & 0xff;
	one_frame[5] = (readcnt >> 16) & 0xff;
	one_frame[6] = (readcnt >> 24) & 0xff;
	
	one_frame[6+readcnt+1] = 0;
	one_frame[6+readcnt+2] = 0;
#endif
	
	printf("%x,%x,%x,%x,%x,%x,%x\r\n",one_frame[0],one_frame[1],one_frame[2],one_frame[3],one_frame[4],one_frame[5],one_frame[6]);
	
	myServer = TransferServer::Create();
	myServer->start();

	char buf[100] = "i am server yuv----";
	//int count = 0;
	while(1)
	{
		//pause();
		//myServer->SendYuvDataToClient(one_frame,(YUV420P_1280X720+9));
		//myServer->SendYuvDataToClient(one_frame,readcnt+9);
		myServer->SendYuvDataToClient((uint8_t*)buf,100);
		sleep(5);
		//count++;
		//printf("--%d--\n",count);
		//usleep(40);
	}

	printf("hello world\r\n");

	delete []one_frame;
	delete myServer;
	return 0;
}

