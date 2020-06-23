//Author jingwenyi create 2016.04.14 for test dfrobot client

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dfrobot_client.h"

#define  RECV_BUF_SIZE             (8*1024*1024)



Dfrobot_client *myClient;
static char *recv_buff;




void *sendFun(void *arg)
{
	char buff[20]= "i am client!";
	printf("sendFun--------------\n");
	while(1)
	{
		sleep(10);
		myClient->SendData(buff,strlen(buff));
	}
	
}

int main()
{
	pthread_t sentThread;
	int *thread_ret = NULL;
	int ret;
	int len;
	recv_buff = new char[RECV_BUF_SIZE];

	char ip[50] = "192.168.1.55";
	
	myClient = Dfrobot_client::Create(ip, DFROBOT_SERVER_PORT);
	myClient->Start();
#if 0
	ret = pthread_create(&sentThread, NULL, sendFun, NULL);
	if(ret != 0)
	{
		printf("create thread error\n");
		myClient->Delete();
		return -1;
	}
#endif

	//主线程来接收数据
	while(1)
	{
		pause();

		sleep(5);
		//len = myClient->GetRecvData(recv_buff,RECV_BUF_SIZE);
		
		//printf("rec msg:%s,len:%d\n",recv_buff,len);
		
	}
	
	pthread_join(sentThread, (void**)&thread_ret);

	delete []recv_buff;

	myClient->Delete();
	return 0;
}


