#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>


void *thread_test_fun(void *arg)
{
	while(1)
	{
		printf("---------------\r\n");
		sleep(5);
	}
}

int main()
{

	pthread_t pthread_test;

	if(pthread_create(&pthread_test, NULL, thread_test_fun, NULL) != 0)
	{
		printf("create handle yuv fun failed\r\n");
	}
	


	while(1)
	{
		printf("hello world\r\n");
		sleep(5);
	}
	
	return 0;
}


