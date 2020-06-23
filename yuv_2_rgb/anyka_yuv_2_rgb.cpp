
#include <stdio.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "yuv420pTorgb24.h"



#define YUV420P_1280X720  1280*720*3/2  //1382400

#define RGB24_1280X720    1280*720*3





int main()
{

	uint8_t* one_frame;
	uint8_t* rgb24;

	one_frame = new uint8_t[YUV420P_1280X720];
	rgb24 = new uint8_t[RGB24_1280X720];
	
	//long fd = open("DC011923.yuv",O_RDONLY);
	long fd = open("DC000237.yuv",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	read(fd,one_frame,YUV420P_1280X720);
	close(fd);

	struct timeval tvs;
	unsigned long t0, t1;
	
	gettimeofday(&tvs, NULL);
	t0 = tvs.tv_sec*1000+tvs.tv_usec/1000;
	//t0 = tvs.tv_sec*1000*1000+tvs.tv_usec;
	yuv420p2rgb24_3(one_frame, rgb24, 1280, 720);
	gettimeofday(&tvs, NULL);
	t1 = tvs.tv_sec*1000+tvs.tv_usec/1000;
	printf("yuv 2 rgb :%ldms\r\n",(t1-t0));
	//t1 = tvs.tv_sec*1000*1000+tvs.tv_usec;
	//printf("yuv 2 rgb :%ldus\r\n",(t1-t0));


	delete []one_frame;
	delete []rgb24;
	return 0;
}



